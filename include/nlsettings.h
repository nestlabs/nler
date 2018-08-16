/*
 *
 *    Copyright (c) 2015-2018 Nest Labs, Inc.
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
 *
 *    @file
 *      Settings. This provides an ID->value store, and supports
 *      subscription to individual IDs or to the set of all IDs.
 *      Subscribers are notified when the value of any subscribed-to
 *      ID changes, and if a value changes while a subscriber is
 *      unsubscribed, the subscriber is guaranteed to get the latest
 *      value when it resubscribes.
 *
 */

#ifndef NL_ER_UTILITIES_SETTINGS_H
#define NL_ER_UTILITIES_SETTINGS_H

#include <stdint.h>

#include <nlerevent.h>
#include <nlereventqueue.h>

#ifdef __cplusplus
extern "C" {
#endif

/* changing this length will allow for shorter
 * or longer values to be stored in settings.
 * the value should be set to the maximum value
 * that will ever need to be stored for a setting.
 * one more byte will be allocated for a '\0' terminator.
 * please keep values as short as possible to
 * reduce RAM usage.
 * to adjust size, please override in build system.
 * MMP
 */

#ifndef NLER_SETTINGS_VALUE_LENGTH
#define NLER_SETTINGS_VALUE_LENGTH    7
#endif

typedef char nl_settings_value_t[NLER_SETTINGS_VALUE_LENGTH + 1];

typedef enum
{
    nl_settings_keyInvalid                = -1,

/* include application specific settings, if any.  the default
 * name for the file containing application-specific settings
 * is nlappsettingkeys.h.  this filename may be overridden by
 * defining NLER_SETTINGS_APPLICATION_SETTINGS_KEYS.
 */

#ifdef HAVE_NLER_SETTINGS_APPLICATION_SETTINGS_KEYS
#ifdef NLER_SETTINGS_APPLICATION_SETTINGS_KEYS
#include NLER_SETTINGS_APPLICATION_SETTINGS_KEYS
#else
#include <nlappsettingkeys.h>
#endif
#endif

    nl_settings_keyMax
} nl_settings_key_t;

/* when subscribing for changes you must supply an event
 * for each key that you wish to be notified of a value
 * change. your event will be returned to you and your
 * subscription cancelled. re-subscribe to get further
 * notification of changes. the settings service will
 * guarantee that you will be notified if the value
 * has changed during the unsubscribed time interval.
 * mReturnQueue event queue to which the change notifications
 * should be sent.
 * mKey key of value that you wish to track changes for. to
 * track all changes use nl_settings_keyInvalid.
 * mNewValue will hold a copy of the latest value when the
 * value changes. when tracking all changes this field
 * is uninitialized.
 * mChangeCounter should be initialized to 0 before ever
 * subscribing but do not alter when re-subscribing.
 * mChain is for internal use of the setting service.
 *
 */

/* note that no type is defined for this event.
 * the application is free to define whatever event type
 * it would like to manage incoming events of this type
 * (or simply use nl_dispatch_event() and set the
 * event handler to call without ever defining an event type).
 */

typedef struct
{
    NL_DECLARE_EVENT
    nleventqueue_t     *mReturnQueue;
    nl_settings_key_t   mKey;
    nl_settings_value_t mNewValue;
    unsigned int        mChangeCount;
    void                *mChain;
} nl_settings_change_event_t;

#define NL_INIT_SETTINGS_CHANGE_EVENT_STATIC(t, h, c, r, k) \
    (t),                                                    \
    (h),                                                    \
    (c),                                                    \
    (r),                                                    \
    (k),                                                    \
    { 0 }, 0, NULL

typedef struct
{
    nl_settings_key_t           mKey;
    nl_settings_value_t         *mDefaultValue;
    nl_settings_value_t         *mCurrentValue;
    uint32_t                    mFlags;
    nl_settings_change_event_t  *mSubscribers;
    unsigned int                mChangeCount;
} nl_settings_entry_t;

#define NLER_SETTINGS_ENTRY_FLAG_DEFAULT  0x0001

typedef void (*nl_settings_enumerator_t)(nl_settings_entry_t *aEntry, void *aClosure);
typedef int (*nl_settings_writer_t)(void *aData, int aDataLength, void *aClosure);

int nl_settings_init(nl_settings_value_t *aDefaults, int aNumDefaults, nl_settings_value_t *aValues, int aNumValues);

int nl_settings_get_value_as_value(nl_settings_key_t aKey, nl_settings_value_t aOutValue);
int nl_settings_get_value_as_int(nl_settings_key_t aKey, int32_t *aOutValue);

int nl_settings_set_value_to_default(nl_settings_key_t aKey);
int nl_settings_set_value_from_value(nl_settings_key_t aKey, const nl_settings_value_t aValue);
int nl_settings_set_value_from_int(nl_settings_key_t aKey, int32_t aValue);

int nl_settings_reset_to_defaults(void);

int nl_settings_write(nl_settings_writer_t aWriter, void *aClosure);

int nl_settings_subscribe_to_changes(nl_settings_change_event_t *aEvent);
int nl_settings_unsubscribe_from_changes(nl_settings_change_event_t *aEvent);

int nl_settings_is_valid(void);
int nl_settings_is_dirty(void);

/* be very careful when calling this. the entire
 * settings table may be locked until the enumeration
 * is complete. the last call to the enumerator
 * will pass NULL for aEntry.
 */

int nl_settings_enumerate(nl_settings_enumerator_t aEnumerator, void *aClosure);

#ifdef __cplusplus
}
#endif

#endif /* NL_ER_UTILITIES_SETTINGS_H */
