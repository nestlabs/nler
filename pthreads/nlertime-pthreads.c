/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *      This file implements NLER time under the POSIX threads
 *      (pthreads) build platform.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <sys/time.h>

#include <nlertime.h>

#if HAVE_CLOCK_GETTIME
#if HAVE_DECL_CLOCK_BOOTTIME
// CLOCK_BOOTTIME is a Linux-specific option to clock_gettime for a clock which compensates for system sleep.
#define MONOTONIC_CLOCK_ID CLOCK_BOOTTIME
#define MONOTONIC_RAW_CLOCK_ID CLOCK_MONOTONIC_RAW
#else // HAVE_DECL_CLOCK_BOOTTIME
// CLOCK_MONOTONIC is defined in POSIX and hence is the default choice
#define MONOTONIC_CLOCK_ID CLOCK_MONOTONIC
#endif
#endif // HAVE_CLOCK_GETTIME

extern nl_time_native_t _nl_get_time_native(void);

nl_time_native_t nl_time_ms_to_time_native(nl_time_ms_t aTime)
{
    nl_time_native_t retval;

    if (aTime == NLER_TIMEOUT_NEVER)
        retval = UINT32_MAX;
    else
        retval = aTime;

    return retval;
}

nl_time_native_t nl_time_ms_to_delay_time_native(nl_time_ms_t aTime)
{
   return nl_time_ms_to_time_native(aTime);
}

nl_time_ms_t nl_time_native_to_time_ms(nl_time_native_t aTime)
{
    nl_time_native_t retval;

    if (aTime == UINT32_MAX)
        retval = NLER_TIMEOUT_NEVER;
    else
        retval = aTime;

    return retval;
}

nl_time_native_t _nl_get_time_native(void)
{
    nl_time_native_t retval;
    int              status;

#if HAVE_CLOCK_GETTIME
    struct timespec ts;

    status = clock_gettime(MONOTONIC_CLOCK_ID, &ts);
    if (status != 0)
    {
        retval = 0;
        goto done;
    }

    retval = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#else // HAVE_CLOCK_GETTIME
    struct timeval tv;

    status = gettimeofday(&tv, NULL);
    if (status != 0)
    {
        retval = 0;
        goto done;
    }

    retval = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#endif // HAVE_CLOCK_GETTIME

 done:
    return (retval);
}


