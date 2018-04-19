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

typedef struct nl_eventqueue_freertos_s {
    void * event_queue;
    bool prev_get_successful;
    bool count_events;
} nl_eventqueue_freertos_t;
#endif

nl_eventqueue_t nl_eventqueue_create(void *aQueueMemory, size_t aQueueMemorySize)
{
    nl_eventqueue_t retval = NULL;
    size_t qsize = aQueueMemorySize / sizeof(nl_event_t *);

    if ((aQueueMemory != NULL) && (qsize > 0))
    {
#if !NLER_FEATURE_SIMULATEABLE_TIME
        retval = (nl_eventqueue_t) xQueueCreate(aQueueMemory, qsize, sizeof(nl_event_t *));
#else
        retval = malloc(sizeof(nl_eventqueue_freertos_t));
        if (retval)
        {
            ((nl_eventqueue_freertos_t *)retval)->event_queue = xQueueCreate(aQueueMemory, qsize, sizeof(nl_event_t *));
            ((nl_eventqueue_freertos_t *)retval)->prev_get_successful = false;
            ((nl_eventqueue_freertos_t *)retval)->count_events = true;
        }
        else
        {
            NL_LOG_CRIT(lrERQUEUE, "failed to malloc nl_eventqueue_sim_t\n");
        }
#endif

#if !NLER_FEATURE_SIMULATEABLE_TIME
        if (retval == NULL)
#else
        if (retval && ((nl_eventqueue_freertos_t *)retval)->event_queue == NULL)
#endif
        {
            NL_LOG_CRIT(lrERQUEUE, "failed to create freertos event queue (%p, %u)\n", aQueueMemory, aQueueMemorySize);
#if NLER_FEATURE_SIMULATEABLE_TIME
            free(retval);
#endif
        }
        else
        {
            NL_LOG_CRIT(lrERQUEUE, "created queue %p at %p\n", retval, aQueueMemory);
        }
    }
    else
    {
        NL_LOG_CRIT(lrERQUEUE, "invalid queue memory %p with size %u specified\n", aQueueMemory, aQueueMemorySize);
    }

    return retval;
}

void nl_eventqueue_destroy(nl_eventqueue_t aEventQueue)
{
#if NLER_FEATURE_SIMULATEABLE_TIME
    nl_eventqueue_freertos_t *eventqueue = (nl_eventqueue_freertos_t *) aEventQueue;
    if (eventqueue->count_events && eventqueue->prev_get_successful)
    {
        nl_eventqueue_sim_count_dec();
    }
    free(eventqueue);
#else
    vQueueDelete(aEventQueue);
#endif
}

void nl_eventqueue_disable_event_counting(nl_eventqueue_t aEventQueue)
{
#if NLER_FEATURE_SIMULATEABLE_TIME
    nl_eventqueue_freertos_t *eventqueue = (nl_eventqueue_freertos_t *) aEventQueue;
    eventqueue->count_events = false;
#else
    // This function comes from the shared nler repository and is included here
    // to accomodate libraries (such as network manager) that expect it.
    (void)aEventQueue;
#endif
}

#if NLER_ASSERT_ON_FULL_QUEUE
static void dump_event_contents(nl_eventqueue_t aEventQueue)
{
    // dump up to NLER_DUMP_QUEUE_COUNT_LIMIT queued events to help debug what it's full with.
    // do in critical section to prevent anything from mucking with the queue while we do the dump.
    size_t count = 0;
    while (count < NLER_DUMP_QUEUE_COUNT_LIMIT)
    {
        nl_event_t *event = NULL;
#if !NLER_FEATURE_SIMULATEABLE_TIME
        if (xQueueReceive((xQueueHandle)aEventQueue, &event, 0) != pdTRUE)
#else
        if (xQueueReceive((xQueueHandle)eventqueue->event_queue, &event, 0) != pdTRUE)
#endif
        {
            break;
        }
        NL_LOG_CRIT(lrERQUEUE, "[%u] event %p, type = %d, handler = %p, closure = %p\n",
                    count, event, event->mType, event->mHandler, event->mHandlerClosure);
        count++;
    }
}
#endif

int nl_eventqueue_post_event(nl_eventqueue_t aEventQueue, const nl_event_t *aEvent)
{
    int             retval = NLER_SUCCESS;
    portBASE_TYPE   err;

#if !NLER_FEATURE_SIMULATEABLE_TIME
    err = xQueueSendToBack((xQueueHandle) aEventQueue, &aEvent, 0);
#else
    nl_eventqueue_freertos_t *eventqueue = (nl_eventqueue_freertos_t *) aEventQueue;
    err = xQueueSendToBack((xQueueHandle) eventqueue->event_queue, &aEvent, 0);
#endif

    if (err != pdTRUE)
    {
        NL_LOG_CRIT(lrERQUEUE, "attempt to post event %d (%p) to full queue %p from task %s\n",
                aEvent->mType, aEvent, aEventQueue,
                nl_task_get_current() ? ((nl_task_t*)nl_task_get_current())->mName : "NONE");
        retval = NLER_ERROR_NO_RESOURCE;

#if NLER_ASSERT_ON_FULL_QUEUE
        NL_LOG_CRIT(lrERQUEUE, "nl_eventqueue_post_event dumping existing events in the full queue:\n");
        taskENTER_CRITICAL();
        dump_event_contents(aEventQueue);
        taskEXIT_CRITICAL();

        NLER_ASSERT(0);
#endif

    }
#if NLER_FEATURE_SIMULATEABLE_TIME
    else if (eventqueue->count_events)
    {
        nl_eventqueue_sim_count_inc();
    }
#endif

    return retval;
}

int nl_eventqueue_post_event_from_isr(nl_eventqueue_t aEventQueue, const nl_event_t *aEvent)
{
    int             retval = NLER_SUCCESS;
    portBASE_TYPE   err, yield;

#if !NLER_FEATURE_SIMULATEABLE_TIME
    err = xQueueSendToBackFromISR((xQueueHandle) aEventQueue, &aEvent, &yield);
#else
    nl_eventqueue_freertos_t *eventqueue = (nl_eventqueue_freertos_t *) aEventQueue;
    err = xQueueSendToBackFromISR((xQueueHandle) eventqueue->event_queue, &aEvent, &yield);
#endif

    if (err != pdTRUE)
    {
        /* don't put logging in here
           this function is executed from an ISR */
        retval = NLER_ERROR_NO_RESOURCE;

#if NLER_ASSERT_ON_FULL_QUEUE
        NL_LOG_CRIT(lrERQUEUE, "nl_eventqueue_post_event_from_isr dumping existing events in the full queue:\n");
        dump_event_contents(aEventQueue);
        /* Assert here indicating that posting event from isr failed */
        NLER_ASSERT(0);
#endif
    }
#if NLER_FEATURE_SIMULATEABLE_TIME
    else if (eventqueue->count_events)
    {
        nl_eventqueue_sim_count_inc();
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
nl_event_t *nl_eventqueue_get_event_with_timeout_native(nl_eventqueue_t aEventQueue, nl_time_native_t aTimeoutNative);
nl_event_t *nl_eventqueue_get_event_with_timeout_native(nl_eventqueue_t aEventQueue, nl_time_native_t aTimeoutNative)
{
    nl_event_t  *retval = NULL;

#if !NLER_FEATURE_SIMULATEABLE_TIME
    if (xQueueReceive((xQueueHandle)aEventQueue, &retval, aTimeoutNative) != pdTRUE)
    {
        retval = NULL;
    }
#else
    nl_eventqueue_freertos_t *eventqueue = (nl_eventqueue_freertos_t *) aEventQueue;
    if (eventqueue->count_events && eventqueue->prev_get_successful)
    {
        nl_eventqueue_sim_count_dec();
    }

    if (xQueueReceive((xQueueHandle) eventqueue->event_queue, &retval, aTimeoutNative) != pdTRUE)
    {
        retval = NULL;
        eventqueue->prev_get_successful = false;
    }
    else
    {
        eventqueue->prev_get_successful = true;
    }
#endif

    return retval;
}

nl_event_t *nl_eventqueue_get_event_with_timeout(nl_eventqueue_t aEventQueue, nl_time_ms_t aTimeoutMS)
{
    return nl_eventqueue_get_event_with_timeout_native(aEventQueue, nl_time_ms_to_delay_time_native(aTimeoutMS));
}

uint32_t nl_eventqueue_get_count(nl_eventqueue_t aEventQueue)
{
    uint32_t retval;

    retval = uxQueueMessagesWaiting(aEventQueue);
}