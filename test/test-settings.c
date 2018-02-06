/*
 *
 *    Copyright (c) 2015 Nest Labs, Inc.
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
 *      This file implements a unit test for the NLER settings
 *      interfaces.
 */

#include "nlertask.h"
#include "nlerinit.h"
#include <stdio.h>
#include "nlerlog.h"
#include "nlerassert.h"
#include "nlsettings.h"
#include "nlererror.h"

nl_task_t taskA;
nl_task_t taskB;
uint8_t stackA[NLER_TASK_STACK_BASE + 96];
uint8_t stackB[NLER_TASK_STACK_BASE + 96];

nl_settings_value_t values[nl_settings_keyMax];
nl_settings_value_t defaults[nl_settings_keyMax] = { "1", "2", "3", "4", "5", "6" };

void taskEntryA(void *aParams)
{
    nl_task_t   *curtask = nl_task_get_current();
    int         idx = 0;

    (void)curtask;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' (%08x, %d)\n", curtask->mName);

    while (1)
    {
        int oldvalue;
        int err;

        err = nl_settings_get_value_as_int(idx % nl_settings_keyMax, &oldvalue);

        if (err == NLER_SUCCESS)
        {
            err = nl_settings_set_value_from_int(idx % nl_settings_keyMax, (idx % nl_settings_keyMax) + oldvalue);

            if (err == NLER_SUCCESS)
            {
                NL_LOG_CRIT(lrTEST, "key: %d, oldvalue: %d\n", idx % nl_settings_keyMax, oldvalue);
            }
            else
            {
                NL_LOG_CRIT(lrTEST, "failed to update value for key: %d (err %d), to %d\n",
                            idx % nl_settings_keyMax, err, (idx % nl_settings_keyMax) + oldvalue);
            }
        }
        else
        {
            NL_LOG_CRIT(lrTEST, "failed to get value for key: %d (%d)\n", idx % nl_settings_keyMax, err);
        }

        idx++;
    }
}

int nl_settings_change_eventhandler(nl_event_t *aEvent, void *aClosure);

static nl_settings_change_event_t changekey =
{
    NL_INIT_SETTINGS_CHANGE_EVENT_STATIC(0, nl_settings_change_eventhandler, NULL, NULL, nl_settings_keyTest3)
};

static nl_settings_change_event_t changeall =
{
    NL_INIT_SETTINGS_CHANGE_EVENT_STATIC(0, nl_settings_change_eventhandler, NULL, NULL, nl_settings_keyInvalid)
};

int nl_settings_change_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    nl_settings_change_event_t  *event = (nl_settings_change_event_t *)aEvent;
    int                         err;

    (void)err;

    NL_LOG_CRIT(lrTEST, "got settings_change_event: key: %d, value: '%s', change count: %d\n", event->mKey, event->mNewValue, event->mChangeCount);

    if (event->mKey == nl_settings_keyInvalid)
    {
        err = nl_settings_subscribe_to_changes(&changeall);
    }
    else
    {
        err = nl_settings_subscribe_to_changes(&changekey);
    }

    return NLER_SUCCESS;
}

int writer(void *aData, int aDataLength, void *aClosure)
{
    NL_LOG_CRIT(lrTEST, "data: %p, length: %d, closure: %p\n", aData, aDataLength, aClosure);

    return NLER_SUCCESS;
}

void enumerator(nl_settings_entry_t *aEntry, void *aClosure)
{
    if (aEntry != NULL)
    {
        NL_LOG_CRIT(lrTEST, "enumerating: key %d, default: '%s', value: '%s', flags: %08x, change count: %d, subscribers %p\n",
                    aEntry->mKey, aEntry->mDefaultValue, aEntry->mCurrentValue, aEntry->mFlags, aEntry->mChangeCount, aEntry->mSubscribers);
    }
}

void taskEntryB(void *aParams)
{
    nl_task_t       *curtask = nl_task_get_current();
    int             idx = 0;
    nl_eventqueue_t queue = (nl_eventqueue_t)aParams;
    int             err;

    (void)curtask;
    (void)err;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' (%08x, %d)\n", curtask->mName);

    changekey.mReturnQueue = queue;
    changeall.mReturnQueue = queue;

    err = nl_settings_subscribe_to_changes(&changekey);

    NL_LOG_CRIT(lrTEST, "subscribe to key changes result: %d\n", err);

    err = nl_settings_subscribe_to_changes(&changeall);

    NL_LOG_CRIT(lrTEST, "subscribe to all changes result: %d\n", err);

    while (1)
    {
        nl_event_t          *ev;
        nl_settings_value_t value;

        ev = nl_eventqueue_get_event(queue);

        /* supplying NULL for the default event handler
         * is generally a pretty bold move because if there
         * were ever to be an event in the queue without
         * a handler specified in the event, dispatching would
         * crash. in this case there had only ever be
         * settings events in the queue, so it is "safe."
         */

        nl_dispatch_event(ev, NULL, NULL);

        nl_settings_get_value_as_value(idx % nl_settings_keyMax, value);

        NL_LOG_CRIT(lrTEST, "key: %d, value: %s\n", idx % nl_settings_keyMax, value);

        if ((idx % 100) == 0)
        {
            err = nl_settings_write(writer, NULL);

            NL_LOG_CRIT(lrTEST, "settings write result: %d\n", err);

            nl_settings_reset_to_defaults();

            nl_settings_enumerate(enumerator, NULL);
        }

        idx++;
    }
}

int main(int argc, char **argv)
{
    int             idx;
    int             retval;
    nl_event_t      *queuemem[50];
    nl_eventqueue_t queue;

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main (after initializing runtime)\n");

    queue = nl_eventqueue_create(queuemem, sizeof(queuemem));

    NL_LOG_CRIT(lrTEST, "event queue: %p\n", queue);

    NL_LOG_CRIT(lrTEST, "sizeof(values): %d, sizeof(defaults): %d\n", sizeof(values), sizeof(defaults));

    for (idx = 0; idx < nl_settings_keyMax; idx++)
    {
        NL_LOG_CRIT(lrTEST, "default[%d]: '%s'\n", idx, defaults[idx]);
    }

    retval = nl_settings_init(defaults, nl_settings_keyMax, values, nl_settings_keyMax);

    NL_LOG_CRIT(lrTEST, "settings initialized: %d\n", retval);

    if (retval == NLER_SUCCESS)
    {
        retval = nl_settings_reset_to_defaults();
    }

    NL_LOG_CRIT(lrTEST, "settings reset to defaults: %d\n", retval);

    if (retval == NLER_SUCCESS)
    {
        for (idx = 0; idx < nl_settings_keyMax; idx++)
        {
            NL_LOG_CRIT(lrTEST, "value[%d]: '%s'\n", idx, values[idx]);
        }
    }

    if (retval == NLER_SUCCESS)
    {
        nl_task_create(taskEntryA, "A", stackA, sizeof(stackA), 10, NULL, &taskA);
        nl_task_create(taskEntryB, "B", stackB, sizeof(stackB), 10, queue, &taskB);

        nl_er_start_running();
    }

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return 0;
}

