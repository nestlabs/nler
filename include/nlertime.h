/*
 *
 *    Copyright (c) 2014-2016 Nest Labs, Inc.
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
 *      Time.
 *
 */

#ifndef NL_ER_TIME_H
#define NL_ER_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Represents a timeout that will never occur
 */
#define NLER_TIMEOUT_NEVER  (nl_time_ms_t)-1

/** Represents a timeout that will happen immediately
 */
#define NLER_TIMEOUT_NOW    0

/** Time interval defined by the underlying runtime implementation.
 * Applications should not attempt to interpret these values but they can be
 * converted to and from milliseconds.
 */
typedef uint32_t nl_time_native_t;

/** Time interval specified in milliseconds.
 */
typedef uint32_t nl_time_ms_t;

/** Get current system time in native time units. One can expect this clock to
 * wrap around at any time. All math done on time values must take this into
 * account.
 *
 * @return the current system clock time in native time units
 */
nl_time_native_t nl_get_time_native(void);

/** Get current system time in native time units. One can expect this clock to
 * wrap around at any time. All math done on time values must take this into
 * account. This may be called in isr context.
 *
 * @return the current system clock time in native time units
 */
#if defined(NLER_BUILD_PLATFORM_FREERTOS)
nl_time_native_t nl_get_time_native_from_isr(void);
#endif

/** Convert time in milliseconds to native time units.
 *
 *  Rounding behavior: For number of milliseconds shorter than 1 tick,
 *  the function will return 1 tick.  NLER_TIMEOUT_NEVER milliseconds
 *  will be mapped onto appropriate internal value corresponding to
 *  TIMEOUT_NEVER.  No overflow checking is performed: input
 *  millisecond values that would result in more than UINT32_MAX
 *  ticks, will produce erroneous results.  For all other input
 *  values, the function rounds down.
 *
 * @param[in] aTime Time in milliseconds.
 *
 * @return Time in native units.
 */
nl_time_native_t nl_time_ms_to_time_native(nl_time_ms_t aTime);

/** Convert time in milliseconds to a delay value in native time units.
 *
 *  Rounding behavior: The output is a value used for native delay APIs
 *  and is rounded up to the nearest native unit.  Also, if the native
 *  unit is in ticks, an additional tick is added to account for the
 *  uncertainty of when in the current tick period the delay is being
 *  requested.  The result is a guarantee that the delay will always be
 *  at least as long as the time in milliseconds and never early.
 *  NLER_TIMEOUT_NEVER milliseconds will be mapped onto appropriate
 *  internal value corresponding to TIMEOUT_NEVER.  0 milliseconds will
 *  be mapped onto appropriate internal value corresponding to no delay
 *  (e.g. non-blocking polling check only).  No overflow checking
 *  is performed: input millisecond values that would result in more than
 *  UINT32_MAX ticks, will produce erroneous results.
 *
 * @param[in] aTime Time in milliseconds.
 *
 * @return Time in native units that guarantees a delay that is at
 *         least aTime milliseconds in duration.
 */
nl_time_native_t nl_time_ms_to_delay_time_native(nl_time_ms_t aTime);

/** Convert time in native time units to time in milliseconds.
 *
 *  Rounding behavior: For input value corresponding to native
 *  TIMEOUT_NEVER, the function will result NLER_TIMEOUT_NEVER.  No
 *  overflow checking is performed: input ticks that would result in
 *  more than UINT32_MAX milliseconds will produce erroneous results.
 *  For all other input values, the function rounds down.
 *
 * @param[in] aTime Time in native units.
 *
 * @return Time in milliseconds.
 */
nl_time_ms_t nl_time_native_to_time_ms(nl_time_native_t aTime);

#ifdef __cplusplus
}
#endif

#endif /* NL_ER_EVENT_QUEUE_SIM_H */
