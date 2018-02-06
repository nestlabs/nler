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
 */

#ifndef DEBUG
#define DEBUG
#endif
#include "nlertask.h"
#include "nlerinit.h"
#include <stdio.h>
#include "nlerlog.h"
#include "nlereventqueue.h"

nl_task_t taskA;
nl_task_t taskB;
uint8_t stackA[NLER_TASK_STACK_BASE + 96];
uint8_t stackB[NLER_TASK_STACK_BASE + 96];

#define NL_EVENT_T_TASK_NAME (NL_EVENT_T_WM_USER + 1)
#define NL_EVENT_T_TASK_PTR  (NL_EVENT_T_WM_USER + 2)

struct nl_event_taskName
{
    NL_DECLARE_EVENT
    const char *mName;
};

struct nl_event_taskPtr
{
    NL_DECLARE_EVENT
    nl_task_t *mTask;
};

int nl_test_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    const nl_task_t   *curtask = nl_task_get_current();

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d\n", curtask->mName, aEvent->mType);

    NL_RANGE_CHECK_EVENT_TYPE(aEvent->mType, NL_EVENT_T_WM_USER, NL_EVENT_T_WM_USER_LAST);

    switch (aEvent->mType)
    {
        case NL_EVENT_T_TASK_NAME:
        {
            const struct nl_event_taskName *ev = (struct nl_event_taskName *)aEvent;

            (void)ev;

            NL_LOG_CRIT(lrTEST, "'%s' got event_taskName: %s\n", curtask->mName, ev->mName);

            break;
        }

        case NL_EVENT_T_TASK_PTR:
        {
            const struct nl_event_taskPtr *ev = (struct nl_event_taskPtr *)aEvent;

            (void)ev;

            NL_LOG_CRIT(lrTEST, "'%s' got event_taskPtr: %p\n", curtask->mName, ev->mTask);

            break;
        }
    }

    return 0;
}

struct taskAData
{
    nl_eventqueue_t mQueue;
    nl_eventqueue_t mBQueue;
};

void taskEntryA(void *aParams)
{
    nl_task_t                   *curtask = nl_task_get_current();
    struct taskAData            *data = (struct taskAData *)aParams;
    struct nl_event_taskPtr     ptrev = { NL_INIT_EVENT_STATIC(NL_EVENT_T_TASK_PTR, NULL, NULL), NULL };
    struct nl_event_taskName    nameev = { NL_INIT_EVENT_STATIC(NL_EVENT_T_TASK_NAME, NULL, NULL), NULL };

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %08x)\n", curtask->mName, data->mQueue);

    ptrev.mTask = curtask;
    nameev.mName = curtask->mName;

    while (1)
    {
        nl_event_t  *ev;

        nl_eventqueue_post_event(data->mBQueue, (nl_event_t *)&ptrev);
        nl_eventqueue_post_event(data->mBQueue, (nl_event_t *)&nameev);

        ev = nl_eventqueue_get_event(data->mQueue);

        nl_dispatch_event(ev, nl_test_eventhandler, NULL);

        ev = nl_eventqueue_get_event(data->mQueue);

        nl_dispatch_event(ev, nl_test_eventhandler, NULL);
    }
}

struct taskBData
{
    nl_eventqueue_t mQueue;
    nl_eventqueue_t mAQueue;
};

void taskEntryB(void *aParams)
{
    nl_task_t                   *curtask = nl_task_get_current();
    struct taskBData            *data = (struct taskBData *)aParams;
    struct nl_event_taskPtr     ptrev = { NL_INIT_EVENT_STATIC(NL_EVENT_T_TASK_PTR, NULL, NULL), NULL };
    struct nl_event_taskName    nameev = { NL_INIT_EVENT_STATIC(NL_EVENT_T_TASK_NAME, NULL, NULL), NULL };

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %08x)\n", curtask->mName, data->mQueue);

    ptrev.mTask = curtask;
    nameev.mName = curtask->mName;

    while (1)
    {
        nl_event_t  *ev;

        nl_eventqueue_post_event(data->mAQueue, (nl_event_t *)&ptrev);
        nl_eventqueue_post_event(data->mAQueue, (nl_event_t *)&nameev);

        ev = nl_eventqueue_get_event(data->mQueue);

        nl_dispatch_event(ev, nl_test_eventhandler, NULL);

        ev = nl_eventqueue_get_event(data->mQueue);

        nl_dispatch_event(ev, nl_test_eventhandler, NULL);
    }
}

int main(int argc, char **argv)
{
    struct taskAData    dataA;
    struct taskBData    dataB;
    nl_event_t          *queuememA[50];
    nl_event_t          *queuememB[50];

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime)\n");

    dataA.mQueue = nl_eventqueue_create(queuememA, sizeof(queuememA));
    dataB.mQueue = nl_eventqueue_create(queuememB, sizeof(queuememB));

    dataA.mBQueue = dataB.mQueue;
    dataB.mAQueue = dataA.mQueue;

    nl_task_create(taskEntryA, "A", stackA, sizeof(stackA), NLER_TASK_PRIORITY_NORMAL, &dataA, &taskA);
    nl_task_create(taskEntryB, "B", stackB, sizeof(stackB), NLER_TASK_PRIORITY_NORMAL, &dataB, &taskB);

    nl_er_start_running();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return 0;
}

