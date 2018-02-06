/*
 *
 *    Copyright (c) 2015-2018 Nest Labs, Inc.
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
 *
 *    @file
 *      Defines timers that can be cleanly cancelled or resent
 *      (refreshed) without race conditions.
 *
 * The resendable timer module provides timers that can be cleanly
 * cancelled or re-sent (refreshed) without race conditions.
 *
 * Usage:
 *
 *   Start timers using nl_resendable_timer_start().
 *
 *   Re-send a timer by starting it again with nl_resendable_timer_start().
 *   Re-sending a timer invalidates the expiration time from the
 *   previous start.
 *
 *   Cancel timers at any time using nl_resendable_timer_cancel().
 *
 *   Clients MUST call nl_resendable_timer_receive() once on EVERY re-sendable
 *   timer received. If the function returns NLER_SUCCESS, handle the timer as
 *   normal, else ignore it.
 *
 * Details:
 *
 *   Re-sendable timers operate by keeping track of how many times
 *   they are sent and received. This leads to a simple rule:
 *
 *     A client receives a timer every time it sends one.
 *
 *   Re-sending a timer causes the original expiration to become
 *   invalid, and causes the timer module to send a copy of the timer
 *   back to the sender.  This raises the question, "how does the
 *   client know to ignore the copy?".
 *
 *   Counting the number of times a timer is sent and received makes
 *   it possible to "ignore the copy", because there will be more
 *   sends than receives.
 *
 *   Clients shouldn't use the counter directly to determine whether
 *   to ignore a timer (as the implementation may change). Clients
 *   should use nl_resendable_timer_receive() or
 *   nl_resendable_timer_is_valid() instead.
 *
 *   If you send and receive a re-sendable timer you should use
 *   nl_resendable_timer_receive(). It updates the timer's counter and
 *   acts as the counterpart to nl_resendable_timer_start(). Call it
 *   once, and only once, on every re-sendable timer you receive. If
 *   it returns NLER_SUCCESS, handle the timer as normal. If it
 *   returns NLER_ERROR_FAILURE, ignore the timer.
 *
 *   If you interact with a re-sendable timer, but aren't the intended
 *   recipient, use nl_resendable_timer_is_valid(). This function
 *   doesn't update the timer's counter and is safe to call multiple
 *   times. If it returns true, deal with the timer as normal. If it
 *   returns false, ignore the timer.
 *
 *   @note
 *     Avoid using nl_resendable_timer_is_valid() when * possible. It
 *     may return different values between calls because * the client
 *     may re-send or cancel the timer at any time.
 *
 *   Most clients implement something like the following to handle
 *   re-sendable timer events (though likely without the else):
 *
 *   @code
 *       if (nl_resendable_timer_receive(aTimer) == NLER_SUCCESS)
 *       {
 *           // Timer expired successfully.
 *           // Do something with it.
 *       }
 *       else
 *       {
 *           // This is an invalid or cancelled timer.
 *       }
 *   @endcode
 *
 */

#ifndef NL_ER_UTILITIES_RESENDABLETIMER_H
#define NL_ER_UTILITIES_RESENDABLETIMER_H

#if NLER_FEATURE_EVENT_TIMER
    // This resendable timer implementation is incompatible with (but maybe
    // also unnecessary for) NLER_FEATURE_EVENT_TIMER.
#else

#include "nlertimer.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Timer event.
 */
typedef struct nl_resendable_timer_s
{
    nl_event_timer_t mEventTimer;
    uint32_t         mActiveTimers; /**< For internal use by resendable timer implementation */
} nl_resendable_timer_t;

/** Initialize a resendable timer
 */
#define NL_INIT_RESENDABLE_TIMER(t, h, c, r)              \
  do {                                                    \
    NL_INIT_EVENT_TIMER((t).mEventTimer, (h), (c), (r));  \
    (t).mActiveTimers = 0;                                \
  } while (0)

/** Static initializing of resendable timer
 *
 * Usage example:
 * nl_resendable_timer_t your_timer_name =
 * {
 *     NL_INIT_RESENDABLE_TIMER_STATIC(your_timer_eventhandler)
 * };
 *
 */
#define NL_INIT_RESENDABLE_TIMER_STATIC(timer_handler)    \
    {                                                     \
        NL_INIT_EVENT_TIMER_STATIC(timer_handler)         \
    },                                                    \
    .mActiveTimers = 0

/** Initialize a timer event with current time values
 * and submit the timer to the timer module queue for tracking
 *
 * @param aTimer the timer event to start.
 *
 * @param aTimeoutMS Time in milliseconds from now at which the timeout
 * should occur.
 *
 * @return NLER_SUCCESS on success or error code.
 */
int nl_resendable_timer_start(nl_resendable_timer_t *aTimer, nl_time_ms_t aTimeoutMS);

/** Cancel resendable timer with NLER_TIMER_FLAG_CANCEL_ECHO if still armed.
 * The copy (ECHO) will be sent to app queue
 *
 * @param aTimer the timer event to cancel.
 *
 * @return none
 */

void nl_resendable_timer_cancel(nl_resendable_timer_t *aTimer);

/** Update bookkeeping variables and determine whether the timer should be
 * ignored.
 *
 *   IMPORTANT: Clients must call this function once, and only once, *EVERY*
 *              time a resendable timer is received.
 *
 * @param aTimer the timer to consider.
 *
 * @return NLER_SUCCESS       if the timer expired successfully.
 *         NLER_ERROR_FAILURE if the timer should be ignored.
 */
int nl_resendable_timer_receive(nl_resendable_timer_t *aTimer);

/** Determine whether the timer should be ignored without updating bookkeeping
 * variables.
 *
 *   IMPORTANT: This is NOT an alternative to nl_resendable_timer_receive().
 *              This function is useful for inspecting a timer instance if
 *              you're NOT the client intended to nl_resendable_timer_receive()
 *              it.
 *
 *              The value returned by this function is only valid until the
 *              timer's intended client receives, re-sends, or cancels the
 *              timer.
 *
 * @param aTimer the timer to evaluate.
 *
 * @return true  if the timer expired successfully.
 *         false if the timer should be ignored.
 */
bool nl_resendable_timer_is_valid(nl_resendable_timer_t *aTimer);

#ifdef __cplusplus
}
#endif

/** @example timertest.c
 * A task that uses the timer service to trigger actions.
 */

#endif // NLER_FEATURE_EVENT_TIMER
#endif // NL_ER_UTILITIES_RESENDABLETIMER_H
