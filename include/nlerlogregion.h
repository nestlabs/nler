/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      Log region. Log regions are used to selectively turn off logging from
 *      subsystems at runtime.
 *
 *      They must all be defined in one place as a continuous sequence
 *      of numbers from 0 to lrERMAX. A byte is assigned to each log
 *      region in a memory array used to dynamically adjust the
 *      runtime log level for each region.
 *
 */

#ifndef NL_ER_LOG_REGION_H
#define NL_ER_LOG_REGION_H

#include <nlermacros.h>
#include <nlercfg.h>

/** Log region. The runtime log region table is broken into two segments
 *
 *   - Built in NLER log regions
 *   - Application defined log regions
 *
 * The log writer looks for a symbol called @c gAppLogRegions. If the value at
 * that symbol is non-NULL it is expected to reference a byte array of size
 * @c lrERMAX elements. Each element defines the log level for that region (i.e.,
 * @c log_region_t is used as the index into the @c gAppLogRegions array).
 *
 * Application log regions are added by defining @c
 * NLER_APPLICATION_LOG_REGION_FILE to be the name of a file containing any log
 * regions that are desired. Application log region values exist between @c
 * lrERLAST and @c lrERMAX, non-inclusive.
 */
typedef enum
{
    lrER        = 0, /**< For nler internal use */
    lrERTASK,        /**< Task log region */
    lrEREVENT,       /**< Event log region */
    lrERINIT,        /**< Initialization log region */
    lrERQUEUE,       /**< Queue region */
    lrERTIMER,       /**< Timer log region */
    lrERPOOLED,      /**< Pooled events log region */
    lrERLAST,        /**< Last log region */
#ifdef NLER_APPLICATION_LOG_REGION_FILE
#include NLER_APPLICATION_LOG_REGION_FILE
#endif
    lrERMAX          /**< Placeholder log region */
} nl_log_region_t;

#endif /* NL_ER_LOG_REGION_H */
