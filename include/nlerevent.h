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
 *      Events.
 *
 */

#ifndef NL_ER_EVENT_H
#define NL_ER_EVENT_H

#include <stdint.h>
#include "nlereventtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Event fields convenience macro.
 */
#define NL_DECLARE_EVENT                    \
    nl_event_type_t     mType;              \
    nl_eventhandler_t   mHandler;           \
    void                *mHandlerClosure;

/** Initialize an event
 */
#define NL_INIT_EVENT(e, t, h, c)           \
    (e).mType = (t);                        \
    (e).mHandler = (h);                     \
    (e).mHandlerClosure = (c);

/** Statically initialize event fields
 */
#define NL_INIT_EVENT_STATIC(t, h, c)       \
    (t),                                    \
    (h),                                    \
    (c)

/** @cond */
typedef struct nl_event_s nl_event_t;
/** @endcond */

/** Event handler function pointer.
 *
 * @param[in] aEvent Event to pass to event handler.
 *
 * @param[in] aClosure Data to pass to the event handler.
 *
 * @return an error code. See nlererror.h.
 */
typedef int (*nl_eventhandler_t)(nl_event_t *aEvent, void *aClosure);

/** Events are structs with a minimal, common set of fields at the beginning.
 * To create custom events, declare a struct whose first field is @c
 * NL_DECLARE_EVENT and add custom event data after the. In all events
 * initialize the fields where appropriate. A partial alternative would be to
 * include the nl_event_t as the first element in all subclassed events. This
 * approach has the negative impact of only dealing with the declaration and
 * requires all code that references the common sub-fields within the
 * sub-classed event go through a name for the common part (e.g.
 * event.common.mType). If you are going to cheat, you may as well cheat all
 * the way.n. */
struct nl_event_s
{
    NL_DECLARE_EVENT
};

/** Dispatch an event. Given an event, call it's mHandler with the
 * mHandlerClosure or call the supplied default handler (and with default
 * closure) in the case that no handler was supplied in the event.  Note that
 * this is only a helper function and is not appropriate in all situations. Use
 * with care.
 *
 * @param[in] aEvent Event to dispatch. If aEvent's handler is non-NULL it will
 *                   be used to handle the event.
 *
 * @param[in] aDefaultHandler Event handler to use in case aEvent->mHandler is
 *                            NULL. *Must not be NULL*.
 *
 * @param[in] aDefaultClosure Data to pass to appropriate handler.
 *
 * @return result of calling event handler
 */
int nl_dispatch_event(nl_event_t *aEvent, nl_eventhandler_t aDefaultHandler, void *aDefaultClosure);

#define NLER_EVENT_IGNORED      1 /**< Event ignored return value */
#define NLER_EVENT_SHIFT_FOCUS  2 /**< Event shift focus return value */
#define NLER_EVENT_REBOOT       3 /**< Event reboot return value */
#define NLER_EVENT_RESTART      4 /**< Event restart return value */

#ifdef __cplusplus
}
#endif

/** @example test-event.c
 * Two tasks that send events to each other.
 */
#endif /* NL_ER_EVENT_POOLED_H */
