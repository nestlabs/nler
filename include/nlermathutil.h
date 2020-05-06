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
 *      Math utilities.
 *
 */

#ifndef NL_ER_MATH_UTIL_H
#define NL_ER_MATH_UTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Macro to compute a shift of a 32-bit number s.t. the number would have the MSB set.
 */

#define NL_MATH_UTIL_COMPUTE_LEFT_SHIFT(x)  \
    (((x) & 0x80000000) ? 0 :     \
     (((x) & 0x40000000) ? 1 :    \
      (((x) & 0x20000000) ? 2 :   \
       (((x) & 0x10000000) ? 3 :  \
        (((x) & 0x08000000) ? 4 : \
         (((x) & 0x04000000) ? 5 :              \
          (((x) & 0x02000000) ? 6 :             \
           (((x) & 0x01000000) ? 7 :            \
            (((x) & 0x00800000) ? 8 :           \
             (((x) & 0x00400000) ? 9 :          \
              (((x) & 0x00200000) ? 10 :        \
               (((x) & 0x00100000) ? 11 :       \
                (((x) & 0x00080000) ? 12 :      \
                 (((x) & 0x00040000) ? 13 :     \
                  (((x) & 0x00020000) ? 14 :    \
                   (((x) & 0x00010000) ? 15 :   \
                    (((x) & 0x00008000) ? 16 :  \
                     (((x) & 0x00004000) ? 17 : \
                      (((x) & 0x00002000) ? 18 :        \
                       (((x) & 0x00001000) ? 19 :       \
                        (((x) & 0x00000800) ? 20 :      \
                         (((x) & 0x00000400) ? 21 :     \
                          (((x) & 0x00000200) ? 22 :    \
                           (((x) & 0x00000100) ? 23 :   \
                            (((x) & 0x00000080) ? 24 :  \
                             (((x) & 0x00000040) ? 25 : \
                              (((x) & 0x00000020) ? 26 :        \
                               (((x) & 0x00000010) ? 27 :       \
                                (((x) & 0x00000008) ? 28 :      \
                                 (((x) & 0x00000004) ? 29 :     \
                                  (((x) & 0x00000002) ? 30 : 31)))))))))))))))))))))))))))))))

#define NL_MATH_UTILS_SCALED_DIVISOR(DIVISOR) (((uint32_t)(DIVISOR)) << NL_MATH_UTIL_COMPUTE_LEFT_SHIFT(DIVISOR)) /**< Scale the divisor s.t. its MSB is set */

#define NL_MATH_UTILS_RECIPROCAL(DIVISOR) ((UINT64_MAX / NL_MATH_UTILS_SCALED_DIVISOR(DIVISOR)) - (1ULL << 32)) /**< Compute the reciprocal of a divisor */

#define NL_MATH_UTILS_SCALED_DIVIDEND(DIVIDEND, DIVISOR) ((DIVIDEND) << NL_MATH_UTIL_COMPUTE_LEFT_SHIFT(DIVISOR)) /**< Scale the dividend by the same factor as the divisor */

/** Function implementing a division of a 64-bit dividend producing a
 * 32 bit quotient using a 32bit divisor and a precomputed 32-bit
 * reciprocal.
 *
 * @param[in] inDividend 64-bit, unsigned dividend.  The dividend must
 *            be scaled along with the divisor s.t. divisor's most
 *            significant bit is set to 1.
 *
 * @param[in] inInverse 32-bit reciprocal of the inDivisor computed
 *            via formula:
 *                floor((2^64-1)/inDivisor) - 2^32
 *
 * @param[in] inDivisor 32-bit divisor.  The divisor MUST be scaled
 *            s.t. its most significant bit is set to 1.
 *
 */
uint32_t nl_div_uint64_into_uint32_helper(uint64_t inDividend, uint32_t inInverse, uint32_t inDivisor);

/** A macro that defines a function that generates a 32-bit quotient
 *  of a 64-bit divident and a constant.
 */
#define NL_GENERATE_UDIV64_BY(DIVISOR) \
    uint32_t nl_udiv64_by_##DIVISOR (uint64_t inDividend)               \
    {                                                                   \
        const uint32_t reciprocal = NL_MATH_UTILS_RECIPROCAL(DIVISOR);  \
        const uint32_t shiftedDivisor = NL_MATH_UTILS_SCALED_DIVISOR(DIVISOR); \
        return nl_div_uint64_into_uint32_helper(                        \
            NL_MATH_UTILS_SCALED_DIVIDEND(inDividend, DIVISOR),         \
            reciprocal,                                                 \
            shiftedDivisor);                                            \
    }

/** Divide a 64 bit unsigned dividend by 1000 producing a 32-bit
 * value.  No checking is performed to ensure that the quotient fits
 * in 32 bits.
 *
 * @param[in] inDividend 64-bit value to be divided
 *
 * @result the 32-bit quotient.
 */
uint32_t nl_udiv64_by_1000ULL (uint64_t inDividend);

#ifdef __cplusplus
}
#endif

#endif // NL_ER_MATH_UTIL_H
