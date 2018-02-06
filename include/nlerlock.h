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

#ifdef __cplusplus
extern "C" {
#endif

/** Binary lock.
 */
typedef void * nl_lock_t;
typedef void * nl_recursive_lock_t;

/** Create a new binary lock.
 *
 * @return a new binary lock.
 */
nl_lock_t nl_er_lock_create(void);

/** Destroy a lock.
 *
 * @param[in] aLock lock to destroy.
 */
void nl_er_lock_destroy(nl_lock_t aLock);

/** Begin an exclusion section.
 *
 * @param[in] aLock lock to enter to begin resource exclusion.
 */
int nl_er_lock_enter(nl_lock_t aLock);

/** End an exclusion section
 *
 * @param[in] aLock lock to exit to end resource exclusion
 */
int nl_er_lock_exit(nl_lock_t aLock);

/** Create a new recursive binary lock.
 *
 * @return a new recursive binary lock.
 */
nl_recursive_lock_t nl_er_recursive_lock_create(void);

/** Destroy a recursive lock.
 *
 * @param[in] aLock lock to destroy.
 */
void nl_er_recursive_lock_destroy(nl_recursive_lock_t aLock);

/** Begin an exclusion section.
 *
 * @param[in] aLock lock to enter to begin resource exclusion.
 */
int nl_er_recursive_lock_enter(nl_recursive_lock_t aLock);

/** End an exclusion section
 *
 * @param[in] aLock lock to exit to end resource exclusion
 */
int nl_er_recursive_lock_exit(nl_recursive_lock_t aLock);

#ifdef __cplusplus
}
#endif

/** @example test-lock.c
 * Two tasks compete for access to an integer using atomic operations. Value at
 * exit must be 0.
 */
#endif /* NL_ER_LOCK_H */
