/*
 *
 *    Copyright (c) 2015-2016 Nest Labs, Inc.
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
 *     Configuration values. This file provides the default values for
 *     various NLER configuration values.
 *
 *     NLER may be specialized to specific platforms by overriding the
 *     values in a platform specific configuration file.
 *
 */

#ifndef NL_ER_CFG_H
#define NL_ER_CFG_H

#ifdef NLER_HAS_CONFIGURATION_FILE
#include <nlercfgproject.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Additional stack space (beyond NLER_TASK_STACK_BASE) for timer task.
 */
#ifndef NLER_TIMER_STACK_SIZE
#define NLER_TIMER_STACK_SIZE 1024
#endif

/**
 * If platforms have not defined optional assert delegate, just trap/fault
 */
#ifndef NLER_PLATFORM_ASSERT_DELEGATE
#define NLER_PLATFORM_ASSERT_DELEGATE(...) __builtin_trap()
#endif

/**
 * For debugging purposes it may be useful to assert when trying to
 * post an event to a full queue.
 */
#ifndef NLER_ASSERT_ON_FULL_QUEUE
#define NLER_ASSERT_ON_FULL_QUEUE 0
#endif

#ifdef __cplusplus
}
#endif

#endif // NL_ER_CFG_H
