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
 *     Logs.
 *
 *     Log macros can all result in log lines that are compiled out
 *     based on the build time log level. Please use them and think
 *     carefully about the priority you assign to your logs lines.
 *
 *     @c nlLOG_PRIORITY is the gate used to compile lines out. By
 *     altering this value in individual source files before including
 *     this file it is possible to increase or decrease logging for
 *     different modules regardless of the level used across the rest
 *     of a build.
 *
 *     The log levels defined here are used to alter the compile time
 *     level of logging built in and control which messages are "live"
 *     at the time that they are asked to be emitted.
 *
 *     Also see nlerlogregion.h.
 *
 *     @b Do not include this header file anywhere except directly in
 *     each .c file!
 */

#ifndef NL_ER_LOG_H
#define NL_ER_LOG_H

#include <stdarg.h>
#include "nlerlogregion.h"
#include "nlerlogtoken.h"
#include "nlermacros.h"
#include "nlertime.h"
#include <nlcompiler.h>

#ifdef __cplusplus
extern "C" {
#endif

#define nlLPDEBG    3   /**< Debug log level */
#define nlLPWARN    2   /**< Warn log level */
#define nlLPCRIT    1   /**< Critical log level */
#define nlLPNONE    0   /**< No log level */

/** Emit a message into a log.
 *
 * @param[in] aRegion Log region of message
 *
 * @param[in] aFormat and the remaining arguments are the actual format
 * and associated variable argument list.
 *
 * This function is intended for use by functions which accept variable
 * arguments directly, like nl_log().  Other modules may directly access
 * this function via NL_LOG_VA_LIST macro.
 */
void nl_log_va_list(nl_log_region_t aRegion, const char *aFormat, va_list aArgList);

/** Emit a message into a log.
 *
 * @param[in] aRegion Log region of message
 *
 * @param[in] aFormat and the remaining arguments are the actual format
 * string and associated variable arguments.
 */
void nl_log(nl_log_region_t aRegion, const char *aFormat, ...);

/** Emit a tokenized message into a log.
 *
 * @param[in] aRegion Log region of message
 *
 * @param[in] aFormat and the remaining arguments are the actual format
 * string and associated variable arguments.
 */
void nl_log_token(nl_log_region_t aRegion, const nl_log_token_entry_t *aFormat, ...);

/** Update the logging priority logging for a given region.
 *
 * @param[in]  aRegion Region whose logging level should be altered.
 *
 * @param[in] aPri the new priority level
 */
void nl_set_log_priority(nl_log_region_t aRegion, int aPri);

/** Retrieve the current logging priority for a given log region.
 *
 * @param[in] aRegion region to get the log priority for.
 *
 * @return the log priority for the region
 */
int nl_get_log_priority(nl_log_region_t aRegion);

#if NLER_FEATURE_LOG_TOKENIZATION

/** Marshal format arguments, encode in base64, and send over serial port.
 *
 * This function handles some of details of sending a tokenized log out
 * over the serial port.
 *
 * @param[in] aOutputCharFunc Function to put a single character on UART
 * @param[in] aTimeMs Current time (in milliseconds)
 * @param[in] aFormat Log table entry for this log
 * @param[in] ap Variadic parameter list with arguments for log
 */
void nl_log_send_tokenized(void (*aOutputCharFunc)(uint8_t c),
                           nl_time_ms_t aTimeMs,
                           nl_log_utc_ms_t aUtcTimeMs,
                           const nl_log_token_entry_t *aFormat,
                           va_list ap);

/*
 *  Macros to create per-log entries in the various tokenization related
 *  tables.
 *
 *  Notes:
 *  - Cannot do structure initialization because comma interpreted by
 *      preprocessor as demarcating a new argument.
 *  - GCC assigns different linkages to variables based on where definition
 *      occurs.  Hence, every variable needs to be in a separate section.
 */
#define NL_LOG_MERGE(a, b) NLER_CONCAT(a, b)
#define NL_LOG_IDENTIFIER(a, u) NL_LOG_MERGE(NL_LOG_MERGE(a, __LINE__), NL_LOG_MERGE(_, u))
#define NL_LOG_SECTION(a, u) NLER_STRINGIFY(NL_LOG_IDENTIFIER(a, u))

#if (defined(NL_COMPILER_GCC_ELF) || defined(NL_COMPILER_GCC_LINUX) || defined(NL_COMPILER_GCC_MACHO)) && NL_COMPILER_VERSION_MAJOR >= 6
#define NL_LOG_CREATE_ENTRY(unique, reg, format_str)                         \
    static const char NL_LOG_IDENTIFIER(_format_str_, unique)[]               \
        __attribute__((section(NL_LOG_SECTION(.log_strings., unique))))       \
        = format_str;                                                        \
    static const uint32_t NL_LOG_IDENTIFIER(_token_region_, unique)           \
        __attribute__((section(NL_LOG_SECTION(.token_region_table., unique))))\
        __attribute__((__used__))                                            \
        = reg;                                                               \
    static const uint32_t NL_LOG_IDENTIFIER(_local_token_, unique)            \
        __attribute__((section(NL_LOG_SECTION(.token_region_table., unique))))\
        __attribute__((__used__))                                            \
        = (uint32_t)NL_LOG_IDENTIFIER(_format_str_, unique);                  \
    static const nl_log_token_entry_t NL_LOG_IDENTIFIER(_entry_, unique)     \
        __attribute__((section(NL_LOG_SECTION(.log_table., unique))))         \
        = {(uint32_t)NL_LOG_IDENTIFIER(_format_str_, unique)};
#else
#define NL_LOG_CREATE_ENTRY(unique, reg, format_str)                         \
    static const char NL_LOG_IDENTIFIER(_format_str_, unique)[]               \
        __attribute__((section(NL_LOG_SECTION(.log_strings., unique))))       \
        = format_str;                                                        \
    static const uint32_t NL_LOG_IDENTIFIER(_local_token_, unique)            \
        __attribute__((section(NL_LOG_SECTION(.token_region_table., unique))))\
        __attribute__((__used__))                                            \
        = (uint32_t)NL_LOG_IDENTIFIER(_format_str_, unique);                  \
    static const uint32_t NL_LOG_IDENTIFIER(_token_region_, unique)           \
        __attribute__((section(NL_LOG_SECTION(.token_region_table., unique))))\
        __attribute__((__used__))                                            \
        = reg;                                                               \
    static const nl_log_token_entry_t NL_LOG_IDENTIFIER(_entry_, unique)     \
        __attribute__((section(NL_LOG_SECTION(.log_table., unique))))         \
        = {(uint32_t)NL_LOG_IDENTIFIER(_format_str_, unique)};
#endif

#define NL_LOG_IMPL_HELPER(unique, reg, fmt, aArgs...)                       \
    NL_LOG_CREATE_ENTRY(unique, reg, fmt);                                   \
    nl_log_token(reg, &NL_LOG_IDENTIFIER(_entry_, unique), ## aArgs);
#define NL_LOG_IMPL(reg, fmt, aArgs...)                                      \
    NL_LOG_IMPL_HELPER(__COUNTER__, reg, fmt, ## aArgs);
#define NL_LOG_IMPL_CLEARTEXT(reg, ...)                                      \
    nl_log(reg, __VA_ARGS__);

#else // NLER_FEATURE_LOG_TOKENIZATION

#define NL_LOG_IMPL(reg, ...) \
    nl_log(reg, __VA_ARGS__);
#define NL_LOG_IMPL_CLEARTEXT(reg, ...) \
    NL_LOG_IMPL(reg, __VA_ARGS__)
#define NL_LOG_VA_LIST(reg, fmt, va_list) \
    nl_log_va_list(reg, fmt, va_list)

#endif // NLER_FEATURE_LOG_TOKENIZATION


/** Debug log macro. Only log messages with priority greater than or equal
 * to nlLPDEBG will be logged.
 */
#if (nlLOG_PRIORITY >= nlLPDEBG)
#define NL_LOG_DEBUG(pri, ...)      \
    NLER_BEGIN_MACRO                \
    NL_LOG_IMPL((pri), __VA_ARGS__) \
    NLER_END_MACRO
#else
#define NL_LOG_DEBUG(pri, ...)
#endif

/** Warning log macro. Only log messages with priority greater than or equal
 * to nlLPWARN will be logged.
 */
#if (nlLOG_PRIORITY >= nlLPWARN)
#define NL_LOG_WARN(pri, ...)       \
    NLER_BEGIN_MACRO                \
    NL_LOG_IMPL((pri), __VA_ARGS__) \
    NLER_END_MACRO
#else
#define NL_LOG_WARN(pri, ...)
#endif

/** Critical log macro. Only log messages with priority greater than or equal
 * to nlLPCRIT will be logged.
 */
#if (nlLOG_PRIORITY >= nlLPCRIT)
#define NL_LOG_CRIT(pri, ...)       \
    NLER_BEGIN_MACRO                \
    NL_LOG_IMPL((pri), __VA_ARGS__) \
    NLER_END_MACRO
#else
#define NL_LOG_CRIT(pri, ...)
#endif

/* Log messages which must not be tokenized. */
#define NL_LOG_CLEARTEXT(pri, ...)            \
    NLER_BEGIN_MACRO                          \
    NL_LOG_IMPL_CLEARTEXT((pri), __VA_ARGS__) \
    NLER_END_MACRO

/* Critical log messages used by QA. IE team should be informed/consulted before
 * modifying any NL_LOG_QA message */
#define NL_LOG_QA(pri, ...)     NL_LOG_CRIT(pri, __VA_ARGS__)

/** IE-Automation log macro. Log messages dedicated to IE automation scripts.
 * All such log messages are immune to log priority and are compiled out with
 * nlLOG_AUTOMATION. Developers should take special care to avoid modifying
 * the contents or scheduled execution of nlLOG_AUTOMATION.  If such modification
 * is unavoidable, IE should be notified/consulted.
 */
#ifdef nlLOG_AUTOMATION
#define NL_LOG_AUTOMATION(...)                 \
    NLER_BEGIN_MACRO                           \
    NL_LOG_IMPL((lrIEAUTOMATION), __VA_ARGS__) \
    NLER_END_MACRO
#else
#define NL_LOG_AUTOMATION(...)
#endif

/** Log macro to use only for log lines that absolutely should never be
 * compiled out. Use with care.
 */
#define NL_LOG(pri, ...)            \
    NLER_BEGIN_MACRO                \
    NL_LOG_IMPL((pri), __VA_ARGS__) \
    NLER_END_MACRO

#ifdef __cplusplus
}
#endif

#endif /* NL_ER_LOG_H */
