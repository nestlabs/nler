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
 *      This file implements NLER initialization under the POSIX
 *      threads (pthreads) build platform.
 *
 */

#include <pthread.h>

#include <nlerlog.h>
#include <nlerlogmanager.h>
#include <nlererror.h>
#include <nleratomicops.h>

#if NLER_FEATURE_FLOW_TRACER
#include <nlerflowtracer.h>
#endif

extern int nltask_pthreads_init(void);
extern void nltask_pthreads_destroy(void);

void pthreads_default_logger(void *aClosure, nl_log_region_t aRegion, int aPriority, const char *format, va_list ap)
{
    vprintf(format, ap);
}

int nl_er_init(void)
{
    int     retval = NLER_SUCCESS;

    retval = nl_er_atomic_init();
    if (retval != NLER_SUCCESS)
    {
        goto done;
    }

    retval = nltask_pthreads_init();
    if (retval != NLER_SUCCESS)
    {
        goto done;
    }    

#if NLER_FEATURE_FLOW_TRACER
    nl_flowtracer_init();
#endif

    nl_set_logging_function(pthreads_default_logger, NULL);

#if NLER_FEATURE_FLOW_TRACER
    nl_flowtracer_init();
#endif

 done:
    return (retval);
}

void nl_er_cleanup(void)
{
    nltask_pthreads_destroy();
}

void nl_er_start_running(void)
{

}

