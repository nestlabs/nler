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
 *      This file implements NLER pooled events under the Netscape
 *      Portable Runtime (NSPR) build platform.
 *
 */

#include "nlerlog.h"
#include "nlereventpooled.h"
#include <nspr/prlock.h>
#include <stdlib.h>

typedef struct nl_event_pool_nspr_s
{
    PRLock  *mLock;
    int     mFreeEvents;
} nl_event_pool_nspr_t;

nl_event_pool_t nl_event_pool_create(void *aPoolMemory, int32_t aPoolMemorySize)
{
    nl_event_pool_nspr_t    *retval;

    retval = (nl_event_pool_t)calloc(1, sizeof(nl_event_pool_nspr_t));

    if (retval != NULL)
    {
        retval->mLock = PR_NewLock();

        if (retval->mLock != NULL)
        {
            retval->mFreeEvents = aPoolMemorySize / sizeof(nl_event_pooled_t);
        }
        else
        {
            NL_LOG_CRIT(lrERPOOLED, "failed to allocate event pool lock\n");

            free(retval);
            retval = NULL;
        }
    }

    return (nl_event_pool_t)retval;
}

void nl_event_pool_destroy(nl_event_pool_t aPool)
{
    nl_event_pool_nspr_t    *pool = (nl_event_pool_nspr_t *)aPool;

    if (pool != NULL)
    {
        if (pool->mLock != NULL)
        {
            PR_DestroyLock(pool->mLock);
            pool->mLock = NULL;
        }

        free(pool);
    }
}

nl_event_pooled_t *nl_event_pool_get_event(nl_event_pool_t aPool)
{
    nl_event_pooled_t       *retval = NULL;
    nl_event_pool_nspr_t    *pool = (nl_event_pool_nspr_t *)aPool;
    int                     freeevs;

    PR_Lock(pool->mLock);

    freeevs = --pool->mFreeEvents;

    PR_Unlock(pool->mLock);

    if (freeevs >= 0)
    {
        retval = (nl_event_pooled_t *)malloc(sizeof(nl_event_pooled_t));
    }
    else
    {
        NL_LOG_DEBUG(lrERPOOLED, "no more events in event pool\n");
    }

    return retval;
}

void nl_event_pool_recycle_event(nl_event_pool_t aPool, nl_event_pooled_t *aEvent)
{
    nl_event_pool_nspr_t    *pool = (nl_event_pool_nspr_t *)aPool;

    PR_Lock(pool->mLock);

    pool->mFreeEvents++;

    PR_Unlock(pool->mLock);

    free(aEvent);
}

