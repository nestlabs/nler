/*
 *
 *    Copyright (c) 2016 Nest Labs, Inc.
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
 *      This file defines the list of logging events used by
 *      NLER flow tracer.
 */

#ifndef _FLOWTRACER_ENUM_H_INCLUDED__
#define _FLOWTRACER_ENUM_H_INCLUDED__

// Please add new types to the end of enum.
typedef enum {
    RX_EVENT, // Used to signal a packet was received.
    TX_EVENT  // Used to signal a packet was transmitted.
} nl_trace_event_t;

#endif // _FLOWTRACER_ENUM_H_INCLUDED__
