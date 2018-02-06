/*
 *
 *    Copyright (c) 2015-2016 Nest Labs, Inc.
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

#ifndef NL_ER_TIMER_SIM_H
#define NL_ER_TIMER_SIM_H

#include <stdbool.h>
#include "nlertimer.h"
#include "nlerlock.h"

#ifdef __cplusplus
extern "C" {
#endif

#if NLER_FEATURE_SIMULATEABLE_TIME
/** Simulated time data.
 */
typedef struct sim_time_info_s {
    nl_time_native_t real_time_when_paused;     /**< most recent pause time */
    nl_time_native_t real_time_when_started;    /**< time when sim_time_init was called */
    nl_time_native_t advance_time_point;        /**< native time to advance to */
    int32_t sim_time_delay;                     /**< positive indicates sim time lags real time */
    bool time_paused;                           /**< track whether system time is paused or not */
} sim_time_info_t;

/** Initialize simulation time.
 *
 * @param[in] pauseTime Booling to control whether time starts paused or not
 */
void nl_time_init_sim(bool pauseTime);

/** Pause time.
 *
 * @pre System timer has been started with nl_timer_start()
 */
void nl_pause_time(void);

/** Unpause time.
 *
 * @pre System timer has been started with nl_timer_start()
 */
void nl_unpause_time(void);

/** Advance time. If time is not paused, this function does nothing.
 *
 * @pre Time is paused.
 *
 * @pre System timer has been started with nl_timer_start()
 *
 * @post Time is paused, all events during the advancement interval have been
 * processed.
 *
 * @param aTime Time in milliseconds to advance. This is some unsigned integer
 * type, if you pass a negative signed integer it will be wrapped to a positive
 * unsigned integer and further than you intended.
 */
int nl_advance_time_ms(nl_time_ms_t aTime);

/** Determine whether time is paused.
 *
 * @return true if time is paused, false otherwise.
 */
bool nl_is_time_paused(void);

/** Get advance event.
 *
 * The advance event is a special event that can be posted to the system timer
 * to cause the system to advance. Prior to posting this event, the
 */
nl_event_timer_t * nl_get_advance_event(void);

/** Get simulation time information.
 */
sim_time_info_t * nl_get_sim_time_info(void);

/** Get simulation time information lock
 */
// nl_lock_t nl_sim_time_get_lock();

#endif /* NLER_FEATURE_SIMULATEABLE_TIME */

#ifdef __cplusplus
}
#endif

/** @example test-oneshot-timer.c
 *  @example test-repeated-timer.c
 *
 */
#endif /* NL_ER_TIMER_SIM_H */
