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
 *      This file implements NLER event queues under the POSIX threads
 *      (pthreads) build platform.
 *
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nlereventqueue.h>
#include <nlerlog.h>
#include <nlererror.h>

#include <pthread.h>

#include <sys/select.h>

#if NLER_FEATURE_SIMULATEABLE_TIME
#include "nlereventqueue_sim.h"
#endif

#define kPipeMagicOctet '\x38'

enum
{
    kReadDescriptor  = 0,
    kWriteDescriptor = 1,

    kMaxDescriptors
};

typedef struct nleventqueue_pthreads_s
{
    pthread_mutex_t   mLock;
    int               mPipe[kMaxDescriptors];
    nl_event_t      **mQueueMemory;
    size_t            mQueueSize;
    size_t            mQueueEnd;
#if NLER_FEATURE_SIMULATEABLE_TIME
    bool              mPrevGetSuccessful;
#endif
} nleventqueue_pthreads_t;


int nleventqueue_create(void *aQueueMemory, size_t aQueueMemorySize, nleventqueue_t *aOutQueue)
{
    const size_t              lQueueSize = (aQueueMemorySize / sizeof (nl_event_t *));
    nleventqueue_pthreads_t  *lQueue = NULL;
    int                       status;
    int                       retval = NLER_SUCCESS;
    pthread_mutexattr_t       mutexattr;

    if ((aQueueMemory == NULL) || (aQueueMemorySize == 0) || (aOutQueue == NULL))
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

    lQueue = (nleventqueue_pthreads_t *)calloc(1, sizeof (nleventqueue_pthreads_t));
    if (lQueue == NULL)
    {
        retval = NLER_ERROR_NO_MEMORY;
        goto mutexattr_destroy;
    }

    status = pipe(lQueue->mPipe);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto dealloc;
    }

    status = pthread_mutex_init(&lQueue->mLock, &mutexattr);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto close;
    }

    lQueue->mQueueMemory = (nl_event_t **)aQueueMemory;
    lQueue->mQueueSize   = lQueueSize;
    lQueue->mQueueEnd    = 0;

    *aOutQueue = (nleventqueue_t)lQueue;

    return (retval);

 close:
    close(lQueue->mPipe[kReadDescriptor]);
    close(lQueue->mPipe[kWriteDescriptor]);

 dealloc:
    free(lQueue);

mutexattr_destroy:
    pthread_mutexattr_destroy(&mutexattr);

 done:
    return (retval);
}

void nleventqueue_destroy(nleventqueue_t *aEventQueue)
{
    nleventqueue_pthreads_t  *lEventQueue = *(nleventqueue_pthreads_t **)aEventQueue;

    if (lEventQueue != NULL)
    {
        pthread_mutex_destroy(&lEventQueue->mLock);

        close(lEventQueue->mPipe[kReadDescriptor]);
        close(lEventQueue->mPipe[kWriteDescriptor]);

        free(lEventQueue);
    }
}

void nleventqueue_disable_event_counting(nleventqueue_t *aEventQueue)
{
    return;
}

int nleventqueue_post_event(nleventqueue_t *aEventQueue, const nl_event_t *aEvent)
{
    static const uint8_t      magic[1] = { kPipeMagicOctet };
    int                       status;
    int                       retval = NLER_SUCCESS;
    nleventqueue_pthreads_t  *lEventQueue = *(nleventqueue_pthreads_t **)aEventQueue;

    status = pthread_mutex_lock(&lEventQueue->mLock);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    if (lEventQueue->mQueueEnd < lEventQueue->mQueueSize)
    {
        lEventQueue->mQueueMemory[lEventQueue->mQueueEnd] = (nl_event_t *)aEvent;
        lEventQueue->mQueueEnd++;

        status = write(lEventQueue->mPipe[kWriteDescriptor], &magic[0], sizeof (magic));
        if (status != sizeof (magic))
        {
            retval = NLER_ERROR_FAILURE;
            goto unlock;
        }
    }
    else
    {
        retval = NLER_ERROR_NO_RESOURCE;
        goto unlock;
    }

 unlock:
    status = pthread_mutex_unlock(&lEventQueue->mLock);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    if (retval == NLER_ERROR_NO_RESOURCE)
    {
        //don't log while holding the lock

        NL_LOG_CRIT(lrERQUEUE, "attempt to post event (%d) to full queue %p with size %d\n",
                    aEvent->mType, lEventQueue->mQueueMemory, lEventQueue->mQueueSize);

#if NLER_ASSERT_ON_FULL_QUEUE
        NLER_ASSERT(0);
#endif
    }
#if NLER_FEATURE_SIMULATEABLE_TIME
    else
    {
        nl_eventqueue_sim_count_inc();
    }
#endif

 done:
    return retval;
}

static nl_event_t *nleventqueue_pthreads_remove_event(nleventqueue_pthreads_t *aQueue)
{
    nl_event_t  *retval;

    retval = aQueue->mQueueMemory[0];

    aQueue->mQueueEnd--;

    if (aQueue->mQueueEnd > 0)
    {
        memmove(aQueue->mQueueMemory, &aQueue->mQueueMemory[1], aQueue->mQueueEnd * sizeof(nl_event_t *));
    }

    return retval;
}

/**
 * NOTE: This function isn't intended for use by clients of NLER.
 *
 * This exists to let NLER functions get events from a queue without incurring a
 * +1 tick offset when converting milliseconds to ticks.
 */
extern nl_event_t *nleventqueue_get_event_with_timeout_native(nleventqueue_t *aEventQueue, nl_time_native_t aTimeoutNative);

nl_event_t *nleventqueue_get_event_with_timeout_native(nleventqueue_t *aEventQueue, nl_time_native_t aTimeoutNative)
{
    int                       status;
    nl_event_t               *retval = NULL;
    nleventqueue_pthreads_t  *lEventQueue = *(nleventqueue_pthreads_t **)aEventQueue;

    status = pthread_mutex_lock(&lEventQueue->mLock);
    if (status != 0)
    {
        retval = NULL;
        goto done;
    }

#if NLER_FEATURE_SIMULATEABLE_TIME
    if (lEventQueue->mPrevGetSuccessful == true)
    {
        nl_eventqueue_sim_count_dec();
    }
#endif

    if (lEventQueue->mQueueEnd > 0)
    {
        retval = nleventqueue_pthreads_remove_event(lEventQueue);
    }

    pthread_mutex_unlock(&lEventQueue->mLock);

    while (retval == NULL)
    {
        int             nfds;
        fd_set          rds, wds, xds;
        struct timeval  timeout;

        FD_ZERO(&rds);
        FD_ZERO(&wds);
        FD_ZERO(&xds);

        FD_SET(lEventQueue->mPipe[kReadDescriptor], &rds);

        nfds = lEventQueue->mPipe[kReadDescriptor] + 1;

        timeout.tv_sec  = aTimeoutNative / 1000;
        timeout.tv_usec = (aTimeoutNative % 1000) * 1000;

        status = select(nfds, &rds, &wds, &xds, &timeout);

        if (status == 1)
        {
            char buf[1];

            status = read(lEventQueue->mPipe[kReadDescriptor], &buf, sizeof (buf));
            if (status == -1)
            {
                retval = NULL;
                goto done;
            }
            else if (status == 1 && buf[0] != kPipeMagicOctet)
            {
                retval = NULL;
                goto done;
            }

            status = pthread_mutex_lock(&lEventQueue->mLock);
            if (status != 0)
            {
                retval = NULL;
                goto done;
            }

            if (lEventQueue->mQueueEnd > 0)
            {
                retval = nleventqueue_pthreads_remove_event(lEventQueue);
            }
            
            pthread_mutex_unlock(&lEventQueue->mLock);
        }
        else
        {
            break;
        }
    }

 done:
#if NLER_FEATURE_SIMULATEABLE_TIME
    lEventQueue->mPrevGetSuccessful = ((retval != NULL) ? true : false);
#endif

    return retval;
}

nl_event_t *nleventqueue_get_event_with_timeout(nleventqueue_t *aEventQueue, nl_time_ms_t aTimeoutMS)
{
    nl_event_t              *retval;

    retval = nleventqueue_get_event_with_timeout_native(aEventQueue, aTimeoutMS);

    return retval;
}

uint32_t nleventqueue_get_count(nleventqueue_t *aEventQueue)
{
    const nleventqueue_pthreads_t  *lEventQueue = *(nleventqueue_pthreads_t **)aEventQueue;

    return (lEventQueue->mQueueEnd);
}
