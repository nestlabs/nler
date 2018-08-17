/*
 *
 *    Copyright (c) 2014-2018 Nest Labs, Inc.
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
 *      This file implements a unit test for the NLER timer interfaces.
 *
 *      In theory, this test could run forever, with the primary timer
 *      expiring and causing a post of the other three one-shot
 *      timers. In practice, the test is bounded with the primary
 *      timer repeating an expected number of times and the subsequent
 *      one-shot timers triggered and fired an expected number of
 *      times.
 *
 *      The test succeeds when the actual number of events meets the
 *      expected number for both triggered (sent) and fired (received)
 *      timer events.
 *
 *      The test fails if more timer events are triggered (sent) and
 *      fired (received) expected or if an errant event occurs.
 *
 */

#include <nlertimer.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 1

#include <nlerassert.h>
#include <nlererror.h>
#include <nlereventqueue.h>
#include <nlerinit.h>
#include <nlerlog.h>
#include <nlertask.h>

/*
 * Preprocessor Defitions
 */

#define kTHREAD_MAIN_SLEEP_MS       241

/**
 *  NOTE: this example uses four timers which is the default built for
 *  the runtime if not overridden by the application. If fewer than
 *  four (4) timers are available, do not expect the example to fully
 *  function.
 */

#define kTIMER_1_ID                   0
#define kTIMER_2_ID                   1
#define kTIMER_3_ID                   2
#define kTIMER_4_ID                   3

#define kTIMER_IDS_MAX               (kTIMER_4_ID + 1)

#if (NLER_MAX_TIMER_EVENTS) < (kTIMER_IDS_MAX)
#warning kTIMER_IDS_MAX exceeds NLER_MAX_TIMER_EVENTS
#endif /* NLER_MAX_TIMER_EVENTS > kTIMER_IDS_MAX */

#define kTIMER_1_MS                 499
#define kTIMER_2_MS                 241
#define kTIMER_3_MS                 113
#define kTIMER_4_MS                 181

/**
 *  Expected number of timer events to be produced and consumed for
 *  each timer.
 *
 *  Timer 1 is a repeating timer, so it will only be sent
 *  once. Receipt of it, in turn, fires Timer 2, 3, and 4, each time
 *  it is received.
 */

/* Timer 1 is setup once as a repeating timer, and allowed to expire
 * 17 times
 */
#define kTIMER_1_ID_EXPECTED_TX_EVENTS  1
#define kTIMER_1_ID_EXPECTED_RX_EVENTS  17

/* Timer 2 is setup for each expiry of Timer 1 as a one-shot timer
 */
#define kTIMER_2_ID_EXPECTED_TX_EVENTS  kTIMER_1_ID_EXPECTED_RX_EVENTS
#define kTIMER_2_ID_EXPECTED_RX_EVENTS  kTIMER_2_ID_EXPECTED_TX_EVENTS

/* Timer 3 is setup for each expiry of Timer 2 as a one-shot timer
 */
#define kTIMER_3_ID_EXPECTED_TX_EVENTS  kTIMER_2_ID_EXPECTED_TX_EVENTS
#define kTIMER_3_ID_EXPECTED_RX_EVENTS  kTIMER_3_ID_EXPECTED_TX_EVENTS

/* Timer 4 is setup for each expiry of Timer 3 as a one-shot timer
 */
#define kTIMER_4_ID_EXPECTED_TX_EVENTS  kTIMER_3_ID_EXPECTED_RX_EVENTS
#define kTIMER_4_ID_EXPECTED_RX_EVENTS  kTIMER_4_ID_EXPECTED_TX_EVENTS

/*
 * Type Defintions
 */

typedef struct timer_stats_s
{
    int32_t                  mExpect;
    int32_t                  mActual;
} timer_stats_t;

typedef struct timer_subpub_stats_s
{
    timer_stats_t            mSent;
    timer_stats_t            mReceived;
} timer_subpub_stats_t;

typedef struct taskData_s
{
    nleventqueue_t        *mTimer;
    nleventqueue_t         mQueue;
    timer_subpub_stats_t   mStats[kTIMER_IDS_MAX];
    bool                   mFailed;
    bool                   mSucceeded;
} taskData_t;

typedef struct nl_test_event_timer_s
{
    nl_event_timer_t    mTimerEvent;
    int                 mID;
} nl_test_event_timer_t;

/*
 * Forward Declarations
 */

static int nl_test_timer_eventhandler(nl_event_t *aEvent, void *aClosure);

/*
 * Global Variables
 */

static nltask_t taskTestDriver;
static DEFINE_STACK(stackTestDriver, NLER_TASK_STACK_BASE + 128);

static nl_test_event_timer_t sTimerEv1 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, nl_test_timer_eventhandler, NULL),
        NULL, 0, 0, 0, 0
    },
    kTIMER_1_ID
};

static nl_test_event_timer_t sTimerEv2 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, nl_test_timer_eventhandler, NULL),
        NULL, 0, 0, 0, 0
    },
    kTIMER_2_ID
};

static nl_test_event_timer_t sTimerEv3 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, nl_test_timer_eventhandler, NULL),
        NULL, 0, 0, 0, 0
    },
    kTIMER_3_ID
};

static nl_test_event_timer_t sTimerEv4 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, nl_test_timer_eventhandler, NULL),
        NULL, 0, 0, 0, 0
    },
    kTIMER_4_ID
};

static int nl_test_maybe_post_timer(nleventqueue_t *aQueue, nl_test_event_timer_t *aTimerEvent, nl_time_ms_t aTimeoutMS, volatile timer_stats_t *aOutSendStats)
{
    int retval = NLER_SUCCESS;

    if (aOutSendStats->mActual < aOutSendStats->mExpect)
    {
        nl_init_event_timer(&aTimerEvent->mTimerEvent, aTimeoutMS);

        retval = nleventqueue_post_event(aQueue, (nl_event_t *)aTimerEvent);
        NLER_ASSERT(retval == NLER_SUCCESS);

        aOutSendStats->mActual++;
    }

    return retval;
}

static int nl_test_timer_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    const nltask_t                *curtask = nltask_get_current();
    nl_test_event_timer_t         *timer = (nl_test_event_timer_t *)aEvent;
    taskData_t                    *data = (taskData_t *)aClosure;
    int                            retval;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' timeout: %d, time: %u\n", nltask_get_name(curtask), timer->mID + 1, nl_get_time_native());

    switch (timer->mID)
    {
        case kTIMER_1_ID:
            /* Timer 1 expiration triggers timer 2.
             */
            data->mStats[timer->mID].mReceived.mActual++;

            if (data->mStats[timer->mID].mReceived.mActual == data->mStats[timer->mID].mReceived.mExpect)
            {
                timer->mTimerEvent.mFlags |= NLER_TIMER_FLAG_CANCELLED;
            }

            retval = nl_test_maybe_post_timer(data->mTimer, &sTimerEv2, kTIMER_2_MS, &data->mStats[sTimerEv2.mID].mSent);
            break;

        case kTIMER_2_ID:
            /* Timer 2 expiration triggers timer 3.
             */

            data->mStats[timer->mID].mReceived.mActual++;

            retval = nl_test_maybe_post_timer(data->mTimer, &sTimerEv3, kTIMER_3_MS, &data->mStats[sTimerEv3.mID].mSent);
            break;

        case kTIMER_3_ID:
            /* Timer 3 expiration triggers timer 4.
             */

            data->mStats[timer->mID].mReceived.mActual++;

            retval = nl_test_maybe_post_timer(data->mTimer, &sTimerEv4, kTIMER_4_MS, &data->mStats[sTimerEv4.mID].mSent);
            break;

        case kTIMER_4_ID:
            data->mStats[timer->mID].mReceived.mActual++;

            retval = NLER_SUCCESS;
            break;

        default:
            NL_LOG_CRIT(lrTEST, "'%s' bad timer id: %d\n", nltask_get_name(curtask), timer->mID);
            data->mFailed = true;

            retval = NLER_ERROR_BAD_STATE;
            break;
    }

    return retval;
}

/* this is a backstop event handler that will be called when
 * an event comes in with no predetermined destination.
 */

static int nl_test_default_handler(nl_event_t *aEvent, void *aClosure)
{
    const nltask_t      *curtask = nltask_get_current();
    taskData_t          *taskData = (taskData_t *)aClosure;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d -- unexpected\n", nltask_get_name(curtask), aEvent->mType);

    taskData->mFailed = true;

    return 0;
}

static void check_succeeded_or_failed(const nltask_t *aTask, uint8_t aID, volatile const timer_subpub_stats_t *aStats, bool *aOutSucceeded, bool *aOutFailed)
{
    NL_LOG_DEBUG(lrTEST, "%2u  %8d  %8d    %8d  %8d\n",
                 aID,
                 aStats->mSent.mExpect,
                 aStats->mSent.mActual,
                 aStats->mReceived.mExpect,
                 aStats->mReceived.mActual);

    if ((aStats->mSent.mActual > aStats->mSent.mExpect) ||
        (aStats->mReceived.mActual > aStats->mReceived.mExpect))
        *aOutFailed = true;
    else if ((aStats->mSent.mActual < aStats->mSent.mExpect) ||
             (aStats->mReceived.mActual < aStats->mReceived.mExpect))
        *aOutSucceeded = false;
}

static bool is_testing(volatile const taskData_t *aTaskData)
{
    bool retval;

    retval = (!aTaskData->mFailed && !aTaskData->mSucceeded);

    return (retval);
}

static void taskEntryTestDriver(void *aParams)
{
    const nltask_t            *curtask = nltask_get_current();
    taskData_t                *data = (taskData_t *)aParams;
    int                        status;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %08x)\n", nltask_get_name(curtask), &data->mQueue);

    sTimerEv1.mTimerEvent.mFlags = NLER_TIMER_FLAG_REPEAT;
    sTimerEv1.mTimerEvent.mHandlerClosure = (void *)data;
    sTimerEv1.mTimerEvent.mReturnQueue = &data->mQueue;

    sTimerEv2.mTimerEvent.mHandlerClosure = (void *)data;
    sTimerEv2.mTimerEvent.mReturnQueue = &data->mQueue;

    sTimerEv3.mTimerEvent.mHandlerClosure = (void *)data;
    sTimerEv3.mTimerEvent.mReturnQueue = &data->mQueue;

    sTimerEv4.mTimerEvent.mHandlerClosure = (void *)data;
    sTimerEv4.mTimerEvent.mReturnQueue = &data->mQueue;

    nl_init_event_timer(&sTimerEv1.mTimerEvent, kTIMER_1_MS);

    /* This provides the initial, repeating "kick" of timer 1
     */

    status = nleventqueue_post_event(data->mTimer, (nl_event_t *)&sTimerEv1);
    NLER_ASSERT(status == NLER_SUCCESS);

    data->mStats[sTimerEv1.mID].mSent.mActual++;

    while (is_testing(data))
    {
        nl_event_t  *ev;
        size_t       i;
        bool         succeeded = true;
        bool         failed = false;

        ev = nleventqueue_get_event(&data->mQueue);

        nl_dispatch_event(ev, nl_test_default_handler, NULL);

        NL_LOG_DEBUG(lrTEST,
                     "ID  Sent                  Received\n"
                     "    Expected   Actual     Expected   Actual\n");

        for (i = 0; i < kTIMER_IDS_MAX; i++)
        {
            const timer_subpub_stats_t *lStats = &data->mStats[i];

            check_succeeded_or_failed(curtask, i + 1, lStats, &succeeded, &failed);
        }

        NL_LOG_CRIT(lrTEST, "'%s' %s number of expected timer events\n", nltask_get_name(curtask), ((succeeded) ? "successfully sent/received" : ((failed) ? "failed to send/receive" : "has not yet sent/received")));

        if (succeeded || failed)
        {
            /* The test is done (success or failure), the publisher
             * may now shut down the timers.
             */
            sTimerEv1.mTimerEvent.mFlags |= NLER_TIMER_FLAG_CANCELLED;
            sTimerEv2.mTimerEvent.mFlags |= NLER_TIMER_FLAG_CANCELLED;
            sTimerEv3.mTimerEvent.mFlags |= NLER_TIMER_FLAG_CANCELLED;
            sTimerEv4.mTimerEvent.mFlags |= NLER_TIMER_FLAG_CANCELLED;

            if (succeeded)
                data->mSucceeded = true;

            if (failed)
                data->mFailed = true;
        }
    }
}

static bool was_successful(volatile const taskData_t *aTaskData)
{
    bool retval = false;

    if (aTaskData->mFailed)
        retval = false;
    else if (aTaskData->mSucceeded)
        retval = true;

    return retval;
}

bool nler_timer_test(nleventqueue_t *aTimerQueue)
{
    taskData_t           taskData;
    nl_event_t          *queuememA[50];
    int                  status;
    bool                 retval = true;

    taskData.mFailed    = false;
    taskData.mSucceeded = false;

    taskData.mStats[kTIMER_1_ID].mSent.mActual = 0;
    taskData.mStats[kTIMER_1_ID].mSent.mExpect = kTIMER_1_ID_EXPECTED_TX_EVENTS;
    taskData.mStats[kTIMER_1_ID].mReceived.mActual = 0;
    taskData.mStats[kTIMER_1_ID].mReceived.mExpect = kTIMER_1_ID_EXPECTED_RX_EVENTS;

    taskData.mStats[kTIMER_2_ID].mSent.mActual = 0;
    taskData.mStats[kTIMER_2_ID].mSent.mExpect = kTIMER_2_ID_EXPECTED_TX_EVENTS;
    taskData.mStats[kTIMER_2_ID].mReceived.mActual = 0;
    taskData.mStats[kTIMER_2_ID].mReceived.mExpect = kTIMER_2_ID_EXPECTED_RX_EVENTS;

    taskData.mStats[kTIMER_3_ID].mSent.mActual = 0;
    taskData.mStats[kTIMER_3_ID].mSent.mExpect = kTIMER_3_ID_EXPECTED_TX_EVENTS;
    taskData.mStats[kTIMER_3_ID].mReceived.mActual = 0;
    taskData.mStats[kTIMER_3_ID].mReceived.mExpect = kTIMER_3_ID_EXPECTED_RX_EVENTS;

    taskData.mStats[kTIMER_4_ID].mSent.mActual = 0;
    taskData.mStats[kTIMER_4_ID].mSent.mExpect = kTIMER_4_ID_EXPECTED_TX_EVENTS;
    taskData.mStats[kTIMER_4_ID].mReceived.mActual = 0;
    taskData.mStats[kTIMER_4_ID].mReceived.mExpect = kTIMER_4_ID_EXPECTED_RX_EVENTS;

    taskData.mTimer = aTimerQueue;
    status = nleventqueue_create(queuememA, sizeof(queuememA), &taskData.mQueue);
    NLER_ASSERT(status == NLER_SUCCESS);

    nltask_create(taskEntryTestDriver, "Test Driver", stackTestDriver, sizeof(stackTestDriver), NLER_TASK_PRIORITY_NORMAL, (void *)&taskData, &taskTestDriver);

    while (is_testing(&taskData))
    {
        nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);
    }

    retval = was_successful(&taskData);

    return retval;
}

static void nler_test_timer_stop(nleventqueue_t *aTimerQueue)
{
    static const nl_event_timer_t sTimerStopEvent = { NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0) };

    int status;

    status = nleventqueue_post_event(aTimerQueue, (nl_event_t *)&sTimerStopEvent);
    NLER_ASSERT(status == NLER_SUCCESS);
}

int main(int argc, char **argv)
{
    bool             status;
    nleventqueue_t  *queue;

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main\n");

    queue = nl_timer_start(NLER_TASK_PRIORITY_HIGH + 1);
    NLER_ASSERT(queue != NULL);

    nl_er_start_running();

    status = nler_timer_test(queue);

    nler_test_timer_stop(queue);

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (status ? EXIT_SUCCESS : EXIT_FAILURE);
}
