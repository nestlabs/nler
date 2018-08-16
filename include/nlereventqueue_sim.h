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
 *      Event queues simulation functionality.
 *
 */

#ifndef NL_ER_EVENT_QUEUE_SIM_H
#define NL_ER_EVENT_QUEUE_SIM_H

#include "nlereventqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

#if NLER_FEATURE_SIMULATEABLE_TIME

/** Outstanding event counter
 *
 * Used in simulation for determining whether all events have been processed. A
 * single static counter is incremented by all calls to post_event and
 * decremented by get_event.
 */
int32_t nleventqueue_sim_count(void);

/** Increment outstanding event counter.
 *
 */
void nleventqueue_sim_count_inc(void);

/** Decrement outstanding event counter.
 *
 */
void nleventqueue_sim_count_dec(void);

#endif

#ifdef __cplusplus
}
#endif

/** @example test-oneshot-timer.c
 * Example of using one shot timers.
 *
 * @example test-repeated-timer.c
 * Example of using repeated timers.
 */
#endif /* NL_ER_EVENT_QUEUE_SIM_H */
