/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file implements NLER build platform-independent timer
 *      event interfaces.
 *
 */

#if NLER_FEATURE_EVENT_TIMER
#include "nlerevent_timer.h"
#if (nlLOG_PRIORITY > nlLPCRIT)
#undef nlLOG_PRIORITY
#define nlLOG_PRIORITY nlLPCRIT
#endif
#include "nlercfg.h"
#include "nlerlog.h"
#include <string.h>
#include "nlererror.h"
#include "nlertask.h"
#include <stdio.h>
#include "nlerassert.h"
#include "nleratomicops.h"
#include <nlcompiler.h>

#if NLER_FEATURE_SIMULATEABLE_TIME
#include "nlereventqueue_sim.h"
#include "nlertimer_sim.h"

/* Simulateable time only supported by task based implementation currently */
#if NLER_FEATURE_TIMER_USING_SWTIMER
#error Cannot use swtimer when doing simulateable time
#endif

/* Should really be number of tasks, but simulators don't always have a max
 * task defined.  We presume there won't be more tasks using timers than
 * simultaneous timers supported.
 */
nltask_t *s_task_list[NLER_MAX_TIMER_EVENTS];
nllock_t s_lock_list[NLER_MAX_TIMER_EVENTS];

#endif

#if NLER_FEATURE_TIMER_USING_SWTIMER
#include <nlplatform/nlswtimer.h>
#endif

typedef struct nl_event_timer_s
{
    NL_DECLARE_EVENT                    /**< Common event fields */
    nleventqueue_t     *mReturnQueue;   /**< Queue to send timer event to on delay expiration */
    bool                mRepeating;     /**< Timer flag: timer is repeating */
    bool                mCancelled;     /**< Timer flag: timer was cancelled */
    uint8_t             mQueuedCount;   /**< Count of times event has been posted to mReturnQueue */
    uint8_t             mIgnoreCount;   /**< Count of times to ignore events in is_valid() checks due to restarts */
#if NLER_FEATURE_TIMER_USING_SWTIMER
    nl_swtimer_t        mTimer;
#else
    nl_time_native_t    mTimeNow;       /**< For internal use by timer implementation */
    nl_time_native_t    mTimeoutNative; /**< For internal use by timer implementation */
#endif
#if NLER_FEATURE_SIMULATEABLE_TIME
    /* In simulator, task scheduling isn't like on a real device.
     * More than one task might be running (due to host machines having
     * more than one core) and task priorities are not honored (consider
     * all tasks to be running at the same priority).
     *
     * The device implementation made assumptions about single core
     * and priorities being honored, so we have to add additional
     * synchronization primitives in the simulator case.
     */
    nllock_t           *mLock;          /**< Lock for accessing structure members atomically */
#endif
#ifdef DEBUG
    /* We require that the task that calls start() be the same as the
     * receiving task that calls is_valid(), cancel(), etc.
     */
    nltask_t           *mTask;          /**< Receiving task */
#endif
} nl_event_timer_internal_t;

_Static_assert(sizeof(nl_event_timer_t) == sizeof(nl_event_timer_internal_t), "sizeof(nl_event_timer_t) != sizeof(nl_event_timer_internal_t)");

static void post_timer_event(nl_event_timer_internal_t *aTimer)
{
    if (nleventqueue_post_event(aTimer->mReturnQueue, (nl_event_t *)aTimer) == NLER_SUCCESS)
    {
        nl_er_atomic_inc8((int8_t*)&aTimer->mQueuedCount);
    }
}

#if NLER_FEATURE_TIMER_USING_SWTIMER
static uint32_t nl_event_timer_function(nl_swtimer_t *aTimer, void *aArg)
{
    nl_time_ms_t repeat_delay_ms;
    nl_event_timer_internal_t *timer = (nl_event_timer_internal_t*)((unsigned)aTimer - (unsigned)&(((nl_event_timer_internal_t*)0x0)->mTimer));
    if (nleventqueue_post_event_from_isr(timer->mReturnQueue, (nl_event_t*)timer) == NLER_SUCCESS)
    {
        // since we're running as an interrupt, atomic API not needed
        timer->mQueuedCount++;
    }
    if (timer->mRepeating && !timer->mCancelled)
    {
        // restart if repeating and not cancelled.
        repeat_delay_ms = (nl_time_ms_t)aArg;
    }
    else
    {
        repeat_delay_ms = 0;
    }
    return repeat_delay_ms;
}

#else /* NLER_FEATURE_TIMER_USING_SWTIMER */

#ifndef NLER_MAX_TIMER_EVENTS
/** Maximum number of simultaneous timer events. This can be increased/decreased
 * by changing this define through the build system.
 */
#define NLER_MAX_TIMER_EVENTS   4
#endif

static int sync_barrier_dummy_function(nl_event_t *aEvent, void *aClosure)
{
    return 0;
}

static nltask_t sTimerTask;

DEFINE_STACK(sTimerStack, (NLER_TASK_STACK_BASE + NLER_TIMER_STACK_SIZE));

/* this has one more event to account for the fact that it can handle an exit
 * request
 */
static nl_event_t *sQueueMemory[NLER_MAX_TIMER_EVENTS + 1];
static nl_event_timer_internal_t *sTimers[NLER_MAX_TIMER_EVENTS];
static nleventqueue_t sQueue;
static nl_time_native_t sTimeoutNative = NLER_TIMEOUT_NEVER; // FIXME: this macro has type nl_time_ms_t
static int sEnd = 0;
static int sRunning = 1;
static nl_time_native_t sTimeoutNeverNative;  // Used store nl_time_ms_to_delay_time_native(NLER_TIMEOUT_NEVER);

static void remove_timer(int aIndex)
{
    if (aIndex != (sEnd - 1))
    {
        memmove(&sTimers[aIndex], &sTimers[aIndex + 1],
                sizeof(nl_event_timer_t *) * (sEnd - (aIndex + 1)));
    }
    else
    {
        /* removing last timer, just set entry to NULL */
        sTimers[aIndex] = NULL;
    }

    sEnd--;
}

static void handle_timer_event(nl_event_timer_internal_t *aEvent)
{
    const nl_time_native_t now = nl_get_time_native();
    nl_time_native_t newtimeout = sTimeoutNeverNative;
    nl_time_native_t curtimeout = 0;

    int idx = 0;
    while (idx < sEnd)
    {
#if NLER_FEATURE_SIMULATEABLE_TIME
        nl_event_timer_internal_t *timer = sTimers[idx];
        NLER_ASSERT(timer->mLock);
        nllock_enter(timer->mLock);
#endif
        if (sTimers[idx] == aEvent)
        {
            NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) replaced\n",
                         sTimers[idx], nl_time_native_to_time_ms(sTimers[idx]->mTimeoutNative));
            aEvent = NULL;
        }
        if (sTimers[idx]->mCancelled)
        {
            NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) cancelled\n",
                         sTimers[idx], nl_time_native_to_time_ms(sTimers[idx]->mTimeoutNative));
            remove_timer(idx);
#if NLER_FEATURE_SIMULATEABLE_TIME
            nllock_exit(timer->mLock);
#endif
            continue;
        }

        if (now - sTimers[idx]->mTimeNow >= sTimers[idx]->mTimeoutNative)
        {
            NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) timedout [idx: %d (%u - %u [%u]) >= %u]\n",
                         sTimers[idx], nl_time_native_to_time_ms(sTimers[idx]->mTimeoutNative),
                         idx, now, sTimers[idx]->mTimeNow,
                         now - sTimers[idx]->mTimeNow, sTimers[idx]->mTimeoutNative);

            post_timer_event(sTimers[idx]);
            if (sTimers[idx]->mRepeating)
            {
                NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) will repeat\n",
                             sTimers[idx], nl_time_native_to_time_ms(sTimers[idx]->mTimeoutNative));
                // mTimeoutNative has an extra tick for the initial delay.
                // repeats shouldn't have that extra tick, so we just remove
                // it from the mTimeNow
                sTimers[idx]->mTimeNow = now - 1;
            }
            else
            {
                remove_timer(idx);
#if NLER_FEATURE_SIMULATEABLE_TIME
                nllock_exit(timer->mLock);
#endif
                continue;
            }
        }

        NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) participates in timeout computation\n",
                     sTimers[idx], nl_time_native_to_time_ms(sTimers[idx]->mTimeoutNative));

        if (now - sTimers[idx]->mTimeNow < sTimers[idx]->mTimeoutNative)
        {
            curtimeout = sTimers[idx]->mTimeNow + sTimers[idx]->mTimeoutNative - now;
        }
        else
        {
            // the timer is already expired. Set curtimeout = 0 to avoid
            // assigning a negative value to an unsigned.
            curtimeout = 0;
        }

        if (curtimeout < newtimeout)
        {
            newtimeout = curtimeout;
        }
        idx++;
#if NLER_FEATURE_SIMULATEABLE_TIME
        nllock_exit(timer->mLock);
#endif
    }

#if NLER_FEATURE_SIMULATEABLE_TIME
    if (aEvent && (aEvent->mHandler == sync_barrier_dummy_function))
    {
        /* Special barrier event.  Synchronize by posting back.
         */
        if (nleventqueue_post_event(aEvent->mReturnQueue, (nl_event_t *)aEvent) != NLER_SUCCESS)
        {
            // assert on failure
            NLER_ASSERT(false);
        }
        aEvent = NULL;
    }
#endif

    if (aEvent != NULL)
    {
        if (sEnd == NLER_MAX_TIMER_EVENTS)
        {
            NL_LOG(lrERTIMER, "timer: no space to add timer (%p). max of %d timers exceeded\n", aEvent, NLER_MAX_TIMER_EVENTS);

            NLER_ASSERT(sEnd < NLER_MAX_TIMER_EVENTS);
        }
        else
        {
            NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) added\n",
                         aEvent, nl_time_native_to_time_ms(aEvent->mTimeoutNative));

            sTimers[sEnd++] = aEvent;

            curtimeout = aEvent->mTimeNow + aEvent->mTimeoutNative - now;

            if (curtimeout < newtimeout)
                newtimeout = curtimeout;

        }
    }

    sTimeoutNative = newtimeout;

    NL_LOG_DEBUG(lrERTIMER, "timer: new timeout: %d\n", nl_time_native_to_time_ms(newtimeout));
}

static int nl_timer_eventhandler(nl_event_t *aEvent)
{
    int         retval = NLER_SUCCESS;

    if (aEvent != NULL)
    {
        switch (aEvent->mType)
        {
            case NL_EVENT_T_TIMER:
                handle_timer_event((nl_event_timer_internal_t *)aEvent);
                break;

            case NL_EVENT_T_EXIT:
                sRunning = 0;
                break;

            default:
                NL_LOG_DEBUG(lrERTIMER, "timer: received unexpected event of type: %d\n", aEvent->mType);
                break;
        }
    }
    else
    {
        handle_timer_event(NULL);
    }

    return retval;
}

#if NLER_FEATURE_SIMULATEABLE_TIME
/** Handle expired timer events and all system-wide events.
 *
 * @pre: Simulation time is paused
 *
 * @post: There are no unhandled events in the system. In other words, all
 * tasks which expect events are blocked awaiting an event. This is the
 * precondition used prior to advancing time.
 */
static void handle_expired_events(void)
{
    nl_event_t * ev;
    do {
        ev = nleventqueue_get_event_with_timeout(&sQueue, 0);
        nl_timer_eventhandler(ev);
    } while (ev || (nleventqueue_sim_count() > 0));
}
#endif

static void nl_timer_run_loop(void *aParams)
{
    (void) aParams;
    while (sRunning)
    {
        nl_event_t *ev;
        // sTimeoutNative is computed from valuese typically converted from MS
        // using nl_time_ms_to_delay_time_ms() already, so we don't want an extra tick
        // added by nleventqueue_get_event_with_timeout() when we convert sTimeoutNative to ms.
        // So, subtract one tick before the conversion.
        ev = nleventqueue_get_event_with_timeout(&sQueue, nl_time_native_to_time_ms(sTimeoutNative-1));

#if !defined(NLER_FEATURE_SIMULATEABLE_TIME) || !NLER_FEATURE_SIMULATEABLE_TIME
        nl_timer_eventhandler(ev);
#else
        sim_time_info_t *sti = nl_get_sim_time_info();

        if (ev == (nl_event_t*) nl_get_advance_event())
        {
            handle_expired_events();

            while (nl_get_time_native() < sti->advance_time_point)
            {
                const nl_time_native_t now = nl_get_time_native();

                nl_time_native_t candidate_time = now + sTimeoutNative;

                if (candidate_time <= sti->advance_time_point)
                {
                    sti->real_time_when_paused += sTimeoutNative;
                }
                else
                {
                    sti->real_time_when_paused += sti->advance_time_point - now;
                }

                handle_expired_events();
            }

            nleventqueue_post_event(((nl_event_timer_internal_t*)ev)->mReturnQueue, ev);
        }
        else
        {
            nl_timer_eventhandler(ev);
        }
#endif
    }
}

void nl_timer_start(nltask_priority_t aPriority)
{
    int err = nleventqueue_create(sQueueMemory, sizeof(sQueueMemory), &sQueue);
    NLER_ASSERT(err >= 0);

    sTimeoutNeverNative = nl_time_ms_to_delay_time_native(NLER_TIMEOUT_NEVER);
    sTimeoutNative = sTimeoutNeverNative;

    nltask_create(nl_timer_run_loop, "tmr", sTimerStack, sizeof(sTimerStack), aPriority, NULL, &sTimerTask);

#if NLER_FEATURE_SIMULATEABLE_TIME
    // Create a pool of locks, which we assign at timer_init.  We don't
    // want to create in timer_init in case anyone ever calls timer_init
    // more than once per timer, which could cause a leak.
    unsigned i;
    for (i = 0; i < NLER_MAX_TIMER_EVENTS; i++)
    {
        err = nllock_create(&s_lock_list[i]);
        NLER_ASSERT(err == NLER_SUCCESS);
    }
#endif
}

nleventqueue_t *nl_get_timer_queue(void)
{
    return &sQueue;
}

#endif /* NLER_FEATURE_TIMER_USING_SWTIMER */

void nl_event_timer_init(nl_event_timer_t *aTimer, nl_eventhandler_t aHandler, void *aHandlerArg, nleventqueue_t *aQueue)
{
    nl_event_timer_internal_t *timer = (nl_event_timer_internal_t*)aTimer;
    NL_INIT_EVENT(*timer, NL_EVENT_T_TIMER, aHandler, aHandlerArg);
    timer->mReturnQueue = aQueue;
    timer->mRepeating = false;
    timer->mCancelled = false;
    timer->mQueuedCount = 0;
    timer->mIgnoreCount = 0;
#if NLER_FEATURE_TIMER_USING_SWTIMER
    nl_swtimer_init(&timer->mTimer, nl_event_timer_function, NULL);
#endif
#if NLER_FEATURE_SIMULATEABLE_TIME
    timer->mLock = NULL;
#endif
#ifdef DEBUG
    timer->mTask = NULL;
#endif
}

#if NLER_FEATURE_SIMULATEABLE_TIME
/* This call makes sure that the timer task has run and
 * processed every event sent to it before this task
 * proceeds.  It works by sending a special "timer" event to the
 * timer task where the return event queue is a local one (not the
 * same as the one used for the task normally) and the
 * timer task will post it back to us as soon as it can.
 * We don't use the task's regular event queue
 * to synchronize on because that queue might
 * contain other events not intended for us.  We could use a
 * a special event with a semaphore to signal as well, but
 * semaphores aren't used in nler.
 */
static void timer_task_barrier(void)
{
    int err;
    nl_event_timer_internal_t sync_event;
    nl_event_t *queue_memory[1];
    nleventqueue_t barrier_queue;
    nl_event_t *result;

    err = nleventqueue_create(queue_memory, sizeof(queue_memory), &barrier_queue);
    NLER_ASSERT(err >= 0);

    nl_event_timer_init((nl_event_timer_t*)&sync_event, sync_barrier_dummy_function, NULL, &barrier_queue);
    sync_event.mTimeNow = 0;
    sync_event.mTimeoutNative = 0;

    err = nleventqueue_post_event(&sQueue, (nl_event_t*)&sync_event);
    NLER_ASSERT(err >= 0);
    result = nleventqueue_get_event_with_timeout(&barrier_queue, NLER_TIMEOUT_NEVER);
    NLER_ASSERT(result == (nl_event_t*)&sync_event);

    nleventqueue_destroy(&barrier_queue);
}

static void lock_timer_from_client_task(nl_event_timer_internal_t *timer)
{
    if (timer->mLock == NULL)
    {
        // find the lock to use.
        unsigned i;
        for (i = 0; i < NLER_MAX_TIMER_EVENTS; i++)
        {
            if (s_task_list[i] == NULL)
            {
                // task hasn't seen yet, record it
                s_task_list[i] = timer->mTask;
                break;
            }
            if (s_task_list[i] == timer->mTask)
            {
                // task has been seen before
                break;
            }
        }
        NLER_ASSERT(i < NLER_MAX_TIMER_EVENTS);
        timer->mLock = &s_lock_list[i];
    }
    nllock_enter(timer->mLock);
}
#endif

void nl_event_timer_start(nl_event_timer_t *aTimer, nl_time_ms_t aTimeoutMS, bool aRepeating)
{
    nl_event_timer_internal_t *timer = (nl_event_timer_internal_t*)aTimer;
#if !NLER_FEATURE_TIMER_USING_SWTIMER
    int err;
#endif

#ifdef DEBUG
    if (timer->mTask == NULL)
    {
        timer->mTask = nltask_get_current();
    }
    else
    {
        NLER_ASSERT(timer->mTask == nltask_get_current());
    }
#endif

    // cancel in case it was already running
    nl_event_timer_cancel(aTimer);

#if NLER_FEATURE_SIMULATEABLE_TIME
    lock_timer_from_client_task(timer);
#endif

    // record that all queued events should be ignored so
    // that nl_event_timer_is_valid() returns false for
    // all the events currently in the queue.
    timer->mIgnoreCount = timer->mQueuedCount;

    timer->mRepeating = aRepeating;
    timer->mCancelled = false;

#if NLER_FEATURE_TIMER_USING_SWTIMER
    // we store the aTimeoutMS as the func arg in case we want repeating behavior.
    nl_swtimer_init(&timer->mTimer, nl_event_timer_function, (void*)aTimeoutMS);
    nl_swtimer_start(&timer->mTimer, aTimeoutMS);
#else
    NLER_ASSERT(sRunning);
    timer->mTimeNow = nl_get_time_native();
    timer->mTimeoutNative = nl_time_ms_to_delay_time_native(aTimeoutMS);
#if NLER_FEATURE_SIMULATEABLE_TIME
    nllock_exit(timer->mLock);
#endif
    // inform the timer task so it can remove the timer from it's list.
    err = nleventqueue_post_event(&sQueue, (nl_event_t *)timer);
    NLER_ASSERT(err >= 0);
#if NLER_FEATURE_SIMULATEABLE_TIME
    timer_task_barrier();
#endif
#endif
}

void nl_event_timer_cancel(nl_event_timer_t *aTimer)
{
    nl_event_timer_internal_t *timer = (nl_event_timer_internal_t*)aTimer;

#ifdef DEBUG
    NLER_ASSERT(timer->mTask == NULL || timer->mTask == nltask_get_current());
#endif

#if NLER_FEATURE_SIMULATEABLE_TIME
    lock_timer_from_client_task(timer);
#endif
    timer->mCancelled = true;
#if NLER_FEATURE_TIMER_USING_SWTIMER
    // cancel timer immediately so that the timer structure
    // is not referenced after this function call returns,
    // in case the structure is not static
    (void)nl_swtimer_cancel(&timer->mTimer);
#else // NLER_FEATURE_TIMER_USING_SWTIMER
    // post to the timer task to have the cancel processed
    // immediately (the timer task is always higher priority)
    // so by time we return from this function, we know
    // the timer is no longer in the sTimers array
#if !NLER_FEATURE_SIMULATEABLE_TIME
    NLER_ASSERT(nltask_get_priority(&sTimerTask) > nltask_get_priority(nltask_get_current()));
#endif
    nleventqueue_post_event(&sQueue, NULL);
#if NLER_FEATURE_SIMULATEABLE_TIME
    nllock_exit(timer->mLock);
    timer_task_barrier();
#endif // NLER_FEATURE_SIMULATEABLE_TIME
#endif // NLER_FEATURE_TIMER_USING_SWTIMER
}

// This is called by the receive thread event handler when it has
// received the timer event.  Check if the timer event is still
// valid, or if it has been cancelled or restarted.  It must be
// called exactly once per dequeued timer event.
// A cancelled and not restarted timer can be identified
// by the mCancelled flag.
// A timer event on the queue is invalid if it was restarted,
// so mCancelled is false, but the mIgnoreCount will be > 0.
bool nl_event_timer_is_valid(nl_event_timer_t *aTimer)
{
    bool retval = false;
    nl_event_timer_internal_t *timer = (nl_event_timer_internal_t*)aTimer;

#ifdef DEBUG
    NLER_ASSERT(timer->mTask == nltask_get_current());
#endif

#if NLER_FEATURE_SIMULATEABLE_TIME
    lock_timer_from_client_task(timer);
#endif

    NLER_ASSERT(timer->mQueuedCount > 0);
    nl_er_atomic_dec8((int8_t*)&timer->mQueuedCount);
    if (timer->mIgnoreCount > 0)
    {
        timer->mIgnoreCount--;
    }
    else
    {
        if (timer->mCancelled == false)
        {
            retval = true;
        }
    }
#if NLER_FEATURE_SIMULATEABLE_TIME
    nllock_exit(timer->mLock);
#endif
    return retval;
}

#endif // NLER_FEATURE_EVENT_TIMER
