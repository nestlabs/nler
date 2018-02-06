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
 *      This file implements NLER instruction set architecture
 *      (ISA)-independent atomic operations.
 *
 */

#include <nler-config.h>

#include <stdlib.h>

#include <nleratomicops.h>
#include <nlererror.h>
#include <nlerlock.h>

#if !NLER_HAVE_ATOMIC_BUILTINS
static nl_lock_t sAtomicsLock = NULL;

int32_t nl_er_atomic_inc(int32_t *aValue)
{
    int32_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = ++(*aValue);

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int16_t nl_er_atomic_inc16(int16_t *aValue)
{
    int16_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = ++(*aValue);

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int8_t nl_er_atomic_inc8(int8_t *aValue)
{
    int8_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = ++(*aValue);

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int32_t nl_er_atomic_dec(int32_t *aValue)
{
    int32_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = --(*aValue);

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int16_t nl_er_atomic_dec16(int16_t *aValue)
{
    int16_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = --(*aValue);

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int8_t nl_er_atomic_dec8(int8_t *aValue)
{
    int8_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = --(*aValue);

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int32_t nl_er_atomic_set(int32_t *aValue, int32_t aNewValue)
{
    int32_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    *aValue = aNewValue;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int16_t nl_er_atomic_set16(int16_t *aValue, int16_t aNewValue)
{
    int16_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    *aValue = aNewValue;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int8_t nl_er_atomic_set8(int8_t *aValue, int8_t aNewValue)
{
    int8_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    *aValue = aNewValue;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int32_t nl_er_atomic_add(int32_t *aValue, int32_t aDelta)
{
    int32_t retval;

    nl_er_lock_enter(sAtomicsLock);

    *aValue += aDelta;

    retval = *aValue;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int16_t nl_er_atomic_add16(int16_t *aValue, int16_t aDelta)
{
    int16_t retval;

    nl_er_lock_enter(sAtomicsLock);

    *aValue += aDelta;

    retval = *aValue;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int8_t nl_er_atomic_add8(int8_t *aValue, int8_t aDelta)
{
    int8_t retval;

    nl_er_lock_enter(sAtomicsLock);

    *aValue += aDelta;

    retval = *aValue;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int32_t nl_er_atomic_set_bits(int32_t *aValue, int32_t aBitMask)
{
    int32_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    *aValue |= aBitMask;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int16_t nl_er_atomic_set_bits16(int16_t *aValue, int16_t aBitMask)
{
    int16_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    *aValue |= aBitMask;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int8_t nl_er_atomic_set_bits8(int8_t *aValue, int8_t aBitMask)
{
    int8_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    *aValue |= aBitMask;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int32_t nl_er_atomic_clr_bits(int32_t *aValue, int32_t aBitMask)
{
    int32_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    *aValue &= ~aBitMask;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int16_t nl_er_atomic_clr_bits16(int16_t *aValue, int16_t aBitMask)
{
    int16_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    *aValue &= ~aBitMask;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int8_t nl_er_atomic_clr_bits8(int8_t *aValue, int8_t aBitMask)
{
    int8_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    *aValue &= ~aBitMask;

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

intptr_t nl_er_atomic_cas(intptr_t *aValue, intptr_t aCmpValue, intptr_t aNewValue)
{
    intptr_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    if (retval == aCmpValue)
    {
        *aValue = aNewValue;
    }

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int16_t nl_er_atomic_cas16(int16_t *aValue, int16_t aCmpValue, int16_t aNewValue)
{
    int16_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    if (retval == aCmpValue)
    {
        *aValue = aNewValue;
    }

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int8_t nl_er_atomic_cas8(int8_t *aValue, int8_t aCmpValue, int8_t aNewValue)
{
    int8_t retval;

    nl_er_lock_enter(sAtomicsLock);

    retval = *aValue;

    if (retval == aCmpValue)
    {
        *aValue = aNewValue;
    }

    nl_er_lock_exit(sAtomicsLock);

    return retval;
}

int nl_er_atomic_init(void)
{
    int retval = 0;

    sAtomicsLock = nl_er_lock_create();

    if (sAtomicsLock == NULL)
        retval = NLER_ERROR_NO_RESOURCE;

    return retval;
}
#else /* !NLER_HAVE_ATOMIC_BUILTINS */
int32_t nl_er_atomic_inc(int32_t *aValue)
{
    return __sync_add_and_fetch(aValue, 1);
}

int16_t nl_er_atomic_inc16(int16_t *aValue)
{
    return __sync_add_and_fetch(aValue, 1);
}

int8_t nl_er_atomic_inc8(int8_t *aValue)
{
    return __sync_add_and_fetch(aValue, 1);
}

int32_t nl_er_atomic_dec(int32_t *aValue)
{
    return __sync_sub_and_fetch(aValue, 1);
}

int16_t nl_er_atomic_dec16(int16_t *aValue)
{
    return __sync_sub_and_fetch(aValue, 1);
}

int8_t nl_er_atomic_dec8(int8_t *aValue)
{
    return __sync_sub_and_fetch(aValue, 1);
}

int32_t nl_er_atomic_set(int32_t *aValue, int32_t aNewValue)
{
    return __sync_lock_test_and_set(aValue, aNewValue);
}

int16_t nl_er_atomic_set16(int16_t *aValue, int16_t aNewValue)
{
    return __sync_lock_test_and_set(aValue, aNewValue);
}

int8_t nl_er_atomic_set8(int8_t *aValue, int8_t aNewValue)
{
    return __sync_lock_test_and_set(aValue, aNewValue);
}

int32_t nl_er_atomic_add(int32_t *aValue, int32_t aDelta)
{
    return __sync_add_and_fetch(aValue, aDelta);
}

int16_t nl_er_atomic_add16(int16_t *aValue, int16_t aDelta)
{
    return __sync_add_and_fetch(aValue, aDelta);
}

int8_t nl_er_atomic_add8(int8_t *aValue, int8_t aDelta)
{
    return __sync_add_and_fetch(aValue, aDelta);
}

int32_t nl_er_atomic_set_bits(int32_t *aValue, int32_t aBitMask)
{
    return __sync_fetch_and_or(aValue, aBitMask);
}

int16_t nl_er_atomic_set_bits16(int16_t *aValue, int16_t aBitMask)
{
    return __sync_fetch_and_or(aValue, aBitMask);
}

int8_t nl_er_atomic_set_bits8(int8_t *aValue, int8_t aBitMask)
{
    return __sync_fetch_and_or(aValue, aBitMask);
}

int32_t nl_er_atomic_clr_bits(int32_t *aValue, int32_t aBitMask)
{
    return __sync_fetch_and_and(aValue, ~aBitMask);
}

int16_t nl_er_atomic_clr_bits16(int16_t *aValue, int16_t aBitMask)
{
    return __sync_fetch_and_and(aValue, ~aBitMask);
}

int8_t nl_er_atomic_clr_bits8(int8_t *aValue, int8_t aBitMask)
{
    return __sync_fetch_and_and(aValue, ~aBitMask);
}

intptr_t nl_er_atomic_cas(intptr_t *aValue, intptr_t aCmpValue, intptr_t aNewValue)
{
    return __sync_val_compare_and_swap(aValue, aCmpValue, aNewValue);
}

int16_t nl_er_atomic_cas16(int16_t *aValue, int16_t aCmpValue, int16_t aNewValue)
{
    return __sync_val_compare_and_swap(aValue, aCmpValue, aNewValue);
}

int8_t nl_er_atomic_cas8(int8_t *aValue, int8_t aCmpValue, int8_t aNewValue)
{
    return __sync_val_compare_and_swap(aValue, aCmpValue, aNewValue);
}

int nl_er_atomic_init(void)
{
    int retval = 0;

    return retval;
}
#endif /* !NLER_HAVE_ATOMIC_BUILTINS */

