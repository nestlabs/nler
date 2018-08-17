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
 *      This file implements a unit test for the NLER event interfaces.
 *
 *      In theory, this test could run forever, with the
 *      two tasks posting events containing their respective task name
 *      and task pointer to one another and getting those events. In
 *      practice, the test is bounded with a number of expected events
 *      produced and consumed.
 *
 *      The test succeeds when the actual number of events meets the
 *      expected number for both threads.
 *
 *      The test fails if more events are produced or consumed than
 *      expected or if an errant event occurs.
 *
 */

#ifndef DEBUG
#define DEBUG
#endif

#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 1

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nlerassert.h>
#include <nlererror.h>
#include <nlereventqueue.h>
#include <nlerinit.h>
#include <nlerlog.h>
#include <nlertask.h>

/**
 *  Preprocessor Definitions
 */

#define kNL_EVENT_T_TASK_NAME_OFFSET  1
#define kNL_EVENT_T_TASK_PTR_OFFSET   2

#define NL_EVENT_T_TASK_ID(aIndex)    (NL_EVENT_T_WM_USER + (aIndex) + 1)
#define NL_EVENT_T_TASK_INDEX(aID)    ((aID) - NL_EVENT_T_WM_USER - 1) 

#define kNL_EVENT_T_TASK_NAME_ID      NL_EVENT_T_TASK_ID(kNL_EVENT_T_TASK_NAME_OFFSET - 1)
#define kNL_EVENT_T_TASK_PTR_ID       NL_EVENT_T_TASK_ID(kNL_EVENT_T_TASK_PTR_OFFSET - 1)

#define kNL_EVENT_T_TASK_FIRST        kNL_EVENT_T_TASK_NAME_ID
#define kNL_EVENT_T_TASK_LAST         kNL_EVENT_T_TASK_PTR_ID

#define kNL_EVENT_T_TASK_MAX          (kNL_EVENT_T_TASK_LAST - kNL_EVENT_T_TASK_FIRST + 1)

/**
 *  Expected number of events to be produced and consumed for each
 *  event type.
 *
 *  This is a symmetric test, with each thread expected to produce and
 *  consume an identical number of events.
 */
#define kNL_EVENT_TYPE_NAME_EXPECTED_TX_EVENTS       11
#define kNL_EVENT_TYPE_NAME_EXPECTED_RX_EVENTS       kNL_EVENT_TYPE_NAME_EXPECTED_TX_EVENTS

#define kNL_EVENT_TYPE_PTR_EXPECTED_TX_EVENTS        kNL_EVENT_TYPE_NAME_EXPECTED_TX_EVENTS
#define kNL_EVENT_TYPE_PTR_EXPECTED_RX_EVENTS        kNL_EVENT_TYPE_NAME_EXPECTED_RX_EVENTS

#define kTHREAD_MAIN_SLEEP_MS                   1001

/**
 *  Type Definitions
 */
typedef struct nl_event_taskName_s
{
    NL_DECLARE_EVENT
    const char *mName;
} nl_event_taskName_t;

typedef struct nl_event_taskPtr_s
{
    NL_DECLARE_EVENT
    nltask_t *mTask;
} nl_event_taskPtr_t;

typedef struct event_stats_s
{
    int32_t                  mExpect;
    int32_t                  mActual;
} event_stats_t;

typedef struct event_subpub_stats_s
{
    event_stats_t            mSent;
    event_stats_t            mReceived;
} event_subpub_stats_t;

typedef struct taskData_s
{
    nleventqueue_t         mMyQueue;
    nleventqueue_t        *mPeerQueue;
    nl_event_taskName_t    mNameEvent;
    nl_event_taskPtr_t     mPtrEvent;
    event_subpub_stats_t   mStats[kNL_EVENT_T_TASK_MAX];
    bool                   mFailed;
    bool                   mSucceeded;
} taskData_t;

/**
 *  Global Data
 */
static nltask_t sTaskA;
static nltask_t sTaskB;
static DEFINE_STACK(sStackA, NLER_TASK_STACK_BASE + 96);
static DEFINE_STACK(sStackB,  NLER_TASK_STACK_BASE + 96);

static const char * const kTaskNameA = "A";
static const char * const kTaskNameB = "B";

static int nl_test_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    taskData_t       *data = (taskData_t *)aClosure;
    const nltask_t   *curtask = nltask_get_current();
    const char       *name = nltask_get_name(curtask);
    size_t            index;
    bool              equal;
    int               status;
    int               retval = NLER_SUCCESS;


    index = NL_EVENT_T_TASK_INDEX(aEvent->mType);

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d, index: %u\n", name, aEvent->mType, index);

    NL_RANGE_CHECK_EVENT_TYPE(aEvent->mType, NL_EVENT_T_WM_USER, NL_EVENT_T_WM_USER_LAST);

    switch (aEvent->mType)
    {
        case kNL_EVENT_T_TASK_NAME_ID:
        {
            const nl_event_taskName_t *ev = (nl_event_taskName_t *)aEvent;

            NL_LOG_CRIT(lrTEST, "'%s' got event_taskName: '%s'\n", name, ev->mName);

            data->mStats[index].mReceived.mActual++;

            status = strcmp(name, ev->mName);
            NLER_ASSERT(status != 0);
            break;
        }

        case kNL_EVENT_T_TASK_PTR_ID:
        {
            const nl_event_taskPtr_t *ev = (nl_event_taskPtr_t *)aEvent;

            NL_LOG_CRIT(lrTEST, "'%s' got event_taskPtr: %p\n", name, ev->mTask);

            data->mStats[index].mReceived.mActual++;

            equal = (curtask == ev->mTask);
            NLER_ASSERT(equal != true);
            break;
        }

        default:
            NL_LOG_CRIT(lrTEST, "'%s' bad event id: %d\n", name, aEvent->mType);
            data->mFailed = true;

            retval = NLER_ERROR_BAD_STATE;
            break;
    }

    return retval;
}

static int nl_test_maybe_post_event(nleventqueue_t *aQueue, nl_event_t *aEvent, volatile event_stats_t *aOutSendStats)
{
    int retval = NLER_SUCCESS;

    if (aOutSendStats->mActual < aOutSendStats->mExpect)
    {
        retval = nleventqueue_post_event(aQueue, aEvent);
        NLER_ASSERT(retval == NLER_SUCCESS);

        aOutSendStats->mActual++;
    }

    return retval;
}

static void check_succeeded_or_failed(const char *aName, uint8_t aIndex, volatile const event_subpub_stats_t *aStats, bool *aOutSucceeded, bool *aOutFailed)
{
    NL_LOG_DEBUG(lrTEST, "%4s: %2u  %8d  %8d    %8d  %8d\n",
                 aName,
                 NL_EVENT_T_TASK_ID(aIndex),
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


static void taskEntry(void *aParams)
{
    taskData_t                  *data = (taskData_t *)aParams;
    nltask_t                    *curtask = nltask_get_current();
    const char                  *name = nltask_get_name(curtask);

    NL_INIT_EVENT(data->mNameEvent, kNL_EVENT_T_TASK_NAME_ID, NULL, data);
    (*(nl_event_taskName_t *)(&data->mNameEvent)).mName = name;

    NL_INIT_EVENT(data->mPtrEvent, kNL_EVENT_T_TASK_PTR_ID, NULL, data);
    (*(nl_event_taskPtr_t *)(&data->mPtrEvent)).mTask = curtask;

    NL_LOG_CRIT(lrTEST, "from the task: %s (%p) (queue: %p)\n", name, curtask, data->mMyQueue);

    while (!data->mFailed && !data->mSucceeded)
    {
        nl_event_t  *ev;
        size_t       i;
        bool         succeeded = true;
        bool         failed = false;

        nl_test_maybe_post_event(data->mPeerQueue, (nl_event_t *)&data->mPtrEvent, &data->mStats[NL_EVENT_T_TASK_INDEX(kNL_EVENT_T_TASK_PTR_ID)].mSent);
        nl_test_maybe_post_event(data->mPeerQueue, (nl_event_t *)&data->mNameEvent, &data->mStats[NL_EVENT_T_TASK_INDEX(kNL_EVENT_T_TASK_NAME_ID)].mSent);

        ev = nleventqueue_get_event(&data->mMyQueue);

        nl_dispatch_event(ev, nl_test_eventhandler, data);

        ev = nleventqueue_get_event(&data->mMyQueue);

        nl_dispatch_event(ev, nl_test_eventhandler, data);

        for (i = 0; i < kNL_EVENT_T_TASK_MAX; i++)
        {
            const event_subpub_stats_t *lStats = &data->mStats[i];

            NL_LOG_DEBUG(lrTEST,
                         "Task  ID  Sent                  Received\n"
                         "          Expected   Actual     Expected   Actual\n");

            check_succeeded_or_failed(name, i, lStats, &succeeded, &failed);

            NL_LOG_CRIT(lrTEST, "'%s' %s number of expected events\n", name, ((succeeded) ? "successfully sent/received" : ((failed) ? "failed to send/receive" : "has not yet sent/received")));
        
            if (succeeded)
                data->mSucceeded = true;

            if (failed)
                data->mFailed = true;
        }
    }
}

static void task_stats_init(taskData_t *aData, size_t aIndex, int32_t aExpectedTxEvents, int32_t aExpectedRxEvents)
{
    aData->mFailed    = false;
    aData->mSucceeded = false;

    aData->mStats[aIndex].mSent.mActual = 0;
    aData->mStats[aIndex].mSent.mExpect = aExpectedTxEvents;
    aData->mStats[aIndex].mReceived.mActual = 0;
    aData->mStats[aIndex].mReceived.mExpect = aExpectedRxEvents;
}

static void task_data_init(taskData_t *aDataA, taskData_t *aDataB)
{
    size_t index;

    aDataA->mPeerQueue = &aDataB->mMyQueue;
    aDataB->mPeerQueue = &aDataA->mMyQueue;

    aDataA->mFailed    = false;
    aDataA->mSucceeded = false;

    aDataB->mFailed    = false;
    aDataB->mSucceeded = false;

    index = NL_EVENT_T_TASK_INDEX(kNL_EVENT_T_TASK_NAME_ID);

    task_stats_init(aDataA,
                    index,
                    kNL_EVENT_TYPE_NAME_EXPECTED_TX_EVENTS,
                    kNL_EVENT_TYPE_NAME_EXPECTED_RX_EVENTS);
    task_stats_init(aDataB,
                    index,
                    kNL_EVENT_TYPE_NAME_EXPECTED_TX_EVENTS,
                    kNL_EVENT_TYPE_NAME_EXPECTED_RX_EVENTS);

    index = NL_EVENT_T_TASK_INDEX(kNL_EVENT_T_TASK_PTR_ID);

    task_stats_init(aDataA,
                    index,
                    kNL_EVENT_TYPE_PTR_EXPECTED_TX_EVENTS,
                    kNL_EVENT_TYPE_PTR_EXPECTED_RX_EVENTS);
    task_stats_init(aDataB,
                    index,
                    kNL_EVENT_TYPE_PTR_EXPECTED_TX_EVENTS,
                    kNL_EVENT_TYPE_PTR_EXPECTED_RX_EVENTS);

}

static bool is_testing(volatile const taskData_t *aTaskA,
                       volatile const taskData_t *aTaskB)
{
    bool retval = false;

    if ((!aTaskA->mFailed && !aTaskB->mFailed) &&
        (!aTaskA->mSucceeded && !aTaskB->mSucceeded))
    {
        retval = true;
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

bool nler_event_test(void)
{
    taskData_t           taskDataA;
    taskData_t           taskDataB;
    nl_event_t          *queuememA[50];
    nl_event_t          *queuememB[50];
    int                  status;
    bool                 retval;

    status = nleventqueue_create(queuememA, sizeof(queuememA), &taskDataA.mMyQueue);
    NLER_ASSERT(status == NLER_SUCCESS);

    status = nleventqueue_create(queuememB, sizeof(queuememB), &taskDataB.mMyQueue);
    NLER_ASSERT(status == NLER_SUCCESS);

    task_data_init(&taskDataA, &taskDataB);
    task_data_init(&taskDataB, &taskDataA);

    nltask_create(taskEntry,
                  kTaskNameA,
                  sStackA,
                  sizeof (sStackA),
                  NLER_TASK_PRIORITY_NORMAL,
                  &taskDataA,
                  &sTaskA);
    nltask_create(taskEntry,
                  kTaskNameB,
                  sStackB,
                  sizeof (sStackB),
                  NLER_TASK_PRIORITY_NORMAL,
                  &taskDataB,
                  &sTaskB);

    while (is_testing(&taskDataA, &taskDataB))
    {
        nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);
    }

    retval = was_successful(&taskDataA, &taskDataB);

    return retval;
}

int main(int argc, char **argv)
{
    bool  status;

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_start_running();

    status = nler_event_test();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (status ? EXIT_SUCCESS : EXIT_FAILURE);
}
