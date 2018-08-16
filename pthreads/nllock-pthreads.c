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

#include <stdlib.h>

#include <nlererror.h>
#include <nlererror.h>
#include <nlerlock.h>

#include <pthread.h>

static int nlpthreads_lock_create(nllock_t *aLock, int aType)
{
    int                   retval = NLER_SUCCESS;
    pthread_mutex_t      *lLock = (pthread_mutex_t *)aLock;
    pthread_mutexattr_t   mutexattr;
    int                   status;

    if (lLock == NULL)
    {
        retval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    status = pthread_mutexattr_init(&mutexattr);
    if (status != 0)
    {
        goto done;
    }

    status = pthread_mutexattr_settype(&mutexattr, aType);
    if (status != 0)
    {
        goto mutexattr_destroy;
    }

    status = pthread_mutex_init(aLock, &mutexattr);
    if (status != 0)
    {
        goto mutexattr_destroy;
    }

    return (retval);

mutexattr_destroy:
    pthread_mutexattr_destroy(&mutexattr);

 done:
    return (retval);
}

static void nlpthreads_lock_destroy(pthread_mutex_t *aLock)
{
    if (aLock != NULL)
    {
        pthread_mutex_destroy(aLock);
    }
}

static int nlpthreads_lock_enter(pthread_mutex_t *aLock)
{
    int              retval = NLER_SUCCESS;
    int              status;

    status = pthread_mutex_lock(aLock);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
    }

    return retval;
}

static int nlpthreads_lock_exit(pthread_mutex_t *aLock)
{
    int              retval = NLER_SUCCESS;
    int              status;

    status = pthread_mutex_unlock(aLock);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
    }

    return retval;
}

int nllock_create(nllock_t *aLock)
{
    return (nlpthreads_lock_create(aLock, PTHREAD_MUTEX_DEFAULT));
}

void nllock_destroy(nllock_t *aLock)
{
    pthread_mutex_t *lLock = (pthread_mutex_t *)aLock;

    nlpthreads_lock_destroy(lLock);
}

int nllock_enter(nllock_t *aLock)
{
    pthread_mutex_t *lLock = (pthread_mutex_t *)aLock;

    return (nlpthreads_lock_enter(lLock));
}

int nllock_exit(nllock_t *aLock)
{
    pthread_mutex_t *lLock = (pthread_mutex_t *)aLock;

    return (nlpthreads_lock_exit(lLock));
}

int nlrecursive_lock_create(nlrecursive_lock_t *aLock)
{
    return (nlpthreads_lock_create(aLock, PTHREAD_MUTEX_RECURSIVE));
}

void nlrecursive_lock_destroy(nlrecursive_lock_t *aLock)
{
    pthread_mutex_t *lLock = (pthread_mutex_t *)aLock;

    nlpthreads_lock_destroy(lLock);
}

int nlrecursive_lock_enter(nlrecursive_lock_t *aLock)
{
    pthread_mutex_t *lLock = (pthread_mutex_t *)aLock;

    return (nlpthreads_lock_enter(lLock));
}

int nlrecursive_lock_exit(nlrecursive_lock_t *aLock)
{
    pthread_mutex_t *lLock = (pthread_mutex_t *)aLock;

    return (nlpthreads_lock_exit(lLock));
}
