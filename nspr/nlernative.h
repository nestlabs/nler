/*
 *
 *    Copyright (c) 2020 Project nler Authors
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
 *      This file defines Netscape Portable Runtime (NSPR)-specific
 *      object types.
 *
 */

#ifndef NLER_NATIVE_H
#define NLER_NATIVE_H

#include <stdint.h>

#include <nspr/prthread.h>

/* the buffer/control block for the task
 */
typedef struct nltask_nspr_s
{
    void *                         mEntry;
    void *                         mParams;
    const char *                   mName;
    PRThread *                     mThread;
} nltask_nspr_t;

typedef nltask_nspr_t nltask_obj_t;

/* the buffer/control block for the eventqueue
 */
typedef uintptr_t nleventqueue_t;

/* the buffer/control block for the lock
 */
typedef uintptr_t nllock_t;
typedef uintptr_t nlrecursive_lock_t;

#define NLLOCK_INITIALIZER 0
#define NLRECURSIVE_LOCK_INITIALIZER 0

/* the buffer/control block for the lock
 */
typedef uintptr_t nlsemaphore_t;

#endif /* NLER_NATIVE_H */
