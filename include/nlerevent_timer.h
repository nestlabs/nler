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
 *     Timer events. A timer event is sent to the current task after
 *     the specified timeout. It can be restarted or cancelled, but
 *     these operations must only be called from the same task that
 *     receives the event.
 *
 */

#ifndef NL_ER_EVENT_TIMER_H
#define NL_ER_EVENT_TIMER_H

#include "nlereventqueue.h"
#include "nlerevent.h"
#include "nlertime.h"
#include "nlertask.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Timer event. Should be initialized using nl_init_event_timer.
 *  Implementation is opaque to the user but defined here so
 *  that the users can provide the memory for timers.
 */
#if NLER_FEATURE_TIMER_USING_SWTIMER

typedef struct
{
#ifdef DEBUG
    uint32_t hidden[10];
#else
    uint32_t hidden[9];
#endif
} nl_event_timer_t;

#else /* NLER_FEATURE_TIMER_USING_SWTIMER */

#include <stdint.h>

typedef struct
{
    /* check for 64-bit simulators */
#if UINTPTR_MAX == 0xffffffff
#ifdef DEBUG
#if NLER_FEATURE_SIMULATEABLE_TIME
    uint32_t hidden[9];
#else  // NLER_FEATURE_SIMULATEABLE_TIME
    uint32_t hidden[8];
#endif // NLER_FEATURE_SIMULATEABLE_TIME
#else  // DEBUG
    uint32_t hidden[7];
#endif // DEBUG
#elif UINTPTR_MAX == 0xffffffffffffffff
#ifdef DEBUG
#if NLER_FEATURE_SIMULATEABLE_TIME
    uint32_t hidden[16];
#else  // NLER_FEATURE_SIMULATEABLE_TIME
    uint32_t hidden[14];
#endif // NLER_FEATURE_SIMULATEABLE_TIME
#else  // DEBUG
    uint32_t hidden[12];
#endif // DEBUG
#else  // UINTPTR_MAX
    #error Unknown size of ptr
#endif // UINTPTR_MAX
} nl_event_timer_t;

/** Start system timer. This needs to be called at an appropriate time by the
 * application if the timer service is desired. Generally this is called after
 * nl_er_init() so that log messages are caught.
 *
 * @param[in] aPriority Task priority for the system timer. Applications can
 * set the priority to whatever is appropriate given the other tasks the
 * application controls.
 */
void nl_timer_start(nltask_priority_t aPriority);

/** Get the pointer to the timer event-queue. This is a convenience helper function
 * to insulate callers from having to locate the timer queue.
 *
 * @pre Timer has already been started with nl_timer_start()
 *
 * @return Timer event queue reperesenting the timer service.
 */
nleventqueue_t *nl_get_timer_queue(void);

#endif /* NLER_FEATURE_TIMER_USING_SWTIMER */

/** Initialize a timer event.  This function must be called prior to starting a timer.
 *
 * @param[in, out] aTimer the timer event to initialize
 *
 * @param[in] aHandler callback function to run for the timer event.
 *
 * @param[in] aHandlerArg argument to pass to callback function
 *
 * @param[in] aQueue eventqueue to post the event to when the timeout has expired
 */
void nl_event_timer_init(nl_event_timer_t *aTimer, nl_eventhandler_t aHandler, void *aHandlerArg, nleventqueue_t *aQueue);

/** Start or restart a timer.
 *
 * If the timer is not yet running, a timer event will be posted to the event queue
 * specified in nl_event_timer_init() at least aTimeoutMS after the current time
 * (the time will always be greater than or equal to aTimeoutMS due to rounding
 * and implementation resolution details).
 *
 * For non-swtimer implementation, a timer that is already running and a repeating
 * timer cannot be restarted using this API.  This is checked by a NLER_ASSERT.
 * For swtimer implementation, starting will implicitly call nl_event_timer_cancel.
 *
 * @param[in] aTimer the timer event to start.
 *
 * @param[in] aTimeoutMS Time in milliseconds from now at which the timeout
 * should occur.
 *
 * @param[in] aRepeating whether the timer is repeating or not.
 */
void nl_event_timer_start(nl_event_timer_t *aTimer, nl_time_ms_t aTimeoutMS, bool aRepeating);

/** Cancel a timer that was previously started.
 * The timer event will be marked as cancelled so any instance
 *  of it on an event queue will be considered invalid.
 *
 * @param[in] aTimer the timer event to cancel.
 */
void nl_event_timer_cancel(nl_event_timer_t *aTimer);

/** Returns whether a timer is valid or not.
 *
 * This function should be called in a task event handler
 * function when processing timer events to make sure
 * the timer is still valid.  Cancelled or restarted
 * timers always send an event to the queue and so
 * need to be checked before they are acted on.  This
 * function should be called exactly once per dequeued
 * timer event.
 *
 * @param[in] aTimer the timer event to check.
 *
 * @return bool if the timer event is valid.
 */
bool nl_event_timer_is_valid(nl_event_timer_t *aTimer);

#ifdef __cplusplus
}
#endif

/** @example test-timer.c
 * A task that uses the timer service to trigger actions.
 */
#endif /* NL_ER_EVENT_TIMER_H */
