/*
 *
 *    Copyright (c) 2020 Project nler Authors
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
 *      This file implements a unit test for the NLER event
 *      publish/subscribe interfaces.
 *
 *      In theory, this test could run forever, with the
 *      producer/publisher expiring timers and posting "sensor" events
 *      and the consumer/subscribing getting those events. In
 *      practice, the test is bounded with a number of expected events
 *      produced and consumed for each "sensor" type.
 *
 *      The test succeeds when the actual number of events meets the
 *      expected number for both producer and consumer.
 *
 *      The test fails if more events are produced or consumed than
 *      expected or if an errant event occurs.
 *
 */

#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 1

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nlerassert.h>
#include <nlereventqueue.h>
#include <nlererror.h>
#include <nlerinit.h>
#include <nlerlog.h>
#include <nlertask.h>
#include <nlertimer.h>

/*
 * Preprocessor Defitions
 */

#define kTHREAD_PUBLISHER_SLEEP_MS  491
#define kTHREAD_MAIN_SLEEP_MS       241

#define kTHREAD_GET_EVENT_WAIT_MS  2003

/**
 *  The "sensor" updates in this unit test are sent on two timer ticks:
 *
 *    - One every kSENSOR_TIMER_INSTANT_MS for instantaneous updates of
 *      current sensor state.
 *    - One every kSENSOR_TIMER_BUFFERED_MS for buffered sensor state
 *      updates.
 */
#define kSENSOR_TIMER_INSTANT_ID             1
#define kSENSOR_TIMER_BUFFERED_ID            2

#define kSENSOR_TIMER_INSTANT_MS           293
#define kSENSOR_TIMER_BUFFERED_MS          743

/**
 *  Expected number of sensor events to be produced and consumed for
 *  each sensor type.
 */
#define kSENSOR_TYPE_CO_EXPECTED_EVENTS     11
#define kSENSOR_TYPE_SMOKE_EXPECTED_EVENTS  13
#define kSENSOR_TYPE_PIR_EXPECTED_EVENTS    17
#define kSENSOR_TYPE_TEMP_EXPECTED_EVENTS   19

/**
 *  The types of "sensors" supported by this unit test.
 */
enum
{
    kSENSOR_TYPE_CO       = 0,
    kSENSOR_TYPE_SMOKE    = 1,
    kSENSOR_TYPE_PIR      = 2,
    kSENSOR_TYPE_TEMP     = 3,

    kSENSOR_TYPES_MAX
};

static nltask_t sTaskSubscriber;
static nltask_t sTaskPublisher;
static DEFINE_STACK(sStackSubscriber, NLER_TASK_STACK_BASE + 128);
static DEFINE_STACK(sStackPublisher,  NLER_TASK_STACK_BASE + 128);

/* data types shared across all tasks
 */

typedef struct globalData_s
{
    nleventqueue_t          *mTimerQueue;
    nleventqueue_t           mPublisher;
    nleventqueue_t           mSubscriber;
} globalData_t;

typedef struct sensor_stats_s
{
    int32_t                  mExpect;
    int32_t                  mActual;
} sensor_stats_t;

typedef struct taskData_s
{
    globalData_t            *mGlobals;
    bool                     mFailed;
    bool                     mSucceeded;
} taskData_t;

typedef struct taskSubscriberData_s
{
    taskData_t               mData;
    sensor_stats_t           mReceiveStats[kSENSOR_TYPES_MAX];
} taskSubscriberData_t;

typedef struct taskPublisherData_s
{
    taskData_t               mData;
} taskPublisherData_t;

#define NUM_SUBS_PER_SENSOR 2

typedef struct bufferState_s
{
    uint8_t *mBuffer;
    int32_t mReadIdx;
    int32_t mEndIdx;
    int32_t mBufferEnd;
} bufferState_t;

struct nl_sensor_event_s;

typedef struct sensor_sub_stats_s
{
    int32_t                      mCurrent;
    int32_t                      mMinimum;
    int32_t                      mMaximum;
} sensor_sub_stats_t;

typedef struct sensor_sub_info_s
{
    struct nl_sensor_event_s    *mSubs[NUM_SUBS_PER_SENSOR];
    sensor_sub_stats_t           mSubStats;
    sensor_stats_t               mSendStats;
    int                          mEchosWaiting;
    bufferState_t                mState;
} sensor_sub_info_t;

typedef struct driverData_s
{
    sensor_sub_info_t       mSmoke;
    sensor_sub_info_t       mCO;
    sensor_sub_info_t       mPIR;
    sensor_sub_info_t       mTemp;
    uint8_t                 mBuffer[1024];
} driverData_t;

#define NL_EVENT_T_SENSOR   (NL_EVENT_T_WM_USER + 1)

#define SENSOR_TYPE_FLAG(aType)  (1 << (aType))

#define SENSOR_TYPE_FLAG_CO      SENSOR_TYPE_FLAG(kSENSOR_TYPE_CO)
#define SENSOR_TYPE_FLAG_SMOKE   SENSOR_TYPE_FLAG(kSENSOR_TYPE_SMOKE)
#define SENSOR_TYPE_FLAG_PIR     SENSOR_TYPE_FLAG(kSENSOR_TYPE_PIR)
#define SENSOR_TYPE_FLAG_TEMP    SENSOR_TYPE_FLAG(kSENSOR_TYPE_TEMP)

#define SENSOR_TYPE_MASK         0x000ff

/* request for instantaneous sensor updates
 * at whatever frequency they come in for a
 * given sensor.
 */

#define SENSOR_FLAG_INSTANT      0x10000

/* request for buffered sensor reports at
 * whatever occasional interval they are reported
 * from the driver layer.
 */

#define SENSOR_FLAG_BUFFERED     0x20000

/* cancel the subscription. set this flag and
 * with the next report you will be excluded.
 */

#define SENSOR_FLAG_UNSUB        0x40000

union instantBuffer_u
{
    bufferState_t   *mState;
    int32_t         mValue;
};

typedef struct nl_sensor_event_s
{
    NL_DECLARE_EVENT
    nleventqueue_t         *mReturnQueue;
    uint32_t                mSensorFlags;
    union instantBuffer_u   mSensorUpdate;
} nl_sensor_event_t;

typedef struct nl_test_event_timer_s
{
    nl_event_timer_t    mTimerEvent;
    int                 mID;
} nl_test_event_timer_t;

/* the application task that makes requests of the driver
 * task and responds to sensor events from the driver task
 */

/* this a generic event handler that all of the sensor specific
 * ones call in to. the individual handlers simply illustrate
 * how the events themselves can cause functions specific to
 * the event itself to be invoked directly by the event handler.
 */

static int nl_test_sensor_eventhandler(nl_event_t *aEvent, void *aClosure);

static int nl_test_smoke_eventhandler(nl_event_t *aEvent, void *aClosure);
static int nl_test_co_eventhandler(nl_event_t *aEvent, void *aClosure);
static int nl_test_pir_eventhandler(nl_event_t *aEvent, void *aClosure);
static int nl_test_temp_eventhandler(nl_event_t *aEvent, void *aClosure);

static nl_sensor_event_t sSmokeev =
{
    NL_INIT_EVENT_STATIC(NL_EVENT_T_SENSOR, nl_test_smoke_eventhandler, NULL),
    NULL, SENSOR_TYPE_FLAG_SMOKE | SENSOR_FLAG_INSTANT,
    { 0 },
};

static nl_sensor_event_t sCoev =
{
    NL_INIT_EVENT_STATIC(NL_EVENT_T_SENSOR, nl_test_co_eventhandler, NULL),
    NULL, SENSOR_TYPE_FLAG_CO | SENSOR_FLAG_INSTANT,
    { 0 },
};

static nl_sensor_event_t sPirev =
{
    NL_INIT_EVENT_STATIC(NL_EVENT_T_SENSOR, nl_test_pir_eventhandler, NULL),
    NULL, SENSOR_TYPE_FLAG_PIR | SENSOR_FLAG_INSTANT,
    { 0 },
};

static nl_sensor_event_t sTempev =
{
    NL_INIT_EVENT_STATIC(NL_EVENT_T_SENSOR, nl_test_temp_eventhandler, NULL),
    NULL, SENSOR_TYPE_FLAG_TEMP | SENSOR_FLAG_BUFFERED,
    { 0 },
};

static nl_test_event_timer_t sSensortimer1 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, NULL, NULL),
        NULL, 0, 0, 0, 0
    },
    kSENSOR_TIMER_INSTANT_ID
};

static nl_test_event_timer_t sSensortimer2 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, NULL, NULL),
        NULL, 0, 0, 0, 0
    },
    kSENSOR_TIMER_BUFFERED_ID
};

static int nl_test_sensor_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    const nltask_t                *curtask = nltask_get_current();
    nl_sensor_event_t             *sensor = (nl_sensor_event_t *)aEvent;
    taskSubscriberData_t          *taskData = (taskSubscriberData_t *)aClosure;
    globalData_t                  *data = taskData->mData.mGlobals;
    int                            status;
    int                            retval = NLER_SUCCESS;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d\n", nltask_get_name(curtask), aEvent->mType);

    if (sensor->mSensorFlags & SENSOR_FLAG_INSTANT)
    {
        NL_LOG_DEBUG(lrTEST, "'%s' sensor: %d, instant value: %d\n",
                     nltask_get_name(curtask), sensor->mSensorFlags & SENSOR_TYPE_MASK,
                     sensor->mSensorUpdate.mValue);
    }

    if (sensor->mSensorFlags & SENSOR_FLAG_BUFFERED)
    {
        if (sPirev.mSensorFlags & SENSOR_FLAG_UNSUB)
        {
            /* the PIR sensor has been unsubscribed from, re-subscribe. */

            sPirev.mSensorFlags &= ~SENSOR_FLAG_UNSUB;
            status = nleventqueue_post_event(&data->mPublisher, (nl_event_t *)&sPirev);
            NLER_ASSERT(status == NLER_SUCCESS);
        }

        NL_LOG_DEBUG(lrTEST, "'%s' sensor: %d, buffered values: %d, %d, %d, %p\n",
                     nltask_get_name(curtask), sensor->mSensorFlags & SENSOR_TYPE_MASK,
                     sensor->mSensorUpdate.mState->mReadIdx, sensor->mSensorUpdate.mState->mEndIdx,
                     sensor->mSensorUpdate.mState->mBufferEnd, sensor->mSensorUpdate.mState->mBuffer);

        status = nleventqueue_post_event(&data->mPublisher, aEvent);
        NLER_ASSERT(status == NLER_SUCCESS);
    }

    switch(sensor->mSensorFlags & SENSOR_TYPE_MASK)
    {

    case SENSOR_TYPE_FLAG_CO:
        taskData->mReceiveStats[kSENSOR_TYPE_CO].mActual++;
        break;

    case SENSOR_TYPE_FLAG_SMOKE:
        taskData->mReceiveStats[kSENSOR_TYPE_SMOKE].mActual++;
        break;

    case SENSOR_TYPE_FLAG_PIR:
        taskData->mReceiveStats[kSENSOR_TYPE_PIR].mActual++;
        break;

    case SENSOR_TYPE_FLAG_TEMP:
        taskData->mReceiveStats[kSENSOR_TYPE_TEMP].mActual++;
        break;

    default:
        taskData->mData.mFailed = true;

        retval = NLER_ERROR_FAILURE;
        break;

    }

    return retval;
}

static int nl_test_smoke_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    return nl_test_sensor_eventhandler(aEvent, aClosure);
}

static int nl_test_co_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    return nl_test_sensor_eventhandler(aEvent, aClosure);
}

static int nl_test_pir_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    int                 retval = 0;
    nl_sensor_event_t   *sensor = (nl_sensor_event_t *)aEvent;

    /* having received a PIR sensor update, unsubscribe. */

    sensor->mSensorFlags |= SENSOR_FLAG_UNSUB;

    retval = nl_test_sensor_eventhandler(aEvent, aClosure);

    return retval;
}

static int nl_test_temp_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    return nl_test_sensor_eventhandler(aEvent, aClosure);
}

/* this is a backstop event handler that will be called when
 * an event comes in with no predetermined destination.
 */

static int nl_test_default_handler(nl_event_t *aEvent, void *aClosure)
{
    const nltask_t                *curtask = nltask_get_current();
    taskSubscriberData_t          *taskData = (taskSubscriberData_t *)aClosure;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d -- unexpected\n", nltask_get_name(curtask), aEvent->mType);

    taskData->mData.mFailed = true;

    return 0;
}

static void check_succeeded_or_failed(const nltask_t *aTask, uint8_t aType, const sensor_stats_t *aStats, bool *aOutSucceeded, bool *aOutFailed)
{
    NL_LOG_DEBUG(lrTEST, "'%s': sensor %d: expected %d actual %d\n",
                 nltask_get_name(aTask),
                 SENSOR_TYPE_FLAG(aType),
                 aStats->mExpect,
                 aStats->mActual);

    if (aStats->mActual > aStats->mExpect)
        *aOutFailed = true;
    else if (aStats->mActual < aStats->mExpect)
        *aOutSucceeded = false;
}

static bool subscriber_is_testing(volatile const taskData_t *aData)
{
    bool retval;

    retval = (!aData->mFailed && !aData->mSucceeded);

    return (retval);
}

/**
 *  Subscriber (consumer) task
 */
static void taskEntrySubscriber(void *aParams)
{
    const nltask_t                   *curtask = nltask_get_current();
    const char                       *name = nltask_get_name(curtask);
    taskSubscriberData_t             *taskData = (taskSubscriberData_t *)aParams;
    globalData_t                     *data = taskData->mData.mGlobals;
    int                               status;

    (void)name;

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %p)\n", name, data->mSubscriber);

    sSmokeev.mHandlerClosure = (void *)taskData;
    sSmokeev.mReturnQueue = &data->mSubscriber;

    sCoev.mHandlerClosure = (void *)taskData;
    sCoev.mReturnQueue = &data->mSubscriber;

    sPirev.mHandlerClosure = (void *)taskData;
    sPirev.mReturnQueue = &data->mSubscriber;

    sTempev.mHandlerClosure = (void *)taskData;
    sTempev.mReturnQueue = &data->mSubscriber;

    /* subscribe to the sensors described by the events.
     * the events used to subscribe will be echoed back
     * when sensor activity occurs.
     */

    status = nleventqueue_post_event(&data->mPublisher, (nl_event_t *)&sSmokeev);
    NLER_ASSERT(status == NLER_SUCCESS);

    status = nleventqueue_post_event(&data->mPublisher, (nl_event_t *)&sCoev);
    NLER_ASSERT(status == NLER_SUCCESS);

    status = nleventqueue_post_event(&data->mPublisher, (nl_event_t *)&sPirev);
    NLER_ASSERT(status == NLER_SUCCESS);

    status = nleventqueue_post_event(&data->mPublisher, (nl_event_t *)&sTempev);
    NLER_ASSERT(status == NLER_SUCCESS);

    while (subscriber_is_testing(&taskData->mData))
    {
        nl_event_t  *ev;
        size_t       i;
        bool         succeeded = true;
        bool         failed = false;

        ev = nleventqueue_get_event_with_timeout(&data->mSubscriber, kTHREAD_GET_EVENT_WAIT_MS);

        if (ev != NULL)
        {
            nl_dispatch_event(ev, nl_test_default_handler, NULL);
        }

        for (i = 0; i < kSENSOR_TYPES_MAX; i++)
        {
            const sensor_stats_t *mStats = &taskData->mReceiveStats[i];

            check_succeeded_or_failed(curtask, i, mStats, &succeeded, &failed);
        }

        NL_LOG_CRIT(lrTEST, "'%s' %s number of expected events\n", name, ((succeeded) ? "successfully received" : ((failed) ? "failed to receive" : "has not yet received")));

        if (succeeded)
            taskData->mData.mSucceeded = true;

        if (failed)
            taskData->mData.mFailed = true;
    }

    NL_LOG_CRIT(lrTEST, "'%s' exiting\n", name);
}

/**
 *  The driver tasks that handles subscriptions, takes input from
 *  the individual drivers (not illustrated in this example), and
 *  sends updates out to the subscribers. The updates in this example
 *  are sent on two timer ticks:
 *
 *    - One every kSENSOR_TIMER_INSTANT_MS for instantaneous updates of
 *      current sensor state.
 *    - One every kSENSOR_TIMER_BUFFERED_MS for buffered sensor state
 *      updates.
 */
static bool check_for_unsubscribe(sensor_sub_info_t *aSubInfo, int aIndex)
{
    bool               retval = false;
    const nltask_t    *curtask = nltask_get_current();

    (void)curtask;

    if (aSubInfo->mSubs[aIndex]->mSensorFlags & SENSOR_FLAG_UNSUB)
    {
        NL_LOG_CRIT(lrTEST, "'%s' removing subscriber for sensor type %x (subscriber count now %d)\n",
                    nltask_get_name(curtask), aSubInfo->mSubs[aIndex]->mSensorFlags & SENSOR_TYPE_MASK, (aSubInfo->mSubStats.mCurrent - 1));

        if (aIndex < (aSubInfo->mSubStats.mCurrent - 1))
        {
            memmove(&aSubInfo->mSubs[aIndex], &aSubInfo->mSubs[aIndex + 1],
                    sizeof(nl_sensor_event_t *) * ((aSubInfo->mSubStats.mCurrent - 1) - aIndex));
        }

        aSubInfo->mSubStats.mCurrent--;

        if (aSubInfo->mSubStats.mCurrent < aSubInfo->mSubStats.mMinimum)
        {
            aSubInfo->mSubStats.mMinimum = aSubInfo->mSubStats.mCurrent;
        }

        retval = true;
    }

    return retval;
}

static bool check_for_test_completion(const nltask_t *aTask, uint8_t aType, const sensor_stats_t *aStats)
{
    const bool retval = (aStats->mActual >= aStats->mExpect);

    return retval;
}

static void send_instant_sensor_events(sensor_sub_info_t *aSubInfo)
{
    int idx;
    int status;

    for (idx = 0; idx < aSubInfo->mSubStats.mCurrent; idx++)
    {
        if (aSubInfo->mSubs[idx] != NULL)
        {
            /* Check for any unsubscription requests that came in and
             * handle them.
             */
            if (check_for_unsubscribe(aSubInfo, idx))
            {
                idx--;
                continue;
            }

            /* Check to see whether the publisher has sent all the
             * events required for the test. If so, skip to the next
             * "sensor".
             */
            if (check_for_test_completion(nltask_get_current(), aSubInfo->mSubs[idx]->mSensorFlags & SENSOR_TYPE_MASK, &aSubInfo->mSendStats))
            {
                continue;
            }

            /* Send the "sensor" instant events to subscribers.
             */
            if (aSubInfo->mSubs[idx]->mSensorFlags & SENSOR_FLAG_INSTANT)
            {
                aSubInfo->mSubs[idx]->mSensorUpdate.mValue = nl_get_time_native() + idx;
                status = nleventqueue_post_event(aSubInfo->mSubs[idx]->mReturnQueue, (nl_event_t *)aSubInfo->mSubs[idx]);
                NLER_ASSERT(status == NLER_SUCCESS);

                aSubInfo->mSendStats.mActual++;
            }
        }
    }
}

void send_buffered_sensor_events(sensor_sub_info_t *aSubInfo)
{
    int idx;
    int status;

    for (idx = 0; idx < aSubInfo->mSubStats.mCurrent; idx++)
    {
        if (aSubInfo->mSubs[idx] != NULL)
        {
            /* Check for any unsubscription requests that came in and
             * handle them.
             */
            if (check_for_unsubscribe(aSubInfo, idx))
            {
                idx--;
                continue;
            }

            /* Check to see whether the publisher has sent all the
             * events required for the test. If so, skip to the next
             * "sensor".
             */
            if (check_for_test_completion(nltask_get_current(), aSubInfo->mSubs[idx]->mSensorFlags & SENSOR_TYPE_MASK, &aSubInfo->mSendStats))
            {
                continue;
            }

            /* Send the "sensor" buffered events to subscribers.
             */
            if (aSubInfo->mSubs[idx]->mSensorFlags & SENSOR_FLAG_BUFFERED)
            {
                aSubInfo->mEchosWaiting++;

                aSubInfo->mSubs[idx]->mSensorUpdate.mState = &aSubInfo->mState;

                status = nleventqueue_post_event(aSubInfo->mSubs[idx]->mReturnQueue, (nl_event_t *)aSubInfo->mSubs[idx]);
                NLER_ASSERT(status == NLER_SUCCESS);

                aSubInfo->mSendStats.mActual++;
            }
        }
    }
}

static bool nl_driver_timer_eventhandler(nl_test_event_timer_t *aEvent, driverData_t *aData)
{
    const nltask_t    *curtask = nltask_get_current();

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' sensor timeout: %d\n", nltask_get_name(curtask), aEvent->mID);

    switch (aEvent->mID)
    {
        case kSENSOR_TIMER_INSTANT_ID:
            send_instant_sensor_events(&aData->mCO);
            send_instant_sensor_events(&aData->mSmoke);
            send_instant_sensor_events(&aData->mPIR);
            send_instant_sensor_events(&aData->mTemp);
            break;

        case kSENSOR_TIMER_BUFFERED_ID:
            send_buffered_sensor_events(&aData->mCO);
            send_buffered_sensor_events(&aData->mSmoke);
            send_buffered_sensor_events(&aData->mPIR);
            send_buffered_sensor_events(&aData->mTemp);
            break;

        default:
            break;
    }

    return false;
}

static bool echo_or_add_sub(sensor_sub_info_t *aSubInfo, nl_sensor_event_t *aEvent)
{
    bool              retval = false;
    int               idx;
    const nltask_t   *curtask = nltask_get_current();

    (void)curtask;

    for (idx = 0; idx < aSubInfo->mSubStats.mCurrent; idx++)
    {
        if (aSubInfo->mSubs[idx] == aEvent)
        {
            if (aSubInfo->mEchosWaiting > 0)
            {
                aSubInfo->mEchosWaiting--;

                if (aSubInfo->mEchosWaiting == 0)
                    retval = true;
            }
            else
            {
                NL_LOG_CRIT(lrTEST, "'%s' got echo for sensor type %x with no echos waiting\n",
                            nltask_get_name(curtask), aEvent->mSensorFlags);
            }

            break;
        }
    }

    if (idx == aSubInfo->mSubStats.mCurrent)
    {
        if (aSubInfo->mSubStats.mCurrent < NUM_SUBS_PER_SENSOR)
        {
            aSubInfo->mSubs[aSubInfo->mSubStats.mCurrent++] = aEvent;

            NL_LOG_CRIT(lrTEST, "'%s' added subscriber for sensor type %x (subscriber count now %d)\n",
                        nltask_get_name(curtask), aEvent->mSensorFlags & SENSOR_TYPE_MASK, aSubInfo->mSubStats.mCurrent);

            if (aSubInfo->mSubStats.mCurrent > aSubInfo->mSubStats.mMaximum)
            {
                aSubInfo->mSubStats.mMaximum = aSubInfo->mSubStats.mCurrent;
            }
        }
    }

    return retval;
}

static bool nl_driver_sensor_eventhandler(nl_sensor_event_t *aEvent, driverData_t *aData)
{
    bool               retval;
    const nltask_t    *curtask = nltask_get_current();

    (void)curtask;

    if (aEvent->mSensorFlags & SENSOR_TYPE_FLAG_CO)
    {
        retval = echo_or_add_sub(&aData->mCO, aEvent);
    }

    if (aEvent->mSensorFlags & SENSOR_TYPE_FLAG_SMOKE)
    {
        retval = echo_or_add_sub(&aData->mSmoke, aEvent);
    }

    if (aEvent->mSensorFlags & SENSOR_TYPE_FLAG_PIR)
    {
        retval = echo_or_add_sub(&aData->mPIR, aEvent);
    }

    if (aEvent->mSensorFlags & SENSOR_TYPE_FLAG_TEMP)
    {
        retval = echo_or_add_sub(&aData->mTemp, aEvent);
    }

    if (retval == true)
    {
        NL_LOG_DEBUG(lrTEST, "'%s' sensor type %d has received all echos, ready to send another buffer\n",
                     nltask_get_name(curtask), aEvent->mSensorFlags & SENSOR_TYPE_MASK);
    }

    return false;
}

static bool nl_test_driver_eventhandler(nl_event_t *aEvent, driverData_t *aData)
{
    bool               retval = false;
    const nltask_t    *curtask = nltask_get_current();

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d\n", nltask_get_name(curtask), aEvent->mType);

    switch (aEvent->mType)
    {
        case NL_EVENT_T_TIMER:
            retval = nl_driver_timer_eventhandler((nl_test_event_timer_t *)aEvent, aData);
            break;

        case NL_EVENT_T_SENSOR:
            retval = nl_driver_sensor_eventhandler((nl_sensor_event_t *)aEvent, aData);
            break;

        default:
            break;
    }

    return retval;
}

static int send_sensor_timer(nleventqueue_t *aTimerQueue, nl_test_event_timer_t *aEvent, nl_time_ms_t aTimeoutMS)
{
    int status;

    nl_init_event_timer(&aEvent->mTimerEvent, aTimeoutMS);

    status = nleventqueue_post_event(aTimerQueue, (nl_event_t *)aEvent);

    return status;
}

static bool publisher_is_testing(volatile const taskData_t *aData)
{
    bool retval;

    retval = (!aData->mFailed && !aData->mSucceeded);

    return (retval);
}

/**
 *  Producer (producer) task
 */
static void taskEntryPublisher(void *aParams)
{
    const nltask_t                     *curtask = nltask_get_current();
    const char                         *name = nltask_get_name(curtask);
    taskPublisherData_t                *taskData = (taskPublisherData_t *)aParams;
    globalData_t                       *data = taskData->mData.mGlobals;
    static driverData_t                 ddata;
    int                                 status;

    (void)name;

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %p)\n", name, data->mSubscriber);

    memset(&ddata, 0, sizeof(ddata));

    ddata.mSmoke.mState.mBuffer = ddata.mBuffer;
    ddata.mSmoke.mState.mBufferEnd = sizeof(ddata.mBuffer);
    ddata.mSmoke.mSendStats.mActual = 0;
    ddata.mSmoke.mSendStats.mExpect = kSENSOR_TYPE_SMOKE_EXPECTED_EVENTS;

    ddata.mCO.mState.mBuffer = ddata.mBuffer;
    ddata.mCO.mState.mBufferEnd = sizeof(ddata.mBuffer);
    ddata.mCO.mSendStats.mActual = 0;
    ddata.mCO.mSendStats.mExpect = kSENSOR_TYPE_CO_EXPECTED_EVENTS;

    ddata.mPIR.mState.mBuffer = ddata.mBuffer;
    ddata.mPIR.mState.mBufferEnd = sizeof(ddata.mBuffer);
    ddata.mPIR.mSendStats.mActual = 0;
    ddata.mPIR.mSendStats.mExpect = kSENSOR_TYPE_PIR_EXPECTED_EVENTS;

    ddata.mTemp.mState.mBuffer = ddata.mBuffer;
    ddata.mTemp.mState.mBufferEnd = sizeof(ddata.mBuffer);
    ddata.mTemp.mSendStats.mActual = 0;
    ddata.mTemp.mSendStats.mExpect = kSENSOR_TYPE_TEMP_EXPECTED_EVENTS;

    sSensortimer1.mTimerEvent.mFlags = NLER_TIMER_FLAG_REPEAT;
    sSensortimer1.mTimerEvent.mHandlerClosure = (void *)data;
    sSensortimer1.mTimerEvent.mReturnQueue = &data->mPublisher;

    sSensortimer2.mTimerEvent.mFlags = NLER_TIMER_FLAG_REPEAT;
    sSensortimer2.mTimerEvent.mHandlerClosure = (void *)data;
    sSensortimer2.mTimerEvent.mReturnQueue = &data->mPublisher;

    status = send_sensor_timer(data->mTimerQueue, &sSensortimer1, kSENSOR_TIMER_INSTANT_MS);
    NLER_ASSERT(status == NLER_SUCCESS);

    status = send_sensor_timer(data->mTimerQueue, &sSensortimer2, kSENSOR_TIMER_BUFFERED_MS);
    NLER_ASSERT(status == NLER_SUCCESS);

    while (publisher_is_testing(&taskData->mData))
    {
        nl_event_t  *ev;
        bool         succeeded = true;
        bool         failed = false;

        ev = nleventqueue_get_event_with_timeout(&data->mPublisher, kTHREAD_GET_EVENT_WAIT_MS);

        if (ev != NULL)
        {
            nl_test_driver_eventhandler(ev, &ddata);
        }

        check_succeeded_or_failed(curtask, kSENSOR_TYPE_CO, &ddata.mCO.mSendStats, &succeeded, &failed);
        check_succeeded_or_failed(curtask, kSENSOR_TYPE_SMOKE, &ddata.mSmoke.mSendStats, &succeeded, &failed);
        check_succeeded_or_failed(curtask, kSENSOR_TYPE_PIR, &ddata.mPIR.mSendStats, &succeeded, &failed);
        check_succeeded_or_failed(curtask, kSENSOR_TYPE_TEMP, &ddata.mTemp.mSendStats, &succeeded, &failed);

        NL_LOG_CRIT(lrTEST, "'%s' %s number of expected events\n", name, ((succeeded) ? "successfully sent" : ((failed) ? "failed to send" : "has not yet sent")));

        if (succeeded || failed)
        {
            /* The test is done (success or failure), the publisher
             * may now shut down the timers.
             */
            sSensortimer1.mTimerEvent.mFlags |= NLER_TIMER_FLAG_CANCELLED;
            sSensortimer2.mTimerEvent.mFlags |= NLER_TIMER_FLAG_CANCELLED;

            if (succeeded)
                taskData->mData.mSucceeded = true;

            if (failed)
                taskData->mData.mFailed = true;

            nltask_sleep_ms(kTHREAD_PUBLISHER_SLEEP_MS);
        }
    }

    NL_LOG_CRIT(lrTEST, "'%s' exiting\n", name);
}

/**
 *  Determine whether or not the main thread should continue to wait
 *  for the test to complete.
 *
 *  The main thread should wait for testing until:
 *
 *    * Either thread failed (mFailed == true).
 *    * Both threads succeeded (mSucceeded == true).
 *
 */
static bool is_testing(volatile const taskData_t *aSubscriber,
                       volatile const taskData_t *aPublisher)
{
    bool retval = true;

    if ((aSubscriber->mFailed || aPublisher->mFailed) ||
        (aSubscriber->mSucceeded && aPublisher->mSucceeded))
    {
        retval = false;
    }

    return retval;
}

static bool was_successful(volatile const taskData_t *aSubscriber,
                           volatile const taskData_t *aPublisher)
{
    bool retval = false;

    if (aSubscriber->mFailed || aPublisher->mFailed)
        retval = false;
    else if (aSubscriber->mSucceeded && aPublisher->mSucceeded)
        retval = true;

    return retval;
}

bool nler_subpub_test(nleventqueue_t *aTimerQueue)
{
    static globalData_t               globals;
    taskSubscriberData_t              taskDataSubscriber;
    taskPublisherData_t               taskDataPublisher;
    nl_event_t                       *queuememSubscriber[50];
    nl_event_t                       *queuememPublisher[50];
    int                               status;
    bool                              retval;


    /* the global data contains all of the event queues used for task
     * to task communication (timer, driver and application).
     */

    globals.mTimerQueue = aTimerQueue;
    NLER_ASSERT(globals.mTimerQueue != NULL);

    status = nleventqueue_create(queuememPublisher, sizeof(queuememPublisher), &globals.mPublisher);
    NLER_ASSERT(status == NLER_SUCCESS);

    status = nleventqueue_create(queuememSubscriber, sizeof(queuememSubscriber), &globals.mSubscriber);
    NLER_ASSERT(status == NLER_SUCCESS);

    taskDataSubscriber.mData.mGlobals = &globals;
    taskDataSubscriber.mData.mFailed = false;
    taskDataSubscriber.mData.mSucceeded = false;
    taskDataSubscriber.mReceiveStats[kSENSOR_TYPE_CO].mActual    = 0;
    taskDataSubscriber.mReceiveStats[kSENSOR_TYPE_CO].mExpect    = kSENSOR_TYPE_CO_EXPECTED_EVENTS;
    taskDataSubscriber.mReceiveStats[kSENSOR_TYPE_SMOKE].mActual = 0;
    taskDataSubscriber.mReceiveStats[kSENSOR_TYPE_SMOKE].mExpect = kSENSOR_TYPE_SMOKE_EXPECTED_EVENTS;
    taskDataSubscriber.mReceiveStats[kSENSOR_TYPE_PIR].mActual   = 0;

    taskDataSubscriber.mReceiveStats[kSENSOR_TYPE_PIR].mExpect   = kSENSOR_TYPE_PIR_EXPECTED_EVENTS;
    taskDataSubscriber.mReceiveStats[kSENSOR_TYPE_TEMP].mActual  = 0;
    taskDataSubscriber.mReceiveStats[kSENSOR_TYPE_TEMP].mExpect  = kSENSOR_TYPE_TEMP_EXPECTED_EVENTS;

    taskDataPublisher.mData.mGlobals = &globals;
    taskDataPublisher.mData.mFailed = false;
    taskDataPublisher.mData.mSucceeded = false;

    nltask_create(taskEntrySubscriber,
                  "Subscriber",
                  sStackSubscriber,
                  sizeof (sStackSubscriber),
                  NLER_TASK_PRIORITY_LOW,
                  (void *)&taskDataSubscriber,
                  &sTaskSubscriber);
    nltask_create(taskEntryPublisher,
                  "Publisher",
                  sStackPublisher,
                  sizeof(sStackPublisher),
                  NLER_TASK_PRIORITY_NORMAL,
                  (void *)&taskDataPublisher,
                  &sTaskPublisher);

    while (is_testing(&taskDataSubscriber.mData, &taskDataPublisher.mData))
    {
        nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);
    }

    retval = was_successful(&taskDataSubscriber.mData, &taskDataPublisher.mData);

    return retval;
}

static void nler_test_timer_stop(nleventqueue_t *aTimerQueue)
{
    static const nl_event_timer_t sTimerStopEvent = { NL_INIT_EVENT_STATIC(NL_EVENT_T_EXIT, 0, 0) };

    int status;

    status = nleventqueue_post_event(aTimerQueue, (nl_event_t *)&sTimerStopEvent);
    NLER_ASSERT(status == NLER_SUCCESS);
}

int main(int argc, char **argv)
{
    nleventqueue_t *queue;
    bool            status;

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main\n");

    queue = nl_timer_start(NLER_TASK_PRIORITY_HIGH);
    NLER_ASSERT(queue != NULL);
 
    nl_er_start_running();

    status = nler_subpub_test(queue);

    nler_test_timer_stop(queue);

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (status ? EXIT_SUCCESS : EXIT_FAILURE);
}
