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
 *      This file implements NLER initialization under the Netscape
 *      Portable Runtime (NSPR) build platform.
 *
 */

#include <nspr/prthread.h>
#include <nspr/prinit.h>
#include "nlerlog.h"
#include "nlerlogmanager.h"
#include "nlererror.h"
#include "nleratomicops.h"

#if NLER_FEATURE_FLOW_TRACER
#include "nlerflowtracer.h"
#endif

extern int nltask_nspr_init(void);

#if NLER_FEATURE_DEFAULT_LOGGER
static void nspr_default_logger(void *aClosure, nl_log_region_t aRegion, int aPriority, const char *format, va_list ap)
{
    vprintf(format, ap);
}
#endif /* NLER_FEATURE_DEFAULT_LOGGER */

int nl_er_init(void)
{
    int     retval = NLER_SUCCESS;

    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);

    retval = nl_er_atomic_init();

#if NLER_FEATURE_FLOW_TRACER
    nl_flowtracer_init();
#endif

#if NLER_FEATURE_DEFAULT_LOGGER
    nl_set_logging_function(nspr_default_logger, NULL);
#endif /* NLER_FEATURE_DEFAULT_LOGGER */

    retval = nltask_nspr_init();
    if (retval != NLER_SUCCESS)
    {
        goto done;
    }

#if NLER_FEATURE_FLOW_TRACER
    nl_flowtracer_init();
#endif

 done:
    return retval;
}

void nl_er_cleanup(void)
{
    PR_Cleanup();
}

void nl_er_start_running(void)
{
}

