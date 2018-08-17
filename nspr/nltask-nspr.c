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
 *      This file implements NLER tasks under the Netscape Portable
 *      Runtime (NSPR) build platform.
 *
 */

#include "nlertask.h"
#include <nspr/prthread.h>
#include "nlererror.h"
#include "nlerlog.h"

/**
 *  Global, somewhat "faked" task structure for the main, parent
 *  thread to ensure that nltask_get_current(), etc. work correctly.
 */
static nltask_t sMainTask;

static PRIntn   sTaskPtrIdx = -1;

static PRThreadPriority nltask_nspr_map_priority_to_native(nltask_priority_t aPriority)
{
    PRThreadPriority    retval;

    if (aPriority > (NLER_TASK_PRIORITY_HIGH + 1))
        retval = PR_PRIORITY_URGENT;
    else if (aPriority > (NLER_TASK_PRIORITY_NORMAL + 1))
        retval = PR_PRIORITY_HIGH;
    else if (aPriority > (NLER_TASK_PRIORITY_LOW + 1))
        retval = PR_PRIORITY_NORMAL;
    else
        retval = PR_PRIORITY_LOW;

    return retval;
}

static void nltask_nspr_entry(void *aClosure)
{
    nltask_t              *lTask = (nltask_t *)aClosure;
    nltask_entry_point_t   lEntry = (nltask_entry_point_t)lTask->mNativeTaskObj.mEntry;
    PRThread              *lThread = PR_GetCurrentThread();

    lTask->mNativeTaskObj.mThread = lThread;

    PR_SetThreadPrivate(sTaskPtrIdx, lTask);

    PR_SetCurrentThreadName(lTask->mNativeTaskObj.mName);

    (*lEntry)(lTask->mNativeTaskObj.mParams);
}

int nltask_create(nltask_entry_point_t aEntry, const char *aName, void *aStack, size_t aStackSize, nltask_priority_t aPriority, void *aParams, nltask_t *aOutTask)
{
    PRThread    *lThread;
    int         retval = NLER_SUCCESS;

    if ((aOutTask != NULL) && (aName != NULL))
    {
        if (sTaskPtrIdx != -1)
        {
            aStackSize = (aStackSize + 0x1fff) & ~0x1fff;

            aOutTask->mStackTop              = aStack + aStackSize;

            aOutTask->mNativeTaskObj.mName   = aName;
            aOutTask->mNativeTaskObj.mEntry  = aEntry;
            aOutTask->mNativeTaskObj.mParams = aParams;

            lThread = PR_CreateThread(PR_USER_THREAD,
                                      nltask_nspr_entry,
                                      aOutTask,
                                      nltask_nspr_map_priority_to_native(aPriority),
                                      PR_GLOBAL_BOUND_THREAD,
                                      PR_UNJOINABLE_THREAD,
                                      aStackSize);

            if (lThread == NULL)
            {
                retval = NLER_ERROR_NO_RESOURCE;
            }
            else
            {

            }
        }
        else
        {
            retval = NLER_ERROR_BAD_STATE;
        }
    }

    if (retval != NLER_SUCCESS)
    {
        NL_LOG_CRIT(lrERTASK, "failed to create task: %s (%d)\n", aName ? aName : "[No name specified]", retval);
    }

    return retval;
}

void nltask_suspend(nltask_t *aTask)
{
}

void nltask_resume(nltask_t *aTask)
{
}

void nltask_set_priority(nltask_t *aTask, int aPriority)
{
    if (aTask != NULL)
    {
        PR_SetThreadPriority((PRThread *)aTask->mNativeTaskObj.mThread, nltask_nspr_map_priority_to_native(aPriority));
    }
}

nltask_t *nltask_get_current(void)
{
    nltask_t   *retval;

    retval = (nltask_t *)PR_GetThreadPrivate(sTaskPtrIdx);

    return retval;
}

void nltask_sleep_ms(nl_time_ms_t aDurationMS)
{
    PR_Sleep(PR_MillisecondsToInterval(aDurationMS));
}

void nltask_yield(void)
{
    PR_Sleep(PR_INTERVAL_NO_WAIT);
}

const char *nltask_get_name(const nltask_t *aTask)
{
    return ((aTask != NULL) ? aTask->mNativeTaskObj.mName : NULL);
}

/**
 *  Estabish a "faked" but sane task structure for the main thread
 *  such that functions such as nltask_get_current() work correctly
 *  when called from it.
 *
 *  @param[out]  aOutTask  A pointer to the task structure to populate
 *                         for use with the main thread.
 *
 *  @retval  #NLER_SUCCESS          on success.
 *  @retval  #NLER_ERROR_FAILURE    on all other errors.
 */
static int nltask_nspr_main_init(nltask_t *aOutTask)
{
    PRThread          *lThread = PR_GetCurrentThread();
    int                retval = NLER_SUCCESS;

    aOutTask->mStackTop              = 0;

    aOutTask->mNativeTaskObj.mEntry  = NULL;
    aOutTask->mNativeTaskObj.mParams = NULL;
    aOutTask->mNativeTaskObj.mName   = NULL;
    aOutTask->mNativeTaskObj.mThread = lThread;

    PR_SetThreadPrivate(sTaskPtrIdx, aOutTask);

    return (retval);
}

/**
 *  Initialize Netscape Portable Runtime (NSPR) task support.
 *
 *  @retval  #NLER_SUCCESS          on success.
 *  @retval  #NLER_ERROR_FAILURE    on all other errors.
 *
 */
int nltask_nspr_init(void)
{
    int retval = NLER_SUCCESS;

    if (sTaskPtrIdx == -1)
    {
        PRUintn     newptridx;
        PRStatus    status;

        status = PR_NewThreadPrivateIndex(&newptridx, NULL);

        if (status == PR_SUCCESS)
        {
            sTaskPtrIdx = (PRIntn)newptridx;
        }
        else
        {
            NL_LOG_CRIT(lrERINIT, "failed to get thread private index for task ptr\n");
            retval = NLER_ERROR_NO_RESOURCE;
        }
    }

    if (retval == NLER_SUCCESS)
    {
        /* Initialize the "faked" main thread task structure
         */

        retval = nltask_nspr_main_init(&sMainTask);
        if (retval != NLER_SUCCESS)
        {
            goto done;
        }
    }

 done:
    return retval;
}


