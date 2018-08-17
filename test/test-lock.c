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

#include <nlerlock.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 1

#include <nlerassert.h>
#include <nlererror.h>
#include <nlerinit.h>
#include <nlerlog.h>
#include <nlertask.h>

/*
 * Preprocessor Defitions
 */

#define kTHREAD_MAIN_SLEEP_MS      1001

#define kNUM_LOCK_ITERS         1000000

#define kSENTINEL_DATA_VALUE          0

/*
 * Type Definitions
 */

typedef struct globalData_s
{
    nllock_t    mLock;
    int32_t     mValue;
} globalData_t;

typedef struct taskData_s
{
    globalData_t          *mGlobals;
    bool                   mFinished;
} taskData_t;

/*
 * Global Variables
 */

static nltask_t taskA;
static nltask_t taskB;
static DEFINE_STACK(stackA, NLER_TASK_STACK_BASE + 96);
static DEFINE_STACK(stackB, NLER_TASK_STACK_BASE + 96);

static void taskEntry(void *aParams)
{
    nltask_t                 *curtask = nltask_get_current();
    taskData_t               *taskData = (taskData_t *)aParams;
    globalData_t             *data = taskData->mGlobals;
    int                       idx;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' entry: (%d)\n", nltask_get_name(curtask), data->mValue);

    for (idx = 0; idx < kNUM_LOCK_ITERS; idx++)
    {
        nllock_enter(&data->mLock);
        data->mValue++;
        nllock_exit(&data->mLock);

        nllock_enter(&data->mLock);
        data->mValue--;
        nllock_exit(&data->mLock);

        if ((idx % 100000) == 0)
        {
            NL_LOG_DEBUG(lrTEST, "'%s' inc/dec: %d\n", nltask_get_name(curtask), idx);
        }
    }

    for (idx = 0; idx < kNUM_LOCK_ITERS; idx++)
    {
        const int32_t kDelta = 12;

        nllock_enter(&data->mLock);
        data->mValue += kDelta;
        nllock_exit(&data->mLock);

        nllock_enter(&data->mLock);
        data->mValue -= kDelta;
        nllock_exit(&data->mLock);

        if ((idx % 100000) == 0)
        {
            NL_LOG_DEBUG(lrTEST, "'%s' add/sub: %d\n", nltask_get_name(curtask), idx);
        }
    }

    taskData->mFinished = true;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' exit: (%d)\n", nltask_get_name(curtask), data->mValue);

    nltask_suspend(curtask);
}

static bool is_testing(volatile taskData_t *aTaskDataA,
                       volatile taskData_t *aTaskDataB)
{
    return (!aTaskDataA->mFinished || !aTaskDataB->mFinished);
}

static bool was_successful(volatile globalData_t *aGlobalData,
                           volatile taskData_t *aTaskDataA,
                           volatile taskData_t *aTaskDataB)
{
    return (aTaskDataA->mFinished &&
            aTaskDataB->mFinished &&
            aGlobalData->mValue == kSENTINEL_DATA_VALUE);
}

bool nler_lock_test(void)
{
    globalData_t          globalData;
    taskData_t            taskDataA;
    taskData_t            taskDataB;
    int                   status;
    bool                  retval;

    status = nllock_create(&globalData.mLock);
    NLER_ASSERT(status == NLER_SUCCESS);

    globalData.mValue = kSENTINEL_DATA_VALUE;

    taskDataA.mGlobals = &globalData;
    taskDataA.mFinished = false;

    taskDataB.mGlobals = &globalData;
    taskDataB.mFinished = false;

    nltask_create(taskEntry, "A", stackA, sizeof (stackA), NLER_TASK_PRIORITY_NORMAL, (void *)&taskDataA, &taskA);
    nltask_create(taskEntry, "B", stackB, sizeof (stackB), NLER_TASK_PRIORITY_NORMAL, (void *)&taskDataB, &taskB);

    while (is_testing(&taskDataA, &taskDataB))
    {
        nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);
    }

    nllock_destroy(&globalData.mLock);

    retval = was_successful(&globalData, &taskDataA, &taskDataB);

    return retval;
}

int main(int argc, char **argv)
{
    bool  status = true;

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_start_running();

    status = nler_lock_test();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (status ? EXIT_SUCCESS : EXIT_FAILURE);
}
