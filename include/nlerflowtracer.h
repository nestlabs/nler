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
 *      This file defines the flow tracer interface which allows for
 *      logging to be performed on time sensitive operations by
 *      storing a timestamp and an event to be outputted later.
 *
 */

#ifndef _FLOWTRACER_H_INCLUDED__
#define _FLOWTRACER_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#include "nlertime.h"
#include "nlerflowtrace-enum.h"

// Configure the number of log entries for the flow trace queue.
#define FLOW_TRACE_QUEUE_SIZE 25

struct nl_trace_entry_t
{
    nl_time_native_t timestamp;
    nltrace_event_t event;
    uint32_t data;
};

struct nl_tracer_t
{
    uint8_t isEmpty;
    uint16_t head;
    uint16_t tail;
    struct nl_trace_entry_t queue[FLOW_TRACE_QUEUE_SIZE];
};

/*
 * This function initializes the flowtracer object, reseting the head and tail indices
 * and marking the queue empty. This must be called prior to any other flow tracer funciton calls.
 */
void nl_flowtracer_init(void);

/*
 * This functions adds a trace event and corresponding data to the queue. The queue is circular
 * and will overwrite previous entries if full. FLOW_TRACE_QUEUE_SIZE is configuredabove.
 * Note: This functon ensures thread safety by disabling all interrupts while accessing the queue
 */
void nl_flowtracer_add_trace(nltrace_event_t event, uint32_t data);

/*
 * Similar to nl_flowtracer_add_trace, this function provides the ability
 * to trace output from an ISR context.
 */
void nl_flowtracer_add_trace_from_isr(nltrace_event_t event, uint32_t data);

/*
 * This function outputs (to console) the contents of the flow tracer queue.
 * The result is a table displaying the time, event, and data in chronological order.
 * Note: Since this function is printing to console it is NOT safe to call in an ISR.
 */
void nl_flowtracer_output_trace(void);

/*
 * This method is used to facilitate unit testing. This method returns the tracer structure
 * so that assertions may be performed on it's various methods.
 */
const struct nl_tracer_t nl_flowtracer_get_tracer(void);

#ifdef __cplusplus
}
#endif

#endif // _FLOWTRACER_H_INCLUDED__
