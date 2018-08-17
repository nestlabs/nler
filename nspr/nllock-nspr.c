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
 *      This file implements NLER binary (mutex) locks under the
 *      Netscape Portable Runtime (NSPR) build platform.
 *
 */

#include "nlererror.h"
#include "nlerlock.h"
#include "nlererror.h"
#include <nspr/prlock.h>

int nllock_create(nllock_t *aLock)
{
    int      retval = NLER_SUCCESS;
    PRLock  *lLock;

    if (aLock == NULL)
    {
        retval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    lLock = PR_NewLock();
    if (lLock == NULL)
    {
        retval = NLER_ERROR_NO_RESOURCE;
        goto done;
    }

    *aLock = (nllock_t)lLock;

 done:
    return retval;
}

void nllock_destroy(nllock_t *aLock)
{
    PRLock *lLock = *(PRLock **)aLock;

    PR_DestroyLock(lLock);
}

int nllock_enter(nllock_t *aLock)
{
    PRLock *lLock = *(PRLock **)aLock;

    PR_Lock(lLock);

    return NLER_SUCCESS;
}

int nllock_exit(nllock_t *aLock)
{
    PRLock *lLock = *(PRLock **)aLock;

    PR_Unlock(lLock);

    return NLER_SUCCESS;
}

int nlrecursive_lock_create(nlrecursive_lock_t *aLock)
{
    return NLER_ERROR_NOT_IMPLEMENTED;
}

void nlrecursive_lock_destroy(nlrecursive_lock_t *aLock)
{
    return;
}

int nlrecursive_lock_enter(nlrecursive_lock_t *aLock)
{
    return NLER_ERROR_NOT_IMPLEMENTED;
}

int nlrecursive_lock_exit(nlrecursive_lock_t *aLock)
{
    return NLER_ERROR_NOT_IMPLEMENTED;
}
