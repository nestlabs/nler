/*
 *
 *    Copyright (c) 2018 Google LLC
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
 *      This file implements a unit and functional test-specific
 *      assertion delegate handler.
 */

#include <stdio.h>

#include <nlerassert.h>

/*
 * Forward Declarations
 */

#if defined(NLER_SMALL_ASSERTS) || NLER_SMALL_ASSERTS
extern void nler_test_assert(const char *aFile, unsigned int aLine);
#else
extern void nler_test_assert(const char *aCondition, const char *aFile, const char *aFunction, unsigned int aLine);
#endif /* defined(NLER_SMALL_ASSERTS) || NLER_SMALL_ASSERTS */

#if defined(NLER_SMALL_ASSERTS) || NLER_SMALL_ASSERTS
void nler_test_assert(const char *aFile, unsigned int aLine)
{
        fprintf(stderr, "ASSERT: file: %s, line: %u\n",
                aFile,
                aLine);
}
#else
void nler_test_assert(const char *aCondition, const char *aFile, const char *aFunction, unsigned int aLine)
{
    fprintf(stderr, "ASSERT: %s, file: %s, line: %u\n",
            aCondition,
            aFile,
            aLine);
}
#endif /* defined(NLER_SMALL_ASSERTS) || NLER_SMALL_ASSERTS */




