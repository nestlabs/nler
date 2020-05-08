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

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include <nlerassert.h>
#include <nlererror.h>
#include <nlerlock.h>

#include <pthread.h>

static int nlpthreads_lock_create(nllock_t *aLock, int aType)
{
    pthread_mutex_t      *lLock = (pthread_mutex_t *)aLock;
    pthread_mutexattr_t   lMutexAttr;
    int                   lStatus;
    int                   lRetval = NLER_SUCCESS;

    if (lLock == NULL)
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    lStatus = pthread_mutexattr_init(&lMutexAttr);
    if (lStatus != 0)
    {
        switch (lStatus)
        {

        case EINVAL:
            lRetval = NLER_ERROR_BAD_INPUT;
            break;

        case ENOMEM:
            lRetval = NLER_ERROR_NO_MEMORY;
            break;

        default:
            lRetval = NLER_ERROR_FAILURE;
            break;

        }

        goto done;
    }

    lStatus = pthread_mutexattr_settype(&lMutexAttr, aType);
    if (lStatus != 0)
    {
        switch (lStatus)
        {

        case EINVAL:
            lRetval = NLER_ERROR_BAD_INPUT;
            break;

        default:
            lRetval = NLER_ERROR_FAILURE;
            break;

        }

        goto mutexattr_destroy;
    }

    lStatus = pthread_mutex_init(aLock, &lMutexAttr);
    if (lStatus != 0)
    {
        switch (lStatus)
        {

        case EAGAIN:
            lRetval = NLER_ERROR_NO_RESOURCE;
            break;

        case ENOMEM:
            lRetval = NLER_ERROR_NO_MEMORY;
            break;

        case EBUSY:
            lRetval = NLER_ERROR_BAD_STATE;
            break;

        case EINVAL:
            lRetval = NLER_ERROR_BAD_INPUT;
            break;

        case EPERM:
        default:
            lRetval = NLER_ERROR_INIT;
            break;
        }

        goto mutexattr_destroy;
    }

mutexattr_destroy:
    lStatus = pthread_mutexattr_destroy(&lMutexAttr);
    if (lStatus != 0)
    {
        lRetval = NLER_ERROR_INIT;
    }

 done:
    return (lRetval);
}

static void nlpthreads_lock_destroy(pthread_mutex_t *aLock)
{
    int lStatus;

    if (aLock != NULL)
    {
        lStatus = pthread_mutex_destroy(aLock);
        NLER_ASSERT(lStatus == 0);
    }
}

static int nlpthreads_lock_enter(pthread_mutex_t *aLock)
{
    int              lRetval = NLER_SUCCESS;
    int              lStatus;

    if (aLock == NULL)
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    lStatus = pthread_mutex_lock(aLock);
    if (lStatus != 0)
    {
        switch (lStatus)
        {

        case EINVAL:
        case EDEADLK:
            lRetval = NLER_ERROR_BAD_STATE;
            break;

        default:
            lRetval = NLER_ERROR_FAILURE;
            break;

        }

        goto done;
    }

 done:
    return lRetval;
}

static int nlpthreads_lock_exit(pthread_mutex_t *aLock)
{
    int              lRetval = NLER_SUCCESS;
    int              lStatus;

    if (aLock == NULL)
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    lStatus = pthread_mutex_unlock(aLock);
    if (lStatus != 0)
    {
        switch (lStatus)
        {

        case EINVAL:
            lRetval = NLER_ERROR_BAD_STATE;
            break;

        case EPERM:
        default:
            lRetval = NLER_ERROR_FAILURE;
            break;

        }

        goto done;
    }

 done:
    return (lRetval);
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
