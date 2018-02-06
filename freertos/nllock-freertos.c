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

nl_lock_t nl_er_lock_create(void)
{
    nl_lock_t   retval = NULL;

    retval = (nl_lock_t)xSemaphoreCreateMutex();

    return retval;
}

void nl_er_lock_destroy(nl_lock_t aLock)
{
#ifdef xSemaphoreDelete
    xSemaphoreDelete((xSemaphoreHandle)aLock);
#endif
}

int nl_er_lock_enter(nl_lock_t aLock)
{
    int retval = NLER_SUCCESS;

    if (pdTRUE != xSemaphoreTake((xSemaphoreHandle)aLock, portMAX_DELAY))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }

    return retval;
}

int nl_er_lock_exit(nl_lock_t aLock)
{
    int retval = NLER_SUCCESS;

    if (pdTRUE != xSemaphoreGive((xSemaphoreHandle)aLock))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }

    return retval;
}

nl_recursive_lock_t nl_er_recursive_lock_create(void)
{
    nl_recursive_lock_t   retval = NULL;

    retval = (nl_recursive_lock_t)xSemaphoreCreateRecursiveMutex();

    return retval;
}

void nl_er_recursive_lock_destroy(nl_recursive_lock_t aLock)
{
#ifdef xSemaphoreDelete
    xSemaphoreDelete((xSemaphoreHandle)aLock);
#endif
}

int nl_er_recursive_lock_enter(nl_recursive_lock_t aLock)
{
    int retval = NLER_SUCCESS;

    if (pdTRUE != xSemaphoreTakeRecursive((xSemaphoreHandle)aLock, portMAX_DELAY))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }

    return retval;
}

int nl_er_recursive_lock_exit(nl_recursive_lock_t aLock)
{
    int retval = NLER_SUCCESS;

    if (pdTRUE != xSemaphoreGiveRecursive((xSemaphoreHandle)aLock))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }

    return retval;
}
