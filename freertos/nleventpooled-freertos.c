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

int nlevent_pool_create(void *aPoolMemory, int32_t aPoolMemorySize, nlevent_pool_t *aPoolObj)
{
    int retval = NLER_SUCCESS;
    int qsize = aPoolMemorySize / (sizeof(nlevent_pooled_t) + sizeof(nlevent_pooled_t *));

    if ((aPoolMemory != NULL) && (qsize > 0) && (aPoolObj != NULL))
    {
        void *queue = xQueueCreateStatic(qsize, sizeof(nlevent_pooled_t *), aPoolMemory, aPoolObj);

        if (queue == NULL)
        {
            NL_LOG_CRIT(lrERPOOLED, "failed to create freertos pooled event queue (%p, %d)\n", aPoolMemory, aPoolMemorySize);
            retval = NLER_ERROR_BAD_INPUT;
        }
        else
        {
            int             idx;
            uint8_t         *events = (uint8_t *)aPoolMemory + (qsize * sizeof(nlevent_pooled_t *));
            portBASE_TYPE   err;

            for (idx = 0; idx < qsize; idx++)
            {
                err = xQueueSendToBack(queue, &events, 0);

                if (err != pdTRUE)
                {
                    NL_LOG_CRIT(lrERPOOLED, "filling pooled event queue failed at %d of %d (%p)\n", idx, qsize, retval);

                    retval = NLER_ERROR_BAD_INPUT;
                    break;
                }
                else
                {
                    events += sizeof(nlevent_pooled_t);
                }
            }
        }
    }
    else
    {
        NL_LOG_CRIT(lrERPOOLED, "invalid event pool memory %p with size %d specified\n", aPoolMemory, aPoolMemorySize);
        retval = NLER_ERROR_BAD_INPUT;
    }

    return retval;
}

void nlevent_pool_destroy(nlevent_pool_t *aPool)
{
}

nlevent_pooled_t *nlevent_pool_get_event(nlevent_pool_t *aPool)
{
    nlevent_pooled_t       *retval = NULL;

    if (xQueueReceive((QueueHandle_t)aPool, &retval, portMAX_DELAY) != pdTRUE)
    {
        NL_LOG_CRIT(lrERPOOLED, "event pool (%p) exhausted\n", aPool);
        retval = NULL;
    }

    return retval;
}

void nlevent_pool_recycle_event(nlevent_pool_t *aPool, nlevent_pooled_t *aEvent)
{
    portBASE_TYPE   err;

    err = xQueueSendToBack((QueueHandle_t)aPool, &aEvent, 0);

    if (err != pdTRUE)
    {
        NL_LOG_CRIT(lrERPOOLED, "attempt to recycle event (%p) to full pool %p\n", aEvent, aPool);
    }
}

