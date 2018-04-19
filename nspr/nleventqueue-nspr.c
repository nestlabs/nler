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
 *      This file implements NLER event queues under the Netscape
 *      Portable Runtime (NSPR) build platform.
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "nlereventqueue.h"
#include <nspr/prlock.h>
#include <nspr/prio.h>
#include "nlerlog.h"
#include "nlererror.h"

#if NLER_FEATURE_SIMULATEABLE_TIME
#include "nlereventqueue_sim.h"
#endif

/* the queueing used here is a simple sliding array rather
 * than a more efficient circular list. the goal is simplicity
 * and readablity. there is no point in making this efficient
 * at the cost of added complexity where it need not be. mmp
 */

typedef struct nl_eventqueue_nspr_s
{
    PRLock      *mLock;
    PRFileDesc  *mPollableEvent;
    nl_event_t  **mQueue;
    size_t      mQueueSize;
    size_t      mQueueEnd;
#if NLER_FEATURE_SIMULATEABLE_TIME
    bool prev_get_successful;
#endif
} nl_eventqueue_nspr_t;

nl_eventqueue_t nl_eventqueue_create(void *aQueueMemory, size_t aQueueMemorySize)
{
    nl_eventqueue_nspr_t *retval = NULL;
    size_t               qsize = aQueueMemorySize / sizeof(nl_event_t *);
    int                  err = NLER_ERROR_FAILURE;

    if ((aQueueMemory != NULL) && (qsize > 0))
    {
        retval = (nl_eventqueue_nspr_t *)calloc(1, sizeof(nl_eventqueue_nspr_t));

        if (retval != NULL)
        {
            retval->mPollableEvent = PR_NewPollableEvent();

            if (retval->mPollableEvent != NULL)
            {
                retval->mLock = PR_NewLock();

                if (retval->mLock != NULL)
                {
                    retval->mQueue = (nl_event_t **)aQueueMemory;
                    retval->mQueueSize = qsize;
                    retval->mQueueEnd = 0;

                    err = NLER_SUCCESS;
                }
                else
                {
                    NL_LOG_CRIT(lrERQUEUE, "failed to allocate lock for nspr event queue\n");
                    err = NLER_ERROR_NO_RESOURCE;
                }
            }
            else
            {
                NL_LOG_CRIT(lrERQUEUE, "failed to allocate pollable event for nspr event queue\n");
                err = NLER_ERROR_NO_RESOURCE;
            }

            if (err != NLER_SUCCESS)
            {
                nl_eventqueue_destroy((nl_eventqueue_t)retval);
                retval = NULL;
            }
        }
        else
        {
            NL_LOG_CRIT(lrERQUEUE, "failed to allocate %d bytes for nspr event queue\n", sizeof(nl_eventqueue_nspr_t));
            err = NLER_ERROR_NO_MEMORY;
        }
    }
    else
    {
        NL_LOG_CRIT(lrERQUEUE, "invalid queue memory %p with size %d specified\n", aQueueMemory, aQueueMemorySize);
        err = NLER_ERROR_BAD_INPUT;
    }

    return (nl_eventqueue_t)retval;
}

void nl_eventqueue_destroy(nl_eventqueue_t aEventQueue)
{
    nl_eventqueue_nspr_t    *queue = (nl_eventqueue_nspr_t *)aEventQueue;

    if (queue->mPollableEvent != NULL)
    {
        PR_DestroyPollableEvent(queue->mPollableEvent);
    }

    if (queue->mLock != NULL)
    {
        PR_DestroyLock(queue->mLock);
    }

    free(queue);
}

int nl_eventqueue_post_event(nl_eventqueue_t aEventQueue, const nl_event_t *aEvent)
{
    int                     retval = NLER_SUCCESS;
    nl_eventqueue_nspr_t    *queue = (nl_eventqueue_nspr_t *)aEventQueue;

    PR_Lock(queue->mLock);

    if (queue->mQueueEnd < queue->mQueueSize)
    {
        queue->mQueue[queue->mQueueEnd] = (nl_event_t *)aEvent;
        queue->mQueueEnd++;

        PR_SetPollableEvent(queue->mPollableEvent);
    }
    else
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }

    PR_Unlock(queue->mLock);

    if (retval == NLER_ERROR_NO_RESOURCE)
    {
        //don't log while holding the lock

        NL_LOG_CRIT(lrERQUEUE, "attempt to post event (%d) to full queue %p with size %d\n",
                     aEvent->mType, queue->mQueue, queue->mQueueSize);

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

    return retval;
}

static nl_event_t *remove_event_from_queue(nl_eventqueue_nspr_t *aQueue)
{
    nl_event_t  *retval;

    retval = aQueue->mQueue[0];

    aQueue->mQueueEnd--;

    if (aQueue->mQueueEnd > 0)
        memmove(aQueue->mQueue, &aQueue->mQueue[1], aQueue->mQueueEnd * sizeof(nl_event_t *));

    return retval;
}

/**
 * NOTE: This function isn't intended for use by clients of NLER.
 *
 * This exists to let NLER functions get events from a queue without incurring a
 * +1 tick offset when converting milliseconds to ticks.
 */
nl_event_t *nl_eventqueue_get_event_with_timeout_native(nl_eventqueue_t aEventQueue, nl_time_native_t aTimeoutNative);
nl_event_t *nl_eventqueue_get_event_with_timeout_native(nl_eventqueue_t aEventQueue, nl_time_native_t aTimeoutNative)
{
    nl_event_t              *retval = NULL;
    nl_eventqueue_nspr_t    *queue = (nl_eventqueue_nspr_t *)aEventQueue;
    PRPollDesc              polldesc;
    PRInt32                 active;

    PR_Lock(queue->mLock);

#if NLER_FEATURE_SIMULATEABLE_TIME
    if (queue->prev_get_successful == true)
    {
        nl_eventqueue_sim_count_dec();
    }
#endif

    if (queue->mQueueEnd > 0)
    {
        retval = remove_event_from_queue(queue);
    }

    PR_Unlock(queue->mLock);

    while (retval == NULL)
    {
        polldesc.fd = queue->mPollableEvent;
        polldesc.in_flags = PR_POLL_READ | PR_POLL_EXCEPT;
        polldesc.out_flags = 0;

        active = PR_Poll(&polldesc, 1, aTimeoutNative);

        if (active == 1)
        {
            PR_WaitForPollableEvent(queue->mPollableEvent);

            PR_Lock(queue->mLock);

            if (queue->mQueueEnd > 0)
            {
                retval = remove_event_from_queue(queue);
            }

            PR_Unlock(queue->mLock);
        }
        else
        {
            break;
        }
    }

#if NLER_FEATURE_SIMULATEABLE_TIME
    if (retval)
    {
        queue->prev_get_successful = true;
    }
    else
    {
        queue->prev_get_successful = false;
    }
#endif

    return retval;
}

nl_event_t *nl_eventqueue_get_event_with_timeout(nl_eventqueue_t aEventQueue, nl_time_ms_t aTimeoutMS)
{
    return nl_eventqueue_get_event_with_timeout_native(aEventQueue, PR_MillisecondsToInterval(aTimeoutMS));
}
