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
 *      This file implements NLER pooled events under the POSIX
 *      threads (pthreads) build platform.
 *
 */

#include <pthread.h>
#include <stdlib.h>

#include <nlererror.h>
#include <nlerlog.h>
#include <nlereventpooled.h>

typedef struct nlevent_pool_pthreads_s
{
    pthread_mutex_t mLock;
    int             mFreeEvents;
} nlevent_pool_pthreads_t;

int nlevent_pool_create(void *aPoolMemory, int32_t aPoolMemorySize, nlevent_pool_t *aPoolObj)
{
    nlevent_pool_pthreads_t     *lPool;
    int                          status;
    int                          retval = NLER_SUCCESS;
    pthread_mutexattr_t          mutexattr;

    if ((aPoolMemory == NULL) || (aPoolMemorySize == 0) || (aPoolObj == NULL))
    {
        retval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    status = pthread_mutexattr_init(&mutexattr);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    lPool = (nlevent_pool_pthreads_t *)calloc(1, sizeof(nlevent_pool_pthreads_t));
    if (lPool == NULL)
    {
        retval = NLER_ERROR_NO_MEMORY;
        goto mutexattr_destroy;
    }

    status = pthread_mutex_init(&lPool->mLock, &mutexattr);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto dealloc;
    }

    lPool->mFreeEvents = aPoolMemorySize / sizeof(nlevent_pooled_t);

    *aPoolObj = (nlevent_pool_t)lPool;

    return (retval);

 dealloc:
    free(lPool);

mutexattr_destroy:
    pthread_mutexattr_destroy(&mutexattr);

 done:
    NL_LOG_CRIT(lrERPOOLED, "failed to allocate event pool lock\n");

    return (retval);
}

void nlevent_pool_destroy(nlevent_pool_t *aPool)
{
    nlevent_pool_pthreads_t    *lPool = *(nlevent_pool_pthreads_t **)aPool;

    if (lPool != NULL)
    {
        pthread_mutex_destroy(&lPool->mLock);

        free(lPool);
    }
}

nlevent_pooled_t *nlevent_pool_get_event(nlevent_pool_t *aPool)
{
    nlevent_pooled_t            *retval = NULL;
    nlevent_pool_pthreads_t     *lPool = *(nlevent_pool_pthreads_t **)aPool;
    int                          freeevs, status;

    if (lPool != NULL)
    {
        status = pthread_mutex_lock(&lPool->mLock);
        if (status != 0)
        {
            goto done;
        }

        freeevs = --lPool->mFreeEvents;

        status = pthread_mutex_unlock(&lPool->mLock);
        if (status != 0)
        {
            goto done;
        }

        if (freeevs >= 0)
        {
            retval = (nlevent_pooled_t *)malloc(sizeof(nlevent_pooled_t));
        }
        else
        {
            NL_LOG_DEBUG(lrERPOOLED, "no more events in event pool\n");
        }
    }

 done:
    return retval;
}

void nlevent_pool_recycle_event(nlevent_pool_t *aPool, nlevent_pooled_t *aEvent)
{
    nlevent_pool_pthreads_t *lPool = *(nlevent_pool_pthreads_t **)aPool;

    if (lPool != NULL)
    {
        pthread_mutex_lock(&lPool->mLock);

        lPool->mFreeEvents++;

        pthread_mutex_unlock(&lPool->mLock);

        if (aEvent != NULL)
        {
            free(aEvent);
        }
    }
}
