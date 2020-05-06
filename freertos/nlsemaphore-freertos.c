/*
 *
 *    Copyright (c) 2014-2019 Google LLC
 *    All rights reserved.
 *
 *    This document is the property of Google. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Google.
 *
 */

/**
 *    @file
 *      Semaphores (binary and counting) implementation. All of the usual caveats
 *      surrounding the use of semaphores in general apply. Semaphores beget
 *      deadlocks. Use with care and avoid unless absolutely
 *      necessary.
 *
 */

#include "nlersemaphore.h"
#include "nlererror.h"
#include "FreeRTOS.h"
#include "semphr.h"

int nlsemaphore_binary_create(nlsemaphore_t *aSemaphore)
{
    SemaphoreHandle_t semaphore_handle = xSemaphoreCreateBinaryStatic(aSemaphore);
    return ((semaphore_handle == NULL) ? NLER_ERROR_BAD_INPUT : NLER_SUCCESS);
}

int nlsemaphore_counting_create(nlsemaphore_t *aSemaphore, size_t aMaxCount, size_t aInitialCount)
{
    SemaphoreHandle_t semaphore_handle = xSemaphoreCreateCountingStatic(aMaxCount, aInitialCount, aSemaphore);
    return ((semaphore_handle == NULL) ? NLER_ERROR_BAD_INPUT : NLER_SUCCESS);
}

void nlsemaphore_destroy(nlsemaphore_t *aSemaphore)
{
#ifdef xSemaphoreDelete
    xSemaphoreDelete((SemaphoreHandle_t)aSemaphore);
#endif
}

int nlsemaphore_take(nlsemaphore_t *aSemaphore)
{
    int retval;

    if (pdTRUE != xSemaphoreTake((SemaphoreHandle_t)aSemaphore, portMAX_DELAY))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    return retval;
}

int nlsemaphore_take_with_timeout(nlsemaphore_t *aSemaphore, nl_time_ms_t aTimeoutMsec)
{
    int retval;

    if (pdTRUE != xSemaphoreTake((SemaphoreHandle_t)aSemaphore, nl_time_ms_to_delay_time_native(aTimeoutMsec)))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    return retval;
}

int nlsemaphore_give(nlsemaphore_t *aSemaphore)
{
    int retval;

    if (pdTRUE != xSemaphoreGive((SemaphoreHandle_t)aSemaphore))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    return retval;
}

int nlsemaphore_give_from_isr(nlsemaphore_t *aSemaphore)
{
    int retval;
    BaseType_t HigherPriorityTaskWoken;

    if (pdTRUE != xSemaphoreGiveFromISR((SemaphoreHandle_t)aSemaphore, &HigherPriorityTaskWoken))
    {
        retval = NLER_ERROR_NO_RESOURCE;
    }
    else
    {
        retval = NLER_SUCCESS;
    }

    portEND_SWITCHING_ISR(HigherPriorityTaskWoken);

    return retval;
}
