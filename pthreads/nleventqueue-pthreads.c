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
 *      This file implements NLER event queues under the POSIX threads
 *      (pthreads) build platform.
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <nlereventqueue.h>
#include <nlerlog.h>
#include <nlererror.h>

#include <pthread.h>

#if NLER_FEATURE_SIMULATEABLE_TIME
#include "nlereventqueue_sim.h"
#endif

nl_eventqueue_t nl_eventqueue_create(void *aQueueMemory, size_t aQueueMemorySize)
{
    void *retval = NULL;

    return (nl_eventqueue_t)retval;
}

void nl_eventqueue_destroy(nl_eventqueue_t aEventQueue)
{

}

int nl_eventqueue_post_event(nl_eventqueue_t aEventQueue, const nl_event_t *aEvent)
{
    int                     retval = NLER_SUCCESS;

    return retval;
}

nl_event_t *nl_eventqueue_get_event_with_timeout(nl_eventqueue_t aEventQueue, nl_time_ms_t aTimeoutMS)
{
    nl_event_t              *retval = NULL;

    return retval;
}

