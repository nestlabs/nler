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

#include <nlertask.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 3

#include <nlerinit.h>
#include <nlerlog.h>
#include <nlerassert.h>

/**
 *  Preprocessor Definitions
 */

#define kTHREAD_A_SLEEP_MS       241
#define kTHREAD_B_SLEEP_MS       491

#define kTHREAD_MAIN_SLEEP_MS    997

#define kTHREAD_A_LOOPS            7
#define kTHREAD_B_LOOPS            9

/**
 *  Type Definitions
 */

typedef struct taskData_s
{
    const nltask_t *mParent;
    const nltask_t *mTask;
    nl_time_ms_t    mSleepMS;
    uint32_t        mLoops;
    const char     *mName;
    bool            mSucceeded;
} taskData_t;

/**
 *  Global Variables
 */

static nltask_t taskA;
static nltask_t taskB;
static DEFINE_STACK(stackA, NLER_TASK_STACK_BASE + 96);
static DEFINE_STACK(stackB, NLER_TASK_STACK_BASE + 96);

static const char * const kTaskNameA = "A";
static const char * const kTaskNameB = "B";

static void taskEntryA(void *aParams)
{
    const nltask_t            *curtask = nltask_get_current();
    const char                *name = nltask_get_name(curtask);
    volatile taskData_t       *data = (volatile taskData_t *)aParams;
    int                        status;


    NL_LOG_CRIT(lrTEST, "from the task: '%s'\n", name);

    status = strcmp(name, data->mName);
    NLER_ASSERT(status == 0);

    NLER_ASSERT(data->mParent != curtask);
    NLER_ASSERT(data->mTask == curtask);
    NLER_ASSERT(data->mSucceeded == false);

    nltask_sleep_ms(data->mSleepMS);

    nltask_yield();

    while (data->mLoops--)
    {
        NL_LOG_DEBUG(lrTEST, "from the loop: '%s'\n", nltask_get_name(curtask));
        nltask_sleep_ms(data->mSleepMS);
    }

    data->mSucceeded = true;
}

static void taskEntryB(void *aParams)
{
    const nltask_t            *curtask = nltask_get_current();
    const char                *name = nltask_get_name(curtask);
    volatile taskData_t       *data = (volatile taskData_t *)aParams;
    int                        status;


    NL_LOG_CRIT(lrTEST, "from the task: '%s'\n", name);

    status = strcmp(name, data->mName);
    NLER_ASSERT(status == 0);

    NLER_ASSERT(data->mParent != curtask);
    NLER_ASSERT(data->mTask == curtask);
    NLER_ASSERT(data->mSucceeded == false);

    nltask_sleep_ms(data->mSleepMS);
   
    nltask_yield();

    while (data->mLoops--)
    {
        NL_LOG_DEBUG(lrTEST, "from the loop: '%s'\n", nltask_get_name(curtask));
        nltask_sleep_ms(data->mSleepMS);
    }

    data->mSucceeded = true;
}

static bool is_testing(volatile const taskData_t *aTaskA,
                       volatile const taskData_t *aTaskB)
{
    const bool retval = (!aTaskA->mSucceeded || !aTaskB->mSucceeded);

    return retval;
}

static bool was_successful(volatile const taskData_t *aTaskA,
                           volatile const taskData_t *aTaskB)
{
    const bool retval = (aTaskA->mSucceeded && aTaskB->mSucceeded);

    return retval;
}

bool nler_task_test(void)
{
    const nltask_t              *curtask;
    volatile taskData_t          taskDataA;
    volatile taskData_t          taskDataB;
    bool                         retval;

    curtask = nltask_get_current();
    NLER_ASSERT(curtask != NULL);

    taskDataA.mParent = curtask;
    taskDataA.mTask = &taskA;
    taskDataA.mSleepMS = kTHREAD_A_SLEEP_MS;
    taskDataA.mLoops = kTHREAD_A_LOOPS;
    taskDataA.mName = kTaskNameA;
    taskDataA.mSucceeded = false;

    taskDataB.mParent = curtask;
    taskDataB.mTask = &taskB;
    taskDataB.mSleepMS = kTHREAD_B_SLEEP_MS;
    taskDataB.mLoops = kTHREAD_B_LOOPS;
    taskDataB.mName = kTaskNameB;
    taskDataB.mSucceeded = false;

    nltask_create(taskEntryA, kTaskNameA, stackA, sizeof(stackA), NLER_TASK_PRIORITY_NORMAL, (void *)&taskDataA, &taskA);
    nltask_create(taskEntryB, kTaskNameB, stackB, sizeof(stackB), NLER_TASK_PRIORITY_HIGH, (void *)&taskDataB, &taskB);

    nltask_yield();

    while (is_testing(&taskDataA, &taskDataB))
    {
        NL_LOG_DEBUG(lrTEST, "from the loop: main\n");

        nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);
    }

    retval = was_successful(&taskDataA, &taskDataB);

    return retval;
}

int main(int argc, char **argv)
{
    bool status;

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_start_running();

    status = nler_task_test();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (status ? EXIT_SUCCESS : EXIT_FAILURE);
}
