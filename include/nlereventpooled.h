/*
 *
 *    Copyright (c) 2014-2016 Nest Labs, Inc.
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
 *      Pooled events.
 *
 */

#ifndef NL_ER_EVENT_POOLED_H
#define NL_ER_EVENT_POOLED_H

#include "nlerevent.h"
#include "nlereventqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Event pool. Pooled events are taken from a global event pool and then
 * recycled to that pool when no longer in use. It is a larger policy decision
 * on when to recycle the event.
 */
typedef nleventqueue_t nlevent_pool_t;

/** Pooled event. Pooled events extend standard events with a queue to send a
 * response to in case that is required and an additional pointer to pass any
 * additional data to the recipient.
 */
typedef struct nlevent_pooled_s
{
    NL_DECLARE_EVENT;                   /**< Common event fields. */
    nleventqueue_t      *mReturnQueue;  /**< Return response queue. */
    void                *mPayload;      /**< Additional data. */
} nlevent_pooled_t;

/** Create event pool.
 *
 * @param[in, out] aPoolMemory A block of memory from which all of the pooled
 * events will be pulled. As many events as can fit in that memory will be
 * available in the global pool.
 *
 * @param[in] aPoolMemorySize Size in bytes of aPoolMemory.
 *
 * @param[in] aPoolObj pointer to storage used for the event pool object
 *
 * @return NLER_SUCCESS if the event pool is created succesfully.
 *
 */

int nlevent_pool_create(void *aPoolMemory, int32_t aPoolMemorySize, nlevent_pool_t *aPoolObj);

/** Destroy the event pool.
 *
 * @param[in, out] aPool Event pool to destroy.
 */
void nlevent_pool_destroy(nlevent_pool_t *aPool);

/** Get an event from the event pool.
 *
 * @param[in] aPool Pool from which to get an event.
 *
 * @return An event if there are events in the pool, NULL otherwise.
 */
nlevent_pooled_t *nlevent_pool_get_event(nlevent_pool_t *aPool);

/** Recycle an event to the pool.
 *
 * @param[in, out] aPool The event pool to recycle the event to.
 *
 * @param[in] aEvent The event to recycle.
 */
void nlevent_pool_recycle_event(nlevent_pool_t *aPool, nlevent_pooled_t *aEvent);

#ifdef __cplusplus
}
#endif

/** @example test-pooledevent.c
 * Two tasks that use an event pool to manage events.
 */
#endif /* NL_ER_EVENT_POOLED_H */
