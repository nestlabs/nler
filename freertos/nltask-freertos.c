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

int nltask_create(nltask_entry_point_t aEntry, const char *aName, void *aStack, size_t aStackSize, nltask_priority_t aPriority, void *aParams, nltask_t *aTask)
{
    int retval = NLER_ERROR_BAD_INPUT;

    NLER_ASSERT(((uintptr_t)aStack % NLER_REQUIRED_STACK_ALIGNMENT) == 0);

    if ((aTask != NULL) && (aName != NULL) && (aPriority < configMAX_PRIORITIES))
    {
        TaskHandle_t task_handle = (TaskHandle_t)&aTask->mNativeTaskObj;

        aTask->mStackTop = aStack + aStackSize;

        // Now create the task
        task_handle = xTaskCreateStatic(aEntry,
                                        (const char *)aName,
                                        aStackSize / sizeof(portSTACK_TYPE),
                                        aParams,
                                        aPriority,
                                        (StackType_t *)aStack,
                                        &aTask->mNativeTaskObj);

        if (task_handle)
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

void nltask_suspend(nltask_t *aTask)
{
    if (aTask != NULL)
    {
        vTaskSuspend((TaskHandle_t)&aTask->mNativeTaskObj);
    }
}

void nltask_resume(nltask_t *aTask)
{
    if (aTask != NULL)
    {
        vTaskResume((TaskHandle_t)&aTask->mNativeTaskObj);
    }
}

void nltask_set_priority(nltask_t *aTask, nltask_priority_t aPriority)
{
    if (aTask != NULL)
    {
        vTaskPrioritySet((TaskHandle_t)&aTask->mNativeTaskObj, aPriority);
    }
}

nltask_priority_t nltask_get_priority(const nltask_t *aTask)
{
    int priority = -1;
    if (aTask != NULL)
    {
        priority = uxTaskPriorityGet((TaskHandle_t)&aTask->mNativeTaskObj);
    }
    return priority;
}

nltask_t *nltask_get_current(void)
{
    return (nltask_t*)xTaskGetCurrentTaskHandle();
}

void nltask_sleep_ms(nl_time_ms_t aDurationMS)
{
    vTaskDelay(nl_time_ms_to_delay_time_native(aDurationMS));
}

void nltask_yield(void)
{
    taskYIELD();
}

const char *nltask_get_name(const nltask_t *aTask)
{
    return pcTaskGetName((TaskHandle_t)&aTask->mNativeTaskObj);
}

static StaticTask_t sIdleTaskObj;
// don't make this stack static so it's symbol is easier to locate in map file.
// configMINIAM_STACK_SIZE is in units of StackType_t, not bytes, but the
// DEFINE_STACK macro takes bytes.
DEFINE_STACK(idleTaskStack, configMINIMAL_STACK_SIZE*sizeof(StackType_t));

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &sIdleTaskObj;
    *ppxIdleTaskStackBuffer = idleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
