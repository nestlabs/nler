/*
 *
 *    Copyright (c) 2018 Google LLC
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
 *      This file defines POSIX threads (pthreads)-specific object types.
 *
 */

#ifndef NLER_NATIVE_H
#define NLER_NATIVE_H

#include <pthread.h>
#include <stdint.h>

/* the buffer/control block for the task
 */
typedef struct nltask_pthreads_s
{
    void *                         mEntry;
    void *                         mParams;
    const char *                   mName;
    pthread_t                      mThread;
} nltask_pthreads_t;

typedef nltask_pthreads_t nltask_obj_t;

/* the buffer/control block for the eventqueue
 */
typedef uintptr_t nleventqueue_t;

/* the buffer/control block for the lock
 */
typedef pthread_mutex_t nllock_t;
typedef pthread_mutex_t nlrecursive_lock_t;

#define NLLOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define NLRECURSIVE_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER

/* the buffer/control block for the semaphore
 */
typedef struct nlsemaphore_pthreads_s
{
    nllock_t                       mLock;
    pthread_cond_t                 mCondition;
    int32_t                        mCurrentCount;
    size_t                         mMaxCount;
} nlsemaphore_pthreads_t;

typedef nlsemaphore_pthreads_t nlsemaphore_t;

#endif /* NLER_NATIVE_H */
