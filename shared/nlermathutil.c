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
 *      This file implements NLER build platform-independent math
 *      utility interfaces.
 *
 */

#include "nlermathutil.h"

/* Implement the division of a 64-bit dividend by a 32-bit divisor
 * into a 32-bit quotient.  The implementation follows the algorithm 4
 * from https://gmplib.org/~tege/division-paper.pdf.  The function
 * expects that the divisor and dividend are scaled s.t. the MSB of
 * the divisor is set.  For Cortex M3, we should expect the following
 * costs of the function:
 *
 * Compiler   | Size (bytes) | Cycles |
 * Clang 3.3  |           28 |        |
 * GCC        |           40 |        |
 *
 * For the helper function, we should expect the following impact on Cortex-M3:
 *
 * Compiler   | Size (bytes) | Cycles |
 * Clang 3.3  |           32 |        |
 * GCC        |           28 |        |
 *
 */

uint32_t nl_div_uint64_into_uint32_helper(uint64_t inDividend, uint32_t inInverse, uint32_t inDivisor)
{
    uint32_t u1 = (inDividend >> 32);
    uint32_t u0 = (inDividend & 0xffffffff);

    uint64_t q  = (((uint64_t)inInverse) * u1) + inDividend;

    uint32_t q1 = ((uint32_t) (q >> 32)) + 1;
    uint32_t q0 = q & 0xffffffff;

    uint32_t r = (((uint64_t) u0) - q1 * inDivisor) & 0xffffffff;

    if (r > q0)
    {
        q1 = q1 - 1;
        r = r + inDivisor;
    }

    if (r >= inDivisor)
    {
        q1 = q1 + 1;
    }

    return q1;
}


/* Generate the actual definition for nl_udiv64_by_1000ULL */

uint32_t nl_udiv64_by_1000ULL (uint64_t inDividend)
{
    const uint32_t reciprocal = NL_MATH_UTILS_RECIPROCAL(1000ULL);
    const uint32_t shiftedDivisor = NL_MATH_UTILS_SCALED_DIVISOR(1000ULL);
    return nl_div_uint64_into_uint32_helper(
        NL_MATH_UTILS_SCALED_DIVIDEND(inDividend, 1000ULL),
        reciprocal,
        shiftedDivisor);
}
