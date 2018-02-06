/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file provides support for tokenization of log format strings.
 *
 *      This file is shared with host code. Log related items just for
 *      use by firmware should be placed in nlerlog.h.
 */

#ifndef NL_ER_LOG_TOKEN_H
#define NL_ER_LOG_TOKEN_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NL_LOG_TOKEN_SENTINEL1 '@'
#define NL_LOG_TOKEN_SENTINEL2 '!'

/*
 * Header for each log entry sent over serial port.
 */
typedef struct __attribute__((packed))
{
    uint8_t mProductId;
    uint8_t mBuildConfig;
    uint8_t mBuildTag[4]; // Uniquely identify this build from all other such builds.
} nl_build_id_t;

/*
 *  Helper macro for statically allocating the build id.  This must be instantiated
 *  exactly once, at file scope, by the app.
 */
#define NL_BUILD_ID_CREATE(product, config) \
    nl_build_id_t gBuildId __attribute__((section(".build_id"))) = \
    {product, config};

/*
 *  Tokenization requires the app to statically allocate a build id object.
 *  NLER takes advantage of this being available by using it before the app
 *  is booted.  The build id must have this name.
 */
extern nl_build_id_t gBuildId;

/*
 * Definitions for working with UTC time.
 *
 * Note: these values are the same as defined in nlweave-platform.  However, these 
 * definitions avoid dependency from NLER to nlweave-platform.
 */

typedef uint64_t nl_log_utc_ms_t;
#define NL_LOG_UTC_UNDEFINED       ~(0ULL)

/*
 * Log header format(s).
 */
typedef struct __attribute__((packed))
{
    nl_build_id_t    mBuildId;
    uint32_t         mToken;
    uint32_t         mTimeMs;
} nl_log_header_v0_t;

typedef struct __attribute__((packed))
{
    uint8_t          mHeaderVersion;
    nl_build_id_t    mBuildId;
    uint32_t         mToken;
    uint32_t         mTimeMs;
    nl_log_utc_ms_t  mUtcTimeMs;
} nl_log_header_v1_t;

typedef nl_log_header_v1_t nl_log_header_t;

/*
 * Version for the header format.
 */

#define NL_LOG_HEADER_VERSION_0    0 /* Original header format did not use version #. */
#define NL_LOG_HEADER_VERSION_1    1

/*
 * The tokenized log string feature causes NL_LOG() family of macros to
 * emit tokenized, base64 data rather than formatted human-readable
 * strings. This saves time, code, RAM, and rodata space on the device.
 *
 * Note that code should not directly call nl_log() or nl_log_va_list()
 * functions, but must use NL_LOG macro invocations.
 */

#define NL_LOG_TOKEN_RECORD_ID 1
#define NL_LOG_TOKEN_VERSION 1UL
typedef uint32_t nl_log_token_version_t;

/*  Longest possible log line supported by tokenization.  This bounds both
 *  the format string length and the final formatted length.  Meant to
 *  be overly generous.
 */
#define NL_LOG_MAX_LINE_LENGTH 1024

/*  Encoding of build configuration in log header.
 */
enum
{
    kBuildConfig_Release = 0,
    kBuildConfig_Development = 1,
    kBuildConfig_Diagnostics = 2,

    kBuildConfig_Num
};

/*  Table to map log header build config encoding to human readable string.
 *
 *  Table deadstripped from firmware, where it is not used.
 */
static const char * build_config_names[] __attribute__((unused)) =
{
    "release",
    "development",
    "diagnostics"
};

/*
 * Log table entry. Defines per-log entry in the log table.
 *
 * Instances of this entry are collected into a table, called the log table.
 * Each log site has an entry in the table.
 */
typedef struct
{
    uint32_t mToken;
        /*
            The token is the value used to identify the corresponding log
            format string.
         */
    uint32_t mFormat;
        /*
            Compressed representation of the original format string.  This
            allows firmware to properly marshal arguments without storing
            full format string.

            Format is 16 element array of 2 bit arg type values.
         */
} nl_log_token_entry_t;

/*
 *  Token region entry.  Defines per-token entry in the token region table.
 *
 *  Conceptually, this is part of the log_token table, but does not need
 *  to be on device.
 */
typedef struct
{
    uint32_t mToken;
    uint32_t mRegionId;
} nl_log_token_region_entry_t;

/*
 *  Argument type.  Minimalist encoding of type information for compressed
 *  format representation.
 */
typedef enum
{
    kArgType_Invalid = 0,    // Must be zero for fast initialization.
    kArgType_Numeric32 = 1,
    kArgType_Numeric64 = 2,
    kArgType_String = 3
} ArgType_t;
#define NL_ER_LOG_ARG_FIELD_WIDTH 2
#define NL_ER_LOG_ARG_FIELD_MASK  ((1 << NL_ER_LOG_ARG_FIELD_WIDTH) - 1)
#define NL_ER_LOG_MAX_ARGS ((sizeof(uint32_t) * CHAR_BIT) / NL_ER_LOG_ARG_FIELD_WIDTH)

/*
 *  Region entry.  Defines an entry in the region table.
 *
 *  The region table and corrresponding region strings sections in the ELF
 *  file allow host detokenizer to translate region ids to human readable
 *  region names.
 *
 *  This information is embedded in the ELF file because the mapping of
 *  regions to ids could change from build-to-build.
 */
typedef struct
{
    uint32_t mRegionIndex; // Not used on device, so bit width not important.
    uint32_t mRegionName;  // Actually pointer, but don't assume host has same size pointer.
} nl_log_region_entry_t;

/*
 *  Macros to create a region table entry.
 *
 *  @param[in] region Region id (nl_log_region_t) for this region
 *  @param[in] region_name Human readable name for this region
 */
#define NL_LOG_CREATE_REGION(region, region_name)                                \
        static const char __attribute__((section(".region_strings"), used))      \
            _ ## region ## _str_[] = region_name;                                \
        static const nl_log_region_entry_t                                       \
            __attribute__((section(".region_table"), used))                      \
            _ ## region ## _entry_ =                                             \
            {(uint32_t)(region), (uint32_t)(_ ## region ## _str_)};

#ifdef __cplusplus
}
#endif

#endif // NL_ER_LOG_TOKEN_H
