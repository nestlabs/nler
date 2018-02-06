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
 *      Atomic operations. These functions are implemented in the arch
 *      directory on a per-cpu architecture basis.
 *
 */

#ifndef NL_ER_ATOMIC_OPS_H
#define NL_ER_ATOMIC_OPS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Atomically increment an integer.
 *
 * @param[in,out] aValue pointer to value to increment
 *
 * @return the incremented value
 */
int32_t nl_er_atomic_inc(int32_t *aValue);
int16_t nl_er_atomic_inc16(int16_t *aValue);
int8_t nl_er_atomic_inc8(int8_t *aValue);

/** Atomically decrement an integer
 *
 * @param[in,out] aValue pointer to value to decrement
 *
 * @return the decremented value
 */
int32_t nl_er_atomic_dec(int32_t *aValue);
int16_t nl_er_atomic_dec16(int16_t *aValue);
int8_t nl_er_atomic_dec8(int8_t *aValue);

/** Atomically set a value to a new value
 *
 * @param[in,out] aValue pointer to value to change
 *
 * @param[in] aNewValue new value to store at aValue
 *
 * @return the old value
 */
int32_t nl_er_atomic_set(int32_t *aValue, int32_t aNewValue);
int16_t nl_er_atomic_set16(int16_t *aValue, int16_t aNewValue);
int8_t nl_er_atomic_set8(int8_t *aValue, int8_t aNewValue);

/** Atomically add a delta to a value
 *
 * @param[in, out] aValue pointer to value to change
 *
 * @param[in] aDelta delta value to apply to *aValue
 *
 * @return the updated value
 */
int32_t nl_er_atomic_add(int32_t *aValue, int32_t aDelta);
int16_t nl_er_atomic_add16(int16_t *aValue, int16_t aDelta);
int8_t nl_er_atomic_add8(int8_t *aValue, int8_t aDelta);

/** Atomically set bits in a value
 *
 * @param[in, out] aValue pointer to value to change
 * @param[in] aBitMask bit mask of bits to set
 *
 * @return the old value
 */
int32_t nl_er_atomic_set_bits(int32_t *aValue, int32_t aBitMask);
int16_t nl_er_atomic_set_bits16(int16_t *aValue, int16_t aBitMask);
int8_t nl_er_atomic_set_bits8(int8_t *aValue, int8_t aBitMask);

/** Atomically clear bits in a value
 *
 * @param[in, out] aValue pointer to value to change
 *
 * @param[in] aBitMask bit mask of bits to clear
 *
 * @return the old value
 */
int32_t nl_er_atomic_clr_bits(int32_t *aValue, int32_t aBitMask);
int16_t nl_er_atomic_clr_bits16(int16_t *aValue, int16_t aBitMask);
int8_t nl_er_atomic_clr_bits8(int8_t *aValue, int8_t aBitMask);

/** Atomically compare and swap a value
 *
 * @param[in, out] aValue pointer to value to change
 *
 * @param[in] aCmpValue comparison value to test against aValue
 *
 * @param[in] aNewValue new value to store at aValue
 *
 * @return the old value
 */
intptr_t nl_er_atomic_cas(intptr_t *aValue, intptr_t aCmpValue, intptr_t aNewValue);
int16_t nl_er_atomic_cas16(int16_t *aValue, int16_t aCmpValue, int16_t aNewValue);
int8_t nl_er_atomic_cas8(int8_t *aValue, int8_t aCmpValue, int8_t aNewValue);

/** Initialize any global data required by the atomic operations.
 *
 * @return NLER_SUCCESS if initialization succeeded
 */
int nl_er_atomic_init(void);

#ifdef __cplusplus
}
#endif

/** @example test-atomic.c
 * Two tasks compete for access to an integer using atomic operations. Value at
 * exit must be 0.
 */
#endif /* NL_ER_ATOMIC_OPS_H */
