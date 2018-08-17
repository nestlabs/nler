/*
 *
 *    Copyright (c) 2015 Nest Labs, Inc.
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
 *      This file implements NLER build platform-independent event queue
 *      interfaces when the NLER simulateable time feature has been
 *      enabled.
 *
 */

#include "nlereventqueue_sim.h"
#include "nleratomicops.h"

#if NLER_FEATURE_SIMULATEABLE_TIME

int32_t sCount = 0;

int32_t nleventqueue_sim_count(void)
{
    return sCount;
}

void nleventqueue_sim_count_inc(void)
{
    nl_er_atomic_inc(&sCount);
}

void nleventqueue_sim_count_dec(void)
{
    nl_er_atomic_dec(&sCount);
}

#endif

