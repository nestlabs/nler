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
 *      This file implements a unit test for the NLER pooled event
 *      interfaces.
 *
 *      In theory, this test could run forever, with the two symmetric
 *      threads producing and consuming pooled events for one another
 *      indefinitely. In practice, the test is bounded with a number
 *      of expected events produced and consumed for each thread.
 *
 *      The test succeeds when the actual number of produced/consumed
 *      events meets the expected number for each thread.
 *
 *      The test fails if more events are produced or consumed than
 *      expected or if an errant event occurs.
 *
 */

#include <nlereventpooled.h>

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

#define kTHREAD_MAIN_SLEEP_MS            241

/**
 *  Expected number of events to be produced and consumed for
 *  each symmetric task.
 *
 */
#define kTASK_A_ID_EXPECTED_TX_EVENTS  20011
#define kTASK_B_ID_EXPECTED_TX_EVENTS  kTASK_A_ID_EXPECTED_TX_EVENTS

#define kTASK_A_ID_EXPECTED_RX_EVENTS  kTASK_A_ID_EXPECTED_TX_EVENTS
#define kTASK_B_ID_EXPECTED_RX_EVENTS  kTASK_B_ID_EXPECTED_TX_EVENTS

/*
 * Type Definitions
 */

typedef struct globalData_s
{
    nlevent_pool_t                mEventPool;
} globalData_t;

typedef struct event_stats_s
{
    int32_t                       mExpect;
    int32_t                       mActual;
} event_stats_t;

typedef struct event_task_stats_s
{
    event_stats_t                 mSent;
    event_stats_t                 mReceived;
} event_task_stats_t;

typedef struct taskData_s
{
    globalData_t                 *mGlobals;
    nleventqueue_t                mMyQueue;
    nleventqueue_t                mPeerQueue;
    event_task_stats_t            mMyStats;
    event_task_stats_t           *mPeerStats;
    bool                          mSucceeded;
    bool                          mFailed;
} taskData_t;

/*
 * Global Variables
 */

static nltask_t taskA;
static nltask_t taskB;
static DEFINE_STACK(stackA, NLER_TASK_STACK_BASE + 96);
static DEFINE_STACK(stackB, NLER_TASK_STACK_BASE + 96);

static uint8_t sPooledEvents[sizeof(nlevent_pooled_t) * 8];

static int nl_test_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    const nltask_t      *curtask = nltask_get_current();
    taskData_t          *data = (taskData_t *)aClosure;

    (void)curtask;

    NL_LOG_DEBUG(lrTEST, "'%s' got event type: %d\n", nltask_get_name(curtask), aEvent->mType);

    switch (aEvent->mType)
    {
        case NL_EVENT_T_POOLED:
        {
            nlevent_pooled_t   *ev = (nlevent_pooled_t *)aEvent;

            NL_LOG_DEBUG(lrTEST, "'%s' got event_pooled: %p, %p, %p, %p\n", nltask_get_name(curtask), ev->mHandler, ev->mHandlerClosure, ev->mReturnQueue, ev->mPayload);

            /* The received event THIS task is receiving contains the
             * task data from its peer. Consequently, when
             * incrementing the statistics, use THAT task's peer
             * statistics, which correspond to THIS task's statistics.
             */

            data->mPeerStats->mReceived.mActual++;

            nlevent_pool_recycle_event(&data->mGlobals->mEventPool, ev);

            break;
        }

        default:
            data->mFailed = true;
            break;
    }

    return 0;
}

static int nl_test_maybe_post_pooled_event(taskData_t *aData)
{
    int retval = NLER_SUCCESS;

    if (aData->mMyStats.mSent.mActual < aData->mMyStats.mSent.mExpect)
    {
        nlevent_pooled_t *ev;

        ev = nlevent_pool_get_event(&aData->mGlobals->mEventPool);

        if (ev != NULL)
        {
            NL_INIT_EVENT(*ev, NL_EVENT_T_POOLED, nl_test_eventhandler, (void *)aData);
            ev->mReturnQueue = &aData->mMyQueue;
            ev->mPayload = nltask_get_current();

            retval = nleventqueue_post_event(&aData->mPeerQueue, (nl_event_t *)ev);
            NLER_ASSERT(retval == NLER_SUCCESS);

            aData->mMyStats.mSent.mActual++;
        }
    }

    return retval;
}


static void check_succeeded_or_failed(const nltask_t *aTask, volatile taskData_t *aTaskData)
{
    volatile const event_task_stats_t *lStats = &aTaskData->mMyStats;
    const char *lName = nltask_get_name(aTask);
    bool  succeeded = true;
    bool  failed = false;

    (void)lName;

    NL_LOG_DEBUG(lrTEST,
                 "Task  Sent                  Received\n"
                 "      Expected   Actual     Expected   Actual\n"
                 "%4s  %8d  %8d    %8d  %8d\n",
                 lName,
                 lStats->mSent.mExpect,
                 lStats->mSent.mActual,
                 lStats->mReceived.mExpect,
                 lStats->mReceived.mActual);

    if ((lStats->mSent.mActual > lStats->mSent.mExpect) ||
        (lStats->mReceived.mActual > lStats->mReceived.mExpect))
        failed = true;
    else if ((lStats->mSent.mActual < lStats->mSent.mExpect) ||
             (lStats->mReceived.mActual < lStats->mReceived.mExpect))
        succeeded = false;

    NL_LOG_DEBUG(lrTEST, "'%s' %s number of expected events\n", lName, ((succeeded) ? "successfully sent/received" : ((failed) ? "failed to send/receive" : "has not yet sent/received")));

    if (succeeded)
        aTaskData->mSucceeded = true;

    if (failed)
        aTaskData->mFailed = true;
}

static void taskEntry(void *aParams)
{
    nltask_t             *curtask = nltask_get_current();
    taskData_t           *data = (taskData_t *)aParams;

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %p)\n", nltask_get_name(curtask), data->mMyQueue);

    while (!data->mFailed && !data->mSucceeded)
    {
        nlevent_pooled_t   *ev;
        int                 status;

        status = nl_test_maybe_post_pooled_event(data);
        NLER_ASSERT(status == NLER_SUCCESS);

        ev = (nlevent_pooled_t *)nleventqueue_get_event(&data->mMyQueue);

        nl_dispatch_event((nl_event_t *)ev, nl_test_eventhandler, NULL);

        check_succeeded_or_failed(curtask, data);
    }
}

/**
 *  Determine whether or not the main thread should continue to wait
 *  for the test to complete.
 *
 *  The main thread should wait for testing until:
 *
 *    * Either thread failed (mFailed == true).
 *    * Both threads succeeded (mSucceeded == true).
 *
 */
static bool is_testing(volatile const taskData_t *aTaskA,
                       volatile const taskData_t *aTaskB)
{
    bool retval = true;

    if ((aTaskA->mFailed || aTaskB->mFailed) ||
        (aTaskA->mSucceeded && aTaskB->mSucceeded))
    {
        retval = false;
    }

    return retval;
}

static bool was_successful(volatile const taskData_t *aTaskA,
                           volatile const taskData_t *aTaskB)
{
    bool retval = false;

    if (aTaskA->mFailed || aTaskB->mFailed)
        retval = false;
    else if (aTaskA->mSucceeded && aTaskB->mSucceeded)
        retval = true;

    return retval;
}

bool nler_event_pool_test(void)
{
    globalData_t          globalData;
    taskData_t            dataA;
    taskData_t            dataB;
    nl_event_t           *queuememA[50];
    nl_event_t           *queuememB[50];
    int                   status;
    bool                  retval;


    status = nlevent_pool_create(sPooledEvents, sizeof(sPooledEvents), &globalData.mEventPool);
    NLER_ASSERT(status == NLER_SUCCESS);

    dataA.mGlobals = &globalData;
    status = nleventqueue_create(queuememA, sizeof(queuememA), &dataA.mMyQueue);
    NLER_ASSERT(status == NLER_SUCCESS);

    dataB.mGlobals = &globalData;
    status = nleventqueue_create(queuememB, sizeof(queuememB), &dataB.mMyQueue);
    NLER_ASSERT(status == NLER_SUCCESS);

    dataA.mPeerQueue = dataB.mMyQueue;
    dataB.mPeerQueue = dataA.mMyQueue;

    dataA.mMyStats.mSent.mExpect = kTASK_A_ID_EXPECTED_TX_EVENTS;
    dataA.mMyStats.mSent.mActual = 0;
    dataA.mMyStats.mReceived.mExpect = kTASK_A_ID_EXPECTED_RX_EVENTS;
    dataA.mMyStats.mReceived.mActual = 0;
    dataA.mPeerStats = &dataB.mMyStats;
    dataA.mSucceeded = false;
    dataA.mFailed = false;

    dataB.mMyStats.mSent.mExpect = kTASK_B_ID_EXPECTED_TX_EVENTS;
    dataB.mMyStats.mSent.mActual = 0;
    dataB.mMyStats.mReceived.mExpect = kTASK_B_ID_EXPECTED_RX_EVENTS;
    dataB.mMyStats.mReceived.mActual = 0;
    dataB.mPeerStats = &dataA.mMyStats;
    dataB.mSucceeded = false;
    dataB.mFailed = false;

    nltask_create(taskEntry, "A", stackA, sizeof(stackA), NLER_TASK_PRIORITY_NORMAL, (void *)&dataA, &taskA);
    nltask_create(taskEntry, "B", stackB, sizeof(stackB), NLER_TASK_PRIORITY_NORMAL, (void *)&dataB, &taskB);

    while (is_testing(&dataA, &dataB))
    {
        nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);
    }

    nlevent_pool_destroy(&globalData.mEventPool);

    retval = was_successful(&dataA, &dataB);

    return retval;
}

int main(int argc, char **argv)
{
    bool status = true;

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_start_running();

    status = nler_event_pool_test();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (status ? EXIT_SUCCESS : EXIT_FAILURE);
}
