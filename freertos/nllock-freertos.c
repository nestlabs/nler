/*
 *
 *    Copyright (c) 2014-2016 Nest Labs, Inc.
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
 *      This file implements NLER binary (mutex) locks under the
 *      FreeRTOS build platform.
 *
 */

#include "nlerlock.h"
#include "nlererror.h"
#include "FreeRTOS.h"
#include "semphr.h"

int nllock_create(nllock_t *aLock)
{
    SemaphoreHandle_t semaphore_handle = xSemaphoreCreateMutexStatic(aLock);
    return ((semaphore_handle == NULL) ? NLER_ERROR_BAD_INPUT : NLER_SUCCESS);
}

void nllock_destroy(nllock_t *aLock)
{
#ifdef xSemaphoreDelete
    xSemaphoreDelete((SemaphoreHandle_t)aLock);
#endif
}

int nllock_enter(nllock_t *aLock)
{
    int retval;

    if (pdTRUE != xSemaphoreTake((SemaphoreHandle_t)aLock, portMAX_DELAY))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    return retval;
}

int nllock_enter_with_timeout(nllock_t *aLock, nl_time_ms_t aTimeoutMsec)
{
    int retval;

    if (pdTRUE != xSemaphoreTake((SemaphoreHandle_t)aLock, nl_time_ms_to_delay_time_native(aTimeoutMsec)))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    return retval;
}

int nllock_exit(nllock_t *aLock)
{
    int retval;

    if (pdTRUE != xSemaphoreGive((SemaphoreHandle_t)aLock))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    return retval;
}

int nlrecursive_lock_create(nlrecursive_lock_t *aLock)
{
    SemaphoreHandle_t semaphore_handle = xSemaphoreCreateRecursiveMutexStatic(aLock);
    return ((semaphore_handle == NULL) ? NLER_ERROR_BAD_INPUT : NLER_SUCCESS);
}

void nlrecursive_lock_destroy(nlrecursive_lock_t *aLock)
{
#ifdef xSemaphoreDelete
    xSemaphoreDelete((SemaphoreHandle_t)aLock);
#endif
}

int nlrecursive_lock_enter(nlrecursive_lock_t *aLock)
{
    int retval;

    if (pdTRUE != xSemaphoreTakeRecursive((SemaphoreHandle_t)aLock, portMAX_DELAY))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    return retval;
}

int nlrecursive_lock_enter_with_timeout(nlrecursive_lock_t *aLock, nl_time_ms_t aTimeoutMsec)
{
    int retval;

    if (pdTRUE != xSemaphoreTakeRecursive((SemaphoreHandle_t)aLock, nl_time_ms_to_delay_time_native(aTimeoutMsec)))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    return retval;
}

int nlrecursive_lock_exit(nlrecursive_lock_t *aLock)
{
    int retval;

    if (pdTRUE != xSemaphoreGiveRecursive((SemaphoreHandle_t)aLock))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    return retval;
}
