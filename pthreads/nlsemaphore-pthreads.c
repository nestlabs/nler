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
 *      Semaphores (binary and counting) implementation for POSIX
 *      Threads.  All of the usual caveats surrounding the use of
 *      semaphores in general apply. Semaphores beget deadlocks. Use
 *      with care and avoid unless absolutely necessary.
 *
 *      This implementation takes the portable approach of using a
 *      POSIX thread mutex, condition variable, and discrete counter
 *      rather than using the POSIX
 *      sem_{init,wait,timedwait,post,destroy} since the latter are
 *      deprecated in macOS (Darwin) and sem_timedwait, in particular,
 *      is not available in macOS (Darwin).
 *
 */

#include <nlersemaphore.h>

#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

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

static int nlsemaphore_pthread_cond_init(pthread_cond_t *aCond)
{
    const int           kCondScope = PTHREAD_PROCESS_PRIVATE;
    pthread_condattr_t  lCondAttr;
    int                 lStatus;
    int                 lRetval = NLER_SUCCESS;

    lStatus = pthread_condattr_init(&lCondAttr);
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

    lStatus = pthread_condattr_setpshared(&lCondAttr, kCondScope);
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

        goto condattr_destroy;
    }

    lStatus = pthread_cond_init(aCond, &lCondAttr);
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

        goto condattr_destroy;
    }

condattr_destroy:
    lStatus = pthread_condattr_destroy(&lCondAttr);
    if (lStatus != 0)
    {
        lRetval = NLER_ERROR_INIT;
    }

 done:
    return (lRetval);
}

static int nlsemaphore_pthread_cond_destroy(pthread_cond_t *aCond)
{

    int lStatus;
    int lRetval = NLER_SUCCESS;

    if (aCond == NULL)
    {
        lRetval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    lStatus = pthread_cond_destroy(aCond);
    if (lStatus != 0)
    {
        switch (lStatus)
        {

        case EBUSY:
            lRetval = NLER_ERROR_BAD_STATE;
            break;

        case EINVAL:
        default:
            lRetval = NLER_ERROR_INIT;
            break;

        }
    }

 done:
    return (lRetval);
}

static int nlsemaphore_pthread_cond_wait(pthread_cond_t *aCond, pthread_mutex_t *aLock, const nl_time_ms_t *aTimeoutMsec)
{
    int lStatus;
    int lRetval = NLER_SUCCESS;

    if (aTimeoutMsec != NULL)
    {
        struct timespec lTimeout;

        lTimeout.tv_sec = *aTimeoutMsec / 1000;
        lTimeout.tv_nsec = (*aTimeoutMsec % 1000) * 1000000;

        lStatus = pthread_cond_timedwait(aCond, aLock, &lTimeout);
    }
    else
    {
        lStatus = pthread_cond_wait(aCond, aLock);
    }

    if (lStatus != 0)
    {
        switch (lStatus)
        {

        case EINVAL:
            lRetval = NLER_ERROR_BAD_INPUT;
            break;

        case ETIMEDOUT:
            lRetval = NLER_ERROR_NO_RESOURCE;
            break;

        case EAGAIN:
        case EDEADLK:
        case EINTR:
        default:
            lRetval = NLER_ERROR_FAILURE;
            break;

        }
    }

    return (lRetval);
}

static int nlsemaphore_pthread_cond_signal(pthread_cond_t *aCond)
{
    int lStatus;
    int lRetval = NLER_SUCCESS;

    lStatus = pthread_cond_signal(aCond);
    if (lStatus != 0)
    {
        switch (lStatus)
        {

        case EINVAL:
            lRetval = NLER_ERROR_BAD_STATE;
            break;

        default:
            lRetval = NLER_ERROR_FAILURE;
            break;

        }
    }

    return (lRetval);
}

int nlsemaphore_counting_create(nlsemaphore_t *aSemaphore, size_t aMaxCount, size_t aInitialCount)
{
    int lStatus;
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

    lRetval = nlsemaphore_pthread_cond_init(&aSemaphore->mCondition);
    if (lRetval != NLER_SUCCESS)
    {
        goto done;
    }

    lRetval = nllock_create(&aSemaphore->mLock);
    if (lRetval != NLER_SUCCESS)
    {
        goto cond_destroy;
    }

    aSemaphore->mCurrentCount = aInitialCount;
    aSemaphore->mMaxCount = aMaxCount;

 done:
    return (lRetval);

 cond_destroy:
    lStatus = nlsemaphore_pthread_cond_destroy(&aSemaphore->mCondition);
    if (lStatus != 0)
    {
        lRetval = lStatus;
    }

    return (lRetval);
}

void nlsemaphore_destroy(nlsemaphore_t *aSemaphore)
{
    int     lStatus;

    lStatus = nlsemaphore_pthread_cond_destroy(&aSemaphore->mCondition);
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
        lRetval = nlsemaphore_pthread_cond_wait(&aSemaphore->mCondition, &aSemaphore->mLock, aTimeoutMsec);

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
        lRetval = nlsemaphore_pthread_cond_signal(&aSemaphore->mCondition);
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
