/*
 *
 *    Copyright (c) 2015-2018 Nest Labs, Inc.
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
 *      This file implements a unit test for the NLER event
 *      interfaces, with the use case of posting events before the
 *      target receiver task has been created.
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
#include "nlererror.h"

nl_task_t taskA;
uint8_t stackA[NLER_TASK_STACK_BASE + 96];

#define NL_EVENT_T_TASK_START   (NL_EVENT_T_WM_USER + 1)

struct nl_event_task_start
{
    NL_DECLARE_EVENT
};

int nl_test_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    int               retval = !NLER_SUCCESS;
    const nl_task_t   *curtask = nl_task_get_current();

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d\n", curtask->mName, aEvent->mType);

    switch (aEvent->mType)
    {
        case NL_EVENT_T_TASK_START:
        {
            NL_LOG_CRIT(lrTEST, "'%s' got event_task_start\n", curtask->mName);

            retval = NLER_SUCCESS;

            break;
        }

        case NL_EVENT_T_EXIT:
        {
            NL_LOG_CRIT(lrTEST, "'%s' got event_task EXIT\n", curtask->mName);

            break;
        }

        default:
            break;
    }

    return retval;
}

void taskEntryA(void *aParams)
{
    const nl_task_t           *curtask = nl_task_get_current();
    nl_eventqueue_t           queue = (nl_eventqueue_t)aParams;
    nl_event_t                exitev = { NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, NULL, NULL) };

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %08x)\n", curtask->mName, queue);

    while (1)
    {
        nl_event_t  *ev;
        int         status;

        ev = nl_eventqueue_get_event(queue);

        status = nl_dispatch_event(ev, nl_test_eventhandler, NULL);

        if (status == NLER_SUCCESS)
        {
            nl_eventqueue_post_event(queue, &exitev);
        }
        else
        {
            break;
        }
    }
}

int main(int argc, char **argv)
{
    nl_event_t                  *queuememA[50];
    nl_eventqueue_t             queue;
    struct nl_event_task_start  startev = { NL_INIT_EVENT_STATIC(NL_EVENT_T_TASK_START, NULL, NULL) };
    int                         status;

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime)\n");

    queue = nl_eventqueue_create(queuememA, sizeof(queuememA));

    status = nl_eventqueue_post_event(queue, (nl_event_t *)&startev);
    (void)status;

    NL_LOG_CRIT(lrTEST, "result of posting initial event to taskA queue: %d\n", status);

    nl_task_create(taskEntryA, "A", stackA, sizeof(stackA), NLER_TASK_PRIORITY_NORMAL, queue, &taskA);

    nl_er_start_running();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return 0;
}

