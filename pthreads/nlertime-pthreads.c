/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *      This file implements NLER time under the POSIX threads
 *      (pthreads) build platform.
 *
 */

#include <nlertime.h>

extern nl_time_native_t _nl_get_time_native(void);

nl_time_native_t nl_time_ms_to_time_native(nl_time_ms_t aTime)
{
    nl_time_native_t retval = 0;

    return retval;
}

nl_time_native_t nl_time_ms_to_delay_time_native(nl_time_ms_t aTime)
{
    nl_time_native_t retval = 0;

    return retval;
}

nl_time_ms_t nl_time_native_to_time_ms(nl_time_native_t aTime)
{
    nl_time_native_t retval = 0;

    return retval;
}

nl_time_native_t _nl_get_time_native(void)
{
    return (0);
}


