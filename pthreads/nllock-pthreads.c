/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *      This file implements NLER binary (mutex) locks under the POSIX
 *      threads (pthreads) build platform.
 *
 */

#include <nlererror.h>
#include <nlererror.h>
#include <nlerlock.h>

#include <pthread.h>

nl_lock_t nl_er_lock_create(void)
{
    nl_lock_t   retval = NULL;

    return retval;
}

void nl_er_lock_destroy(nl_lock_t aLock)
{

}

int nl_er_lock_enter(nl_lock_t aLock)
{
    return NLER_SUCCESS;
}

int nl_er_lock_exit(nl_lock_t aLock)
{
    return NLER_SUCCESS;
}

nl_recursive_lock_t nl_er_recursive_lock_create(void)
{
    return NULL;
}

void nl_er_recursive_lock_destroy(nl_recursive_lock_t aLock)
{
    return;
}

int nl_er_recursive_lock_enter(nl_recursive_lock_t aLock)
{
    return NLER_ERROR_NOT_IMPLEMENTED;
}

int nl_er_recursive_lock_exit(nl_recursive_lock_t aLock)
{
    return NLER_ERROR_NOT_IMPLEMENTED;
}
