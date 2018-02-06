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
 *      Implements timers that can be cleanly cancelled or resent
 *      (refreshed) without race conditions.
 *
 */

#ifdef NLER_FEATURE_EVENT_TIMER
    // This resendable timer implementation is incompatible with (but maybe
    // also unnecessary for) NLER_FEATURE_EVENT_TIMER.
#else

#include "nlerassert.h"
#include "nlercfg.h"
#include "nlererror.h"
#include "nlerlock.h"
#include "nlerlog.h"
#include "nlermacros.h"
#include "nlresendabletimer.h"

static nl_lock_t sLock;

static void lock_enter(void)
{
    if (!sLock)
    {
        sLock = nl_er_lock_create();
    }

    nl_er_lock_enter(sLock);
}

static void lock_exit(void)
{
    nl_er_lock_exit(sLock);
}

/**
 * This function reads values from aTimer, and must be called between
 * lock_enter() and lock_exit().
 */
static bool is_valid(nl_resendable_timer_t *aTimer)
{
  bool cancelled;
  bool active;

  cancelled = aTimer->mEventTimer.mFlags & NLER_TIMER_FLAG_ANY_CANCEL;
  active = false;

  if ((aTimer->mActiveTimers == 1) && !cancelled)
  {
      active = true;
  }

  return active;
}

int nl_resendable_timer_start(nl_resendable_timer_t *aTimer, nl_time_ms_t aTimeoutMS)
{
    int retval;

    lock_enter();

    /* to keep precise records of every armed, canceled or re-armed timers the
     * combination of flags (NLER_TIMER_FLAG_REPEAT | NLER_TIMER_FLAG_DISPLACE)
     * is not allowed. The client app is responsible for re-arming the timer
     * when needed every time the previously armed timer expired or canceled
     */
    aTimer->mEventTimer.mFlags &= ~NLER_TIMER_FLAG_REPEAT;
    aTimer->mEventTimer.mFlags |= NLER_TIMER_FLAG_DISPLACE;

    aTimer->mActiveTimers++;

    nl_init_event_timer(&aTimer->mEventTimer, aTimeoutMS);
    retval = nl_start_event_timer(&aTimer->mEventTimer);

    if (retval != NLER_SUCCESS)
    {
        aTimer->mActiveTimers--;
    }

    lock_exit();

    return retval;
}

void nl_resendable_timer_cancel(nl_resendable_timer_t *aTimer)
{
    lock_enter();

    if (aTimer->mActiveTimers > 0)
    {
        aTimer->mEventTimer.mFlags |= NLER_TIMER_FLAG_CANCEL_ECHO;
    }

    lock_exit();
}

bool nl_resendable_timer_is_valid(nl_resendable_timer_t *aTimer)
{
    bool retval;

    lock_enter();

    retval = is_valid(aTimer);

    lock_exit();

    return retval;
}

int nl_resendable_timer_receive(nl_resendable_timer_t *aTimer)
{
    int  retval;

    lock_enter();

    /* combination of flags (NLER_TIMER_FLAG_REPEAT | NLER_TIMER_FLAG_DISPLACE)
     * is not allowed - see description in nl_resendable_timer_start()
     */
    NLER_ASSERT(!(aTimer->mEventTimer.mFlags & NLER_TIMER_FLAG_REPEAT));

    retval = NLER_ERROR_FAILURE;

    if (is_valid(aTimer))
    {
        retval = NLER_SUCCESS;
    }

    if (aTimer->mActiveTimers > 0)
    {
        aTimer->mActiveTimers--;
    }

    lock_exit();

    return retval;
}

#endif // NLER_FEATURE_EVENT_TIMER
