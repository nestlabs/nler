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
 *    @file
 *      This file implements NLER Utilities ID->value store interfaces.
 *
 */

#include <nlsettings.h>

#include <stdlib.h>
#include <string.h>

#include <nlererror.h>
#include <nlerlock.h>
#include <nlerlog.h>

typedef struct
{
    nl_settings_entry_t         *mSettings;
    uint32_t                    mFlags;
    nllock_t                    mLock;
    nl_settings_change_event_t  *mSubscribers;
    unsigned int                mChangeCount;
    void                        *aValueStore;
    int                         aValueStoreSize;
} nl_settings_t;

#define NLER_SETTINGS_FLAG_VALID  0x0001
#define NLER_SETTINGS_FLAG_DIRTY  0x0002

static nl_settings_entry_t sSettingsEntries[nl_settings_keyMax];

static nl_settings_t sSettings =
{
    sSettingsEntries,
    0,
    NLLOCK_INITIALIZER,
    NULL,
    0,
    NULL,
    0
};

static nl_settings_entry_t *nl_settings_get_entry(nl_settings_key_t aKey)
{
    nl_settings_entry_t *retval = NULL;

    if ((aKey >= 0) && (aKey < nl_settings_keyMax))
    {
        retval = &sSettings.mSettings[aKey];
    }

    return retval;
}

int nl_settings_init(nl_settings_value_t *aDefaults, int aNumDefaults, nl_settings_value_t *aValues, int aNumValues)
{
    int retval = NLER_SUCCESS;

    if (aNumDefaults != aNumValues)
    {
        retval = NLER_ERROR_BAD_INPUT;
    }

    if ((retval == NLER_SUCCESS) && ((sSettings.mFlags & NLER_SETTINGS_FLAG_VALID) == 0))
    {
        int idx;

        sSettings.aValueStore = aValues;

        for (idx = 0; idx < nl_settings_keyMax; idx++)
        {
            if ((idx >= aNumDefaults) || (idx >= aNumValues))
            {
                retval = NLER_ERROR_BAD_INPUT;
                break;
            }

            sSettingsEntries[idx].mKey = idx;
            sSettingsEntries[idx].mDefaultValue = aDefaults++;
            sSettingsEntries[idx].mCurrentValue = aValues++;

            if (strcmp((char *)(sSettingsEntries[idx].mDefaultValue), (char *)(sSettingsEntries[idx].mCurrentValue)) == 0)
            {
                sSettingsEntries[idx].mFlags = NLER_SETTINGS_ENTRY_FLAG_DEFAULT;
            }
            else
            {
                sSettingsEntries[idx].mFlags = 0;
            }

            sSettingsEntries[idx].mSubscribers = NULL;
            sSettingsEntries[idx].mChangeCount = 0;
        }

        if (retval == NLER_SUCCESS)
        {
            retval = nllock_create(&sSettings.mLock);

            if (retval == NLER_SUCCESS)
            {
                sSettings.mSettings = sSettingsEntries;
                sSettings.mFlags = NLER_SETTINGS_FLAG_VALID;
                sSettings.mSubscribers = NULL;
                sSettings.mChangeCount = 0;
                sSettings.aValueStoreSize = aNumValues * sizeof(nl_settings_value_t);
            }
            else
            {
                retval = NLER_ERROR_NO_RESOURCE;
            }
        }
    }
    else
    {
        retval = NLER_ERROR_BAD_STATE;
    }

    return retval;
}

static int nl_settings_notify_subscriber(nl_settings_value_t aValue,
                                         nl_settings_change_event_t *aSubscriber,
                                         unsigned int aChangeCount)
{
    if (aValue != NULL)
    {
        memcpy(aSubscriber->mNewValue, aValue, sizeof(nl_settings_value_t));
    }

    aSubscriber->mChangeCount = aChangeCount;

    return nleventqueue_post_event(aSubscriber->mReturnQueue, (nl_event_t *)aSubscriber);
}

static int nl_settings_notify_subscriber_chain(nl_settings_value_t aValue,
                                               nl_settings_change_event_t **aHead,
                                               nl_settings_change_event_t *aSubscribers,
                                               unsigned int aChangeCount)
{
    int retval = NLER_SUCCESS;

    while (aSubscribers != NULL)
    {
        retval = nl_settings_notify_subscriber(aValue, aSubscribers, aChangeCount);

        if (retval == NLER_SUCCESS)
        {
            /* modifying *aHead will unlink the subscriber from
             * the list of subscribers (unsubscribe). however, mChain
             * is not set to NULL.
             */

            aSubscribers = *aHead = aSubscribers->mChain;
        }
        else
        {
            break;
        }
    }

    return retval;
}

static int nl_settings_notify_subscribers(nl_settings_entry_t *aEntry)
{
    int retval;

    if (aEntry != NULL)
    {
        retval = nl_settings_notify_subscriber_chain(*aEntry->mCurrentValue,
                                                     &aEntry->mSubscribers,
                                                     aEntry->mSubscribers,
                                                     aEntry->mChangeCount);
    }
    else
    {
        retval = nl_settings_notify_subscriber_chain(NULL,
                                                     &sSettings.mSubscribers,
                                                     sSettings.mSubscribers,
                                                     sSettings.mChangeCount);
    }

    return retval;
}

int nl_settings_get_value_as_value(nl_settings_key_t aKey, nl_settings_value_t aOutValue)
{
    int                 retval = NLER_ERROR_BAD_INPUT;
    nl_settings_entry_t *entry;

    nllock_enter(&sSettings.mLock);

    entry = nl_settings_get_entry(aKey);

    if (entry != NULL)
    {
        memcpy(aOutValue, entry->mCurrentValue, sizeof(nl_settings_value_t));
        retval = NLER_SUCCESS;
    }

    nllock_exit(&sSettings.mLock);

    return retval;
}

int nl_settings_get_value_as_int(nl_settings_key_t aKey, int32_t *aOutValue)
{
    int                 retval = NLER_ERROR_BAD_INPUT;
    nl_settings_entry_t *entry;

    nllock_enter(&sSettings.mLock);

    entry = nl_settings_get_entry(aKey);

    if (entry != NULL)
    {
        int32_t intval;
        char    *next;

        intval = strtol((char *)entry->mCurrentValue, &next, 10);

        if (next != (char *)entry->mCurrentValue)
        {
            *aOutValue = intval;
            retval = NLER_SUCCESS;
        }
        else
        {
            retval = NLER_ERROR_NO_RESOURCE;
        }
    }

    nllock_exit(&sSettings.mLock);

    return retval;
}

static void nl_check_for_default(nl_settings_entry_t *aEntry)
{
    if (strcmp((char *)aEntry->mDefaultValue, (char *)aEntry->mCurrentValue) == 0)
    {
        aEntry->mFlags |= NLER_SETTINGS_ENTRY_FLAG_DEFAULT;
    }
    else
    {
        aEntry->mFlags &= ~NLER_SETTINGS_ENTRY_FLAG_DEFAULT;
    }
}

static void nl_settings_effect_change(nl_settings_entry_t *aEntry, const nl_settings_value_t aNewValue)
{
    memcpy(aEntry->mCurrentValue, aNewValue, sizeof(*aEntry->mCurrentValue));

    nl_check_for_default(aEntry);

    aEntry->mChangeCount++;
    sSettings.mChangeCount++;

    sSettings.mFlags |= NLER_SETTINGS_FLAG_DIRTY;
}

static int nl_settings_copy_default_to_value(nl_settings_entry_t *aEntry)
{
    int retval = 0;

    if (!(aEntry->mFlags & NLER_SETTINGS_ENTRY_FLAG_DEFAULT))
    {
        nl_settings_effect_change(aEntry, *aEntry->mDefaultValue);
        retval = 1;
    }

    return retval;
}

int nl_settings_set_value_to_default(nl_settings_key_t aKey)
{
    int                 retval = NLER_ERROR_BAD_INPUT;
    nl_settings_entry_t *entry;
    int                 changed = 0;

    nllock_enter(&sSettings.mLock);

    entry = nl_settings_get_entry(aKey);

    if (entry != NULL)
    {
        changed = nl_settings_copy_default_to_value(entry);
        retval = NLER_SUCCESS;
    }

    if (changed)
    {
        nl_settings_notify_subscribers(entry);
        nl_settings_notify_subscribers(NULL);
    }

    nllock_exit(&sSettings.mLock);

    return retval;
}

int nl_settings_set_value_from_value(nl_settings_key_t aKey, const nl_settings_value_t aValue)
{
    int                 retval = NLER_ERROR_BAD_INPUT;
    nl_settings_entry_t *entry;

    nllock_enter(&sSettings.mLock);

    entry = nl_settings_get_entry(aKey);

    if (entry != NULL)
    {
        if (strcmp((char *)entry->mCurrentValue, (char *)aValue) != 0)
        {
            nl_settings_effect_change(entry, aValue);

            nl_settings_notify_subscribers(entry);
            nl_settings_notify_subscribers(NULL);
        }

        retval = NLER_SUCCESS;
    }

    nllock_exit(&sSettings.mLock);

    return retval;
}

static const uint32_t p10[10] =
{
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
};

static int sprintf_unsigned(char *buffer, int buflen, uint32_t u)
{
    int len = 0;
    uint32_t i;
    uint32_t d;
    uint32_t o = u;

    i = 1;
    while (i < sizeof(p10)/sizeof(uint32_t) && u >= p10[i])
        i++;
    while ((i > 0) && (buflen > 0))
    {
        i--;
        d = (i == 0 ? u : u / p10[i]);
        *buffer++ = (d + '0');
        len++;
        u -= d*p10[i];
        buflen--;
    }

    if (i > 0)
    {
        NL_LOG_CRIT(lrER, "out of space in settings value for int: %u\n", o);
        ((void)(o)); // to avoid "unused variable" warning
    }

    return len;
}

static int sprintf_decimal(char *buffer, int buflen, int v)
{
    int len = 0;
    uint32_t u = 0;

    if (buflen >= 1)
    {
        if (v >= 0)
        {
            u = v;
        }
        else
        {
            *buffer++ = '-';
            len++;
            u = -v;
            buflen--;
        }
    }

    return len + sprintf_unsigned(buffer, buflen, u);
}

int nl_settings_set_value_from_int(nl_settings_key_t aKey, int32_t aValue)
{
    int                 retval = NLER_ERROR_BAD_INPUT;
    nl_settings_entry_t *entry;

    nllock_enter(&sSettings.mLock);

    entry = nl_settings_get_entry(aKey);

    if (entry != NULL)
    {
        int                 len;
        nl_settings_value_t newval;

        len = sprintf_decimal(newval, sizeof(*entry->mCurrentValue) - 1, aValue);

        newval[len] = '\0';

        if (strcmp(newval, (char *)entry->mCurrentValue) != 0)
        {
            nl_settings_effect_change(entry, newval);

            nl_settings_notify_subscribers(entry);
            nl_settings_notify_subscribers(NULL);
        }

        retval = NLER_SUCCESS;
    }

    nllock_exit(&sSettings.mLock);

    return retval;
}

int nl_settings_reset_to_defaults(void)
{
    int retval = NLER_SUCCESS;
    int idx;
    int changed = 0;

    nllock_enter(&sSettings.mLock);

    for (idx = 0; idx < nl_settings_keyMax; idx++)
    {
        nl_settings_entry_t *entry;

        entry = nl_settings_get_entry(idx);

        if (entry != NULL)
        {
            int thischanged = nl_settings_copy_default_to_value(entry);

            if (thischanged)
            {
                nl_settings_notify_subscribers(entry);
                changed = 1;
            }
        }
        else
        {
            retval = NLER_ERROR_BAD_STATE;
            break;
        }
    }

    if (changed)
        nl_settings_notify_subscribers(NULL);

    nllock_exit(&sSettings.mLock);

    return retval;
}

int nl_settings_write(nl_settings_writer_t aWriter, void *aClosure)
{
    int retval = NLER_SUCCESS;

    nllock_enter(&sSettings.mLock);

    if (sSettings.mFlags & NLER_SETTINGS_FLAG_DIRTY)
    {
        retval = (*aWriter)(sSettings.aValueStore, sSettings.aValueStoreSize, aClosure);

        if (retval == NLER_SUCCESS)
        {
            sSettings.mFlags &= ~NLER_SETTINGS_FLAG_DIRTY;
        }
    }

    nllock_exit(&sSettings.mLock);

    return retval;
}

static nl_settings_change_event_t *nl_settings_find_subscriber(nl_settings_change_event_t **aSubscribers, nl_settings_change_event_t *aEvent)
{
    while ((*aSubscribers != NULL) && (*aSubscribers != aEvent))
    {
        aSubscribers = (nl_settings_change_event_t **)&((nl_settings_change_event_t *)*aSubscribers)->mChain;
    }

    return *aSubscribers;
}

int nl_settings_subscribe_to_changes(nl_settings_change_event_t *aEvent)
{
    int retval = NLER_ERROR_BAD_INPUT;

    nllock_enter(&sSettings.mLock);

    /* need to check for subscriber already in list */

    if (aEvent->mKey != nl_settings_keyInvalid)
    {
        nl_settings_entry_t *entry;

        entry = nl_settings_get_entry(aEvent->mKey);

        if (entry != NULL)
        {
            if (entry->mChangeCount != aEvent->mChangeCount)
            {
                retval = nl_settings_notify_subscriber(*entry->mCurrentValue, aEvent, entry->mChangeCount);
            }
            else
            {
                nl_settings_change_event_t *search;

                search = nl_settings_find_subscriber(&entry->mSubscribers, aEvent);

                if (search == NULL)
                {
                    aEvent->mChain = entry->mSubscribers;
                    entry->mSubscribers = aEvent;
                }

                retval = NLER_SUCCESS;
            }
        }
    }
    else
    {
        if (sSettings.mChangeCount != aEvent->mChangeCount)
        {
            retval = nl_settings_notify_subscriber(NULL, aEvent, sSettings.mChangeCount);
        }
        else
        {
            nl_settings_change_event_t *search;

            search = nl_settings_find_subscriber(&sSettings.mSubscribers, aEvent);

            if (search == NULL)
            {
                aEvent->mChain = sSettings.mSubscribers;
                sSettings.mSubscribers = aEvent;
            }

            retval = NLER_SUCCESS;
        }
    }

    nllock_exit(&sSettings.mLock);

    return retval;
}

static void nl_settings_unsubscribe(nl_settings_change_event_t **aSubscribers, nl_settings_change_event_t *aEvent)
{
    while ((*aSubscribers != NULL) && (*aSubscribers != aEvent))
    {
        aSubscribers = (nl_settings_change_event_t **)&((nl_settings_change_event_t *)*aSubscribers)->mChain;
    }

    if (*aSubscribers != NULL)
    {
        *aSubscribers = aEvent->mChain;
        aEvent->mChain = NULL;
    }
}

int nl_settings_unsubscribe_from_changes(nl_settings_change_event_t *aEvent)
{
    int retval = NLER_SUCCESS;

    nllock_enter(&sSettings.mLock);

    if (aEvent->mKey != nl_settings_keyInvalid)
    {
        nl_settings_entry_t *entry;

        entry = nl_settings_get_entry(aEvent->mKey);

        if (entry != NULL)
        {
            nl_settings_unsubscribe(&entry->mSubscribers, aEvent);
        }
        else
        {
            retval = NLER_ERROR_NO_RESOURCE;
        }
    }
    else
    {
        nl_settings_unsubscribe(&sSettings.mSubscribers, aEvent);
    }

    nllock_exit(&sSettings.mLock);

    return retval;
}

int nl_settings_is_valid(void)
{
    return (sSettings.mFlags & NLER_SETTINGS_FLAG_VALID) != 0;
}

int nl_settings_is_dirty(void)
{
    return (sSettings.mFlags & NLER_SETTINGS_FLAG_DIRTY) != 0;
}

int nl_settings_enumerate(nl_settings_enumerator_t aEnumerator, void *aClosure)
{
    int retval = NLER_SUCCESS;
    int idx;

    nllock_enter(&sSettings.mLock);

    for (idx = 0; idx < nl_settings_keyMax; idx++)
    {
        nl_settings_entry_t *entry;

        entry = nl_settings_get_entry(idx);

        if (entry != NULL)
        {
            (aEnumerator)(entry, aClosure);
        }
        else
        {
            retval = NLER_ERROR_NO_RESOURCE;
            break;
        }
    }

    (aEnumerator)(NULL, aClosure);

    nllock_exit(&sSettings.mLock);

    return retval;
}

