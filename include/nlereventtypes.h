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
 *      Event types.
 *
 */

#ifndef NL_ER_EVENT_TYPES_H
#define NL_ER_EVENT_TYPES_H

/** All events need to be defined in module specific event type files.  Note
 * that it is ok to block out a range of events for your module so that you
 * don't need to define all events as part of the enum. To do so, define @c
 * NLER_APPLICATION_EVENTS_FILE to be the filename of a file which contains the
 * enum members you would like to add. This file will be included into the enum
 * definition. For example, you can use a file with the following two lines:
 *
 * NL_EVENT_T_MY_MODULE_EVENT_FIRST,
 *
 * NL_EVENT_T_MY_MODULE_EVENT_LAST = NL_EVENT_T_MY_MODULE_EVENT_FIRST + 999,
 *
 * doing so will carve out 1000 events for your module. These events, should
 * you define them, will be placed after @c NL_EVENT_T_WM_USER_LAST.
 */
typedef enum
{

    NL_EVENT_T_RUNTIME = 0,     /**< Runtime event */
    NL_EVENT_T_TIMER,           /**< Timer event */
    NL_EVENT_T_EXIT,            /**< Exit event */
    NL_EVENT_T_POOLED,          /**< Pooled event */

    /** First user defined event. The purpose of this sort of event is to allow
     * for quick and dirty definitions of private events that other modules
     * don't know about and that don't require participating in the application
     * wide event numbering system. If you use these events, it is *entirely
     * up to you* to make sure that there is no conflict with other modules
     * that may use them (i.e. keep them to yourself).
     */
    NL_EVENT_T_WM_USER,
    NL_EVENT_T_WM_USER_LAST = 999, /**< Last user defined event */

#ifdef NLER_APPLICATION_EVENTS_FILE
#include "nlappevents-all.h"
#endif

    NL_EVENT_T_LAST_DEFINED_EVENT   /**< Place holder event */
} nl_event_type_t;

/** Event type range check. Use this if you are managing event type enumeration
 * in your own module within a range to ensure that you never exceeed the range
 * that you have defined.
 */
#ifdef DEBUG
#define NL_RANGE_CHECK_EVENT_TYPE(et, rs, re)                                                  \
    NLER_BEGIN_MACRO                                                                           \
        if (((et) < (rs)) || ((et) > (re)))                                                   \
        {                                                                                      \
            NL_LOG(lrER, "event exceeds allowable range file: %s, function: %s, line: %d\n",   \
                   __FILE__, __FUNCTION__, __LINE__);                                          \
            *((volatile uint8_t *)0) = 0;                                                               \
        }                                                                                      \
    NLER_END_MACRO
#else
#define NL_RANGE_CHECK_EVENT_TYPE(et, rs, re) ((void)0)
#endif

#endif /* NL_ER_EVENT_TYPES_H */
