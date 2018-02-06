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
 *      Timer events. The timer event serves as both a timeout request
 *      and a response. Send a timer event to the timer event queue
 *      and it will treat it as a timeout request. It is then a shared
 *      structure between the requester and the timer. The requester
 *      can ask for the event to be cancelled by setting a flag. The
 *      requester can repeatedly send the timer event to the timer
 *      event queue in which case any old active timeouts will be
 *      clobbered.
 *
 */

#ifndef NL_ER_TIMER_H
#define NL_ER_TIMER_H

#if NLER_FEATURE_EVENT_TIMER

#include "nlerevent_timer.h"

#else /* NLER_FEATURE_EVENT_TIMER */

#include "nlereventqueue.h"
#include "nlertime.h"
#include "nlertask.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Timer event. Should be initialized using nl_init_event_timer.
 */
typedef struct nl_event_timer_s
{
    NL_DECLARE_EVENT                    /**< Common event fields */
    nl_eventqueue_t     mReturnQueue;   /**< Queue of timer events */
    nl_time_ms_t        mTimeoutMS;     /**< Timeout in milliseconds */
    uint32_t            mFlags;         /**< Timer flags */
    nl_time_native_t    mTimeNow;       /**< For internal use by timer implementation */
    nl_time_native_t    mTimeoutNative; /**< For internal use by timer implementation */
} nl_event_timer_t;

/** Initialize a timer event
 */
#define NL_INIT_EVENT_TIMER(e, h, c, r)                           \
    NL_INIT_EVENT((e), NL_EVENT_T_TIMER, (h), (c))                \
    (e).mReturnQueue  = (r);                                      \
    (e).mTimeoutMS    = 0;                                        \
    (e).mFlags        = 0;                                        \
    (e).mTimeNow      = 0;                                        \
    (e).mTimeoutNative= 0

/** Static initializing of event timer
 *
 * Usage example:
 * nl_event_timer_t your_timer_name =
 * {
 *     NL_INIT_EVENT_TIMER_STATIC(your_timer_eventhandler)
 * };
 *
 */
#define NL_INIT_EVENT_TIMER_STATIC(timer_handler)            \
    NL_INIT_EVENT_STATIC(NL_EVENT_T_TIMER,timer_handler,NULL),\
    .mReturnQueue  = NULL,                                      \
    .mTimeoutMS    = 0,                                         \
    .mFlags        = 0,                                         \
    .mTimeNow      = 0,                                         \
    .mTimeoutNative= 0,

/** This timeout event has been cancelled. Set by timeout requester when the
 * timeout is no longer desired.  The timer may still however send the event
 * back in a timeout response if the timer was already in the process of timing
 * out when the timer was cancelled.
 */
#define NLER_TIMER_FLAG_CANCELLED   0x0001

/** Repeat timer flag. If set then the timer will continue to send the event
 * back to the sender each time the timer expires. Upon each expiration the
 * timer is reset.
 */
#define NLER_TIMER_FLAG_REPEAT      0x0002

/** Cancel timer flag. If set then request that this timeout be cancelled and
 * echo the event back once the cancel has been acknowledged. This is a closed
 * loop way of cancelling a timeout in the case that it is very important for
 * the user to track resources carefully. You are guaranteed to get the event
 * sent back to you unlike NLER_TIMER_FLAG_CANCELLED.
 */
#define NLER_TIMER_FLAG_CANCEL_ECHO 0x0004

#ifndef NLER_MAX_TIMER_EVENTS
/** Maximum number of simultaneous timer events. This can be increased/decreased
 * by changing this define through the build system.
 */
#define NLER_MAX_TIMER_EVENTS   4
#endif

/** Setting this flag will indicate that this timer should be tracked to be
 * used externally as a wakeup timer. The shortest wake interval of all timers
 * is tracked and returned by nl_get_wake_time().
 */
#if NLER_FEATURE_WAKE_TIMER
#define NLER_TIMER_FLAG_WAKE        0x0008
#endif // NLER_FEATURE_WAKE_TIMER

/** setting this flag will indicate that on cancel and re-arm the timer
 * event will always be echoed to the client queue.
 */
#define NLER_TIMER_FLAG_DISPLACE    0x0010

#define NLER_TIMER_FLAG_ANY_CANCEL  (NLER_TIMER_FLAG_CANCELLED | NLER_TIMER_FLAG_CANCEL_ECHO)

/** Initialize a timer event with current time values. This function must be
 * called immediately prior to posting a timer event to timer event queue. Time
 * countdown begins from the this function is called. If the timer event is not
 * posted to the timer event queue, it will not fire.
 *
 * See nl_eventqueue_post_event for more information.
 *
 * @param[in, out] aTimer the timer event to initialize
 *
 * @param[in] aTimeoutMS Time in milliseconds from now at which the timeout
 * should occur.
 */
void nl_init_event_timer(nl_event_timer_t *aTimer, nl_time_ms_t aTimeoutMS);

/** Submit the timer to the timer module for tracking. The timer can only
 * trigger after it has been submitted to the timer module.
 *
 * NOTE: While starting a timer is straightforward, stopping it isn't.  There
 * are a number of ways to achieve this, each tailored to the requirements of
 * the user. See pull request #6 in embedded-runtime for details.
 *
 * @param[in] aTimer the timer event to start.
 *
 * @return NLER_SUCCESS on success or error code. See nlererror.h.
 */
int nl_start_event_timer(nl_event_timer_t *aTimer);

/* returns the native time of the shortest timeout of
 * all pending timers that have NLER_TIMER_FLAG_WAKE set.
 * if no wake timers are enqueued, NLER_TIMEOUT_NEVER is
 * returned.
 */
nl_time_native_t nl_get_wake_time(void);

/** Start system timer. This needs to be called at an appropriate time by the
 * application if the timer service is desired. Generally this is called after
 * nl_er_init() so that log messages are caught.
 *
 * @param[in] aPriority Task priority for the system timer. Applications can
 * set the priority to whatever is appropriate given the other tasks the
 * application controls.
 *
 * @return Timer event queue representing the timer service. Timer events
 * are sent to this queue to be acted upon by the timer.
 */
nl_eventqueue_t nl_timer_start(nl_task_priority_t aPriority);

/** Get the pointer to the timer event-queue. This is a convenience helper function
 * to insulate callers from having to locate the timer queue.
 *
 * @pre Timer has already been started with nl_timer_start()
 *
 * @return Timer event queue reperesenting the timer service.
 */
nl_eventqueue_t nl_get_timer_queue(void);

#ifdef __cplusplus
}
#endif

/** @example test-timer.c
 * A task that uses the timer service to trigger actions.
 */
#endif /* NLER_FEATURE_EVENT_TIMER */
#endif /* NL_ER_TIMER_H */
