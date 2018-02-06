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
 *      This file implements NLER time under the Netscape Portable
 *      Runtime (NSPR) build platform.
 *
 */

#include "nlertime.h"
#include <nspr/prinrval.h>

extern nl_time_native_t _nl_get_time_native(void);

nl_time_native_t nl_time_ms_to_time_native(nl_time_ms_t aTime)
{
    if (aTime == NLER_TIMEOUT_NEVER)
        return PR_INTERVAL_NO_TIMEOUT;
    else
        return PR_MillisecondsToInterval(aTime);
}

nl_time_native_t nl_time_ms_to_delay_time_native(nl_time_ms_t aTime)
{
    return nl_time_ms_to_time_native(aTime);
}

nl_time_ms_t nl_time_native_to_time_ms(nl_time_native_t aTime)
{
    if (aTime == PR_INTERVAL_NO_TIMEOUT)
        return NLER_TIMEOUT_NEVER;
    else
        return PR_IntervalToMilliseconds(aTime);
}

nl_time_native_t _nl_get_time_native(void)
{
    return (PR_IntervalNow());
}


