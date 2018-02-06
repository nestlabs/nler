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
 *      This file implements a unit test for the NLER math utility
 *      interfaces.
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include "nlerlog.h"
#include "nlermathutil.h"

#define  MAX_TESTS 1000000000

int main(int argc, char **argv)
{
    bool success = true;

    NL_LOG_CRIT(lrTEST, "start main\n");

    if (argc == 3) {
        uint64_t start, end;
        uint64_t i,j;
        uint32_t q, r;

        start = strtoull(argv[1], NULL, 0);
        end = strtoull(argv[2], NULL, 0);

        NL_LOG_CRIT(lrTEST, "checking range from %" PRIu64 " (0x%" PRIx64 ") to %" PRIu64 "(0x%" PRIx64 ")\n", start, start, end, end);
        q = start / 1000;
        j = ((q+1) * 1000ULL);
        for (i = start; i < end; i++)
        {
            if (i == j)
            {
                j+=1000;
                q++;
            }
            r = nl_udiv64_by_1000ULL(i);
            if (r != q)
            {
                NL_LOG_CRIT(lrTEST, "Math util failed: Dividend: %" PRIu64", result: %" PRIu32 " expected: %" PRIu32 "\n",
                        i, r, q);
                success = false;
            }
        }
    }
    else
    {

    size_t i;
    uint64_t tests[] = {     0,      100,
                             1000,   1100,
                             10000,  10100,
                             32998, 32999, 32501, 33000,
                             100000, 100100,
                             1ULL<<32,
                             125ULL << 32,
                             (1000ULL << 32) - 1000};
    for (i = 0; i  < sizeof(tests)/sizeof(uint64_t); i++)
    {
        uint32_t r = nl_udiv64_by_1000ULL(tests[i]);
        uint32_t q = (uint32_t)(tests[i] / 1000);
        if (r != q)
        {
            NL_LOG_CRIT(lrTEST, "Math util failed: Dividend: %" PRIu64 ", result: %" PRIu32 " expected: %" PRIu32 "\n",
                        tests[i], r, q);
            success = false;
        }
    }

    for (i = 0; i < MAX_TESTS; i++)
    {
        uint32_t q = (uint32_t) random();
        uint32_t rem = (uint32_t) ( (1000ULL * random()) / RAND_MAX);
        uint64_t div = 1000ULL * q + rem;
        uint32_t r = nl_udiv64_by_1000ULL(div);

        if (r != q)
        {
            NL_LOG_CRIT(lrTEST, "Math util failed: Dividend: %" PRIu64 ", result: %" PRIu32 " expected: %" PRIu32" \n",
                        tests[i], r, q);
            success = false;
        }
    }
    }
    if (success)
    {
        NL_LOG_CRIT(lrTEST, "all tests pass\n");
    }

    NL_LOG_CRIT(lrTEST, "end main\n");

    return 0;
}
