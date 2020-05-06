/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file implements NLER event queues under the FreeRTOS
 *      build platform.
 *
 */

#include "nlereventqueue.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdlib.h>
#include "nlerlog.h"
#include "nlererror.h"
#include "nlertask.h"
#include <string.h>
#include <stdbool.h>

#if NLER_ASSERT_ON_FULL_QUEUE
#include "nlerassert.h"

#ifndef NLER_DUMP_QUEUE_COUNT_LIMIT
#define NLER_DUMP_QUEUE_COUNT_LIMIT 64
#endif /* NLER_DUMP_QUEUE_COUNT_LIMIT */

#endif /* NLER_ASSERT_ON_FULL_QUEUE */

#if NLER_FEATURE_SIMULATEABLE_TIME
#include "nlereventqueue_sim.h"

typedef struct nleventqueue_freertos_s {
    bool prev_get_successful;
    bool count_events;
} nleventqueue_freertos_t;
#endif

int nleventqueue_create(void *aQueueMemory, const size_t aQueueMemorySize, nleventqueue_t *aQueueObj)
{
    int retval = NLER_SUCCESS;
    const size_t qsize = aQueueMemorySize / sizeof(nl_event_t *);

    if ((aQueueMemory != NULL) && (qsize > 0) && (aQueueObj != NULL))
    {
        xQueueCreateStatic(qsize, sizeof(nl_event_t *), aQueueMemory, aQueueObj);
#if NLER_FEATURE_SIMULATEABLE_TIME
        /* We use the uxDummy8 field of the StaticQueue_t to store
         * additional information.
         */
        nleventqueue_freertos_t *sim_queue_info = (nleventqueue_freertos_t *)&aQueueObj->uxDummy8;
        sim_queue_info->prev_get_successful = false;
        sim_queue_info->count_events = true;
#endif

        NL_LOG_CRIT(lrERQUEUE, "created queue %p at %p\n", aQueueObj, aQueueMemory);
    }
    else
    {
        NL_LOG_CRIT(lrERQUEUE, "invalid queue memory %p with size %u specified\n", aQueueMemory, aQueueMemorySize);
        retval = NLER_ERROR_BAD_INPUT;
    }

    return retval;
}

void nleventqueue_destroy(nleventqueue_t *aEventQueue)
{
#if NLER_FEATURE_SIMULATEABLE_TIME
    nleventqueue_freertos_t *sim_queue_info = (nleventqueue_freertos_t *)&aEventQueue->uxDummy8;
    if (sim_queue_info->count_events && sim_queue_info->prev_get_successful)
    {
        nleventqueue_sim_count_dec();
    }
#else
    vQueueDelete((QueueHandle_t)aEventQueue);
#endif
}

void nleventqueue_disable_event_counting(nleventqueue_t *aEventQueue)
{
#if NLER_FEATURE_SIMULATEABLE_TIME
    nleventqueue_freertos_t *sim_queue_info = (nleventqueue_freertos_t *)&aEventQueue->uxDummy8;
    sim_queue_info->count_events = false;
#else
    // This function comes from the shared nler repository and is included here
    // to accomodate libraries (such as network manager) that expect it.
    (void)aEventQueue;
#endif
}

#if NLER_ASSERT_ON_FULL_QUEUE
static void dump_event_contents(nleventqueue_t *aEventQueue, bool aFromIsr)
{
    // dump up to NLER_DUMP_QUEUE_COUNT_LIMIT queued events to help debug what it's full with.
    // do in critical section to prevent anything from mucking with the queue while we do the dump.
    size_t count = 0;
    while (count < NLER_DUMP_QUEUE_COUNT_LIMIT)
    {
        nl_event_t *event = NULL;
        if (aFromIsr)
        {
            if (xQueueReceiveFromISR((QueueHandle_t)aEventQueue, &event, NULL) != pdTRUE)
            {
                break;
            }
        }
        else
        {
            if (xQueueReceive((QueueHandle_t)aEventQueue, &event, 0) != pdTRUE)
            {
                break;
            }
        }
        // event shouldn't be NULL, but we've seen cases where it was
        // in crash logs sent to service (could indicate memory corruption)
        // so be extra cautious not to fault here
        if (event)
        {
            NL_LOG_CRIT(lrERQUEUE, "[%u] event %p, type = %d, handler = %p, closure = %p\n",
                        count, event, event->mType, event->mHandler, event->mHandlerClosure);
        }
        else
        {
            NL_LOG_CRIT(lrERQUEUE, "[%u] event is NULL in queue, unexpected\n", count);
        }
        count++;
    }
}
#endif

int nleventqueue_post_event(nleventqueue_t *aEventQueue, const nl_event_t *aEvent)
{
    int             retval = NLER_SUCCESS;
    portBASE_TYPE   err;
#if NLER_FEATURE_SIMULATEABLE_TIME
    nleventqueue_freertos_t *sim_queue_info = (nleventqueue_freertos_t *)&aEventQueue->uxDummy8;
#endif
    err = xQueueSendToBack((QueueHandle_t) aEventQueue, &aEvent, 0);

    if (err != pdTRUE)
    {
        NL_LOG_CRIT(lrERQUEUE, "attempt to post event %d (%p) to full queue %p from task %s\n",
                aEvent->mType, aEvent, aEventQueue,
                    nltask_get_current() ? nltask_get_name(nltask_get_current()) : "NONE");
        retval = NLER_ERROR_NO_RESOURCE;

#if NLER_ASSERT_ON_FULL_QUEUE
        NL_LOG_CRIT(lrERQUEUE, "nleventqueue_post_event dumping existing events in the full queue:\n");
        taskENTER_CRITICAL();
        dump_event_contents(aEventQueue, false);
        taskEXIT_CRITICAL();

        NLER_ASSERT(0);
#endif

    }
#if NLER_FEATURE_SIMULATEABLE_TIME
    else if (sim_queue_info->count_events)
    {
        nleventqueue_sim_count_inc();
    }
#endif

    return retval;
}

int nleventqueue_post_event_from_isr(nleventqueue_t *aEventQueue, const nl_event_t *aEvent)
{
    int             retval = NLER_SUCCESS;
    portBASE_TYPE   err, yield;
#if NLER_FEATURE_SIMULATEABLE_TIME
    nleventqueue_freertos_t *sim_queue_info = (nleventqueue_freertos_t *)&aEventQueue->uxDummy8;
#endif
    err = xQueueSendToBackFromISR((QueueHandle_t) aEventQueue, &aEvent, &yield);

    if (err != pdTRUE)
    {
        /* don't put logging in here
           this function is executed from an ISR */
        retval = NLER_ERROR_NO_RESOURCE;

#if NLER_ASSERT_ON_FULL_QUEUE
        NL_LOG_CRIT(lrERQUEUE, "nleventqueue_post_event_from_isr dumping existing events in the full queue:\n");
        dump_event_contents(aEventQueue, true);
        /* Assert here indicating that posting event from isr failed */
        NLER_ASSERT(0);
#endif
    }
#if NLER_FEATURE_SIMULATEABLE_TIME
    else if (sim_queue_info->count_events)
    {
        nleventqueue_sim_count_inc();
    }
#endif

    portEND_SWITCHING_ISR(yield);

    return retval;
}

/**
 * NOTE: This function isn't intended for use by clients of NLER.
 *
 * This exists to let NLER functions get events from a queue without incurring a
 * +1 tick offset when converting milliseconds to ticks.
 */
nl_event_t *nleventqueue_get_event_with_timeout_native(nleventqueue_t *aEventQueue, nl_time_native_t aTimeoutNative);
nl_event_t *nleventqueue_get_event_with_timeout_native(nleventqueue_t *aEventQueue, nl_time_native_t aTimeoutNative)
{
    nl_event_t  *retval = NULL;

#if !NLER_FEATURE_SIMULATEABLE_TIME
    if (xQueueReceive((QueueHandle_t)aEventQueue, &retval, aTimeoutNative) != pdTRUE)
    {
        retval = NULL;
    }
#else
    nleventqueue_freertos_t *sim_queue_info = (nleventqueue_freertos_t *)&aEventQueue->uxDummy8;
    if (sim_queue_info->count_events && sim_queue_info->prev_get_successful)
    {
        nleventqueue_sim_count_dec();
    }

    if (xQueueReceive((QueueHandle_t)aEventQueue, &retval, aTimeoutNative) != pdTRUE)
    {
        retval = NULL;
        sim_queue_info->prev_get_successful = false;
    }
    else
    {
        sim_queue_info->prev_get_successful = true;
    }
#endif

    return retval;
}

nl_event_t *nleventqueue_get_event_with_timeout(nleventqueue_t *aEventQueue, nl_time_ms_t aTimeoutMS)
{
    return nleventqueue_get_event_with_timeout_native(aEventQueue, nl_time_ms_to_delay_time_native(aTimeoutMS));
}

uint32_t nleventqueue_get_count(nleventqueue_t *aEventQueue)
{
    uint32_t retval;

    retval = uxQueueMessagesWaiting(aEventQueue);

    return retval;
}
