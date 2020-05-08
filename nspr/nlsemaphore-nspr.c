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

#include <nspr/prcvar.h>
#include <nspr/prerr.h>

#include <nlerassert.h>
#include <nlererror.h>
#include <nlerlock.h>

int nlsemaphore_binary_create(nlsemaphore_t *aSemaphore)
{
    const size_t kMaxCount = 1;
    const size_t kInitialCount = 0;
    int          lRetval = NLER_SUCCESS;

    lRetval = nlsemaphore_counting_create(aSemaphore, kMaxCount, kInitialCount);

    return (lRetval);
}

static int nlsemaphore_nspr_cond_init(nlsemaphore_t *aSemaphore)
{
    PRCondVar *         lCond;
    int                 lRetval = NLER_SUCCESS;

    if ((aSemaphore == NULL) || ((PRLock *)(aSemaphore->mLock) == NULL))
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    lCond = PR_NewCondVar((PRLock *)(aSemaphore->mLock));
    if (lCond == NULL)
    {
        lRetval = NLER_ERROR_NO_RESOURCE;
        goto done;
    }

    aSemaphore->mCondition = lCond;

 done:
    return (lRetval);
}

static int nlsemaphore_nspr_cond_destroy(nlsemaphore_t *aSemaphore)
{
    int lRetval = NLER_SUCCESS;

    if ((aSemaphore == NULL) || (aSemaphore->mCondition == NULL))
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    PR_DestroyCondVar(aSemaphore->mCondition);

    aSemaphore->mCondition = NULL;

 done:
    return (lRetval);
}

static int nlsemaphore_nspr_cond_wait(PRCondVar *aCond, const nl_time_ms_t *aTimeoutMsec)
{
    PRIntervalTime lTimeout;
    PRStatus       lStatus;
    int            lRetval = NLER_SUCCESS;

    if (aTimeoutMsec != NULL)
    {
        lTimeout = PR_MillisecondsToInterval(*aTimeoutMsec);
    }
    else
    {
        lTimeout = PR_INTERVAL_NO_TIMEOUT;
    }

    // NOTE: NSPR does NOT explicitly report timeouts on
    // PR_WaitCondVar and, in cases such as POSIX threads for the
    // underlying implementation, explicitly and intentionally maps
    // ETIMEDOUT to 0.

    lStatus = PR_WaitCondVar(aCond, lTimeout);
    if (lStatus != PR_SUCCESS)
    {
        switch (lStatus)
        {

        case PR_INVALID_ARGUMENT_ERROR:
            lRetval = NLER_ERROR_BAD_INPUT;
            break;

        // Per above, this is unlikely to ever happen. However, if it
        // does, map it correctly.

        case PR_IO_TIMEOUT_ERROR:
            lRetval = NLER_ERROR_NO_RESOURCE;
            break;

        case PR_WOULD_BLOCK_ERROR:
        case PR_PENDING_INTERRUPT_ERROR:
        case PR_DEADLOCK_ERROR:
        case PR_FAILURE:
        default:
            lRetval = NLER_ERROR_FAILURE;
            break;

        }
    }

    return (lRetval);
}

static int nlsemaphore_nspr_cond_signal(PRCondVar *aCond)
{
    PRStatus lStatus;
    int      lRetval = NLER_SUCCESS;

    lStatus = PR_NotifyCondVar(aCond);
    if (lStatus != PR_SUCCESS)
    {
        switch (lStatus)
        {

        case PR_FAILURE:
        default:
            lRetval = NLER_ERROR_FAILURE;
            break;

        }
    }

    return (lRetval);
}

int nlsemaphore_counting_create(nlsemaphore_t *aSemaphore, size_t aMaxCount, size_t aInitialCount)
{
    int lRetval = NLER_SUCCESS;

    if (aSemaphore == NULL)
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    if (aMaxCount == 0)
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    if (aInitialCount > aMaxCount)
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    // The NSPR condition variable needs to be initialized with a
    // lock, so initialize the lock first.

    lRetval = nllock_create(&aSemaphore->mLock);
    if (lRetval != NLER_SUCCESS)
    {
        goto done;
    }

    // Now that the lock has been initialized, attempt to initialized
    // the condition variable.

    lRetval = nlsemaphore_nspr_cond_init(aSemaphore);
    if (lRetval != NLER_SUCCESS)
    {
        goto lock_destroy;
    }

    aSemaphore->mCurrentCount = aInitialCount;
    aSemaphore->mMaxCount = aMaxCount;

 done:
    return (lRetval);

 lock_destroy:
    nllock_destroy(&aSemaphore->mLock);

    return (lRetval);
}

void nlsemaphore_destroy(nlsemaphore_t *aSemaphore)
{
    int     lStatus;

    lStatus = nlsemaphore_nspr_cond_destroy(aSemaphore);
    NLER_ASSERT(lStatus == NLER_SUCCESS);

    nllock_destroy(&aSemaphore->mLock);

    aSemaphore->mCurrentCount = 0;
    aSemaphore->mMaxCount = 0;
}

static int nlsemaphore_take_with_timeout_internal(nlsemaphore_t *aSemaphore, const nl_time_ms_t *aTimeoutMsec)
{
    int     lStatus;
    int     lRetval = NLER_SUCCESS;

    if (aSemaphore == NULL)
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    lRetval = nllock_enter(&aSemaphore->mLock);
    if (lRetval != NLER_SUCCESS)
    {
        goto done;
    }

    if (--aSemaphore->mCurrentCount < 0)
    {
        lRetval = nlsemaphore_nspr_cond_wait(aSemaphore->mCondition, aTimeoutMsec);

        if (lRetval != NLER_SUCCESS)
        {
            aSemaphore->mCurrentCount++;
        }
    }

    lStatus = nllock_exit(&aSemaphore->mLock);
    NLER_ASSERT(lStatus == NLER_SUCCESS);

 done:
    return (lRetval);
}

int nlsemaphore_take(nlsemaphore_t *aSemaphore)
{
    const nl_time_ms_t *kNoTimeout = NULL;

    return (nlsemaphore_take_with_timeout_internal(aSemaphore, kNoTimeout));
}

int nlsemaphore_take_with_timeout(nlsemaphore_t *aSemaphore, nl_time_ms_t aTimeoutMsec)
{
    return (nlsemaphore_take_with_timeout_internal(aSemaphore, &aTimeoutMsec));
}

int nlsemaphore_give(nlsemaphore_t *aSemaphore)
{
    int     lStatus;
    int     lRetval = NLER_SUCCESS;

    if (aSemaphore == NULL)
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    lRetval = nllock_enter(&aSemaphore->mLock);
    if (lRetval != NLER_SUCCESS)
    {
        goto done;
    }

    if (aSemaphore->mCurrentCount == aSemaphore->mMaxCount)
    {
        lRetval = NLER_ERROR_BAD_STATE;
        goto unlock;
    }

    if (aSemaphore->mCurrentCount++ < 0)
    {
        lRetval = nlsemaphore_nspr_cond_signal(aSemaphore->mCondition);
        if (lRetval != NLER_SUCCESS)
        {
            --aSemaphore->mCurrentCount;
        }
    }

 unlock:
    lStatus = nllock_exit(&aSemaphore->mLock);
    NLER_ASSERT(lStatus == NLER_SUCCESS);

 done:
    return (lRetval);
}

int nlsemaphore_give_from_isr(nlsemaphore_t *aSemaphore)
{
    return (nlsemaphore_give(aSemaphore));
}
