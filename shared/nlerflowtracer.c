/*
 *
 *    Copyright (c) 2016-2018 Nest Labs, Inc.
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
 *      This file implements NLER build platform-independent flow tracer
 *      interfaces, when enabled.
 *
 *      The flow tracer interfaces allow for logging to be performed
 *      on time sensitive operations by storing a timestamp and an
 *      event to be outputted later.
 */

#include <FreeRTOS.h>
#include <task.h>
#include "nlertime.h"
#include "nlerlog.h"
#include "nlerflowtracer.h"
#include "nlerflowtrace-enum.h"

static struct nl_tracer_t sTracer;

inline static void nl_flowtracer_add_trace_internal(nl_time_native_t timestamp, nl_trace_event_t event, uint32_t data)
{
    const struct nl_trace_entry_t entry = {timestamp, event, data};

    if (sTracer.isEmpty)
    {
        sTracer.isEmpty = 0;
    }
    else if (sTracer.tail == sTracer.head)
    {
        sTracer.head = ((sTracer.head + 1) < FLOW_TRACE_QUEUE_SIZE) ? (sTracer.head + 1) : 0;
    }

    sTracer.queue[sTracer.tail] = entry;
    sTracer.tail = ((sTracer.tail + 1) < FLOW_TRACE_QUEUE_SIZE) ? (sTracer.tail + 1) : 0;
}

void nl_flowtracer_init(void)
{
    sTracer.head = 0;
    sTracer.tail = 0;
    sTracer.isEmpty = 1;
}

void nl_flowtracer_add_trace(nl_trace_event_t event, uint32_t data)
{
    nl_time_native_t timestamp = nl_get_time_native();
    taskENTER_CRITICAL();
    nl_flowtracer_add_trace_internal(timestamp, event, data);
    taskEXIT_CRITICAL();
}

void nl_flowtracer_add_trace_from_isr(nl_trace_event_t event, uint32_t data)
{
    nl_flowtracer_add_trace_internal(nl_get_time_native_from_isr(), event, data);
}

void nl_flowtracer_output_trace(void)
{
    struct nl_trace_entry_t entry;
    uint16_t tmpHead = sTracer.head;

    if (!sTracer.isEmpty)
    {
        NL_LOG_CRIT(lrEREVENT, "Time (ms)     Event         Data\n");
        do
        {
            entry = sTracer.queue[tmpHead];
            NL_LOG_CRIT(lrEREVENT, "%-14u%-14d%-14u\n", nl_time_native_to_time_ms(entry.timestamp), entry.event, entry.data);
            tmpHead = ((tmpHead + 1) < FLOW_TRACE_QUEUE_SIZE) ? (tmpHead + 1) : 0;
        } while(tmpHead != sTracer.tail);
    }
}

const struct nl_tracer_t nl_flowtracer_get_tracer(void)
{
    return sTracer;
}
