/*
 *
 *    Copyright (c) 2014-2016 Nest Labs, Inc.
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
 *      Locks. Binary lock implementation. All of the usual caveats
 *      surrounding the use of locks in general apply. Locks beget
 *      deadlocks. Use with care and avoid unless absolutely
 *      necessary.
 *
 */

#ifndef NL_ER_LOCK_H
#define NL_ER_LOCK_H

#include "nlernative.h"
#include "nlertime.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Binary lock.
 */

/** Create a new binary lock.
 *
 * @param[in] aLock storage used for the lock control block
 *
 * @return NLER_SUCCESS if lock created successfully
 */
int nllock_create(nllock_t *aLock);

/** Destroy a lock.
 *
 * @param[in] aLock lock to destroy.
 */
void nllock_destroy(nllock_t *aLock);

/** Begin an exclusion section.
 *
 * @param[in] aLock lock to enter to begin resource exclusion.
 */
int nllock_enter(nllock_t *aLock);

/** Attempt to begin an exclusion section until timeout
 *
 * @param[in] aLock lock to enter to begin resource exclusion.
 * @param[in] aTimeoutMsec number of msec to attempt to acquire lock
 */
int nllock_enter_with_timeout(nllock_t *aLock, nl_time_ms_t aTimeoutMsec);

/** End an exclusion section
 *
 * @param[in] aLock lock to exit to end resource exclusion
 */
int nllock_exit(nllock_t *aLock);

/** Create a new recursive lock.
 *
 * @param[in] aLock storage used for the lock control block
 *
 * @return NLER_SUCCESS if lock created successfully
 */
int nlrecursive_lock_create(nlrecursive_lock_t *aLock);

/** Destroy a recursive lock.
 *
 * @param[in] aLock lock to destroy.
 */
void nlrecursive_lock_destroy(nlrecursive_lock_t *aLock);

/** Begin an exclusion section.
 *
 * @param[in] aLock lock to enter to begin resource exclusion.
 */
int nlrecursive_lock_enter(nlrecursive_lock_t *aLock);

/** Attempt to begin an exclusion section until timeout
 *
 * @param[in] aLock lock to enter to begin resource exclusion.
 * @param[in] aTimeoutMsec number of msec to attempt to acquire lock
 */
int nlrecursive_lock_enter_with_timeout(nlrecursive_lock_t *aLock, nl_time_ms_t aTimeoutMsec);

/** End an exclusion section
 *
 * @param[in] aLock lock to exit to end resource exclusion
 */
int nlrecursive_lock_exit(nlrecursive_lock_t *aLock);

#ifdef __cplusplus
}
#endif

/** @example test-lock.c
 * Two tasks compete for access to an integer using atomic operations. Value at
 * exit must be 0.
 */
#endif /* NL_ER_LOCK_H */
