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
 */

#include "nlertask.h"
#include "nlerinit.h"
#include <stdio.h>
#include "nlerlog.h"
#include "nlereventqueue.h"
#include "nlertimer.h"
#if NLER_FEATURE_SIMULATEABLE_TIME
#include "nlertimer_sim.h"
#endif

/* NOTE: this example uses four timers
 * which is the default built for the
 * runtime if not overridden by the 
 * application. if fewer than four timers
 * are available, do not expect the example
 * to fully function.
 */

nl_task_t taskA;
uint8_t stackA[NLER_TASK_STACK_BASE + 128];

struct taskAData
{
    nl_eventqueue_t mTimer;
    nl_eventqueue_t mQueue;
};

int nl_test_timer_eventhandler(nl_event_t *aEvent, void *aClosure);

typedef struct nl_test_event_timer_s
{
    nl_event_timer_t    mTimerEvent;
    int                 mID;
} nl_test_event_timer_t;

nl_test_event_timer_t timerev1 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, nl_test_timer_eventhandler, NULL),
        NULL, 0, 0, 0, 0
    },
    1
};

nl_test_event_timer_t timerev2 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, nl_test_timer_eventhandler, NULL),
        NULL, 0, 0, 0, 0
    },
    2
};

nl_test_event_timer_t timerev3 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, nl_test_timer_eventhandler, NULL),
        NULL, 0, 0, 0, 0
    },
    3
};

nl_test_event_timer_t timerev4 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, nl_test_timer_eventhandler, NULL),
        NULL, 0, 0, 0, 0
    },
    4
};

int nl_test_timer_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    const nl_task_t               *curtask = nl_task_get_current();
    nl_test_event_timer_t         *timer = (nl_test_event_timer_t *)aEvent;
    struct taskAData              *data = (struct taskAData *)aClosure;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' timeout: %d, time: %u\n", curtask->mName, timer->mID, nl_get_time_native());

    switch (timer->mID)
    {
        case 1:
            nl_init_event_timer(&timerev2.mTimerEvent, 500);
            nl_eventqueue_post_event(data->mTimer, (nl_event_t *)&timerev2);
            break;

        case 2:
            nl_init_event_timer(&timerev3.mTimerEvent, 250);
            nl_eventqueue_post_event(data->mTimer, (nl_event_t *)&timerev3);
            break;

        case 3:
            nl_init_event_timer(&timerev4.mTimerEvent, 3000);
            nl_eventqueue_post_event(data->mTimer, (nl_event_t *)&timerev4);
            break;

        case 4:
            break;

        default:
            NL_LOG_CRIT(lrTEST, "'%s' bad timer id: %d\n", curtask->mName, timer->mID);
            break;
    }

    return 0;
}

int nl_test_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    const nl_task_t   *curtask = nl_task_get_current();

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d -- unexpected\n", curtask->mName, aEvent->mType);

    return 0;
}

void taskEntryA(void *aParams)
{
    const nl_task_t           *curtask = nl_task_get_current();
    struct taskAData          *data = (struct taskAData *)aParams;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %08x)\n", curtask->mName, data->mQueue);

    timerev1.mTimerEvent.mFlags = NLER_TIMER_FLAG_REPEAT;
    timerev1.mTimerEvent.mHandlerClosure = data;
    timerev1.mTimerEvent.mReturnQueue = data->mQueue;

    timerev2.mTimerEvent.mHandlerClosure = data;
    timerev2.mTimerEvent.mReturnQueue = data->mQueue;

    timerev3.mTimerEvent.mHandlerClosure = data;
    timerev3.mTimerEvent.mReturnQueue = data->mQueue;

    timerev4.mTimerEvent.mHandlerClosure = data;
    timerev4.mTimerEvent.mReturnQueue = data->mQueue;

    nl_init_event_timer(&timerev1.mTimerEvent, 2000);

    nl_eventqueue_post_event(data->mTimer, (nl_event_t *)&timerev1);

    while (1)
    {
        nl_event_t  *ev;

        ev = nl_eventqueue_get_event(data->mQueue);

        nl_dispatch_event(ev, nl_test_eventhandler, NULL);
    }
}

int main(int argc, char **argv)
{
    struct taskAData    dataA;
    nl_event_t          *queuememA[50];

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_init();
#if NLER_FEATURE_SIMULATEABLE_TIME
    nl_time_init_sim(false);
#endif

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime)\n");

    dataA.mTimer = nl_timer_start(NLER_TASK_PRIORITY_HIGH + 1);
    dataA.mQueue = nl_eventqueue_create(queuememA, sizeof(queuememA));

    nl_task_create(taskEntryA, "A", stackA, sizeof(stackA), NLER_TASK_PRIORITY_NORMAL, &dataA, &taskA);

    nl_er_start_running();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return 0;
}

