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
 *      This file implements a unit test for the NLER task creation
 *      interface.
 *
 */

#include "nlertask.h"
#include "nlerinit.h"
#include <stdio.h>
#include "nlerlog.h"
#include "nlerassert.h"

nl_task_t taskA;
nl_task_t taskB;
uint8_t stackA[NLER_TASK_STACK_BASE + 96];
uint8_t stackB[NLER_TASK_STACK_BASE + 96];

struct taskAData
{
    int value1;
    int argc;
};

static void taskEntryA(void *aParams)
{
    const nl_task_t           *curtask = nl_task_get_current();
    const struct taskAData    *data = (struct taskAData *)aParams;

    (void)curtask;
    (void)data;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' (%08x, %d)\n", curtask->mName, data->value1, data->argc);

    while (1)
    {
        NL_LOG_CRIT(lrTEST, "from the loop: '%s'\n", curtask->mName);
        nl_task_sleep_ms(1000);
    }
}

struct taskBData
{
    int value1;
    int argc;
};

static void taskEntryB(void *aParams)
{
    const nl_task_t           *curtask = nl_task_get_current();
    const struct taskBData    *data = (struct taskBData *)aParams;

    (void)curtask;
    (void)data;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' (%08x, %d)\n", curtask->mName, data->value1, data->argc);

    while (1)
    {
        NL_LOG_CRIT(lrTEST, "from the loop: '%s'\n", curtask->mName);
        nl_task_sleep_ms(1000);
    }
}

int main(int argc, char **argv)
{
    struct taskAData    dataA;
    struct taskBData    dataB;

    NL_LOG_CRIT(lrTEST, "start main\n");

    dataA.value1 = 0xdeadbeef;
    dataA.argc = argc;

    dataB.value1 = 0xc0dedbad;
    dataB.argc = argc;

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime)\n");

    /* random test of asserts -- give this an argument and it should fail */

    NLER_ASSERT(argc == 1);

    nl_task_create(taskEntryA, "A", stackA, sizeof(stackA), NLER_TASK_PRIORITY_NORMAL, &dataA, &taskA);
    nl_task_create(taskEntryB, "B", stackB, sizeof(stackB), NLER_TASK_PRIORITY_HIGH, &dataB, &taskB);

    nl_er_start_running();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return 0;
}

