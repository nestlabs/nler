/*
 *
 *    Copyright (c) 2014-2018 Nest Labs, Inc.
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
 *      This file implements NLER time under the FreeRTOS build
 *      platform.
 *
 */

#include "nler-config.h"

#include "nlertime.h"
#include "FreeRTOS.h"
#include "task.h"
#include "nlermathutil.h"

extern nl_time_native_t _nl_get_time_native(void);

nl_time_native_t nl_time_ms_to_time_native(nl_time_ms_t aTime)
{
    if (aTime == NLER_TIMEOUT_NEVER)
    {
        return portMAX_DELAY;
    }
    else if (aTime == 0)
    {
        return 0;
    }
    else
    {
        nl_time_native_t t = nl_udiv64_by_1000ULL( ((uint64_t)aTime) * configTICK_RATE_HZ );
        return (t != 0 ? t : 1);
    }
}

/* Convert delay_ms to ticks (rounding up).
 * Also add one tick to account for not knowing where in the tick
 * period we are.
 * E.g. if a tick period is 4 ms, and the request is for a 1ms
 * delay, but we don't know when that next tick is going to
 * arrive (could be anywhere from just over 0ms up to almost 4ms
 * from now), we have to schedule it at least 1 tick ahead, which
 * is 2 ticks.
 */
nl_time_native_t nl_time_ms_to_delay_time_native(nl_time_ms_t aTime)
{
    if (aTime == NLER_TIMEOUT_NEVER)
    {
        return portMAX_DELAY;
    }
    else if (aTime == 0)
    {
        return 0;
    }
    else
    {
        return nl_udiv64_by_1000ULL( ((uint64_t)aTime) * configTICK_RATE_HZ + 999) + 1;
    }
}

#if !((configTICK_RATE_HZ == 100) || (configTICK_RATE_HZ == 128) || (configTICK_RATE_HZ == 200) || (configTICK_RATE_HZ == 250) || (configTICK_RATE_HZ == 256) || (configTICK_RATE_HZ == 500) || (configTICK_RATE_HZ == 512) || (configTICK_RATE_HZ == 1024) || (configTICK_RATE_HZ == 2048) || (configTICK_RATE_HZ == 32768) || (configTICK_RATE_HZ == 1000))

static uint32_t nl_udiv64_by_clock_tick_rate(uint64_t inDividend)
{
    const uint32_t reciprocal = NL_MATH_UTILS_RECIPROCAL(configTICK_RATE_HZ);
    const uint32_t shiftedDivisor = NL_MATH_UTILS_SCALED_DIVISOR(configTICK_RATE_HZ);
    return nl_div_uint64_into_uint32_helper(
        NL_MATH_UTILS_SCALED_DIVIDEND(inDividend, configTICK_RATE_HZ),
        reciprocal,
        shiftedDivisor);
}

#endif

nl_time_ms_t nl_time_native_to_time_ms(nl_time_native_t aTime)
{
    if (aTime == portMAX_DELAY)
    {
        return NLER_TIMEOUT_NEVER;
    }
    else
    {
#if (configTICK_RATE_HZ == 128) || (configTICK_RATE_HZ == 256) || (configTICK_RATE_HZ == 512) || (configTICK_RATE_HZ == 1024) || (configTICK_RATE_HZ == 2048) || (configTICK_RATE_HZ == 32768)
        return (nl_time_ms_t) ((((uint64_t) aTime) * 1000) / configTICK_RATE_HZ);
#elif (configTICK_RATE_HZ == 100) || (configTICK_RATE_HZ == 200) || (configTICK_RATE_HZ == 250) || (configTICK_RATE_HZ == 500) || (configTICK_RATE_HZ == 1000)
        // For some common even divisors of 1000, let the preprocessor do the math with the constant configTICK_RATE_HZ
        return (nl_time_ms_t) aTime * (1000 / configTICK_RATE_HZ);
#else
        return (nl_time_ms_t) nl_udiv64_by_clock_tick_rate(((uint64_t) aTime) * 1000);
#endif
    }
}

nl_time_native_t _nl_get_time_native(void)
{
    return (xTaskGetTickCount());
}

nl_time_native_t nl_get_time_native_from_isr(void)
{
    return xTaskGetTickCountFromISR();
}

