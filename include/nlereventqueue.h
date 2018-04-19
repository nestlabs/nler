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
 *      Event queues.
 *
 */

#ifndef NL_ER_EVENT_QUEUE_H
#define NL_ER_EVENT_QUEUE_H

#include <stdint.h>
#include <stddef.h>
#include "nlerevent.h"
#include "nlertime.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Event queues hold pointers to events. They are sized according to the
 * queue depth requirements of the creator. The creator must supply the memory
 * to be used for the queue storage. Queues are used as FIFOs.
 */
typedef void * nl_eventqueue_t;

/** Create an event queue.
 *
 * @param[in] aQueueMemory storage used to hold event pointers in the queue.
 *
 * @param[in] aQueueMemorySize Size in bytes of the memory given to hold the
 * event pointers. The number of events the queue can hold is directly
 * determined by this size.
 *
 * @return The event queue if successful, NULL otherwise.
 */
nl_eventqueue_t nl_eventqueue_create(void *aQueueMemory, size_t aQueueMemorySize);

/** Destroy an event queue
 *
 * @param[in] aEventQueue The event queue to destroy.
 */
void nl_eventqueue_destroy(nl_eventqueue_t aEventQueue);

/** Disable event counting for the queue in the simulator.
 *
 * The simulator tracks outstanding work for each task by counting events going
 * into and out of event queues. In the nominal case, an event queue is used by
 * a task in an event loop and the task continually services the queue until all
 * the events are consumed. In cases where an event queue is used outside an
 * event loop, event counting should be disabled for the queue if no new work is
 * being tracked by the events.  An example of this is when a queue is used as
 * synchronization primative.
 *
 * @param[in] aEventQueue The event queue.
 */
void nl_eventqueue_disable_event_counting(nl_eventqueue_t aEventQueue);

/** Post an event to the tail of the queue.
 *
 * @param[in, out] aEventQueue The queue to post an event to. Because events
 * are posted to the queue as pointers, a single event can be posted to
 * multiple queues simultaneously.
 *
 * @param[in] aEvent pointer to the event to post to the queue.
 *
 * @return NLER_SUCCESS if there was enough space in the queue for the event.
 */
int nl_eventqueue_post_event(nl_eventqueue_t aEventQueue, const nl_event_t *aEvent);

/** Post an event to the tail of the queue from an ISR.
 *
 * @param[in, out] aEventQueue The queue to post an event to.
 * Because events are posted to the queue as
 * pointers, a single event can be posted to
 * multiple queues simultaneously.
 *
 * @param[in] aEvent Pointer to the event to post to the queue.
 *
 * @return NLER_SUCCESS if there was enough space in the queue for the event.
 */
int nl_eventqueue_post_event_from_isr(nl_eventqueue_t aEventQueue, const nl_event_t *aEvent);

/** Receive an event from the queue with a timeout.
 *
 * @param[in] aEventQueue Queue from which to pull the event. If no
 * events are currently in the queue the call will block until an event is
 * posted or the timeout expires.
 *
 * @param[in] aTimeoutMS timeout in milliseconds to wait until giving up on
 * event receipt.
 *
 * @return a pointer to an event or NULL if the timeout expires.
 */
nl_event_t *nl_eventqueue_get_event_with_timeout(nl_eventqueue_t aEventQueue, nl_time_ms_t aTimeoutMS);

/** Receive an event from the queue.
 *
 * @param[in] aEventQueue is the queue from which to pull the event. If no
 * events are currently in the queue the call will block until an event is
 * posted.
 *
 * @return a pointer to an event
 */
#define nl_eventqueue_get_event(aEventQueue) \
    nl_eventqueue_get_event_with_timeout(aEventQueue, NLER_TIMEOUT_NEVER)

/** Get number of events in a queue.
 *
 * @param[in] aEventQueue Queue from which to read the event count
 *
 * @return a count of pending events in the event queue
 */
uint32_t nl_eventqueue_get_count(nl_eventqueue_t aEventQueue);

#ifdef __cplusplus
}
#endif

/** @example test-subpub.c
 * Example of how one event type can be used to manange sub/pub and share.
 */
#endif /* NL_ER_EVENT_QUEUE_H */
