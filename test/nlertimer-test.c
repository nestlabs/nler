/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file implements shared unit test infrastructure for NLER
 *      timer interfaces.
 *
 */

#if NLER_FEATURE_EVENT_TIMER
#include <nltest.h>

#include <nlertimer.h>
#include <nlmacros.h>
#include <nlplatform.h>
#include <nlplatform/nlwatchdog.h>
#if NLER_BUILD_PLATFORM_FREERTOS
#include <FreeRTOS.h>
#include <task.h>
#endif
#include <nlertask.h>

#define TIMER_TIMEOUT_50_MS    50
#define TIMER_TIMEOUT_100_MS   100
#define TIMER_TIMEOUT_125_MS   125
#define TIMER_TIMEOUT_500_MS   500
#define TIMER_TIMEOUT_1000_MS  1000
#define TIMER_TIMEOUT_2000_MS  2000
#define TIMER_TIMEOUT_5000_MS  5000

#define TIMER1_TIMEOUT_MS TIMER_TIMEOUT_125_MS
#define TIMER2_TIMEOUT_MS TIMER_TIMEOUT_500_MS
#define TIMER3_TIMEOUT_MS TIMER_TIMEOUT_1000_MS
#define TIMER4_TIMEOUT_MS TIMER_TIMEOUT_2000_MS

#define TIMER_TIMEOUT_TOLERANCE_TIME 2

#if NLER_FEATURE_SIMULATEABLE_TIME
/* Timing is often not accurate enough to test in simulator.
 * Process can get swapped out or other scheduling issues
 * with the host machine, causing timers to run many milliseconds
 * to even over a second later than scheduled.
 * We limit ourself to testing correctness in terms of the
 * timer did run/cancel as expected, but not when.
 */
#define TIMER_ACCURACY_TEST_ASSERT(...)
#define TIMER_ACCURACY_TEST_PRINTF(...)
#else
#define TIMER_ACCURACY_TEST_ASSERT NL_TEST_ASSERT
#define TIMER_ACCURACY_TEST_PRINTF printf
#endif

#if NLER_BUILD_PLATFORM_FREERTOS
/* On FreeRTOS, a tick is used to keep time, and since we can never
 * be sure when in the current tick we're starting a timer, the
 * function nl_time_ms_to_delay_time_native() adds an extra tick
 * to the converted value to make sure the real delay is at least
 * as long as the requested time and never early.  So for our
 * testing, we need to account for that extra native delay.
 */
#define TIME_NATIVE_EXTRA_DELAY 1
#else
#define TIME_NATIVE_EXTRA_DELAY 0
#endif

nl_event_t *sQueueMemory[8];
nl_eventqueue_t sQueue;

#if NLER_FEATURE_TIMER_USING_SWTIMER
extern bool g_swtimer_prevent_sleep;
#endif

static void Test_one(nlTestSuite *inSuite, void *inContext)
{
    nl_time_native_t start_time_native;
    nl_time_native_t end_time_native;
    __attribute__((unused)) nl_time_native_t elapsed_time_native;
    nl_event_timer_t timer1;
    nl_event_t *receivedEvent;

    nl_event_timer_init(&timer1, NULL, NULL, sQueue);

    printf("\nTest one event timer ----\n");
    start_time_native = nl_get_time_native();

    nl_event_timer_start(&timer1, TIMER1_TIMEOUT_MS, false);
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, TIMER1_TIMEOUT_MS * 2);
    end_time_native = nl_get_time_native();
    elapsed_time_native = end_time_native - start_time_native;
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);
    TIMER_ACCURACY_TEST_PRINTF("%s: start = %u, end = %u, elapsed = %u\n", __func__, start_time_native, end_time_native, elapsed_time_native);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, elapsed_time_native <= (nl_time_ms_to_delay_time_native(TIMER1_TIMEOUT_MS) + TIMER_TIMEOUT_TOLERANCE_TIME));

    // check no more events
    NL_TEST_ASSERT(inSuite, nl_eventqueue_get_event_with_timeout(sQueue, 0) == NULL);
}

/* Test four timers with different timeouts and verify:
 *   timer1: 100ms
 *   timer2: 500ms
 *   timer3: 1000ms
 *   timer4: 2000ms
 */
static void Test_four(nlTestSuite *inSuite, void *inContext)
{
    nl_time_native_t start_time_native;
    __attribute__((unused)) nl_time_native_t end_time_native;
    nl_event_timer_t timer1;
    nl_event_timer_t timer2;
    nl_event_timer_t timer3;
    nl_event_timer_t timer4;
    nl_time_native_t timer1_expected_time_native_min;
    nl_time_native_t timer2_expected_time_native_min;
    nl_time_native_t timer3_expected_time_native_min;
    nl_time_native_t timer4_expected_time_native_min;
    __attribute__((unused)) nl_time_native_t timer1_expected_time_native_max;
    __attribute__((unused)) nl_time_native_t timer2_expected_time_native_max;
    __attribute__((unused)) nl_time_native_t timer3_expected_time_native_max;
    __attribute__((unused)) nl_time_native_t timer4_expected_time_native_max;
    nl_event_t *receivedEvent;

    nl_event_timer_init(&timer1, NULL, NULL, sQueue);
    nl_event_timer_init(&timer2, NULL, NULL, sQueue);
    nl_event_timer_init(&timer3, NULL, NULL, sQueue);
    nl_event_timer_init(&timer4, NULL, NULL, sQueue);

    printf("\nTest four event timers ----\n");

    start_time_native = nl_get_time_native();

    nl_event_timer_start(&timer1, TIMER1_TIMEOUT_MS, false);
    nl_event_timer_start(&timer2, TIMER2_TIMEOUT_MS, false);
    nl_event_timer_start(&timer3, TIMER3_TIMEOUT_MS, false);
    nl_event_timer_start(&timer4, TIMER4_TIMEOUT_MS, false);

    timer1_expected_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER1_TIMEOUT_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;
    timer2_expected_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER2_TIMEOUT_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;
    timer3_expected_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER3_TIMEOUT_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;
    timer4_expected_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER4_TIMEOUT_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;

    timer1_expected_time_native_max = timer1_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;
    timer2_expected_time_native_max = timer2_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;
    timer3_expected_time_native_max = timer3_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;
    timer4_expected_time_native_max = timer4_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;

    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);
    TIMER_ACCURACY_TEST_PRINTF("timer1: end_time_native = %u, expected_min = %u, expected_max = %u\n",
                               end_time_native, timer1_expected_time_native_min, timer1_expected_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= timer1_expected_time_native_min) &&
                                         (end_time_native <= timer1_expected_time_native_max)));

    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer2);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer2) == true);
    TIMER_ACCURACY_TEST_PRINTF("timer2: end_time_native = %u, expected_min = %u, expected_max = %u\n",
                               end_time_native, timer2_expected_time_native_min, timer2_expected_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= timer2_expected_time_native_min) &&
                                         (end_time_native <= timer2_expected_time_native_max)));

    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer3);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer3) == true);
    TIMER_ACCURACY_TEST_PRINTF("timer3: end_time_native = %u, expected_min = %u, expected_max = %u\n",
                               end_time_native, timer3_expected_time_native_min, timer3_expected_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= timer3_expected_time_native_min) &&
                                         (end_time_native <= timer3_expected_time_native_max)));

    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer4);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer4) == true);
    TIMER_ACCURACY_TEST_PRINTF("timer4: end_time_native = %u, expected_min = %u, expected_max = %u\n",
                               end_time_native, timer4_expected_time_native_min, timer4_expected_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= timer4_expected_time_native_min) &&
                                         (end_time_native <= timer4_expected_time_native_max)));

    // check no more events
    NL_TEST_ASSERT(inSuite, nl_eventqueue_get_event_with_timeout(sQueue, 0) == NULL);
}

/* Test two timers with one cancelled with echo:
 *   timer1: 1000ms
 *   timer2: 2000ms
 *   cancel with echo timer1
 *   verify timer1
 *   verify timer2
 */
static void Test_cancel(nlTestSuite *inSuite, void *inContext)
{
    nl_time_native_t start_time_native;
    __attribute__((unused)) nl_time_native_t end_time_native;
    nl_event_timer_t timer1;
    nl_event_timer_t timer2;
    nl_time_native_t timer2_expected_time_native_min;
    __attribute__((unused)) nl_time_native_t timer2_expected_time_native_max;
    nl_event_t *receivedEvent;

    nl_event_timer_init(&timer1, NULL, NULL, sQueue);
    nl_event_timer_init(&timer2, NULL, NULL, sQueue);

    printf("\nTest cancel event timer ----\n");

    // start the two timers but immediately cancel the first
    start_time_native = nl_get_time_native();

    nl_event_timer_start(&timer1, TIMER1_TIMEOUT_MS, false);
    nl_event_timer_start(&timer2, TIMER2_TIMEOUT_MS, false);

    nl_event_timer_cancel(&timer1);

    timer2_expected_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER2_TIMEOUT_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;
    timer2_expected_time_native_max = timer2_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;

    // should not have any events in the queue
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
    NL_TEST_ASSERT(inSuite, receivedEvent == NULL);

    // wait for timer2
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer2);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer2) == true);
    TIMER_ACCURACY_TEST_PRINTF("timer2: end_time_native = %u, expected_min = %u, expected_max = %u\n",
                               end_time_native, timer2_expected_time_native_min, timer2_expected_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= timer2_expected_time_native_min) &&
                                         (end_time_native <= timer2_expected_time_native_max)));

    // check no more events
    NL_TEST_ASSERT(inSuite, nl_eventqueue_get_event_with_timeout(sQueue, 0) == NULL);
}

static void Test_resend(nlTestSuite *inSuite, void *inContext)
{
    nl_time_native_t start_time_native;
    __attribute__((unused)) nl_time_native_t end_time_native;
    nl_time_native_t expected_end_time_native_min;
    __attribute__((unused)) nl_time_native_t expected_end_time_native_max;
    nl_event_timer_t timer1;
    nl_event_t *receivedEvent;

    nl_event_timer_init(&timer1, NULL, NULL, sQueue);

    printf("\nTest starting already running event timer ----\n");
    start_time_native = nl_get_time_native();
    // start timer
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_1000_MS, false);
    nl_task_sleep_ms(TIMER_TIMEOUT_500_MS);
    // restart timer with same timeout, but reset to current time
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_1000_MS, false);
    // since we restarted before timer ran, should have no events
    // in the queue
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
    NL_TEST_ASSERT(inSuite, receivedEvent == NULL);
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    expected_end_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER_TIMEOUT_1000_MS + TIMER_TIMEOUT_500_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;
    expected_end_time_native_max = expected_end_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;
    TIMER_ACCURACY_TEST_PRINTF("start_time_native = %u, end_time_native = %u, expected_min = %u, expected max = %u\n",
                               start_time_native, end_time_native, expected_end_time_native_min, expected_end_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= expected_end_time_native_min) &&
                                         (end_time_native <= expected_end_time_native_max)));
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);
    NL_TEST_ASSERT(inSuite, nl_eventqueue_get_event_with_timeout(sQueue, 0) == NULL);
}

static void Test_resend_cancel(nlTestSuite *inSuite, void *inContext)
{
    nl_time_native_t start_time_native;
    __attribute__((unused)) nl_time_native_t end_time_native;
    nl_time_native_t expected_end_time_native_min;
    __attribute__((unused)) nl_time_native_t expected_end_time_native_max;
    nl_event_timer_t timer1;
    nl_event_t *receivedEvent;

    nl_event_timer_init(&timer1, NULL, NULL, sQueue);

    printf("\nTest start, cancel, start event timer ----\n");

    start_time_native = nl_get_time_native();
    // start timer
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_1000_MS, false);
    nl_task_sleep_ms(TIMER_TIMEOUT_500_MS);
    // cancel before restarting while timer has not run yet
    nl_event_timer_cancel(&timer1);
    // restart timer with same timeout, but reset to current time
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_1000_MS, false);
    // since we cancelled before timer ran, should have no events
    // in the queue
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
    NL_TEST_ASSERT(inSuite, receivedEvent == NULL);
    // check for second timer event, which should be valid
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    expected_end_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER_TIMEOUT_1000_MS + TIMER_TIMEOUT_500_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;
    expected_end_time_native_max = expected_end_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;
    TIMER_ACCURACY_TEST_PRINTF("start_time_native = %u, end_time_native = %u, expected_min = %u, expected max = %u\n",
                               start_time_native, end_time_native, expected_end_time_native_min, expected_end_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= expected_end_time_native_min) &&
                                         (end_time_native <= expected_end_time_native_max)));
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);
    NL_TEST_ASSERT(inSuite, nl_eventqueue_get_event_with_timeout(sQueue, 0) == NULL);

    start_time_native = nl_get_time_native();
    // start timer
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_1000_MS, false);
    nl_task_sleep_ms(TIMER_TIMEOUT_500_MS + TIMER_TIMEOUT_1000_MS);
    // cancel before restarting while timer is not running (should have already run)
    nl_event_timer_cancel(&timer1);
    // restart timer with same timeout, but reset to current time
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_1000_MS, false);
    // check for the cancelled timer event (since we cancelled after
    // timer was already supposed to run, there should already be
    // an event on the queue but it should be invalid)
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == false);
    // check for the real timer event
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    expected_end_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER_TIMEOUT_1000_MS + TIMER_TIMEOUT_500_MS + TIMER_TIMEOUT_1000_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;
    expected_end_time_native_max = expected_end_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;
    TIMER_ACCURACY_TEST_PRINTF("start_time_native = %u, end_time_native = %u, expected_min = %u, expected max = %u\n",
                               start_time_native, end_time_native, expected_end_time_native_min, expected_end_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= expected_end_time_native_min) &&
                                         (end_time_native <= expected_end_time_native_max)));
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);

    // check no more events
    NL_TEST_ASSERT(inSuite, nl_eventqueue_get_event_with_timeout(sQueue, 0) == NULL);
}

/* Test four timers with different timeouts with
 * resend in between and verify:
 *   timer1: 100ms
 *   timer2: 500ms
 *   timer3: 1000ms
 *   resend timer1: 270ms
 *   timer4: 2000ms
 *   verify timer1
 *   verify timer2
 *   verify timer3
 *   verify timer4
 */
static void Test_four_resend(nlTestSuite *inSuite, void *inContext)
{
    nl_time_native_t start_time_native;
    __attribute__((unused)) nl_time_native_t end_time_native;
    nl_event_timer_t timer1;
    nl_event_timer_t timer2;
    nl_event_timer_t timer3;
    nl_event_timer_t timer4;
    nl_time_native_t timer1_expected_time_native_min;
    nl_time_native_t timer2_expected_time_native_min;
    nl_time_native_t timer3_expected_time_native_min;
    nl_time_native_t timer4_expected_time_native_min;
    __attribute__((unused)) nl_time_native_t timer1_expected_time_native_max;
    __attribute__((unused)) nl_time_native_t timer2_expected_time_native_max;
    __attribute__((unused)) nl_time_native_t timer3_expected_time_native_max;
    __attribute__((unused)) nl_time_native_t timer4_expected_time_native_max;
    nl_event_t *receivedEvent;

    nl_event_timer_init(&timer1, NULL, NULL, sQueue);
    nl_event_timer_init(&timer2, NULL, NULL, sQueue);
    nl_event_timer_init(&timer3, NULL, NULL, sQueue);
    nl_event_timer_init(&timer4, NULL, NULL, sQueue);

    printf("\nTest four event timers with restart ----\n");

    start_time_native = nl_get_time_native();

    nl_event_timer_start(&timer1, TIMER1_TIMEOUT_MS, false);
    nl_event_timer_start(&timer2, TIMER2_TIMEOUT_MS, false);
    nl_event_timer_start(&timer3, TIMER3_TIMEOUT_MS, false);
    nl_event_timer_start(&timer4, TIMER4_TIMEOUT_MS, false);

    nl_event_timer_start(&timer1, 270, false);

    timer2_expected_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER2_TIMEOUT_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;
    timer3_expected_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER3_TIMEOUT_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;
    timer4_expected_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(TIMER4_TIMEOUT_MS) - TIMER_TIMEOUT_TOLERANCE_TIME;

    timer2_expected_time_native_max = timer2_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;
    timer3_expected_time_native_max = timer3_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;
    timer4_expected_time_native_max = timer4_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;

    // since we restarted timer before it ran, should have
    // no events in the queue
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
    NL_TEST_ASSERT(inSuite, receivedEvent == NULL);

    // check for valid timer1
    timer1_expected_time_native_min = start_time_native + nl_time_ms_to_delay_time_native(270) - TIMER_TIMEOUT_TOLERANCE_TIME;
    timer1_expected_time_native_max = timer1_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);
    TIMER_ACCURACY_TEST_PRINTF("timer1: start_time_native = %u, end_time_native = %u, expected_min = %u, expected_max = %u\n",
                               start_time_native, end_time_native, timer1_expected_time_native_min, timer1_expected_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= timer1_expected_time_native_min) &&
                                         (end_time_native <= timer1_expected_time_native_max)));

    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer2);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer2) == true);
    TIMER_ACCURACY_TEST_PRINTF("timer2: end_time_native = %u, expected_min = %u, expected_max = %u\n",
                               end_time_native, timer2_expected_time_native_min, timer2_expected_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= timer2_expected_time_native_min) &&
                                         (end_time_native <= timer2_expected_time_native_max)));

    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer3);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer3) == true);
    TIMER_ACCURACY_TEST_PRINTF("timer3: end_time_native = %u, expected_min = %u, expected_max = %u\n",
                               end_time_native, timer3_expected_time_native_min, timer3_expected_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= timer3_expected_time_native_min) &&
                                         (end_time_native <= timer3_expected_time_native_max)));

    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
    end_time_native = nl_get_time_native();
    NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer4);
    NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer4) == true);
    TIMER_ACCURACY_TEST_PRINTF("timer4: end_time_native = %u, expected_min = %u, expected_max = %u\n",
                               end_time_native, timer4_expected_time_native_min, timer4_expected_time_native_max);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, ((end_time_native >= timer4_expected_time_native_min) &&
                                         (end_time_native <= timer4_expected_time_native_max)));

    NL_TEST_ASSERT(inSuite, nl_eventqueue_get_event_with_timeout(sQueue, 0) == NULL);
}

// test one repeating timer, verify we receive number we expect, then cancel
static void Test_one_repeating(nlTestSuite *inSuite, void *inContext)
{
    nl_time_native_t start_time_native;
    nl_time_native_t end_time_native;
    __attribute__((unused)) nl_time_native_t elapsed_time_native;
    nl_event_timer_t timer1;
    nl_event_t *receivedEvent;
    const unsigned total_event_count = 10;
    __attribute__((unused)) nl_time_native_t timer1_event_received_times[total_event_count];
    unsigned i;

    nl_event_timer_init(&timer1, NULL, NULL, sQueue);

    printf("\nTest one repeating event timer ----\n");

    start_time_native = nl_get_time_native();
    nl_event_timer_start(&timer1, TIMER1_TIMEOUT_MS, true);

    // wait for events
    for (i = 0; i < total_event_count; i++)
    {
        receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, portMAX_DELAY);
        timer1_event_received_times[i] = nl_get_time_native();
        NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);
        NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
    }

    // check for drift.  we don't expect a repeating timer to drift.
    end_time_native = nl_get_time_native();
    elapsed_time_native = end_time_native - start_time_native;
    TIMER_ACCURACY_TEST_PRINTF("%s: start_time_native = %u, end_time_native = %u, elapsed = %u, expected max elapsed = %u\n",
                               __func__, start_time_native, end_time_native, elapsed_time_native,
                               (nl_time_ms_to_delay_time_native(TIMER1_TIMEOUT_MS) * total_event_count) + TIMER_TIMEOUT_TOLERANCE_TIME);
    TIMER_ACCURACY_TEST_ASSERT(inSuite, elapsed_time_native <= (nl_time_ms_to_delay_time_native(TIMER1_TIMEOUT_MS) * total_event_count) + TIMER_TIMEOUT_TOLERANCE_TIME);

    // cancel timer and wait for last event, which should be marked invalid
    nl_event_timer_cancel(&timer1);
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, TIMER1_TIMEOUT_MS * 2);
    // there could be an event posted or not, depending on whether timer
    // was still running at time of cancel. if there was one received,
    // assert it is invalid.
    if (receivedEvent == (nl_event_t*)&timer1)
    {
        NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == false);
    }

    // check no more events
    NL_TEST_ASSERT(inSuite, nl_eventqueue_get_event_with_timeout(sQueue, 0) == NULL);

    for (i = 0; i < total_event_count; i++)
    {
        TIMER_ACCURACY_TEST_PRINTF("timer1[%u] received at %u, delta = %u\n",
                                   i, timer1_event_received_times[i],
                                   i == 0 ? timer1_event_received_times[i] - start_time_native :
                                   timer1_event_received_times[i] - timer1_event_received_times[i-1]);
    }
}

/* Test four repeating timers with different timeouts and verify.
 * Stop after 5050ms.
 *   timer1: 125ms, expect 40 events
 *   timer2: 500ms, expect 10 events
 *   timer3: 1000ms, expect 5 events
 *   timer4: 2000ms, expect 2 events
 */
static void Test_four_repeating(nlTestSuite *inSuite, void *inContext)
{
    nl_time_native_t start_time_native;
    nl_time_native_t current_time_native;
    nl_time_native_t end_time_native;
    __attribute__((unused)) nl_time_native_t elapsed_time_native;
    nl_time_native_t target_time_native;
    uint8_t timer1_valid_event_count = 0;
    uint8_t timer2_valid_event_count = 0;
    uint8_t timer3_valid_event_count = 0;
    uint8_t timer4_valid_event_count = 0;
    uint8_t timer1_invalid_event_count = 0;
    uint8_t timer2_invalid_event_count = 0;
    uint8_t timer3_invalid_event_count = 0;
    uint8_t timer4_invalid_event_count = 0;
    nl_event_timer_t timer1;
    nl_event_timer_t timer2;
    nl_event_timer_t timer3;
    nl_event_timer_t timer4;
    nl_event_timer_t *receivedEvent;
    nl_time_native_t timer1_period_native;
    nl_time_native_t timer2_period_native;
    nl_time_native_t timer3_period_native;
    nl_time_native_t timer4_period_native;
    nl_time_native_t timer1_expected_time_native_min;
    nl_time_native_t timer2_expected_time_native_min;
    nl_time_native_t timer3_expected_time_native_min;
    nl_time_native_t timer4_expected_time_native_min;
    nl_time_native_t timer1_expected_time_native_max;
    nl_time_native_t timer2_expected_time_native_max;
    nl_time_native_t timer3_expected_time_native_max;
    nl_time_native_t timer4_expected_time_native_max;
    unsigned i;

    /* A bit more dangerous since there's no checks about indexing into
     * the arrays, but can be useful for debugging if something isn't working.
     */
#define PRINT_RECEIVE_TIMES 0
#if PRINT_RECEIVE_TIMES
    nl_time_native_t timer1_event_received_times[40];
    nl_time_native_t timer2_event_received_times[10];
    nl_time_native_t timer3_event_received_times[5];
    nl_time_native_t timer4_event_received_times[2];
#endif

    nl_event_timer_init(&timer1, NULL, NULL, sQueue);
    nl_event_timer_init(&timer2, NULL, NULL, sQueue);
    nl_event_timer_init(&timer3, NULL, NULL, sQueue);
    nl_event_timer_init(&timer4, NULL, NULL, sQueue);

    printf("\nTest four repeating event timers, takes about 5 seconds ----\n");

    timer1_period_native = nl_time_ms_to_delay_time_native(TIMER1_TIMEOUT_MS) - TIME_NATIVE_EXTRA_DELAY;
    timer2_period_native = nl_time_ms_to_delay_time_native(TIMER2_TIMEOUT_MS) - TIME_NATIVE_EXTRA_DELAY;
    timer3_period_native = nl_time_ms_to_delay_time_native(TIMER3_TIMEOUT_MS) - TIME_NATIVE_EXTRA_DELAY;
    timer4_period_native = nl_time_ms_to_delay_time_native(TIMER4_TIMEOUT_MS) - TIME_NATIVE_EXTRA_DELAY;

    start_time_native = current_time_native = nl_get_time_native();
    target_time_native = current_time_native + nl_time_ms_to_delay_time_native(TIMER_TIMEOUT_5000_MS+TIMER_TIMEOUT_50_MS);

    timer1_expected_time_native_min = current_time_native + timer1_period_native;
    timer1_expected_time_native_max = timer1_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;

    timer2_expected_time_native_min = current_time_native + timer2_period_native;
    timer2_expected_time_native_max = timer2_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;

    timer3_expected_time_native_min = current_time_native + timer3_period_native;
    timer3_expected_time_native_max = timer3_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;

    timer4_expected_time_native_min = current_time_native + timer4_period_native;
    timer4_expected_time_native_max = timer4_expected_time_native_min + 2*TIMER_TIMEOUT_TOLERANCE_TIME;

    nl_event_timer_start(&timer1, TIMER1_TIMEOUT_MS, true);
    nl_event_timer_start(&timer2, TIMER2_TIMEOUT_MS, true);
    nl_event_timer_start(&timer3, TIMER3_TIMEOUT_MS, true);
    nl_event_timer_start(&timer4, TIMER4_TIMEOUT_MS, true);

    while (current_time_native < target_time_native)
    {
        receivedEvent = (nl_event_timer_t*)nl_eventqueue_get_event_with_timeout(sQueue, TIMER_TIMEOUT_50_MS);
        current_time_native = nl_get_time_native();
        if (receivedEvent == NULL)
        {
            continue;
        }
        NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(receivedEvent) == true);
        if (receivedEvent == &timer1)
        {
            timer1_valid_event_count++;
            if ((current_time_native < timer1_expected_time_native_min) ||
                (current_time_native > timer1_expected_time_native_max))
            {
                TIMER_ACCURACY_TEST_PRINTF("timer1: current_time_native = %u, expected_min = %u, expected_max = %u\n",
                                           current_time_native, timer1_expected_time_native_min, timer1_expected_time_native_max);
                TIMER_ACCURACY_TEST_ASSERT(inSuite, ((current_time_native >= timer1_expected_time_native_min) &&
                                                     (current_time_native <= timer1_expected_time_native_max)));
            }
            // repeating timers don't have the extra delay added for ticks
            timer1_expected_time_native_min += timer1_period_native;
            timer1_expected_time_native_max += timer1_period_native;
#if PRINT_RECEIVE_TIMES
            timer1_event_received_times[timer1_valid_event_count-1] = current_time_native;
#endif
        }
        else if (receivedEvent == &timer2)
        {
            timer2_valid_event_count++;
            if ((current_time_native < timer2_expected_time_native_min) ||
                (current_time_native > timer2_expected_time_native_max))
            {
                TIMER_ACCURACY_TEST_PRINTF("timer2: current_time_native = %u, expected_min = %u, expected_max = %u\n",
                                           current_time_native, timer2_expected_time_native_min, timer2_expected_time_native_max);
                TIMER_ACCURACY_TEST_ASSERT(inSuite, ((current_time_native >= timer2_expected_time_native_min) &&
                                                     (current_time_native <= timer2_expected_time_native_max)));
            }
            // repeating timers don't have the extra delay added for ticks
            timer2_expected_time_native_min += timer2_period_native;
            timer2_expected_time_native_max += timer2_period_native;
#if PRINT_RECEIVE_TIMES
            timer2_event_received_times[timer2_valid_event_count-1] = current_time_native;
#endif
        }
        else if (receivedEvent == &timer3)
        {
            timer3_valid_event_count++;
            if ((current_time_native < timer3_expected_time_native_min) ||
                (current_time_native > timer3_expected_time_native_max))
            {
                TIMER_ACCURACY_TEST_PRINTF("timer3: current_time_native = %u, expected_min = %u, expected_max = %u\n",
                                           current_time_native, timer3_expected_time_native_min, timer3_expected_time_native_max);
                TIMER_ACCURACY_TEST_ASSERT(inSuite, ((current_time_native >= timer3_expected_time_native_min) &&
                                         (current_time_native <= timer3_expected_time_native_max)));
            }
            // repeating timers don't have the extra delay added for ticks
            timer3_expected_time_native_min += timer3_period_native;
            timer3_expected_time_native_max += timer3_period_native;
#if PRINT_RECEIVE_TIMES
            timer3_event_received_times[timer3_valid_event_count-1] = current_time_native;
#endif
        }
        else if (receivedEvent == &timer4)
        {
            timer4_valid_event_count++;
            if ((current_time_native < timer4_expected_time_native_min) ||
                (current_time_native > timer4_expected_time_native_max))
            {
                TIMER_ACCURACY_TEST_PRINTF("timer4: current_time_native = %u, expected_min = %u, expected_max = %u\n",
                                           current_time_native, timer4_expected_time_native_min, timer4_expected_time_native_max);
                TIMER_ACCURACY_TEST_ASSERT(inSuite, ((current_time_native >= timer4_expected_time_native_min) &&
                                                     (current_time_native <= timer4_expected_time_native_max)));
            }
            // repeating timers don't have the extra delay added for ticks
            timer4_expected_time_native_min += timer4_period_native;
            timer4_expected_time_native_max += timer4_period_native;
#if PRINT_RECEIVE_TIMES
            timer4_event_received_times[timer4_valid_event_count-1] = current_time_native;
#endif
        }
    }
    end_time_native = nl_get_time_native();

    // cancel timers and wait for last event from each, which should be marked invalid
    nl_event_timer_cancel(&timer1);
    nl_event_timer_cancel(&timer2);
    nl_event_timer_cancel(&timer3);
    nl_event_timer_cancel(&timer4);

    NL_TEST_ASSERT(inSuite, timer1_valid_event_count == 40);
    NL_TEST_ASSERT(inSuite, timer2_valid_event_count == 10);
    NL_TEST_ASSERT(inSuite, timer3_valid_event_count == 5);
    NL_TEST_ASSERT(inSuite, timer4_valid_event_count == 2);

    // flush any extra events
    for (i = 0; i < 4; i++) {
        receivedEvent = (nl_event_timer_t*)nl_eventqueue_get_event_with_timeout(sQueue, 0);
        if (receivedEvent == NULL)
        {
            break;
        }
        if (receivedEvent == &timer1)
        {
            timer1_invalid_event_count++;
        }
        else if (receivedEvent == &timer2)
        {
            timer2_invalid_event_count++;
        }
        else if (receivedEvent == &timer3)
        {
            timer3_invalid_event_count++;
        }
        else if (receivedEvent == &timer4)
        {
            timer4_invalid_event_count++;
        }
    }
    NL_TEST_ASSERT(inSuite, timer1_invalid_event_count <= 1);
    NL_TEST_ASSERT(inSuite, timer2_invalid_event_count <= 1);
    NL_TEST_ASSERT(inSuite, timer3_invalid_event_count <= 1);
    NL_TEST_ASSERT(inSuite, timer4_invalid_event_count <= 1);

    elapsed_time_native = end_time_native - start_time_native;
    TIMER_ACCURACY_TEST_PRINTF("%s: start_time_native = %u, end_time_native = %u, elapsed = %u, target_time_native = %u\n",
                               __func__, start_time_native, end_time_native, elapsed_time_native, target_time_native);
#if PRINT_RECEIVE_TIMES
    for (i = 0; i < timer1_valid_event_count; i++)
    {
        nl_time_native_t delta = (i == 0 ? timer1_event_received_times[i] - start_time_native :
                            timer1_event_received_times[i] - timer1_event_received_times[i-1]);
        TIMER_ACCURACY_TEST_PRINTF("timer1[%u] received at %u, delta = %u\n",
                                   i, timer1_event_received_times[i], delta);
        TIMER_ACCURACY_TEST_ASSERT(inSuite, (delta >= timer1_period_native &&
                                             delta <= timer1_period_native + TIMER_TIMEOUT_TOLERANCE_TIME));
    }
    TIMER_ACCURACY_TEST_PRINTF("\n");
    for (i = 0; i < timer2_valid_event_count; i++)
    {
        nl_time_native_t delta = (i == 0 ? timer2_event_received_times[i] - start_time_native :
                            timer2_event_received_times[i] - timer2_event_received_times[i-1]);
        TIMER_ACCURACY_TEST_PRINTF("timer2[%u] received at %u, delta = %u\n",
                                   i, timer2_event_received_times[i], delta);
        TIMER_ACCURACY_TEST_ASSERT(inSuite, (delta >= timer2_period_native &&
                                             delta <= timer2_period_native + TIMER_TIMEOUT_TOLERANCE_TIME));
    }
    TIMER_ACCURACY_TEST_PRINTF("\n");
    for (i = 0; i < timer3_valid_event_count; i++)
    {
        nl_time_native_t delta = (i == 0 ? timer3_event_received_times[i] - start_time_native :
                            timer3_event_received_times[i] - timer3_event_received_times[i-1]);
        TIMER_ACCURACY_TEST_PRINTF("timer3[%u] received at %u, delta = %u\n",
                                   i, timer3_event_received_times[i], delta);
        TIMER_ACCURACY_TEST_ASSERT(inSuite, (delta >= timer3_period_native &&
                                             delta <= timer3_period_native + TIMER_TIMEOUT_TOLERANCE_TIME));
    }
    TIMER_ACCURACY_TEST_PRINTF("\n");
    for (i = 0; i < timer4_valid_event_count; i++)
    {
        nl_time_native_t delta = (i == 0 ? timer4_event_received_times[i] - start_time_native :
                            timer4_event_received_times[i] - timer4_event_received_times[i-1]);
        TIMER_ACCURACY_TEST_PRINTF("timer4[%u] received at %u, delta = %u\n",
                                   i, timer4_event_received_times[i], delta);
        TIMER_ACCURACY_TEST_ASSERT(inSuite, (delta >= timer4_period_native &&
                                             delta <= timer4_period_native + TIMER_TIMEOUT_TOLERANCE_TIME));
    }
    TIMER_ACCURACY_TEST_PRINTF("\n");
#endif

}

static void Test_resend_repeating(nlTestSuite *inSuite, void *inContext)
{
    nl_time_native_t start_time_native;
    nl_time_native_t end_time_native;
    nl_event_timer_t timer1;
    nl_event_t *receivedEvent;
    unsigned i;

    nl_event_timer_init(&timer1, NULL, NULL, sQueue);

    printf("\nTest restarting repeating event timer ----\n");
    // start timer
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_1000_MS, true);
    nl_task_sleep_ms(TIMER_TIMEOUT_500_MS);

    // restart timer with same timeout before it even runs once,
    // should reset to current time
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_100_MS, true);

    // since we restarted before timer ran, should have no events
    // in the queue
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
    NL_TEST_ASSERT(inSuite, receivedEvent == NULL);

    // delay for 550MS and check that we have 5 events in our queue
    nl_task_sleep_ms(TIMER_TIMEOUT_500_MS + TIMER_TIMEOUT_50_MS);
    for (i = 0; i < 5; i++)
    {
        receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
        NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
        NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);
    }

    // delay for 500MS more and then restart the timer, and check that
    // we have 5 invalid events in our queue (none for the restart)
    nl_task_sleep_ms(TIMER_TIMEOUT_500_MS);
    start_time_native = nl_get_time_native();
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_100_MS, true);
    for (i = 0; i < 5; i++)
    {
        receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
        NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
        NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == false);
    }

    // queue should be empty
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
    NL_TEST_ASSERT(inSuite, receivedEvent == NULL);

    // wait for 10 more of the events
    for (i = 0; i < 10; i++)
    {
        receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, TIMER_TIMEOUT_100_MS * 2);
        end_time_native = nl_get_time_native();
        __attribute__((unused)) nl_time_native_t elapsed_time_native = end_time_native - start_time_native;
        start_time_native = end_time_native;
        NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
        NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);
        TIMER_ACCURACY_TEST_ASSERT(inSuite, elapsed_time_native <= (nl_time_ms_to_delay_time_native(TIMER_TIMEOUT_100_MS) + TIMER_TIMEOUT_TOLERANCE_TIME));
    }

    // restart again with a new timeout
    // should reset to current time
    start_time_native = nl_get_time_native();
    nl_event_timer_start(&timer1, TIMER_TIMEOUT_500_MS, true);

    // we restarted right after receiving last event so should
    // not have any events in the queue
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
    NL_TEST_ASSERT(inSuite, receivedEvent == NULL);

    // wait for 10 of the events
    for (i = 0; i < 10; i++)
    {
        receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, TIMER_TIMEOUT_500_MS * 2);
        end_time_native = nl_get_time_native();
        __attribute__((unused)) nl_time_native_t elapsed_time_native = end_time_native - start_time_native;
        start_time_native = end_time_native;
        NL_TEST_ASSERT(inSuite, receivedEvent == (nl_event_t*)&timer1);
        NL_TEST_ASSERT(inSuite, nl_event_timer_is_valid(&timer1) == true);
        TIMER_ACCURACY_TEST_ASSERT(inSuite, elapsed_time_native <= (nl_time_ms_to_delay_time_native(TIMER_TIMEOUT_500_MS) + TIMER_TIMEOUT_TOLERANCE_TIME));
    }

    // cancel timer and wait for last event, which should be marked invalid
    nl_event_timer_cancel(&timer1);

    // we cancelled right after receiving last event so should
    // not have any events in the queue
    receivedEvent = nl_eventqueue_get_event_with_timeout(sQueue, 0);
    NL_TEST_ASSERT(inSuite, receivedEvent == NULL);

    // check no more events
    NL_TEST_ASSERT(inSuite, nl_eventqueue_get_event_with_timeout(sQueue, 0) == NULL);
}

static const nlTest sTests[] = {
    NL_TEST_DEF("single event test", Test_one),
    NL_TEST_DEF("four event test", Test_four),
    NL_TEST_DEF("cancel test", Test_cancel),
    NL_TEST_DEF("resend test", Test_resend),
    NL_TEST_DEF("resend cancel test", Test_resend_cancel),
    NL_TEST_DEF("resend with four timers test", Test_four_resend),

    NL_TEST_DEF("repeating event timer test", Test_one_repeating),
    NL_TEST_DEF("four repeating event timer test", Test_four_repeating),
    NL_TEST_DEF("resend repeating test", Test_resend_repeating),
    NL_TEST_SENTINEL()
};

#if NLER_FEATURE_TIMER_USING_SWTIMER
#if NLER_BUILD_PLATFORM_FREERTOS
static void dummy_task(void *arg)
{
    volatile int *end_flag = (int*)arg;
    printf("dummy_task start\n");
    while (*end_flag == 0)
    {
        nlwatchdog_refresh();
    }
    printf("dummy_task end\n");
    vTaskDelete(NULL);
}
#endif
#endif

int nler_timer_test(void);
int nler_timer_test(void)
{
    nlTestSuite theSuite = {
        "nlertimer",
        &sTests[0],
    };

    sQueue = nl_eventqueue_create(sQueueMemory, sizeof(sQueueMemory));

#if NLER_FEATURE_TIMER_USING_SWTIMER
#if NLER_BUILD_PLATFORM_FREERTOS
    nl_task_t task_handle;
    int end_dummy_task = 0;
    uint8_t dummy_stack[512];

    /* Align the dummy_stack_ptr to required alignment */
    uint8_t *dummy_stack_ptr = ALIGN_POINTER(dummy_stack, NLER_FEATURE_STACK_ALIGNMENT);

    /* To allow our timer functions to test timer accuracy,
     * we spawn a thread that just spins at low (but not lowest
     * priority).  The FreeRTOS idle thread function disables
     * task scheduling in it's main loop, and if the tick
     * interrupt happens during scheduler suspend, the tick
     * function is called, but the tick count reported by
     * xTaskGetTickCount() won't be right, so any tests of
     * accuracy from within timer functions can fail.
     */
    nl_task_create(dummy_task, "dum", dummy_stack_ptr, sizeof(dummy_stack) - (dummy_stack_ptr - dummy_stack),
                   kIdleTaskPrio + 1, &end_dummy_task, &task_handle);

    // block sleep since many of our tests check for timer accuracy.
    // will be released by the last test
    g_swtimer_prevent_sleep = true;
#endif
#endif

    nlTestRunner(&theSuite, NULL);

#if NLER_FEATURE_TIMER_USING_SWTIMER
#if NLER_BUILD_PLATFORM_FREERTOS
    g_swtimer_prevent_sleep = false;

    end_dummy_task = 1;
    // yield to give chance for dummy task to exit since it's using our stack for its own
    vTaskDelay(10);
#endif
#endif

    nl_eventqueue_destroy(sQueue);

    return nlTestRunnerStats(&theSuite);
}
#endif
