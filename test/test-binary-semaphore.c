/*
 *
 *    Copyright (c) 2020 Project nler Authors
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
 *      This file implements a unit test for the NLER binary semaphore
 *      interfaces.
 *
 */

#include <nlersemaphore.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <nlererror.h>
#include <nlertask.h>
#include <nlerinit.h>
#include <nlerlog.h>
#include <nlerassert.h>


void test_unthreaded(void)
{
    const nl_time_ms_t kTimeoutNever = NLER_TIMEOUT_NEVER;
    nlsemaphore_t *    lNullSemaphore = NULL;
    int                lStatus;

    // Negative Tests

    lStatus = nlsemaphore_binary_create(lNullSemaphore);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    lStatus = nlsemaphore_give(lNullSemaphore);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    lStatus = nlsemaphore_give_from_isr(lNullSemaphore);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    lStatus = nlsemaphore_take(lNullSemaphore);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    lStatus = nlsemaphore_take_with_timeout(lNullSemaphore, kTimeoutNever);
    NLER_ASSERT(lStatus == NLER_ERROR_BAD_INPUT);

    // Positive Tests
}

void test_threaded(void)
{
    nl_er_start_running();
}

int main(int argc, char **argv)
{
    int             err;

    NL_LOG_CRIT(lrTEST, "start main\n");

    err = nl_er_init();
    NLER_ASSERT(err == NLER_SUCCESS);

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime: %d)\n", err);

    NLER_ASSERT(argc == 1);

    test_unthreaded();

    test_threaded();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (EXIT_SUCCESS);
}
