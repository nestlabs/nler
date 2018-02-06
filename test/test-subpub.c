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
 *      This file implements a unit test for the NLER event
 *      publish/subscribe interfaces.
 *
 */

#include "nlertask.h"
#include "nlerinit.h"
#include <stdio.h>
#include "nlerlog.h"
#include "nlereventqueue.h"
#include "nlertimer.h"
#include <string.h>

nl_task_t taskA;
nl_task_t taskD;
uint8_t stackA[NLER_TASK_STACK_BASE + 128];
uint8_t stackD[NLER_TASK_STACK_BASE + 128];

/* data types shared across all tasks
 */

typedef struct globalData_s
{
    nl_eventqueue_t mTimer;
    nl_eventqueue_t mDrivers;
    nl_eventqueue_t mQueue;
} globalData_t;

#define NUM_SUBS_PER_SENSOR 2

typedef struct bufferState_s
{
    uint8_t *mBuffer;
    int32_t mReadIdx;
    int32_t mEndIdx;
    int32_t mBufferEnd;
} bufferState_t;

struct nl_sensor_event_s;

typedef struct sensor_sub_info_s
{
    struct nl_sensor_event_s    *mSubs[NUM_SUBS_PER_SENSOR];
    int                         mNumSubs;
    int                         mEchosWaiting;
    bufferState_t               mState;
} sensor_sub_info_t;

typedef struct driverData_s
{
    globalData_t        *mGlobals;
    sensor_sub_info_t   mSmoke;
    sensor_sub_info_t   mCO;
    sensor_sub_info_t   mPIR;
    sensor_sub_info_t   mTemp;
    uint8_t             mBuffer[1024];
} driverData_t;

#define NL_EVENT_T_SENSOR   (NL_EVENT_T_WM_USER + 1)

#define SENSOR_FLAG_CO      1
#define SENSOR_FLAG_SMOKE   2
#define SENSOR_FLAG_PIR     4
#define SENSOR_FLAG_TEMP    8

#define SENSOR_TYPE_FLAGS   0x000ff

/* request for instantaneous sensor updates
 * at whatever frequency they come in for a
 * given sensor.
 */

#define SENSOR_FLAG_INSTANT  0x10000

/* request for buffered sensor reports at
 * whatever occasional interval they are reported
 * from the driver layer.
 */

#define SENSOR_FLAG_BUFFERED 0x20000

/* cancel the subscription. set this flag and
 * with the next report you will be excluded.
 */

#define SENSOR_FLAG_UNSUB    0x40000

union instantBuffer_u
{
    bufferState_t   *mState;
    int32_t         mValue;
};

typedef struct nl_sensor_event_s
{
    NL_DECLARE_EVENT
    nl_eventqueue_t         mReturnQueue;
    uint32_t                mSensorFlags;
    union instantBuffer_u   mSensorUpdate;
} nl_sensor_event_t;

/* the application task that makes requests of the driver
 * task and responds to sensor events from the driver task
 */

/* this a generic event handler that all of the sensor specific
 * ones call in to. the individual handlers simply illustrate
 * how the events themselves can cause functions specific to
 * the event itself to be invoked directly by the event handler.
 */

int nl_test_sensor_eventhandler(nl_event_t *aEvent, void *aClosure);

int nl_test_smoke_eventhandler(nl_event_t *aEvent, void *aClosure);
int nl_test_co_eventhandler(nl_event_t *aEvent, void *aClosure);
int nl_test_pir_eventhandler(nl_event_t *aEvent, void *aClosure);
int nl_test_temp_eventhandler(nl_event_t *aEvent, void *aClosure);

nl_sensor_event_t smokeev =
{
    NL_INIT_EVENT_STATIC(NL_EVENT_T_SENSOR, nl_test_smoke_eventhandler, NULL),
    NULL, SENSOR_FLAG_SMOKE | SENSOR_FLAG_INSTANT,
    { 0 },
};

nl_sensor_event_t coev =
{
    NL_INIT_EVENT_STATIC(NL_EVENT_T_SENSOR, nl_test_co_eventhandler, NULL),
    NULL, SENSOR_FLAG_CO | SENSOR_FLAG_INSTANT,
    { 0 },
};

nl_sensor_event_t pirev =
{
    NL_INIT_EVENT_STATIC(NL_EVENT_T_SENSOR, nl_test_pir_eventhandler, NULL),
    NULL, SENSOR_FLAG_PIR | SENSOR_FLAG_INSTANT,
    { 0 },
};

nl_sensor_event_t tempev =
{
    NL_INIT_EVENT_STATIC(NL_EVENT_T_SENSOR, nl_test_temp_eventhandler, NULL),
    NULL, SENSOR_FLAG_TEMP | SENSOR_FLAG_BUFFERED,
    { 0 },
};

int nl_test_sensor_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    const nl_task_t     *curtask = nl_task_get_current();
    nl_sensor_event_t   *sensor = (nl_sensor_event_t *)aEvent;
    globalData_t        *data = (globalData_t *)aClosure;

    (void)curtask;

    if (sensor->mSensorFlags & SENSOR_FLAG_INSTANT)
    {
        NL_LOG_DEBUG(lrTEST, "'%s' sensor: %d, instant value: %d\n",
                     curtask->mName, sensor->mSensorFlags & SENSOR_TYPE_FLAGS,
                     sensor->mSensorUpdate.mValue);
    }

    if (sensor->mSensorFlags & SENSOR_FLAG_BUFFERED)
    {
        if (pirev.mSensorFlags & SENSOR_FLAG_UNSUB)
        {
            /* the PIR sensor has been unsibscribed from, re-subscribe. */

            pirev.mSensorFlags &= ~SENSOR_FLAG_UNSUB;
            nl_eventqueue_post_event(data->mDrivers, (nl_event_t *)&pirev);
        }

        NL_LOG_DEBUG(lrTEST, "'%s' sensor: %d, buffered values: %d, %d, %d, %p\n",
                     curtask->mName, sensor->mSensorFlags & SENSOR_TYPE_FLAGS,
                     sensor->mSensorUpdate.mState->mReadIdx, sensor->mSensorUpdate.mState->mEndIdx,
                     sensor->mSensorUpdate.mState->mBufferEnd, sensor->mSensorUpdate.mState->mBuffer);

        nl_eventqueue_post_event(data->mDrivers, aEvent);
    }

    return 0;
}

int nl_test_smoke_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    return nl_test_sensor_eventhandler(aEvent, aClosure);
}

int nl_test_co_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    return nl_test_sensor_eventhandler(aEvent, aClosure);
}

int nl_test_pir_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    int                 retval = 0;
    nl_sensor_event_t   *sensor = (nl_sensor_event_t *)aEvent;

    /* having received a PIR sensor update, unsubscribe. */

    sensor->mSensorFlags |= SENSOR_FLAG_UNSUB;

    retval = nl_test_sensor_eventhandler(aEvent, aClosure);

    return retval;
}

int nl_test_temp_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    return nl_test_sensor_eventhandler(aEvent, aClosure);
}

/* this is a backstop event handler that will be called when
 * an event comes in with no predetermined destination.
 */

int nl_test_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    const nl_task_t   *curtask = nl_task_get_current();

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d -- unexpected\n", curtask->mName, aEvent->mType);

    return 0;
}

void taskEntryA(void *aParams)
{
    const nl_task_t       *curtask = nl_task_get_current();
    globalData_t          *data = (globalData_t *)aParams;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %08x)\n", curtask->mName, data->mQueue);

    smokeev.mHandlerClosure = data;
    smokeev.mReturnQueue = data->mQueue;

    coev.mHandlerClosure = data;
    coev.mReturnQueue = data->mQueue;

    pirev.mHandlerClosure = data;
    pirev.mReturnQueue = data->mQueue;

    tempev.mHandlerClosure = data;
    tempev.mReturnQueue = data->mQueue;

    /* subscribe to the sensors described by the events.
     * the events used to subscribe will be echoed back
     * when sensor activity occurs.
     */

    nl_eventqueue_post_event(data->mDrivers, (nl_event_t *)&smokeev);
    nl_eventqueue_post_event(data->mDrivers, (nl_event_t *)&coev);
    nl_eventqueue_post_event(data->mDrivers, (nl_event_t *)&pirev);
    nl_eventqueue_post_event(data->mDrivers, (nl_event_t *)&tempev);

    while (1)
    {
        nl_event_t  *ev;

        ev = nl_eventqueue_get_event(data->mQueue);

        nl_dispatch_event(ev, nl_test_eventhandler, NULL);
    }
}

/* the driver tasks that handles subscriptions, takes input from
 * the individual drivers (not illustrated in this example), and
 * sends updates out to the subscribers. the updates in this example
 * are sent on two timer ticks: one per-second for instantaneous
 * updates of current sensor state and one every thirty seconds for
 * buffered sensor state updates.
 */

int check_for_unsubscribe(sensor_sub_info_t *aSubInfo, int aIndex)
{
    int               retval = 0;
    const nl_task_t   *curtask = nl_task_get_current();

    (void)curtask;

    if (aSubInfo->mSubs[aIndex]->mSensorFlags & SENSOR_FLAG_UNSUB)
    {
        NL_LOG_CRIT(lrTEST, "'%s' removing subscriber for sensor type %x (now %d)\n",
                    curtask->mName, aSubInfo->mSubs[aIndex]->mSensorFlags, (aSubInfo->mNumSubs - 1));

        if (aIndex < (aSubInfo->mNumSubs - 1))
        {
            memmove(&aSubInfo->mSubs[aIndex], &aSubInfo->mSubs[aIndex + 1],
                    sizeof(nl_sensor_event_t *) * ((aSubInfo->mNumSubs - 1) - aIndex));
        }

        aSubInfo->mNumSubs--;

        retval = 1;
    }

    return retval;
}

void send_instant_sensor_events(sensor_sub_info_t *aSubInfo)
{
    int idx;

    for (idx = 0; idx < aSubInfo->mNumSubs; idx++)
    {
        if (aSubInfo->mSubs[idx] != NULL)
        {
            if (check_for_unsubscribe(aSubInfo, idx))
            {
                idx--;
                continue;
            }

            if (aSubInfo->mSubs[idx]->mSensorFlags & SENSOR_FLAG_INSTANT)
            {
                aSubInfo->mSubs[idx]->mSensorUpdate.mValue = nl_get_time_native() + idx;
                nl_eventqueue_post_event(aSubInfo->mSubs[idx]->mReturnQueue, (nl_event_t *)aSubInfo->mSubs[idx]);
            }
        }
    }
}

void send_buffered_sensor_events(sensor_sub_info_t *aSubInfo)
{
    int idx;

    for (idx = 0; idx < aSubInfo->mNumSubs; idx++)
    {
        if (aSubInfo->mSubs[idx] != NULL)
        {
            if (check_for_unsubscribe(aSubInfo, idx))
            {
                idx--;
                continue;
            }

            if (aSubInfo->mSubs[idx]->mSensorFlags & SENSOR_FLAG_BUFFERED)
            {
                aSubInfo->mEchosWaiting++;

                aSubInfo->mSubs[idx]->mSensorUpdate.mState = &aSubInfo->mState;

                nl_eventqueue_post_event(aSubInfo->mSubs[idx]->mReturnQueue, (nl_event_t *)aSubInfo->mSubs[idx]);
            }
        }
    }
}

typedef struct nl_test_event_timer_s
{
    nl_event_timer_t    mTimerEvent;
    int                 mID;
} nl_test_event_timer_t;

nl_test_event_timer_t sensortimer1 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, NULL, NULL),
        NULL, 0, 0, 0, 0
    },
    1
};

nl_test_event_timer_t sensortimer2 =
{
    {
        NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER, NULL, NULL),
        NULL, 0, 0, 0, 0
    },
    2
};

int nl_driver_timer_eventhandler(nl_test_event_timer_t *aEvent, driverData_t *aData)
{
    const nl_task_t   *curtask = nl_task_get_current();

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' sensor timeout: %d\n", curtask->mName, aEvent->mID);

    switch (aEvent->mID)
    {
        case 1:
            send_instant_sensor_events(&aData->mCO);
            send_instant_sensor_events(&aData->mSmoke);
            send_instant_sensor_events(&aData->mPIR);
            send_instant_sensor_events(&aData->mTemp);
            break;

        case 2:
            send_buffered_sensor_events(&aData->mCO);
            send_buffered_sensor_events(&aData->mSmoke);
            send_buffered_sensor_events(&aData->mPIR);
            send_buffered_sensor_events(&aData->mTemp);
            break;

        default:
            break;
    }

    return 0;
}

int echo_or_add_sub(sensor_sub_info_t *aSubInfo, nl_sensor_event_t *aEvent)
{
    int               retval = 0;
    int               idx;
    const nl_task_t   *curtask = nl_task_get_current();

    (void)curtask;

    for (idx = 0; idx < aSubInfo->mNumSubs; idx++)
    {
        if (aSubInfo->mSubs[idx] == aEvent)
        {
            if (aSubInfo->mEchosWaiting > 0)
            {
                aSubInfo->mEchosWaiting--;

                if (aSubInfo->mEchosWaiting == 0)
                    retval = 1;
            }
            else
            {
                NL_LOG_CRIT(lrTEST, "'%s' got echo for sensor type %x with no echos waiting\n",
                            curtask->mName, aEvent->mSensorFlags);
            }

            break;
        }
    }

    if (idx == aSubInfo->mNumSubs)
    {
        if (aSubInfo->mNumSubs < NUM_SUBS_PER_SENSOR)
        {
            aSubInfo->mSubs[aSubInfo->mNumSubs++] = aEvent;

            NL_LOG_CRIT(lrTEST, "'%s' added subscriber for sensor type %x (now %d)\n",
                        curtask->mName, aEvent->mSensorFlags, aSubInfo->mNumSubs);
        }
    }

    return retval;
}

int nl_driver_sensor_eventhandler(nl_sensor_event_t *aEvent, driverData_t *aData)
{
    int               retval;
    const nl_task_t   *curtask = nl_task_get_current();

    (void)curtask;

    if (aEvent->mSensorFlags & SENSOR_FLAG_CO)
    {
        retval = echo_or_add_sub(&aData->mCO, aEvent);
    }

    if (aEvent->mSensorFlags & SENSOR_FLAG_SMOKE)
    {
        retval = echo_or_add_sub(&aData->mSmoke, aEvent);
    }

    if (aEvent->mSensorFlags & SENSOR_FLAG_PIR)
    {
        retval = echo_or_add_sub(&aData->mPIR, aEvent);
    }

    if (aEvent->mSensorFlags & SENSOR_FLAG_TEMP)
    {
        retval = echo_or_add_sub(&aData->mTemp, aEvent);
    }

    if (retval == 1)
    {
        NL_LOG_DEBUG(lrTEST, "'%s' sensor type %d has received all echos, ready to send another buffer\n",
                     curtask->mName, aEvent->mSensorFlags & SENSOR_TYPE_FLAGS);
    }

    return 0;
}

int nl_test_driver_eventhandler(nl_event_t *aEvent, driverData_t *aData)
{
    int               retval = 0;
    const nl_task_t   *curtask = nl_task_get_current();

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "'%s' got event type: %d\n", curtask->mName, aEvent->mType);

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

void send_sensor_timer(nl_test_event_timer_t *aEvent, nl_eventqueue_t aQueue, nl_time_ms_t aTimeoutMS)
{
    nl_init_event_timer(&aEvent->mTimerEvent, aTimeoutMS);
    nl_eventqueue_post_event(aQueue, (nl_event_t *)aEvent);
}

void taskEntryD(void *aParams)
{
    const nl_task_t           *curtask = nl_task_get_current();
    globalData_t              *data = (globalData_t *)aParams;
    static driverData_t       ddata;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "from the task: %s (queue: %08x)\n", curtask->mName, data->mQueue);

    memset(&ddata, 0, sizeof(ddata));

    ddata.mSmoke.mState.mBuffer = ddata.mBuffer;
    ddata.mSmoke.mState.mBufferEnd = sizeof(ddata.mBuffer);

    ddata.mCO.mState.mBuffer = ddata.mBuffer;
    ddata.mCO.mState.mBufferEnd = sizeof(ddata.mBuffer);

    ddata.mPIR.mState.mBuffer = ddata.mBuffer;
    ddata.mPIR.mState.mBufferEnd = sizeof(ddata.mBuffer);

    ddata.mTemp.mState.mBuffer = ddata.mBuffer;
    ddata.mTemp.mState.mBufferEnd = sizeof(ddata.mBuffer);

    sensortimer1.mTimerEvent.mFlags = NLER_TIMER_FLAG_REPEAT;
    sensortimer1.mTimerEvent.mHandlerClosure = data;
    sensortimer1.mTimerEvent.mReturnQueue = data->mDrivers;

    sensortimer2.mTimerEvent.mFlags = NLER_TIMER_FLAG_REPEAT;
    sensortimer2.mTimerEvent.mHandlerClosure = data;
    sensortimer2.mTimerEvent.mReturnQueue = data->mDrivers;

    send_sensor_timer(&sensortimer1, data->mTimer, 1000);
    send_sensor_timer(&sensortimer2, data->mTimer, 30000);

    while (1)
    {
        nl_event_t  *ev;

        ev = nl_eventqueue_get_event(data->mDrivers);

        nl_test_driver_eventhandler(ev, &ddata);
    }
}

int main(int argc, char **argv)
{
    globalData_t    globals;
    nl_event_t      *queuememA[50];
    nl_event_t      *queuememD[50];

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime)\n");

    /* the global data contains all of the event queues used for task
     * to task communication (timer, driver and application).
     */

    globals.mTimer = nl_timer_start(NLER_TASK_PRIORITY_HIGH);
    globals.mDrivers = nl_eventqueue_create(queuememD, sizeof(queuememD));
    globals.mQueue = nl_eventqueue_create(queuememA, sizeof(queuememA));

    nl_task_create(taskEntryA, "A", stackA, sizeof(stackA), NLER_TASK_PRIORITY_LOW, &globals, &taskA);
    nl_task_create(taskEntryD, "D", stackD, sizeof(stackD), NLER_TASK_PRIORITY_NORMAL, &globals, &taskD);

    nl_er_start_running();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return 0;
}

