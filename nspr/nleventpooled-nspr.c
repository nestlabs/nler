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

#include <stdlib.h>

#include <nlererror.h>
#include <nlerlog.h>
#include <nlereventpooled.h>

#include <nspr/prlock.h>

typedef struct nlevent_pool_nspr_s
{
    PRLock  *mLock;
    int      mFreeEvents;
} nlevent_pool_nspr_t;

int nlevent_pool_create(void *aPoolMemory, int32_t aPoolMemorySize, nlevent_pool_t *aPoolObj)
{
    nlevent_pool_nspr_t         *lPool;
    int                          retval = NLER_SUCCESS;

    if ((aPoolMemory == NULL) || (aPoolMemorySize == 0) || (aPoolObj == NULL))
    {
        retval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    lPool = (nlevent_pool_nspr_t *)calloc(1, sizeof(nlevent_pool_nspr_t));

    if (lPool != NULL)
    {
        lPool->mLock = PR_NewLock();

        if (lPool->mLock != NULL)
        {
            lPool->mFreeEvents = aPoolMemorySize / sizeof(nlevent_pooled_t);

            *aPoolObj = (nlevent_pool_t)lPool;
        }
        else
        {
            NL_LOG_CRIT(lrERPOOLED, "failed to allocate event pool lock\n");

            free(lPool);

            retval = NLER_ERROR_NO_RESOURCE;
        }
    }
	else
	{
        retval = NLER_ERROR_NO_MEMORY;
	}

done:
    return (nlevent_pool_t)retval;
}

void nlevent_pool_destroy(nlevent_pool_t *aPool)
{
    nlevent_pool_nspr_t         *lPool = *(nlevent_pool_nspr_t **)aPool;

    if (lPool != NULL)
    {
        if (lPool->mLock != NULL)
        {
            PR_DestroyLock(lPool->mLock);
            lPool->mLock = NULL;
        }

        free(lPool);
    }
}

nlevent_pooled_t *nlevent_pool_get_event(nlevent_pool_t *aPool)
{
    nlevent_pooled_t            *retval = NULL;
    nlevent_pool_nspr_t         *lPool = *(nlevent_pool_nspr_t **)aPool;
    int                          freeevs;

    PR_Lock(lPool->mLock);

    freeevs = --lPool->mFreeEvents;

    PR_Unlock(lPool->mLock);

    if (freeevs >= 0)
    {
        retval = (nlevent_pooled_t *)malloc(sizeof(nlevent_pooled_t));
    }
    else
    {
        NL_LOG_DEBUG(lrERPOOLED, "no more events in event pool\n");
    }

    return retval;
}

void nlevent_pool_recycle_event(nlevent_pool_t *aPool, nlevent_pooled_t *aEvent)
{
    nlevent_pool_nspr_t *lPool = *(nlevent_pool_nspr_t **)aPool;

    PR_Lock(lPool->mLock);

    lPool->mFreeEvents++;

    PR_Unlock(lPool->mLock);

    free(aEvent);
}
