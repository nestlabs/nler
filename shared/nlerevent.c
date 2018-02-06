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
 *      This file implements NLER build platform-independent event
 *      interfaces.
 *
 */
 
#include <stdint.h>
#include <stdlib.h>
#include "nlerevent.h"
#include <nlertimer.h>

int nl_dispatch_event(nl_event_t *aEvent, nl_eventhandler_t aDefaultHandler, void *aDefaultClosure)
{
    int retval;

#if NLER_FEATURE_EVENT_TIMER
    if ((aEvent->mType == NL_EVENT_T_TIMER) && (nl_event_timer_is_valid((nl_event_timer_t*)aEvent) == false))
    {
        retval = 0;
    }
    else
#endif /* NLER_FEATURE_EVENT_TIMER */
    {
        if (aEvent->mHandler != NULL)
        {
            retval = (aEvent->mHandler)(aEvent, aEvent->mHandlerClosure);
        }
        else
        {
            retval = (aDefaultHandler)(aEvent, aDefaultClosure);
        }
    }

    return retval;
}

