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
 *      Semaphores (binary and counting) implementation for the
 *      Netscape Portable Runtime (NSPR). All of the usual caveats
 *      surrounding the use of semaphores in general apply. Semaphores
 *      beget deadlocks. Use with care and avoid unless absolutely
 *      necessary.
 *
 */

#include <nlersemaphore.h>

#include <nlerassert.h>
#include <nlererror.h>
#include <nlerlock.h>

int nlsemaphore_binary_create(nlsemaphore_t *aSemaphore)
{
    int          lRetval = NLER_ERROR_NOT_IMPLEMENTED;

    return (lRetval);
}

int nlsemaphore_counting_create(nlsemaphore_t *aSemaphore, size_t aMaxCount, size_t aInitialCount)
{
    int          lRetval = NLER_ERROR_NOT_IMPLEMENTED;

    return (lRetval);
}

void nlsemaphore_destroy(nlsemaphore_t *aSemaphore)
{
    return;
}

int nlsemaphore_take(nlsemaphore_t *aSemaphore)
{
    int          lRetval = NLER_ERROR_NOT_IMPLEMENTED;

    return (lRetval);
}

int nlsemaphore_take_with_timeout(nlsemaphore_t *aSemaphore, nl_time_ms_t aTimeoutMsec)
{
    int          lRetval = NLER_ERROR_NOT_IMPLEMENTED;

    return (lRetval);
}

int nlsemaphore_give(nlsemaphore_t *aSemaphore)
{
    int          lRetval = NLER_ERROR_NOT_IMPLEMENTED;

    return (lRetval);
}

int nlsemaphore_give_from_isr(nlsemaphore_t *aSemaphore)
{
    int          lRetval = NLER_ERROR_NOT_IMPLEMENTED;

    return (lRetval);
}
