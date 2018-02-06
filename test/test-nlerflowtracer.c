/*
 *
 *    Copyright (c) 2016 Nest Labs, Inc.
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
 *      This file implements a unit test for the NLER flow tracer
 *      interfaces.
 *
 */

// Provide Stubbed implementations to decouple tests dependency on freertos
void vPortEnterCritical(void);
void vPortExitCritical(void);
unsigned int nl_get_time_native(void);
unsigned int nl_get_time_native_from_isr(void);
unsigned int nl_time_native_to_time_ms(unsigned int time);

void vPortEnterCritical() {}
void vPortExitCritical() {}
unsigned int nl_get_time_native() { return 1; }
unsigned int nl_get_time_native_from_isr() { return 1; }
unsigned int nl_time_native_to_time_ms(unsigned int time) { return time; }

#include <stdio.h>
#include <assert.h>
#include "nlerflowtracer.h"
#include "nlerflowtrace-enum.h"
#include "nlerassert.h"

void test_init(void);
void test_add_single_entry(void);
void test_add_mulitple_entries(void);
void test_add_until_queue_cycles(void);

void test_init(void)
{
    printf( "%s\n", __func__);
    nl_flowtracer_init();
    const struct nl_tracer_t result = nl_flowtracer_get_tracer();
    assert(result.head == 0);
    assert(result.tail == 0);
    assert(result.isEmpty == 1);

    // Quell Compiler Warning about unused variables
    (void) result;
}

void test_add_single_entry(void)
{
    printf( "%s\n", __func__);

    nl_flowtracer_init();
    nl_flowtracer_add_trace(RX_EVENT, 1);
    const struct nl_tracer_t result = nl_flowtracer_get_tracer();

    assert(result.head == 0);
    assert(result.tail == 1);
    assert(result.isEmpty == 0);
    assert(result.queue[0].data == 1);
    assert(result.queue[0].event == 0);

    // Quell Compiler Warning about unused variables
    (void) result;
}

void test_add_mulitple_entries(void)
{
    printf( "%s\n", __func__);

    nl_flowtracer_init();
    nl_flowtracer_add_trace(RX_EVENT, 1);
    nl_flowtracer_add_trace(TX_EVENT, 2);
    const struct nl_tracer_t result = nl_flowtracer_get_tracer();

    assert(result.head == 0);
    assert(result.tail == 2);
    assert(result.isEmpty == 0);
    assert(result.queue[0].data == 1);
    assert(result.queue[0].event == 0);
    assert(result.queue[1].data == 2);
    assert(result.queue[1].event == 1);

    // Quell Compiler Warning about unused variables
    (void) result;
}

void test_add_until_queue_cycles(void)
{
    printf( "%s\n", __func__);

    nl_flowtracer_init();
    int i;
    for (i = 0; i < FLOW_TRACE_QUEUE_SIZE + 1; i++)
    {
        nl_flowtracer_add_trace(TX_EVENT, i);
    }

    const struct nl_tracer_t result = nl_flowtracer_get_tracer();

    assert(result.head == 1);
    assert(result.tail == 1);
    assert(result.queue[0].data == FLOW_TRACE_QUEUE_SIZE);

    // Quell Compiler Warning about unused variables
    (void) result;
}

#include <stdio.h>
int main(int argc, char **argv)
{
    printf( "Start main\n");
    test_init();
    test_add_single_entry();
    test_add_mulitple_entries();
    test_add_until_queue_cycles();
    printf("End main\n");
    return 0;
}
