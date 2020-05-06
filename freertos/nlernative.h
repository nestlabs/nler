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
 *      This file defines FreeRTOS-specific object types.
 *
 */

#ifndef NLER_NATIVE_H
#define NLER_NATIVE_H

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

/* the buffer/control block for the task
 */
typedef StaticTask_t nltask_obj_t;

/* the buffer/control block for the eventqueue
 */
typedef StaticQueue_t nleventqueue_t;

/* the buffer/control block for the lock
 */
typedef StaticSemaphore_t nllock_t;
typedef StaticSemaphore_t nlrecursive_lock_t;

#define NLLOCK_INITIALIZER {}
#define NLRECURSIVE_LOCK_INITIALIZER {}

/* the buffer/control block for semaphores
 */
typedef StaticSemaphore_t nlsemaphore_t;

#endif /* NLER_NATIVE_H */
