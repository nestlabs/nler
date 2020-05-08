/*
 *
 *    Copyright (c) 2020 Project nler Authors
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
 *      This file implements a unit test for the NLER counting semaphore
 *      interfaces.
 *
 */

#include <nlersemaphore.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 1

#include <nlererror.h>
#include <nlertask.h>
#include <nlerinit.h>
#include <nlerlog.h>
#include <nlerassert.h>

/*
 * Preprocessor Defitions
 */

#define kTHREAD_MAIN_SLEEP_MS      1001

/*
 * Type Definitions
 */

typedef struct globalData_s
{
    nlsemaphore_t    mBarrier;
    int32_t          mActualValue;
    int32_t          mExpectedValue;
} globalData_t;

typedef struct taskData_s
{
    globalData_t *   mGlobals;
    bool             mFinished;
} taskData_t;

/*
 * Global Variables
 */

static nltask_t sTaskA;
static nltask_t sTaskB;
static DEFINE_STACK(sStackA, NLER_TASK_STACK_BASE + 96);
static DEFINE_STACK(sStackB, NLER_TASK_STACK_BASE + 96);

void test_unthreaded(void)
{
    const nl_time_ms_t    kTimeoutNever = NLER_TIMEOUT_NEVER;
    const nl_time_ms_t    kTimeoutFast = 307;
    nlsemaphore_t *       lNullSemaphore = NULL;
    nlsemaphore_t         lSemaphore;
    size_t                lInitialValue;
    size_t                lMaximumValue;
    int                   lStatus;

    // Negative Tests

    lInitialValue = 0;
    lMaximumValue = 1;

    // NULL semaphore but valid initial and maximum values.

    lStatus = nlsemaphore_counting_create(lNullSemaphore, lMaximumValue, lInitialValue);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    // Valid semaphore and valid initial value but invalid maximum value.

    lInitialValue = 0;
    lMaximumValue = 0;

    lStatus = nlsemaphore_counting_create(&lSemaphore, lMaximumValue, lInitialValue);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    // Valid semaphore but invalid initial and maximum values.

    lInitialValue = 2;
    lMaximumValue = 1;

    lStatus = nlsemaphore_counting_create(&lSemaphore, lMaximumValue, lInitialValue);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    // NULL semaphore

    lStatus = nlsemaphore_give(lNullSemaphore);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    // NULL semaphore

    lStatus = nlsemaphore_give_from_isr(lNullSemaphore);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    // NULL semaphore

    lStatus = nlsemaphore_take(lNullSemaphore);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    // NULL semaphore

    lStatus = nlsemaphore_take_with_timeout(lNullSemaphore, kTimeoutNever);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    // Valid semaphore and valid but equal initial and maximum values
    // which should result in give failing.

    lInitialValue = 1;
    lMaximumValue = 1;

    lStatus = nlsemaphore_counting_create(&lSemaphore, lMaximumValue, lInitialValue);
    NLER_ASSERT(lStatus == NLER_SUCCESS);

    // Equal initial and maximum values which should result in give
    // failing.

    lStatus = nlsemaphore_give(&lSemaphore);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_STATE);

    // Equal initial and maximum values which should result in give
    // failing.

    lStatus = nlsemaphore_give_from_isr(&lSemaphore);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_STATE);

    nlsemaphore_destroy(&lSemaphore);

    // Positive Tests

    lInitialValue = 3;
    lMaximumValue = 3;

    lStatus = nlsemaphore_counting_create(&lSemaphore, lMaximumValue, lInitialValue);
    NLER_ASSERT(lStatus == NLER_SUCCESS);

    // First take should succeed without blocking (count at 2).

    lStatus = nlsemaphore_take(&lSemaphore);
    NLER_ASSERT(lStatus == NLER_SUCCESS);

    // Second take should succeed without blocking (count at 1).

    lStatus = nlsemaphore_take(&lSemaphore);
    NLER_ASSERT(lStatus == NLER_SUCCESS);

    // Third take should succeed without blocking (count at 0).

    lStatus = nlsemaphore_take(&lSemaphore);
    NLER_ASSERT(lStatus == NLER_SUCCESS);

    // Fourth take with timeout should timeout (count still at 0).

    lStatus = nlsemaphore_take_with_timeout(&lSemaphore, kTimeoutFast);
    NLER_ASSERT((lStatus == NLER_ERROR_NO_RESOURCE) ||
                (lStatus == NLER_SUCCESS));

    // Fifth take with timeout should timeout (count still at 0).

    lStatus = nlsemaphore_take_with_timeout(&lSemaphore, kTimeoutFast);
    NLER_ASSERT((lStatus == NLER_ERROR_NO_RESOURCE) ||
                (lStatus == NLER_SUCCESS));

    // A give should succeed (count at 1).

    lStatus = nlsemaphore_give(&lSemaphore);
    NLER_ASSERT(lStatus == NLER_SUCCESS);

    nlsemaphore_destroy(&lSemaphore);
}

static void taskEntry(void *aParams)
{
    nltask_t *     curtask = nltask_get_current();
    taskData_t *   taskData = (taskData_t *)aParams;
    globalData_t * data = taskData->mGlobals;
    int            status;

    status = nlsemaphore_take(&data->mBarrier);
    NLER_ASSERT(status == NLER_SUCCESS);

    data->mActualValue++;

    taskData->mFinished = true;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' exit: (%d)\n", nltask_get_name(curtask), data->mActualValue);

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
            aGlobalData->mActualValue == aGlobalData->mExpectedValue);
}

bool test_threaded(void)
{
    const size_t numThreads = 2;
    const size_t initialValue = 0;
    const size_t maximumValue = numThreads;
    globalData_t globalData;
    taskData_t   taskDataA;
    taskData_t   taskDataB;
    int          status;
    bool         retval;

    globalData.mActualValue = 0;
    globalData.mExpectedValue = numThreads;

    status = nlsemaphore_counting_create(&globalData.mBarrier, maximumValue, initialValue);
    NLER_ASSERT(status == NLER_SUCCESS);

    taskDataA.mGlobals = &globalData;
    taskDataA.mFinished = false;

    taskDataB.mGlobals = &globalData;
    taskDataB.mFinished = false;

    nltask_create(taskEntry, "A", sStackA, sizeof (sStackA), NLER_TASK_PRIORITY_NORMAL, (void *)&taskDataA, &sTaskA);
    nltask_create(taskEntry, "B", sStackB, sizeof (sStackB), NLER_TASK_PRIORITY_NORMAL, (void *)&taskDataB, &sTaskB);

    nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);

    for (size_t i = 0; i < numThreads; i++)
    {
        status = nlsemaphore_give(&globalData.mBarrier);
        NLER_ASSERT(status == NLER_SUCCESS);
    }

    nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);

    while (is_testing(&taskDataA, &taskDataB))
    {
        nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);
    }

    nlsemaphore_destroy(&globalData.mBarrier);

    retval = was_successful(&globalData, &taskDataA, &taskDataB);

    return (retval);
}

int main(int argc, char **argv)
{
    int             err;
    bool            status;

    NL_LOG_CRIT(lrTEST, "start main\n");

    err = nl_er_init();
    NLER_ASSERT(err == NLER_SUCCESS);

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime: %d)\n", err);

    NLER_ASSERT(argc == 1);

    test_unthreaded();

    nl_er_start_running();

    status = test_threaded();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (status ? EXIT_SUCCESS : EXIT_FAILURE);
}
