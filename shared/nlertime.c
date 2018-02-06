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
 *    @file
 *      This file implements NLER build platform-independent time
 *      interfaces.
 *
 */

#include "nler-config.h"

#include "nlertime.h"

extern nl_time_native_t _nl_get_time_native(void);

#if NLER_FEATURE_SIMULATEABLE_TIME
#include "nlertimer_sim.h"
#endif

nl_time_native_t nl_get_time_native(void)
{
    nl_time_native_t time = _nl_get_time_native();

#if NLER_FEATURE_SIMULATEABLE_TIME
    sim_time_info_t * SimTimeInfo = nl_get_sim_time_info();
    if (SimTimeInfo->time_paused)
    {
        time = SimTimeInfo->real_time_when_paused;
    }
    time -= (SimTimeInfo->sim_time_delay + SimTimeInfo->real_time_when_started);
#endif

    return time;
}
