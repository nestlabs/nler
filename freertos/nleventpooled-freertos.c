/*
 *
 *    Copyright (c) 2014 Nest Labs, Inc.
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
 *      This file implements NLER pooled events under the FreeRTOS
 *      build platform.
 *
 */

#include "nlereventpooled.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdlib.h>
#include "nlerlog.h"
#include "nlererror.h"
#include <string.h>

nl_event_pool_t nl_event_pool_create(void *aPoolMemory, int32_t aPoolMemorySize)
{
    xQueueHandle    retval = NULL;
    int             qsize = aPoolMemorySize / (sizeof(nl_event_pooled_t) + sizeof(nl_event_pooled_t *));

    if ((aPoolMemory != NULL) && (qsize > 0))
    {
        retval = xQueueCreate(aPoolMemory, qsize, sizeof(nl_event_pooled_t *));

        if (retval == NULL)
        {
            NL_LOG_CRIT(lrERPOOLED, "failed to create freertos pooled event queue (%p, %d)\n", aPoolMemory, aPoolMemorySize);
        }
        else
        {
            int             idx;
            uint8_t         *events = (uint8_t *)aPoolMemory + (qsize * sizeof(nl_event_pooled_t *));
            portBASE_TYPE   err;

            for (idx = 0; idx < qsize; idx++)
            {
                err = xQueueSendToBack(retval, &events, 0);

                if (err != pdTRUE)
                {
                    NL_LOG_CRIT(lrERPOOLED, "filling pooled event queue failed at %d of %d (%p)\n", idx, qsize, retval);

                    retval = NULL;
                    break;
                }
                else
                {
                    events += sizeof(nl_event_pooled_t);
                }
            }
        }
    }
    else
    {
        NL_LOG_CRIT(lrERPOOLED, "invalid event pool memory %p with size %d specified\n", aPoolMemory, aPoolMemorySize);
    }

    return (nl_event_pool_t)retval;
}

void nl_event_pool_destroy(nl_event_pool_t aPool)
{
}

nl_event_pooled_t *nl_event_pool_get_event(nl_event_pool_t aPool)
{
    nl_event_pooled_t       *retval = NULL;

    if (xQueueReceive((xQueueHandle)aPool, &retval, portMAX_DELAY) != pdTRUE)
    {
        NL_LOG_CRIT(lrERPOOLED, "event pool (%p) exhausted\n", aPool);
        retval = NULL;
    }

    return retval;
}

void nl_event_pool_recycle_event(nl_event_pool_t aPool, nl_event_pooled_t *aEvent)
{
    portBASE_TYPE   err;

    err = xQueueSendToBack((xQueueHandle)aPool, &aEvent, 0);

    if (err != pdTRUE)
    {
        NL_LOG_CRIT(lrERPOOLED, "attempt to recycle event (%p) to full pool %p\n", aEvent, aPool);
    }
}

