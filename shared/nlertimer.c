/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      interfaces.
 *
 */

#if NLER_FEATURE_EVENT_TIMER != 1
#include "nlertimer.h"

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

#if NLER_FEATURE_SIMULATEABLE_TIME
#include "nlereventqueue_sim.h"
#include "nlertimer_sim.h"
#endif

void nl_init_event_timer(nl_event_timer_t *aTimer, nl_time_ms_t aTimeoutMS)
{
    aTimer->mTimeoutMS = aTimeoutMS;
    aTimer->mTimeNow = nl_get_time_native();
    aTimer->mTimeoutNative = nl_time_ms_to_delay_time_native(aTimeoutMS);
}

static nl_task_t sTimerTask;

DEFINE_STACK(sTimerStack, (NLER_TASK_STACK_BASE + NLER_TIMER_STACK_SIZE));

/* this has one more event to account for the fact that it can handle an exit
 * request
 */
static nl_event_t *sQueueMemory[NLER_MAX_TIMER_EVENTS + 1];
static nl_event_timer_t *sTimers[NLER_MAX_TIMER_EVENTS];
static nl_eventqueue_t sQueue = NULL;
static nl_time_native_t sTimeoutNative;
#if NLER_FEATURE_WAKE_TIMER
static nl_time_native_t sMinWakeTimeNative;
#endif // NLER_FEATURE_WAKE_TIMER
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

    sEnd--;
}

static void handle_timer_event(nl_event_timer_t *aEvent)
{
    const nl_time_native_t now = nl_get_time_native();
    nl_time_native_t newtimeout = sTimeoutNeverNative;
#if NLER_FEATURE_WAKE_TIMER
    nl_time_native_t newwaketime = sTimeoutNeverNative;
#endif // NLER_FEATURE_WAKE_TIMER
    nl_time_native_t curtimeout = 0;

    int idx = 0;
    while (idx < sEnd)
    {
        if (sTimers[idx] == aEvent)
        {
            if (sTimers[idx]->mFlags & NLER_TIMER_FLAG_DISPLACE)
            {
                nl_eventqueue_post_event(sTimers[idx]->mReturnQueue, (nl_event_t *)sTimers[idx]);
            }
            NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) replaced\n", sTimers[idx], sTimers[idx]->mTimeoutMS);
            aEvent = NULL;
        }

        if (sTimers[idx]->mFlags & NLER_TIMER_FLAG_CANCEL_ECHO)
        {
            NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) cancelled with echo\n", sTimers[idx], sTimers[idx]->mTimeoutMS);
            nl_eventqueue_post_event(sTimers[idx]->mReturnQueue, (nl_event_t *)sTimers[idx]);
            remove_timer(idx);
            continue;
        }
        else if (sTimers[idx]->mFlags & NLER_TIMER_FLAG_CANCELLED)
        {
            NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) cancelled\n", sTimers[idx], sTimers[idx]->mTimeoutMS);
            remove_timer(idx);
            continue;
        }

        if (now - sTimers[idx]->mTimeNow >= sTimers[idx]->mTimeoutNative)
        {
            NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) timedout [idx: %d (%u - %u [%u]) >= %u]\n",
                         sTimers[idx], sTimers[idx]->mTimeoutMS, idx, now, sTimers[idx]->mTimeNow,
                         now - sTimers[idx]->mTimeNow, sTimers[idx]->mTimeoutNative);

            nl_eventqueue_post_event(sTimers[idx]->mReturnQueue, (nl_event_t *)sTimers[idx]);

            if (sTimers[idx]->mFlags & NLER_TIMER_FLAG_REPEAT)
            {
                NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) will repeat\n", sTimers[idx], sTimers[idx]->mTimeoutMS);
                sTimers[idx]->mTimeNow = now;
            }
            else
            {
                remove_timer(idx);
                continue;
            }
        }
#if NLER_FEATURE_WAKE_TIMER
        NL_LOG_DEBUG(lrERTIMER, "timer: %s timer %p (%d) participates in timeout computation\n",
                (sTimers[idx]->mFlags & NLER_TIMER_FLAG_WAKE) ? "wake" : "", sTimers[idx], sTimers[idx]->mTimeoutMS);
#else
        NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) participates in timeout computation\n",
                sTimers[idx], sTimers[idx]->mTimeoutMS);
#endif // NLER_FEATURE_WAKE_TIMER

        curtimeout = sTimers[idx]->mTimeNow + sTimers[idx]->mTimeoutNative - now;

        if (curtimeout < newtimeout)
            newtimeout = curtimeout;

#if NLER_FEATURE_WAKE_TIMER
        if (sTimers[idx]->mFlags & NLER_TIMER_FLAG_WAKE)
        {
            nl_time_native_t curwaketime = sTimers[idx]->mTimeNow + sTimers[idx]->mTimeoutNative;

            if (curwaketime < newwaketime)
            {
                newwaketime = curwaketime;
            }
        }
#endif // NLER_FEATURE_WAKE_TIMER
        idx++;
    }

    if (aEvent != NULL)
    {

        NLER_ASSERT(sEnd < NLER_MAX_TIMER_EVENTS);

        NL_LOG_DEBUG(lrERTIMER, "timer: timer %p (%d) added\n", aEvent, aEvent->mTimeoutMS);

        sTimers[sEnd++] = aEvent;

        if (now - aEvent->mTimeNow < aEvent->mTimeoutNative)
        {
            curtimeout = aEvent->mTimeNow + aEvent->mTimeoutNative - now;
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

#if NLER_FEATURE_WAKE_TIMER
        if (aEvent->mFlags & NLER_TIMER_FLAG_WAKE)
        {
            nl_time_native_t curwaketime = (nl_time_native_t)(aEvent->mTimeNow + aEvent->mTimeoutNative);

            if (curwaketime < newwaketime)
            {
                newwaketime = curwaketime;
            }
        }
#endif
    }
  

    sTimeoutNative = newtimeout;
#if NLER_FEATURE_WAKE_TIMER
    sMinWakeTimeNative = newwaketime;
#endif // NLER_FEATURE_WAKE_TIMER

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
                handle_timer_event((nl_event_timer_t *)aEvent);
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
        ev = nl_eventqueue_get_event_with_timeout(sQueue, 0);
        nl_timer_eventhandler(ev);
    } while (ev || (nl_eventqueue_sim_count() > 0));
}
#endif

static void nl_timer_run_loop(void *aParams)
{
    (void) aParams;
    while (sRunning)
    {
        nl_event_t *ev;

        ev = nl_eventqueue_get_event_with_timeout(sQueue, nl_time_native_to_time_ms(sTimeoutNative));

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

            nl_eventqueue_post_event(nl_get_advance_event()->mReturnQueue,
                    (nl_event_t *) nl_get_advance_event());
        }
        else
        {
            nl_timer_eventhandler(ev);
        }
#endif
    }
}

int nl_start_event_timer(nl_event_timer_t *aTimer)
{
    int retval = NLER_ERROR_INIT;

    if (sQueue && sRunning)
    {
        aTimer->mFlags &= ~(NLER_TIMER_FLAG_ANY_CANCEL);

        retval = nl_eventqueue_post_event(sQueue, (nl_event_t *)aTimer);
    }

    return retval;
}

// Broken out to support unit test
static void timer_init(void)
{
    sTimeoutNeverNative = nl_time_ms_to_delay_time_native(NLER_TIMEOUT_NEVER);
    sTimeoutNative = sTimeoutNeverNative;
#if NLER_FEATURE_WAKE_TIMER
    sMinWakeTimeNative = sTimeoutNeverNative;
#endif // NLER_FEATURE_WAKE_TIMER
}

nl_eventqueue_t nl_timer_start(nl_task_priority_t aPriority)
{
    sQueue = nl_eventqueue_create(sQueueMemory, sizeof(sQueueMemory));

    timer_init();

    nl_task_create(nl_timer_run_loop, "tmr", sTimerStack, sizeof(sTimerStack), aPriority, NULL, &sTimerTask);

    return sQueue;
}

nl_eventqueue_t nl_get_timer_queue(void)
{
    return sQueue;
}

nl_time_native_t nl_get_wake_time(void)
{
#if NLER_FEATURE_WAKE_TIMER
    return sMinWakeTimeNative;
#else
    return sTimeoutNative;
#endif // NLER_FEATURE_WAKE_TIMER
}
#endif /* NLER_FEATURE_EVENT_TIMER != 1 */
