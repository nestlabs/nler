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
 *      This file implements a unit test for the NLER atomic operation
 *      interfaces.
 *
 */

#include <stdio.h>
#include <stdint.h>

#include <nlererror.h>
#include <nlertask.h>
#include <nlerinit.h>
#include <nlerlog.h>
#include <nlerassert.h>
#include <nleratomicops.h>

static nltask_t taskA;
static nltask_t taskB;
static uint8_t stackA[NLER_TASK_STACK_BASE + 96];
static uint8_t stackB[NLER_TASK_STACK_BASE + 96];

#define NUM_ATOM_ITERS  1000000

struct taskData
{
    int32_t value;
};

static void taskEntry(void *aParams)
{
    nltask_t        *curtask = nltask_get_current();
    const char      *name = nltask_get_name(curtask);
    struct taskData *data = (struct taskData *)aParams;
    int             idx;

    (void)name;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' entry: (%d)\n", name, data->value);

    for (idx = 0; idx < NUM_ATOM_ITERS; idx++)
    {
        nl_er_atomic_inc(&data->value);
        nl_er_atomic_dec(&data->value);

        if ((idx % 100000) == 0)
        {
            NL_LOG_DEBUG(lrTEST, "'%s' inc/dec: %d\n", nltask_get_name(curtask), idx);
        }
    }

    for (idx = 0; idx < NUM_ATOM_ITERS; idx++)
    {
        nl_er_atomic_add(&data->value, 12);
        nl_er_atomic_add(&data->value, -12);

        if ((idx % 100000) == 0)
        {
            NL_LOG_DEBUG(lrTEST, "'%s' add/-add: %d\n", name, idx);
        }
    }

    NL_LOG_CRIT(lrTEST, "from the task: '%s' exit: (%d)\n", name, data->value);

    nltask_suspend(curtask);
}

void test_unthreaded(void)
{
    int8_t value8, returnValue8;
    int16_t value16, returnValue16;
    int32_t value32, returnValue32;
    intptr_t value, returnValue;

    //
    // Increment
    //

    // 8-bit Increment

    value8 = 0;
    returnValue8 = nl_er_atomic_inc8(&value8);
    NLER_ASSERT(value8 == 1);
    NLER_ASSERT(returnValue8 == 1);

    value8 = INT8_MAX;
    returnValue8 = nl_er_atomic_inc8(&value8);
    NLER_ASSERT(value8 == INT8_MIN);
    NLER_ASSERT(returnValue8 == INT8_MIN);

    // 16-bit Increment

    value16 = 0;
    returnValue16 = nl_er_atomic_inc16(&value16);
    NLER_ASSERT(value16 == 1);
    NLER_ASSERT(returnValue16 == 1);

    value16 = INT16_MAX;
    returnValue16 = nl_er_atomic_inc16(&value16);
    NLER_ASSERT(value16 == INT16_MIN);
    NLER_ASSERT(returnValue16 == INT16_MIN);

    // 32-bit Increment

    value32 = 0;
    returnValue32 = nl_er_atomic_inc(&value32);
    NLER_ASSERT(value32 == 1);
    NLER_ASSERT(returnValue32 == 1);

    value32 = INT32_MAX;
    returnValue32 = nl_er_atomic_inc(&value32);
    NLER_ASSERT(value32 == INT32_MIN);
    NLER_ASSERT(returnValue32 == INT32_MIN);

    //
    // Decrement
    //

    // 8-bit Decrement

    value8 = 0;
    returnValue8 = nl_er_atomic_dec8(&value8);
    NLER_ASSERT(value8 == -1);
    NLER_ASSERT(returnValue8 == -1);

    value8 = INT8_MIN;
    returnValue8 = nl_er_atomic_dec8(&value8);
    NLER_ASSERT(value8 == INT8_MAX);
    NLER_ASSERT(returnValue8 == INT8_MAX);

    // 16-bit Decrement

    value16 = 0;
    returnValue16 = nl_er_atomic_dec16(&value16);
    NLER_ASSERT(value16 == -1);
    NLER_ASSERT(returnValue16 == -1);

    value16 = INT16_MIN;
    returnValue16 = nl_er_atomic_dec16(&value16);
    NLER_ASSERT(value16 == INT16_MAX);
    NLER_ASSERT(returnValue16 == INT16_MAX);

    // 32-bit Decrement

    value32 = 0;
    returnValue32 = nl_er_atomic_dec(&value32);
    NLER_ASSERT(value32 == -1);
    NLER_ASSERT(returnValue32 == -1);

    value32 = INT32_MIN;
    returnValue32 = nl_er_atomic_dec(&value32);
    NLER_ASSERT(value32 == INT32_MAX);
    NLER_ASSERT(returnValue32 == INT32_MAX);

    //
    // Set
    //

    // 8-bit Set

    value8 = 0;
    returnValue8 = nl_er_atomic_set8(&value8, 1);
    NLER_ASSERT(value8 == 1);
    NLER_ASSERT(returnValue8 == 0);

    // 16-bit Set

    value16 = 0;
    returnValue16 = nl_er_atomic_set16(&value16, 1);
    NLER_ASSERT(value16 == 1);
    NLER_ASSERT(returnValue16 == 0);

    // 32-bit Set

    value32 = 0;
    returnValue32 = nl_er_atomic_set(&value32, 1);
    NLER_ASSERT(value32 == 1);
    NLER_ASSERT(returnValue32 == 0);

    //
    // Add
    //

    // 8-bit Add

    value8 = 0;
    returnValue8 = nl_er_atomic_add8(&value8, 1);
    NLER_ASSERT(value8 == 1);
    NLER_ASSERT(returnValue8 == 1);

    value8 = 0;
    returnValue8 = nl_er_atomic_add8(&value8, INT8_MAX);
    NLER_ASSERT(value8 == INT8_MAX);
    NLER_ASSERT(returnValue8 == INT8_MAX);

    value8 = 0;
    returnValue8 = nl_er_atomic_add8(&value8, INT8_MIN);
    NLER_ASSERT(value8 == INT8_MIN);
    NLER_ASSERT(returnValue8 == INT8_MIN);

    value8 = INT8_MAX;
    returnValue8 = nl_er_atomic_add8(&value8, 1);
    NLER_ASSERT(value8 == INT8_MIN);
    NLER_ASSERT(returnValue8 == INT8_MIN);

    value8 = INT8_MIN;
    returnValue8 = nl_er_atomic_add8(&value8, -1);
    NLER_ASSERT(value8 == INT8_MAX);
    NLER_ASSERT(returnValue8 == INT8_MAX);

    // 16-bit Add

    value16 = 0;
    returnValue16 = nl_er_atomic_add16(&value16, 1);
    NLER_ASSERT(value16 == 1);
    NLER_ASSERT(returnValue16 == 1);

    value16 = 0;
    returnValue16 = nl_er_atomic_add16(&value16, INT16_MAX);
    NLER_ASSERT(value16 == INT16_MAX);
    NLER_ASSERT(returnValue16 == INT16_MAX);

    value16 = 0;
    returnValue16 = nl_er_atomic_add16(&value16, INT16_MIN);
    NLER_ASSERT(value16 == INT16_MIN);
    NLER_ASSERT(returnValue16 == INT16_MIN);

    value16 = INT16_MAX;
    returnValue16 = nl_er_atomic_add16(&value16, 1);
    NLER_ASSERT(value16 == INT16_MIN);
    NLER_ASSERT(returnValue16 == INT16_MIN);

    value16 = INT16_MIN;
    returnValue16 = nl_er_atomic_add16(&value16, -1);
    NLER_ASSERT(value16 == INT16_MAX);
    NLER_ASSERT(returnValue16 == INT16_MAX);

    // 32-bit Add

    value32 = 0;
    returnValue32 = nl_er_atomic_add(&value32, 1);
    NLER_ASSERT(value32 == 1);
    NLER_ASSERT(returnValue32 == 1);

    value32 = 0;
    returnValue32 = nl_er_atomic_add(&value32, INT32_MAX);
    NLER_ASSERT(value32 == INT32_MAX);
    NLER_ASSERT(returnValue32 == INT32_MAX);

    value32 = 0;
    returnValue32 = nl_er_atomic_add(&value32, INT32_MIN);
    NLER_ASSERT(value32 == INT32_MIN);
    NLER_ASSERT(returnValue32 == INT32_MIN);

    value32 = INT32_MAX;
    returnValue32 = nl_er_atomic_add(&value32, 1);
    NLER_ASSERT(value32 == INT32_MIN);
    NLER_ASSERT(returnValue32 == INT32_MIN);

    value32 = INT32_MIN;
    returnValue32 = nl_er_atomic_add(&value32, -1);
    NLER_ASSERT(value32 == INT32_MAX);
    NLER_ASSERT(returnValue32 == INT32_MAX);

    //
    // Set Bits
    //

    // 8-bit Set Bits

    value8 = 0xAA;
    returnValue8 = nl_er_atomic_set_bits8(&value8, 0x55);
    NLER_ASSERT(value8 == (int8_t)0xFF);
    NLER_ASSERT(returnValue8 == (int8_t)0xAA);

    value8 = 0x55;
    returnValue8 = nl_er_atomic_set_bits8(&value8, 0xAA);
    NLER_ASSERT(value8 == (int8_t)0xFF);
    NLER_ASSERT(returnValue8 == (int8_t)0x55);

    value8 = 0xF0;
    returnValue8 = nl_er_atomic_set_bits8(&value8, 0x0F);
    NLER_ASSERT(value8 == (int8_t)0xFF);
    NLER_ASSERT(returnValue8 == (int8_t)0xF0);

    value8 = 0x0F;
    returnValue8 = nl_er_atomic_set_bits8(&value8, 0xF0);
    NLER_ASSERT(value8 == (int8_t)0xFF);
    NLER_ASSERT(returnValue8 == (int8_t)0x0F);

    // 16-bit Set Bits

    value16 = 0xAAAA;
    returnValue16 = nl_er_atomic_set_bits16(&value16, 0x5555);
    NLER_ASSERT(value16 == (int16_t)0xFFFF);
    NLER_ASSERT(returnValue16 == (int16_t)0xAAAA);

    value16 = 0x5555;
    returnValue16 = nl_er_atomic_set_bits16(&value16, 0xAAAA);
    NLER_ASSERT(value16 == (int16_t)0xFFFF);
    NLER_ASSERT(returnValue16 == (int16_t)0x5555);

    value16 = 0xFF00;
    returnValue16 = nl_er_atomic_set_bits16(&value16, 0x00FF);
    NLER_ASSERT(value16 == (int16_t)0xFFFF);
    NLER_ASSERT(returnValue16 == (int16_t)0xFF00);

    value16 = 0x00FF;
    returnValue16 = nl_er_atomic_set_bits16(&value16, 0xFF00);
    NLER_ASSERT(value16 == (int16_t)0xFFFF);
    NLER_ASSERT(returnValue16 == (int16_t)0x00FF);

    // 32-bit Set Bits

    value32 = 0xAAAAAAAA;
    returnValue32 = nl_er_atomic_set_bits(&value32, 0x55555555);
    NLER_ASSERT(value32 == (int32_t)0xFFFFFFFF);
    NLER_ASSERT(returnValue32 == (int32_t)0xAAAAAAAA);

    value32 = 0x55555555;
    returnValue32 = nl_er_atomic_set_bits(&value32, 0xAAAAAAAA);
    NLER_ASSERT(value32 == (int32_t)0xFFFFFFFF);
    NLER_ASSERT(returnValue32 == (int32_t)0x55555555);

    value32 = 0xFFFF0000;
    returnValue32 = nl_er_atomic_set_bits(&value32, 0x0000FFFF);
    NLER_ASSERT(value32 == (int32_t)0xFFFFFFFF);
    NLER_ASSERT(returnValue32 == (int32_t)0xFFFF0000);

    value32 = 0x0000FFFF;
    returnValue32 = nl_er_atomic_set_bits(&value32, 0xFFFF0000);
    NLER_ASSERT(value32 == (int32_t)0xFFFFFFFF);
    NLER_ASSERT(returnValue32 == (int32_t)0x0000FFFF);

    //
    // Clear Bits
    //

    // 8-bit Clear Bits

    value8 = 0xFF;
    returnValue8 = nl_er_atomic_clr_bits8(&value8, 0x55);
    NLER_ASSERT(value8 == (int8_t)0xAA);
    NLER_ASSERT(returnValue8 == (int8_t)0xFF);

    value8 = 0xFF;
    returnValue8 = nl_er_atomic_clr_bits8(&value8, 0xAA);
    NLER_ASSERT(value8 == (int8_t)0x55);
    NLER_ASSERT(returnValue8 == (int8_t)0xFF);

    value8 = 0xFF;
    returnValue8 = nl_er_atomic_clr_bits8(&value8, 0x0F);
    NLER_ASSERT(value8 == (int8_t)0xF0);
    NLER_ASSERT(returnValue8 == (int8_t)0xFF);

    value8 = 0xFF;
    returnValue8 = nl_er_atomic_clr_bits8(&value8, 0xF0);
    NLER_ASSERT(value8 == (int8_t)0x0F);
    NLER_ASSERT(returnValue8 == (int8_t)0xFF);

    // 16-bit Clear Bits

    value16 = 0xFFFF;
    returnValue16 = nl_er_atomic_clr_bits16(&value16, 0x5555);
    NLER_ASSERT(value16 == (int16_t)0xAAAA);
    NLER_ASSERT(returnValue16 == (int16_t)0xFFFF);

    value16 = 0xFFFF;
    returnValue16 = nl_er_atomic_clr_bits16(&value16, 0xAAAA);
    NLER_ASSERT(value16 == (int16_t)0x5555);
    NLER_ASSERT(returnValue16 == (int16_t)0xFFFF);

    value16 = 0xFFFF;
    returnValue16 = nl_er_atomic_clr_bits16(&value16, 0x00FF);
    NLER_ASSERT(value16 == (int16_t)0xFF00);
    NLER_ASSERT(returnValue16 == (int16_t)0xFFFF);

    value16 = 0xFFFF;
    returnValue16 = nl_er_atomic_clr_bits16(&value16, 0xFF00);
    NLER_ASSERT(value16 == (int16_t)0x00FF);
    NLER_ASSERT(returnValue16 == (int16_t)0xFFFF);

    // 32-bit Clear Bits

    value32 = 0xFFFFFFFF;
    returnValue32 = nl_er_atomic_clr_bits(&value32, 0x55555555);
    NLER_ASSERT(value32 == (int32_t)0xAAAAAAAA);
    NLER_ASSERT(returnValue32 == (int32_t)0xFFFFFFFF);

    value32 = 0xFFFFFFFF;
    returnValue32 = nl_er_atomic_clr_bits(&value32, 0xAAAAAAAA);
    NLER_ASSERT(value32 == (int32_t)0x55555555);
    NLER_ASSERT(returnValue32 == (int32_t)0xFFFFFFFF);

    value32 = 0xFFFFFFFF;
    returnValue32 = nl_er_atomic_clr_bits(&value32, 0x0000FFFF);
    NLER_ASSERT(value32 == (int32_t)0xFFFF0000);
    NLER_ASSERT(returnValue32 == (int32_t)0xFFFFFFFF);

    value32 = 0xFFFFFFFF;
    returnValue32 = nl_er_atomic_clr_bits(&value32, 0xFFFF0000);
    NLER_ASSERT(value32 == (int32_t)0x0000FFFF);
    NLER_ASSERT(returnValue32 == (int32_t)0xFFFFFFFF);

    //
    // Compare and Swap
    //

    // 8-bit Compare and Swap

    value8 = 0;
    returnValue8 = nl_er_atomic_cas8(&value8, 0, 1);
    NLER_ASSERT(value8 == 1);
    NLER_ASSERT(returnValue8 == 0);

    value8 = 0;
    returnValue8 = nl_er_atomic_cas8(&value8, -1, -2);
    NLER_ASSERT(value8 == 0);
    NLER_ASSERT(returnValue8 == 0);

    // 16-bit Compare and Swap

    value16 = 0;
    returnValue16 = nl_er_atomic_cas16(&value16, 0, 1);
    NLER_ASSERT(value16 == 1);
    NLER_ASSERT(returnValue16 == 0);

    value16 = 0;
    returnValue16 = nl_er_atomic_cas16(&value16, -1, -2);
    NLER_ASSERT(value16 == 0);
    NLER_ASSERT(returnValue16 == 0);

    // Pointer-sized Compare and Swap

    value = 0;
    returnValue = nl_er_atomic_cas(&value, 0, 1);
    NLER_ASSERT(value == 1);
    NLER_ASSERT(returnValue == 0);

    value = 0;
    returnValue = nl_er_atomic_cas(&value, -1, -2);
    NLER_ASSERT(value == 0);
    NLER_ASSERT(returnValue == 0);
}

void test_threaded(void)
{
    struct taskData data;

    data.value = 0;

    nltask_create(taskEntry, "A", stackA, sizeof(stackA), NLER_TASK_PRIORITY_NORMAL, &data, &taskA);
    nltask_create(taskEntry, "B", stackB, sizeof(stackB), NLER_TASK_PRIORITY_NORMAL, &data, &taskB);

    nl_er_start_running();
}

int main(int argc, char **argv)
{
    int             err;

    NL_LOG_CRIT(lrTEST, "start main\n");

    err = nl_er_init();
    NLER_ASSERT(err == NLER_SUCCESS);

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime: %d)\n", err);

    /* random test of asserts -- give this an argument and it should fail */

    NLER_ASSERT(argc == 1);

    test_unthreaded();

    test_threaded();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return 0;
}
