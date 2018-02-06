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
 *      Log manager. The log manager is used to set a function through
 *      which all requests to log will flow. Each runtime
 *      implementation will have it's own notion of a default logging
 *      function, but until the runtime is initialized all log
 *      messages are guaranteed to be discarded. It is up to the user
 *      of the runtime (and NOT the runtime implementation itself) to
 *      perform any necessary guarding against multiple tasks logging
 *      simultaneously and interfering with each other.
 *
 */

#ifndef NL_ER_LOG_MANAGER_H
#define NL_ER_LOG_MANAGER_H

#include <stdarg.h>

#if NLER_FEATURE_LOG_TOKENIZATION
#include "nlerlogtoken.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Log printer.
 *
 * @param[in] aClosure Pointer to data to be used by logger
 * @param[in] aRegion Log region
 * @param[in] aPriority Log priority
 * @param[in] aFormat printf() style format string
 * @param[in] aArgs printf() style variable arguments
 */
typedef void (*nl_log_printer_t)(void *aClosure, nl_log_region_t aRegion, int aPriority, const char *aFormat, va_list aArgs);

/** Tokenized log printer.
 *
 * @param[in] aClosure Pointer to data to be used by logger
 * @param[in] aRegion Log region
 * @param[in] aPriority Log priority
 * @param[in] aFormat Token table entry
 * @param[in] aArgs printf() style variable arguments
 */
typedef void (*nl_log_token_printer_t)(void *aClosure, nl_log_region_t aRegion, int aPriority, const nl_log_token_entry_t *aFormat, va_list aArgs);

/** Set the function to be called when a log message needs to be emitted.
 *
 * @param[in] aPrinter Log printer to call
 * @param[in] aClosure Pointer to data to be used by logger when it is called
 * logging function with each message
 */
void nl_set_logging_function(nl_log_printer_t aPrinter, void *aClosure);

/** Set the function to be called when a tokenized log message needs to be emitted.
 *
 * @param[in] aPrinter Log printer to call
 * @param[in] aClosure Pointer to data to be used by logger when it is called
 * logging function with each message
 */
void nl_set_token_logging_function(nl_log_token_printer_t aPrinter, void *aClosure);

#ifdef __cplusplus
}
#endif

#endif /* NL_ER_LOG_MANAGER_H */
