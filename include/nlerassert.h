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
 *      Assertions.
 *
 */

#ifndef NL_ER_ASSERT_H
#define NL_ER_ASSERT_H

#include "nlermacros.h"
#include "nlerlog.h"

/** Assert condition.
 */
/* FILE_NAME is usually defined by the build system, but in case it is not
 * give it a default.
 */

#if !defined(NLER_FEATURE_ASSERTS) || NLER_FEATURE_ASSERTS

#ifndef FILE_NAME
#define FILE_NAME __FILE__
#endif

#ifndef NLER_SMALL_ASSERTS

#define NLER_ASSERT(condition)                                      \
    NLER_BEGIN_MACRO                                                \
        if (!(condition))                                           \
        {                                                           \
            NLER_PLATFORM_ASSERT_DELEGATE(#condition, FILE_NAME, __FUNCTION__, __LINE__); \
        }                                                           \
    NLER_END_MACRO

#else /* NLER_SMALL_ASSERTS */


#define NLER_ASSERT(condition)                                      \
    NLER_BEGIN_MACRO                                                \
        if (!(condition))                                           \
        {                                                           \
            NLER_PLATFORM_ASSERT_DELEGATE(FILE_NAME, __LINE__);     \
        }                                                           \
    NLER_END_MACRO
#endif /* NLER_SMALL_ASSERTS */
#else  /* NLER_FEATURE_ASSERTS */
#define NLER_ASSERT(x) ((void)0)
#endif /* NLER_FEATURE_ASSERTS */

/** @cond */
#define NLER_ASSERT_CONCAT_(a, b) a##b

#define NLER_ASSERT_CONCAT(a, b) NLER_ASSERT_CONCAT_(a, b)
/** @endcond */

/** Compile time (static) assertion. Do not use after a statement or twice on
 * the same line to ensure compatibility with older C standards.
 *
 * @param[in] condition Condition to verify.
 *
 * @param[in] message Message to log if condition is false.
 */
// NLER_STATIC_ASSERT is deprecated.  Please use nlSTATIC_ASSERT instead.
#if defined(__cplusplus) && (__cplusplus >= 201103L)
# define NLER_STATIC_ASSERT(condition, message) static_assert(condition, message)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
# define NLER_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#else
# ifdef __COUNTER__
#   define NLER_STATIC_ASSERT(condition, message) typedef char NLER_ASSERT_CONCAT(STATIC_ASSERT_t_, __COUNTER__)[(condition) ? 1 : -1] __attribute__ ((unused))
# else
#   define NLER_STATIC_ASSERT(condition, message) typedef char NLER_ASSERT_CONCAT(STATIC_ASSERT_t_, __LINE__)[(condition) ? 1 : -1] __attribute__ ((unused))
# endif
#endif /* defined(__cplusplus) && (__cplusplus >= 201103L) */

#endif /* NL_ER_ASSERT_H */
