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

#ifndef NL_ER_SEMAPHORE_H
#define NL_ER_SEMAPHORE_H

#include "nlernative.h"
#include "nlertime.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Binary and Counting semaphores.
 */

/** Create a new binary semaphore.
 *
 * @param[in] aSemaphore storage used for the semaphore control block
 *
 * @return NLER_SUCCESS if semaphore created successfully
 */
int nlsemaphore_binary_create(nlsemaphore_t *aSemaphore);

/** Create a new counting semaphore.
 *
 * @param[in] aSemaphore storage used for the semaphore control block
 * @param[in] aMaxCount maximum count the semaphore can be given before blocking
 * @param[in] aInitialCount count assigned to semaphore upon creation
 *
 * @return NLER_SUCCESS if semaphore created successfully
 */
int nlsemaphore_counting_create(nlsemaphore_t *aSemaphore, size_t aMaxCount, size_t aInitialCount);

/** Destroy a semaphore.
 *
 * @param[in] aSemaphore semaphore to destroy.
 */
void nlsemaphore_destroy(nlsemaphore_t *aSemaphore);

/** Obtain a semaphore.
 *
 * @param[in] aSemaphore semaphore to obtain from.
 *
 * @return NLER_SUCCESS if semaphore obtained successfully
 *         NLER_ERROR_NO_RESOURCE upon failure
 */
int nlsemaphore_take(nlsemaphore_t *aSemaphore);

/** Attempt to obtain a semaphore until timeout
 *
 * @param[in] aSemaphor semaphore to obtain from.
 * @param[in] aTimeoutMsec number of msec to attempt to obtain semaphore. A timeout
 *            of 0 will return immediately, effectively polling for a semaphore.
 *
 * @return NLER_SUCCESS if semaphore obtained successfully
 *         NLER_ERROR_NO_RESOURCE if timeout occurs and no semaphore could be obtained
 */
int nlsemaphore_take_with_timeout(nlsemaphore_t *aSemaphore, nl_time_ms_t aTimeoutMsec);

/** Give/release a semaphore
 *
 * @param[in] aSemaphore semaphore to give to
 *
 * @return NLER_SUCCESS if semaphore released successfully
 *         NLER_ERROR_NO_RESOURCE upon failure
 */
int nlsemaphore_give(nlsemaphore_t *aSemaphore);

/** Give/release a semaphore from ISR context
 *
 * @param[in] aSemaphore semaphore to give to
 *
 * @return NLER_SUCCESS if semaphore released successfully
 *         NLER_ERROR_NO_RESOURCE upon failure
 */
int nlsemaphore_give_from_isr(nlsemaphore_t *aSemaphore);

#ifdef __cplusplus
}
#endif

#endif /* NL_ER_SEMAPHORE_H */
