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
 *      General macros used within runtime implementation and other
 *      preprocessor macros are defined here.
 *
 */

#ifndef NL_ER_MACROS_H
#define NL_ER_MACROS_H

/** Macro begin guard.
 */
#define NLER_BEGIN_MACRO    do {

/** Macro end guard.
 */
#define NLER_END_MACRO      } while (0)

/** Macros to obtain string representation of another macro's value.
 */
#define NLER_STRINGIFY2(s)  #s
#define NLER_STRINGIFY(s)   NLER_STRINGIFY2(s)

/** Concatenate two strings.
 */
#define NLER_CONCAT(a, b)   a##b

#endif /* NL_ER_MACROS_H */
