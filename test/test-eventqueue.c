/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *      This file implements a unit test for the NLER event queue
 *      interface.
 *
 */

#include <nlereventqueue.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 1

#include <nlererror.h>
#include <nlerinit.h>
#include <nlerlog.h>

#include <nltest.h>

#define NL_EVENT_T_TEST (NL_EVENT_T_WM_USER + 1)

typedef struct nl_event_test_s
{
    NL_DECLARE_EVENT
    uint32_t mIdentifier;
} nl_event_test_t;

static void TestCreateAndDestroy(nlTestSuite *inSuite, void *inContext)
{
    nl_event_t             *test_queuemem[5];
    nleventqueue_t          test_queue;
    int                     status;

    /*
     * Creation
     */

    /* Test known, positive failure cases
     */

    /* NULL memory */

    status = nleventqueue_create(NULL, sizeof (test_queuemem), &test_queue);
    NL_TEST_ASSERT(inSuite, status == NLER_ERROR_BAD_INPUT);

    /* Zero size */

    status = nleventqueue_create(&test_queuemem[0], 0, &test_queue);
    NL_TEST_ASSERT(inSuite, status == NLER_ERROR_BAD_INPUT);

    /* NULL object */

    status = nleventqueue_create(&test_queuemem[0], sizeof (test_queuemem), NULL);
    NL_TEST_ASSERT(inSuite, status == NLER_ERROR_BAD_INPUT);

    /* Test success case
     */

    status = nleventqueue_create(&test_queuemem[0], sizeof (test_queuemem), &test_queue);
    NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

    /*
     * Destruction
     */

    nleventqueue_destroy(&test_queue);
}

static void TestGetCount(nlTestSuite *inSuite, void *inContext)
{
    nl_event_t             *test_queuemem[5];
    nleventqueue_t          test_queue;
    int                     status;
    uint32_t                count;

    /*
     * Creation
     */

    status = nleventqueue_create(&test_queuemem[0], sizeof (test_queuemem), &test_queue);
    NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

    /*
     * Get Count
     */

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 0);

    /*
     * Destruction
     */

    nleventqueue_destroy(&test_queue);
}

static void TestDisableCounting(nlTestSuite *inSuite, void *inContext)
{
    nl_event_t             *test_queuemem[5];
    nleventqueue_t          test_queue;
    int                     status;

    /*
     * Creation
     */

    status = nleventqueue_create(&test_queuemem[0], sizeof (test_queuemem), &test_queue);
    NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

    /*
     * Disable Counting
     */

    nleventqueue_disable_event_counting(&test_queue);

    /*
     * Destruction
     */

    nleventqueue_destroy(&test_queue);
}

static void TestPostEvent(nlTestSuite *inSuite, void *inContext)
{
    nl_event_t             *test_queuemem[3];
    nleventqueue_t          test_queue;
    nl_event_test_t         test_events[5] = {
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x1 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x2 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x3 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x4 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x5 }
    };
    int                     status;
    uint32_t                count;

    /*
     * Creation
     */

    status = nleventqueue_create(&test_queuemem[0], sizeof (test_queuemem), &test_queue);
    NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

    /*
     * Post Events
     */

    /* Success Cases
     */

    /* Post First */

    status = nleventqueue_post_event(&test_queue, (nl_event_t *)&test_events[0]);
    NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 1);

    /* Post Second */

    status = nleventqueue_post_event(&test_queue, (nl_event_t *)&test_events[1]);
    NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 2);

    /* Post Third */

    status = nleventqueue_post_event(&test_queue, (nl_event_t *)&test_events[2]);
    NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 3);

    /* Failure Cases
     */

    /* Post Fourth */

    status = nleventqueue_post_event(&test_queue, (nl_event_t *)&test_events[3]);
    NL_TEST_ASSERT(inSuite, status == NLER_ERROR_NO_RESOURCE);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 3);

    /* Post Fifth */

    status = nleventqueue_post_event(&test_queue, (nl_event_t *)&test_events[4]);
    NL_TEST_ASSERT(inSuite, status == NLER_ERROR_NO_RESOURCE);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 3);

    /*
     * Destruction
     */

    nleventqueue_destroy(&test_queue);
}

static void TestGetEvent(nlTestSuite *inSuite, void *inContext)
{
    nl_event_t             *test_queuemem[5];
    nleventqueue_t          test_queue;
    nl_event_test_t         test_events[5] = {
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x1 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x2 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x3 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x4 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x5 }
    };
    const size_t            event_count = sizeof (test_events) / sizeof (test_events[0]);
    nl_event_test_t        *evp;
    int                     status;
    uint32_t                count;
    size_t                  i;

    /*
     * Creation
     */

    status = nleventqueue_create(&test_queuemem[0], sizeof (test_queuemem), &test_queue);
    NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

    /*
     * Post Events
     */

    for (i = 0; i < event_count; i++)
    {
        status = nleventqueue_post_event(&test_queue, (nl_event_t *)&test_events[i]);
        NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

        count = nleventqueue_get_count(&test_queue);
        NL_TEST_ASSERT(inSuite, count == i + 1);
    }

    /*
     * Get Events
     */

    /* First */

    evp = (nl_event_test_t *)nleventqueue_get_event(&test_queue);
    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x1);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 4);

    /* Second */

    evp = (nl_event_test_t *)nleventqueue_get_event(&test_queue);
    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x2);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 3);

    /* Third */

    evp = (nl_event_test_t *)nleventqueue_get_event(&test_queue);
    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x3);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 2);

    /* Fourth */

    evp = (nl_event_test_t *)nleventqueue_get_event(&test_queue);
    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x4);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 1);

    /* Fifth */

    evp = (nl_event_test_t *)nleventqueue_get_event(&test_queue);
    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x5);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 0);

    /*
     * Destruction
     */

    nleventqueue_destroy(&test_queue);
}

static void TestGetEventWithTimeout(nlTestSuite *inSuite, void *inContext)
{
    nl_event_t             *test_queuemem[5];
    nleventqueue_t          test_queue;
    nl_event_test_t         test_events[5] = {
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x1 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x2 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x3 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x4 },
        { NL_INIT_EVENT_STATIC(NL_EVENT_T_TEST, NULL, NULL), 0x5 }
    };
    const size_t            event_count = sizeof (test_events) / sizeof (test_events[0]);
    nl_event_test_t        *evp;
    int                     status;
    uint32_t                count;
    size_t                  i;
    nl_time_native_t        start_time, end_time;
    nl_time_ms_t            delay_time_ms, delta_time_ms;

    /*
     * Creation
     */

    status = nleventqueue_create(&test_queuemem[0], sizeof (test_queuemem), &test_queue);
    NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

    /*
     * Get Event with Timeout
     */

    /* No Event Expected
     */

    /* Zero Timeout, No Event Expected */

    start_time = nl_get_time_native();
    delay_time_ms = NLER_TIMEOUT_NOW;

    evp = (nl_event_test_t *)nleventqueue_get_event_with_timeout(&test_queue, delay_time_ms);

    end_time = nl_get_time_native();
    delta_time_ms = nl_time_native_to_time_ms(end_time - start_time);

    NL_TEST_ASSERT(inSuite, evp == NULL);
    NL_TEST_ASSERT(inSuite, delay_time_ms <= delta_time_ms);

    /* Non-zero Timeout, No Event Expected */

    start_time = nl_get_time_native();
    delay_time_ms = 101;

    evp = (nl_event_test_t *)nleventqueue_get_event_with_timeout(&test_queue, delay_time_ms);

    end_time = nl_get_time_native();
    delta_time_ms = nl_time_native_to_time_ms(end_time - start_time);

    NL_TEST_ASSERT(inSuite, evp == NULL);
    NL_TEST_ASSERT(inSuite, delay_time_ms <= delta_time_ms);

    /*
     * Post Events
     */

    for (i = 0; i < event_count; i++)
    {
        status = nleventqueue_post_event(&test_queue, (nl_event_t *)&test_events[i]);
        NL_TEST_ASSERT(inSuite, status == NLER_SUCCESS);

        count = nleventqueue_get_count(&test_queue);
        NL_TEST_ASSERT(inSuite, count == i + 1);
    }

    /*
     * Get Event with Timeout
     */

    delay_time_ms = 503;

    /* Event Expected
     */

    /* First */

    start_time = nl_get_time_native();

    evp = (nl_event_test_t *)nleventqueue_get_event_with_timeout(&test_queue, delay_time_ms);

    end_time = nl_get_time_native();
    delta_time_ms = nl_time_native_to_time_ms(end_time - start_time);

    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x1);
    NL_TEST_ASSERT(inSuite, delay_time_ms > delta_time_ms);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 4);

    /* Second */

    start_time = nl_get_time_native();

    evp = (nl_event_test_t *)nleventqueue_get_event_with_timeout(&test_queue, delay_time_ms);

    end_time = nl_get_time_native();
    delta_time_ms = nl_time_native_to_time_ms(end_time - start_time);

    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x2);
    NL_TEST_ASSERT(inSuite, delay_time_ms > delta_time_ms);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 3);

    /* Third */

    start_time = nl_get_time_native();

    evp = (nl_event_test_t *)nleventqueue_get_event_with_timeout(&test_queue, delay_time_ms);

    end_time = nl_get_time_native();
    delta_time_ms = nl_time_native_to_time_ms(end_time - start_time);

    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x3);
    NL_TEST_ASSERT(inSuite, delay_time_ms > delta_time_ms);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 2);

    /* Fourth */

    start_time = nl_get_time_native();

    evp = (nl_event_test_t *)nleventqueue_get_event_with_timeout(&test_queue, delay_time_ms);

    end_time = nl_get_time_native();
    delta_time_ms = nl_time_native_to_time_ms(end_time - start_time);

    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x4);
    NL_TEST_ASSERT(inSuite, delay_time_ms > delta_time_ms);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 1);

    /* Fifth */

    start_time = nl_get_time_native();

    evp = (nl_event_test_t *)nleventqueue_get_event_with_timeout(&test_queue, delay_time_ms);

    end_time = nl_get_time_native();
    delta_time_ms = nl_time_native_to_time_ms(end_time - start_time);

    NL_TEST_ASSERT(inSuite, evp != NULL);
    NL_TEST_ASSERT(inSuite, evp->mIdentifier == 0x5);
    NL_TEST_ASSERT(inSuite, delay_time_ms > delta_time_ms);

    count = nleventqueue_get_count(&test_queue);
    NL_TEST_ASSERT(inSuite, count == 0);

    /*
     * Get Event with Timeout
     */

    /* No Event Expected
     */

    /* Zero Timeout, No Event Expected */

    start_time = nl_get_time_native();
    delay_time_ms = NLER_TIMEOUT_NOW;

    evp = (nl_event_test_t *)nleventqueue_get_event_with_timeout(&test_queue, delay_time_ms);

    end_time = nl_get_time_native();
    delta_time_ms = nl_time_native_to_time_ms(end_time - start_time);

    NL_TEST_ASSERT(inSuite, evp == NULL);
    NL_TEST_ASSERT(inSuite, delay_time_ms <= delta_time_ms);

    /* Non-zero Timeout, No Event Expected */

    start_time = nl_get_time_native();
    delay_time_ms = 101;

    evp = (nl_event_test_t *)nleventqueue_get_event_with_timeout(&test_queue, delay_time_ms);

    end_time = nl_get_time_native();
    delta_time_ms = nl_time_native_to_time_ms(end_time - start_time);

    NL_TEST_ASSERT(inSuite, evp == NULL);
    NL_TEST_ASSERT(inSuite, delay_time_ms <= delta_time_ms);

    /*
     * Destruction
     */

    nleventqueue_destroy(&test_queue);
}

static const nlTest sTests[] = {
    NL_TEST_DEF("create and destroy",      TestCreateAndDestroy),
    NL_TEST_DEF("get count"         ,      TestGetCount),
    NL_TEST_DEF("disable counting",        TestDisableCounting),
    NL_TEST_DEF("post event",              TestPostEvent),
    NL_TEST_DEF("get event",               TestGetEvent),
    NL_TEST_DEF("get event with timeout",  TestGetEventWithTimeout),
    NL_TEST_SENTINEL()
};

int nler_eventqueue_test(void)
{
    nlTestSuite theSuite = {
        "nlereventqueue",
        &sTests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}

int main(int argc, char **argv)
{
    int status;

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_start_running();

    status = nler_eventqueue_test();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (status);
}
