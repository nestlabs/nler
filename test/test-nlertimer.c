/*
 *
 *    Copyright (c) 2015-2016 Nest Labs, Inc.
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
 *      This file implements a unit test for the NLER timer interfaces.
 *
 */

#include "nlerinit.h"

#define nl_eventqueue_create                  ut_nl_eventqueue_create
#define nl_task_create                        ut_nl_task_create
#define nl_eventqueue_get_event_with_timeout  ut_nl_eventqueue_get_event_with_timeout
#define nl_eventqueue_post_event              ut_nl_eventqueue_post_event

#include "../shared/nlertimer.c"
#include "../shared/nlerlog.c"
#include "../shared/nlerlogmanager.c"

#ifdef NL_LOG_DEBUG
#undef NL_LOG_DEBUG
#undef NL_LOG_WARN
#undef NL_LOG_CRIT
#undef NL_LOG
#endif

#ifdef NL_ER_LOG_H
#undef NL_ER_LOG_H
#endif
#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 3
#include "nlerlog.h"

#define lrUTEST 0

#ifdef DEBUG_TRACE_FUNCTIONS
#define TRACE() NL_LOG(lrUTEST, "%s", __FUNCTION__)
#define TRACELN() NL_LOG(lrUTEST, "%s\n", __FUNCTION__)
#else
#define TRACE()
#define TRACELN()
#endif

#define report_asserts()

#ifndef assert_test
#include <assert.h>
static int asserts = 0;
static int failed_asserts = 0;
#undef report_asserts

static void report_asserts()
{
    NL_LOG(lrUTEST, "asserts failed vs total = %d/%d\n", failed_asserts, asserts);
}

static void assert_test_failed()
{
    failed_asserts++;
    assert(0);
}
static void assert_test(int x)
{
    asserts++;
    if (!x)
    {
        assert_test_failed();
    }
}
#endif

uint8_t gAppLogLevels[] =
{
    [lrUTEST] = 3
};

#define NL_UT_EVENT_T_ADVANCE         (NL_EVENT_T_WM_USER)
#define NL_UT_EVENT_T_VERIFY          (NL_EVENT_T_WM_USER+1)
#define NL_UT_EVENT_T_REPLACE         (NL_EVENT_T_WM_USER+2)
#define NL_UT_EVENT_T_DUMMY           (NL_EVENT_T_WM_USER+3)
#define NL_UT_EVENT_T_REPLACE_NO_SEND (NL_EVENT_T_WM_USER+4)


nl_time_native_t        _sCurTime;
nl_event_timer_t       *_sCurEventHead = NULL;
int                     _sNumEvents = 0;
int                     _sSkipPost = 0;

nl_time_native_t nl_get_time_native(void)
{
    NL_LOG_DEBUG(lrUTEST, "%d = ", _sCurTime);
    TRACELN();
    return _sCurTime;
}

nl_time_ms_t nl_time_native_to_time_ms(nl_time_native_t aTime)
{
    TRACE();
    NL_LOG_DEBUG(lrUTEST, "(%d)\n", aTime);
    return aTime;
}

nl_time_native_t nl_time_ms_to_delay_time_native(nl_time_ms_t aTime)
{
    TRACE();
    NL_LOG_DEBUG(lrUTEST, "(%d)\n", aTime);
    return aTime;
}

int ut_nl_eventqueue_post_event(nl_eventqueue_t aEventQueue, const nl_event_t *aEvent)
{
    TRACE();
    if(!_sSkipPost)
    {
        NL_LOG_DEBUG(lrUTEST, "(%p %p)\n", aEventQueue, aEvent);

    if (_sCurEventHead->mType == NL_UT_EVENT_T_VERIFY)
        {
        assert_test(_sCurEventHead->mReturnQueue == aEventQueue);
        assert_test(_sCurEventHead->mFlags == ((nl_event_timer_t*)aEvent)->mFlags);
        _sCurEventHead++;
    }
    else
    {
          assert_test(0);
    }
    }

    return 0;
}

nl_event_t *ut_nl_eventqueue_get_event_with_timeout(nl_eventqueue_t aEventQueue, nl_time_ms_t aTimeoutMS)
{
    nl_event_t *retval = NULL;
    TRACE();
    NL_LOG_DEBUG(lrUTEST, "(%p %d)\n", aEventQueue, aTimeoutMS);
    
    int done;

    do
    {
        done = 1;

        switch (_sCurEventHead->mType)
    {
    case NL_EVENT_T_TIMER:
        nl_init_event_timer(_sCurEventHead, _sCurEventHead->mTimeoutMS);
        // intentional fall through
    case NL_UT_EVENT_T_DUMMY:
        NL_LOG_DEBUG(lrUTEST, "returning (ev:%d)\n", _sCurEventHead->mType);
        retval = (nl_event_t*) _sCurEventHead;
        _sNumEvents++;
        break;
    case NL_EVENT_T_EXIT:
            _sNumEvents++;
        retval = (nl_event_t*) _sCurEventHead;
            break;
    case NL_UT_EVENT_T_ADVANCE:
        NL_LOG_DEBUG(lrUTEST, "advancing by %d ms\n", aTimeoutMS);
        assert_test(aTimeoutMS == _sCurEventHead->mTimeoutMS);
        _sCurTime += nl_time_ms_to_delay_time_native(aTimeoutMS);
        break;
    case NL_UT_EVENT_T_REPLACE_NO_SEND:
        done = 0;
        // intentional fall through
    case NL_UT_EVENT_T_REPLACE:
        retval = (nl_event_t*) (_sCurEventHead - (intptr_t)_sCurEventHead->mReturnQueue);
        ((nl_event_timer_t*)retval)->mFlags = _sCurEventHead->mFlags;
        nl_init_event_timer((nl_event_timer_t *) retval,  _sCurEventHead->mTimeoutMS);
        break;
    default:
        NL_LOG_CRIT(lrUTEST, "Error: mType == %d\n" , _sCurEventHead->mType);
        assert_test(0);
    }

    _sCurEventHead++;
    } while (!done);

    return retval;
}

nl_eventqueue_t ut_nl_eventqueue_create(void *aQueueMemory, size_t aQueueMemorySize)
{
    TRACE();
    return NULL;
}

int ut_nl_task_create(nl_task_entry_point_t aEntry,
                   const char *aName,
                   void *aStack,
                   size_t aStackSize,
                   nl_task_priority_t aPriority,
                   void *aParams,
                   nl_task_t *aOutTask)
{
    TRACE();
    return 0;
}

static void default_logger(void *aClosure, nl_log_region_t aRegion, int aPriority, const char *format, va_list ap);

void default_logger(void *aClosure, nl_log_region_t aRegion, int aPriority, const char *format, va_list ap)
{
    vprintf(format, ap);
}

int nl_er_init(void)
{
    int retval = NLER_SUCCESS;

    nl_set_logging_function(default_logger, NULL);

    return retval;
}

// simple 1 exit event
nl_event_timer_t test1_events[] = 
{
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_DUMMY, 0, 0)
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0)
    }
};


// 1 sec timout
// 1 sec advance
// verify
// exit
nl_event_timer_t test2_events[] = 
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0xdeadbeef,
    .mTimeoutMS = 1000,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 1000
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0xdeadbeef,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0)
    }
};

// 2000 ms  (id 4)
// 1000 ms  (id 3)
//  500 ms  (id 2)
//  250 ms  (id 1)
// advance 250
// verify 1
// advance 250
// verify 2
// advance 500
// verify 3
// advance 1000
// verify 4
// exit
nl_event_timer_t test3_events[] = 
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x4,
    .mTimeoutMS = 2000,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x3,
    .mTimeoutMS = 1000,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mTimeoutMS = 500,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mTimeoutMS = 250,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 250
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 250
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 500
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x3,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 1000
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x4,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0)
    }
};

//  250 ms  (id 1)
//  500 ms  (id 2)
// 1001 ms *(id 3)
//  270 ms  (id 1)
// 2000 ms  (id 4)
// advance 270
// verify 1
// advance 230
// verify 2
// advance 501
// verify 3
// advance 999
// verify 4
// advance 2
// verify 3
// cancel 3
// exit
nl_event_timer_t test4_events[] = 
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mTimeoutMS = 250,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mTimeoutMS = 500,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x3,
    .mTimeoutMS = 1001,
    .mFlags = NLER_TIMER_FLAG_REPEAT,
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_REPLACE, 0, 0),
    .mReturnQueue = (void*) 3,
    .mFlags = 0,
    .mTimeoutMS = 270
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x4,
    .mTimeoutMS = 2000,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 270
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 230
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 501
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x3,
    .mFlags = NLER_TIMER_FLAG_REPEAT
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 999
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x4,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 2
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x3,
    .mFlags = NLER_TIMER_FLAG_REPEAT
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_REPLACE, 0, 0),
        .mReturnQueue = (void*) 13,
    .mFlags = NLER_TIMER_FLAG_CANCELLED
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0)
    }
};

/*
     2000 ms  qid 1
     3000 ms  qid 2

     replace cancel echo qid 1
     advance 2000:q
 */
nl_event_timer_t test5_events[] =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mTimeoutMS = 2000,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mTimeoutMS = 3000,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_REPLACE, 0, 0),
        .mReturnQueue = (void*) 2,
    .mFlags = NLER_TIMER_FLAG_CANCEL_ECHO
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mFlags = NLER_TIMER_FLAG_CANCEL_ECHO
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 3000
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0),
    },
};

/*
         2000 ms wake      qid 1
         1000 ms regular   qid 2
     86400000 ms wake      qid 3

     advance 1000
     verify      qid1
     exit

     assert_test that there are 2 events left in the timer
     call nl_get_wake_time
     verify that the wake time is 1000
 */
nl_event_timer_t test6_events[] =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mTimeoutMS = 2000,
    .mFlags = NLER_TIMER_FLAG_WAKE
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mTimeoutMS = 1000,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x3,
    .mTimeoutMS = 86400000,
    .mFlags = NLER_TIMER_FLAG_WAKE
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 1000
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0),
    },
};

/*
         2000 ms wake      qid 1
         1000 ms regular   qid 2
     86400000 ms wake      qid 3

     advance 1000
     verify      qid2
     advance 1000
     verify      qid1
     exit

     assert_test that there is 1 event left in the queue
     call nl_get_wake_time
     verify that the wake time is 86399000
 */
nl_event_timer_t test7_events[] =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mTimeoutMS = 86400000,
    .mFlags = NLER_TIMER_FLAG_WAKE
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mTimeoutMS = 1000,
    .mFlags = 0
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_REPLACE_NO_SEND, 0, 0),
        .mReturnQueue = (void*) 1,
    .mTimeoutMS = 1000,
    .mFlags = NLER_TIMER_FLAG_CANCELLED
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x3,
    .mTimeoutMS = 1500,
    .mFlags = NLER_TIMER_FLAG_WAKE
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_ADVANCE, 0, 0),
    .mTimeoutMS = 1500
    },
    {
        NL_INIT_EVENT_STATIC(NL_UT_EVENT_T_VERIFY, 0, 0),
    .mReturnQueue = (void*) 0x3,
    .mFlags = NLER_TIMER_FLAG_WAKE
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0),
    },
};

nl_event_timer_t test8_events[] = 
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mTimeoutMS = 250,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x2,
    .mTimeoutMS = 500,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x3,
    .mTimeoutMS = 750,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x4,
    .mTimeoutMS = 1000,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0),
    }
};

nl_event_timer_t test9_events[] = 
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, 0, 0),
    .mReturnQueue = (void*) 0x1,
    .mTimeoutMS = 250,
    .mFlags = 0,
    },
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0),
    }
};



static void reinit_stimer(void)
{
    size_t idx;
    _sCurTime = -1000;

    sQueue = (void*) 0x12345678;
    sTimeoutNative = nl_time_ms_to_delay_time_native(NLER_TIMEOUT_NEVER);
    sMinWakeTimeNative = NLER_TIMEOUT_NEVER;
    sEnd = 0;
    sRunning = 1;

    for (idx = 0; idx < sizeof(sTimers)/sizeof(sTimers[0]); idx++)
    {
        sTimers[idx] = (nl_event_timer_t *) 0xdeadbeef;
    }
}

int main(int argc, char **argv)
{
    nl_er_init();

    // hacks just to see 100% code coverage
    _sSkipPost = 1;
    nl_timer_start(0);
    sQueue = NULL;
    sRunning = 0;
    nl_event_timer_t dummyEvent;
    int retval = nl_start_event_timer(&dummyEvent);
    assert_test(retval == NLER_ERROR_INIT);
    sQueue = NULL;
    sRunning = 1;
    retval = nl_start_event_timer(&dummyEvent);
    assert_test(retval == NLER_ERROR_INIT);
    sQueue = (nl_eventqueue_t) 0xdeadbeef;
    sRunning = 0;
    retval = nl_start_event_timer(&dummyEvent);
    assert_test(retval == NLER_ERROR_INIT);
    sQueue = (nl_eventqueue_t) 0xdeadbeef;
    sRunning = 1;
    retval = nl_start_event_timer(&dummyEvent);
    assert_test(retval == NLER_SUCCESS);

    _sSkipPost = 0;

    NL_LOG_DEBUG(lrUTEST, "TEST 1 ----\n\n");
    _sCurEventHead = &test1_events[0];
    _sNumEvents = 0;
    reinit_stimer();
    nl_timer_run_loop(NULL);

    assert_test(_sNumEvents == 2);
    assert_test(sEnd == 0);


    NL_LOG_DEBUG(lrUTEST, "TEST 2 ----\n\n");
    _sCurEventHead = &test2_events[0];
    _sNumEvents = 0;
    reinit_stimer();
    nl_timer_run_loop(NULL);

    assert_test(_sNumEvents == 2);
    assert_test(sEnd == 0);

    NL_LOG_DEBUG(lrUTEST, "TEST 3 ----\n\n");
    _sCurEventHead = &test3_events[0];
    _sNumEvents = 0;
    reinit_stimer();
    nl_timer_run_loop(NULL);

    assert_test(_sNumEvents == 5);
    assert_test(sEnd == 0);

    NL_LOG_DEBUG(lrUTEST, "TEST 4 ----\n\n");
    _sCurEventHead = &test4_events[0];
    _sNumEvents = 0;
    reinit_stimer();
    nl_timer_run_loop(NULL);

    assert_test(_sNumEvents == 5);
    assert_test(sEnd == 0);

    NL_LOG_DEBUG(lrUTEST, "TEST 5 ----\n\n");
    _sCurEventHead = &test5_events[0];
    _sNumEvents = 0;
    reinit_stimer();
    nl_timer_run_loop(NULL);

    assert_test(_sNumEvents == 3);
    assert_test(sEnd == 0);

    NL_LOG_DEBUG(lrUTEST, "TEST 6 ----\n\n");
    _sCurEventHead = &test6_events[0];
    _sNumEvents = 0;
    reinit_stimer();
    nl_timer_run_loop(NULL);

    assert_test(_sNumEvents == 4);
    assert_test(sEnd == 2);
    
    nl_time_native_t wtime = nl_get_wake_time();
    assert_test(wtime == 1000);

    NL_LOG_DEBUG(lrUTEST, "TEST 7 ----\n\n");
    _sCurEventHead = &test7_events[0];
    _sNumEvents = 0;
    reinit_stimer();
    nl_timer_run_loop(NULL);

    assert_test(_sNumEvents == 4);
    assert_test(sEnd == 1);
    
    wtime = nl_get_wake_time();
    assert_test(wtime == 86399000);

    NL_LOG_DEBUG(lrUTEST, "TEST 8 ----\n\n");
    _sCurEventHead = &test8_events[0];
    _sNumEvents = 0;
    reinit_stimer();
    nl_timer_run_loop(NULL);

    assert_test(_sNumEvents == 5);
    assert_test(sEnd == 4);

    _sCurEventHead = &test9_events[0];
    sRunning = 1;
    nl_timer_run_loop(NULL);

    assert_test(_sNumEvents == 7);
    assert_test(sEnd == 4);    

    report_asserts();

    return 0;
}

