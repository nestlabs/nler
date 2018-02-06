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

static PRIntn taskptridx = -1;

static PRThreadPriority map_thread_pri(nl_task_priority_t aPriority)
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

static void global_entry(void *aClosure)
{
    nl_task_t               *task = (nl_task_t *)aClosure;
    nl_task_entry_point_t   entry;

    entry = (nl_task_entry_point_t)task->mNativeTask;

    task->mNativeTask = (nl_task_entry_point_t)PR_GetCurrentThread();

    PR_SetThreadPrivate(taskptridx, task);

    PR_SetCurrentThreadName(task->mName);

    (*entry)(task->mParams);
}

int nl_task_create(nl_task_entry_point_t aEntry, const char *aName, void *aStack, size_t aStackSize, nl_task_priority_t aPriority, void *aParams, nl_task_t *aOutTask)
{
    PRThread    *thread;
    int         retval = NLER_SUCCESS;

    if ((aOutTask != NULL) && (aName != NULL))
    {
        if (taskptridx != -1)
        {
            aStackSize = (aStackSize + 0x1fff) & ~0x1fff;

            aOutTask->mName = aName;
            aOutTask->mStack = aStack;
            aOutTask->mStackSize = aStackSize;
            aOutTask->mPriority = aPriority;
            aOutTask->mParams = aParams;
            aOutTask->mNativeTask = aEntry;

            thread = PR_CreateThread(PR_USER_THREAD, global_entry, aOutTask,
                                     map_thread_pri(aPriority), PR_GLOBAL_BOUND_THREAD,
                                     PR_JOINABLE_THREAD, aStackSize);

            if (thread == NULL)
            {
                retval = NLER_ERROR_NO_RESOURCE;
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

void nl_task_suspend(nl_task_t *aTask)
{
}

void nl_task_resume(nl_task_t *aTask)
{
}

void nl_task_set_priority(nl_task_t *aTask, int aPriority)
{
    if (aTask != NULL)
    {
        aTask->mPriority = aPriority;
        PR_SetThreadPriority((PRThread *)aTask->mNativeTask, map_thread_pri(aPriority));
    }
}

nl_task_t *nl_task_get_current(void)
{
    nl_task_t   *retval;

    retval = (nl_task_t *)PR_GetThreadPrivate(taskptridx);

    return retval;
}

void nl_task_sleep_ms(nl_time_ms_t aDurationMS)
{
    PR_Sleep(PR_MillisecondsToInterval(aDurationMS));
}

void nl_task_yield(void)
{
    PR_Sleep(PR_INTERVAL_NO_WAIT);
}

PRIntn *nl_er_nspr_get_taskptridx(void)
{
    return &taskptridx;
}


