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
 *      This file implements NLER build platform-independent timer
 *      interfaces when the NLER simulateable time feature has been
 *      enabled.
 *
 */

#include "nler-config.h"

#include "nlererror.h"
#include "nlerlock.h"
#include "nlertimer.h"
#include "nlertimer_sim.h"
#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 1
#include "nlerlog.h"

#if NLER_BUILD_PLATFORM_NSPR
#include <nspr/prinrval.h>
#elif NLER_BUILD_PLATFORM_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

extern nl_time_native_t _nl_get_time_native(void);

#if NLER_FEATURE_SIMULATEABLE_TIME
static sim_time_info_t sSimTimeInfo = {.real_time_when_paused = 0,
                                       .real_time_when_started = 0,
                                       .advance_time_point = 0,
                                       .sim_time_delay = 0,
                                       .time_paused = false};

static nl_event_t *sAdvanceEventReturnQueueMem;
static nleventqueue_t sAdvanceQueue;
static nl_event_timer_t sAdvanceEvent;

nl_event_timer_t * nl_get_advance_event(void)
{
    return &sAdvanceEvent;
}

sim_time_info_t * nl_get_sim_time_info(void)
{
    return &sSimTimeInfo;
}

void nl_time_init_sim(bool pauseTime)
{
    sSimTimeInfo.real_time_when_started = _nl_get_time_native();

    if (pauseTime)
    {
        sSimTimeInfo.real_time_when_paused = sSimTimeInfo.real_time_when_started;
        sSimTimeInfo.time_paused = true;
    }

    nleventqueue_create(&sAdvanceEventReturnQueueMem,
                         sizeof(sAdvanceEventReturnQueueMem),
                         &sAdvanceQueue);
    // these events are never actually dispatched but just sent
    // back to us for synchronization, so no function or arg needed
    nl_event_timer_init(&sAdvanceEvent, NULL, NULL, &sAdvanceQueue);
}

void nl_pause_time(void)
{
    const nl_time_native_t now = _nl_get_time_native();

    if (!sSimTimeInfo.time_paused)
    {
        sSimTimeInfo.real_time_when_paused = now;
        sSimTimeInfo.time_paused = true;
    }
}

void nl_unpause_time(void)
{
    const nl_time_native_t now = _nl_get_time_native();

    if (sSimTimeInfo.time_paused)
    {
        sSimTimeInfo.sim_time_delay += (int32_t) (now - sSimTimeInfo.real_time_when_paused);
        sSimTimeInfo.time_paused = false;
    }
}

int nl_advance_time_ms(nl_time_ms_t aTime)
{
    int retval = NLER_ERROR_BAD_STATE;

    if (sSimTimeInfo.time_paused)
    {
        sSimTimeInfo.advance_time_point = nl_get_time_native() + nl_time_ms_to_time_native(aTime);

        nl_event_timer_start(&sAdvanceEvent, 0, false);
        nleventqueue_get_event(&sAdvanceQueue);
        retval = NLER_SUCCESS;
    }

    return retval;
}

bool nl_is_time_paused(void)
{
    return sSimTimeInfo.time_paused;
}

#endif

