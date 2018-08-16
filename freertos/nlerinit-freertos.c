/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file implements NLER initialization under the FreeRTOS
 *      build platform.
 *
 */

#include "nlerinit.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "nlerlog.h"
#include "nlerlogmanager.h"
#include "nleratomicops.h"
#include "nlererror.h"
#include "nlerlock.h"

#if NLER_FEATURE_FLOW_TRACER
#include <nlerflowtracer.h>
#endif

#ifndef NLER_PUTCHAR_FUNC
#define NLER_PUTCHAR_FUNC putchar
#endif

#if NLER_FEATURE_DEFAULT_LOGGER
static nllock_t logger_lock;

static void freertos_default_logger(void *aClosure, nl_log_region_t aRegion, int aPriority, const char *format, va_list ap)
{
    nllock_enter(&logger_lock);
    vprintf(format, ap);
    nllock_exit(&logger_lock);
}

#if NLER_FEATURE_LOG_TOKENIZATION
static void _nl_erinit_freertos_putchar(uint8_t c, void *context)
{
    NLER_PUTCHAR_FUNC(c);
}

static void freertos_default_token_logger(void *aClosure, nl_log_region_t aRegion, int aPriority, const nl_log_token_entry_t *format, va_list ap)
{
    nllock_enter(&logger_lock);
    // Marshal log header, format arguments, encode, and send entire log out serial port.
    nl_log_send_tokenized(_nl_erinit_freertos_putchar, 0, NL_LOG_UTC_UNDEFINED, format, ap);
    nllock_exit(&logger_lock);
}
#endif /* NLER_FEATURE_LOG_TOKENIZATION */
#endif /* NLER_FEATURE_DEFAULT_LOGGER */

int nl_er_init(void)
{
    int retval = NLER_SUCCESS;

#if NLER_FEATURE_FLOW_TRACER
    nl_flowtracer_init();
#endif

#if NLER_FEATURE_DEFAULT_LOGGER
    nllock_create(&logger_lock);

    nl_set_logging_function(freertos_default_logger, NULL);
#if NLER_FEATURE_LOG_TOKENIZATION
    nl_set_token_logging_function(freertos_default_token_logger, NULL);
#endif /* NLER_FEATURE_LOG_TOKENIZATION */
#endif /* NLER_FEATURE_DEFAULT_LOGGER */

    retval = nl_er_atomic_init();

    return retval;
}

void nl_er_cleanup(void)
{
}

void nl_er_start_running(void)
{
    vTaskStartScheduler();
}

