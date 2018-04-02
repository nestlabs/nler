/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file implements NLER tasks under the FreeRTOS build
 *      platform.
 *
 */

#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "nlerassert.h"
#include "nlererror.h"
#include "nlertask.h"
#include "nlerlog.h"
#include "nlertime.h"
#include "nlerassert.h"

#if HAVE_NLER_STACK_SECTION
extern char NLER_STACK_SECTION_START[];
extern char NLER_STACK_SECTION_END[];
#endif // HAVE_NLER_STACK_SECTION

static void global_entry(void *aClosure)
{
    nl_task_t               *task = (nl_task_t *)aClosure;
    nl_task_entry_point_t   entry;

    entry = (nl_task_entry_point_t)task->mNativeTask;

    task->mNativeTask = xTaskGetCurrentTaskHandle();

    NL_LOG_DEBUG(lrERTASK, "setting application tag for task %p ('%s') to %p\n", task->mNativeTask, task->mName, task);

    vTaskSetApplicationTaskTag(task->mNativeTask, (pdTASK_HOOK_CODE)task);

    (*entry)(task->mParams);
}

int nl_task_create(nl_task_entry_point_t aEntry, const char *aName, void *aStack, size_t aStackSize, nl_task_priority_t aPriority, void *aParams, nl_task_t *aOutTask)
{
    int retval = NLER_ERROR_BAD_INPUT;


    NLER_ASSERT(((uintptr_t)aStack % NLER_REQUIRED_STACK_ALIGNMENT) == 0);

    if ((aOutTask != NULL) && (aName != NULL) && (aPriority < configMAX_PRIORITIES))
    {
        portBASE_TYPE   err;
        xTaskHandle     task;

        aOutTask->mName = aName;
        aOutTask->mStack = aStack;
        aOutTask->mStackSize = aStackSize;
        aOutTask->mPriority = aPriority;
        aOutTask->mParams = aParams;
        aOutTask->mNativeTask = (void *)aEntry;

        err = xTaskGenericCreate(global_entry,
                                 (const char * const)aName,
                                 aStackSize / sizeof(portSTACK_TYPE),
                                 aOutTask,
                                 aPriority,
                                 &task,
                                 (portSTACK_TYPE *)aStack,
                                 NULL);

        if (err == pdPASS)
        {
            retval = NLER_SUCCESS;
        }
        else
        {
            retval = NLER_ERROR_NO_RESOURCE;
        }
    }

    if (retval != NLER_SUCCESS)
    {
        NL_LOG_CRIT(lrERTASK, "failed to create task: '%s' with priority %d (%d)\n", aName ? aName : "[No name specified]", aPriority, retval);
    }

    return retval;
}

void nl_task_suspend(nl_task_t *aTask)
{
    if (aTask != NULL)
    {
        vTaskSuspend(aTask->mNativeTask);
    }
}

void nl_task_resume(nl_task_t *aTask)
{
    if (aTask != NULL)
    {
        vTaskResume(aTask->mNativeTask);
    }
}

void nl_task_set_priority(nl_task_t *aTask, int aPriority)
{
    if (aTask != NULL)
    {
        aTask->mPriority = aPriority;
        vTaskPrioritySet(aTask->mNativeTask, aPriority);
    }
}

nl_task_t *nl_task_get_current(void)
{
    nl_task_t  *retval = NULL;
    xTaskHandle task = xTaskGetCurrentTaskHandle();

    if (task != NULL)
    {
        retval = (nl_task_t *)xTaskGetApplicationTaskTag(task);
    }

    return retval;
}

void nl_task_sleep_ms(nl_time_ms_t aDurationMS)
{
    vTaskDelay(nl_time_ms_to_delay_time_native(aDurationMS));
}

void nl_task_yield(void)
{
    taskYIELD();
}

