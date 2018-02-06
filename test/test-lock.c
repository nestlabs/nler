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
 *      This file implements a unit test for the NLER mutual exclusion
 *      lock interfaces.
 */

#include "nlertask.h"
#include "nlerinit.h"
#include <stdio.h>
#include "nlerlog.h"
#include "nlerassert.h"
#include "nlerlock.h"

nl_task_t taskA;
nl_task_t taskB;
uint8_t stackA[NLER_TASK_STACK_BASE + 96];
uint8_t stackB[NLER_TASK_STACK_BASE + 96];

#define NUM_ATOM_ITERS  1000000

struct taskData
{
    nl_lock_t   lock;
    int32_t     value;
};

void taskEntry(void *aParams)
{
    nl_task_t       *curtask = nl_task_get_current();
    struct taskData *data = (struct taskData *)aParams;
    int             idx;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' entry: (%d)\n", curtask->mName, data->value);

    for (idx = 0; idx < NUM_ATOM_ITERS; idx++)
    {
        nl_er_lock_enter(data->lock);
        data->value++;
        nl_er_lock_exit(data->lock);

        nl_er_lock_enter(data->lock);
        data->value--;
        nl_er_lock_exit(data->lock);

        if ((idx % 100000) == 0)
        {
            NL_LOG_DEBUG(lrTEST, "'%s' inc/dec: %d\n", curtask->mName, idx);
        }
    }

    for (idx = 0; idx < NUM_ATOM_ITERS; idx++)
    {
        nl_er_lock_enter(data->lock);
        data->value += 12;
        nl_er_lock_exit(data->lock);

        nl_er_lock_enter(data->lock);
        data->value -= 12;
        nl_er_lock_exit(data->lock);

        if ((idx % 100000) == 0)
        {
            NL_LOG_DEBUG(lrTEST, "'%s' add/-add: %d\n", curtask->mName, idx);
        }
    }

    NL_LOG_CRIT(lrTEST, "from the task: '%s' exit: (%d)\n", curtask->mName, data->value);

    nl_task_suspend(curtask);
}

int main(int argc, char **argv)
{
    struct taskData data;
    int             err;

    NL_LOG_CRIT(lrTEST, "start main\n");

    err = nl_er_init();
    (void)err;

    data.lock = nl_er_lock_create();
    data.value = 0;

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime: %d)\n", err);

    NLER_ASSERT(data.lock != NULL);

    nl_task_create(taskEntry, "A", stackA, sizeof(stackA), NLER_TASK_PRIORITY_NORMAL, &data, &taskA);
    nl_task_create(taskEntry, "B", stackB, sizeof(stackB), NLER_TASK_PRIORITY_NORMAL, &data, &taskB);

    nl_er_start_running();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return 0;
}

