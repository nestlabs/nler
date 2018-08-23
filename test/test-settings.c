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

#ifdef nlLOG_PRIORITY
#undef nlLOG_PRIORITY
#endif
#define nlLOG_PRIORITY 3

#include <nlsettings.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nlerassert.h>
#include <nlereventqueue.h>
#include <nlererror.h>
#include <nlerinit.h>
#include <nlerlog.h>
#include <nlertask.h>

/*
 * Preprocessor Defitions
 */

#define kTHREAD_MAIN_SLEEP_MS      241

/*
 * Type Definitions
 */

typedef struct taskData_s
{
    nleventqueue_t          *mQueue;
    bool                     mFailed;
    bool                     mSucceeded;
} taskData_t;

/*
 * Forward Declarations
 */

static int nl_settings_change_eventhandler(nl_event_t *aEvent, void *aClosure);

/*
 * Global Variables
 */

static const char * const kTaskNameA = "A";
static const char * const kTaskNameB = "B";

static nltask_t sTaskA;
static nltask_t sTaskB;
static DEFINE_STACK(sStackA, NLER_TASK_STACK_BASE + 96);
static DEFINE_STACK(sStackB, NLER_TASK_STACK_BASE + 96);

static nl_settings_value_t sValues[nl_settings_keyMax];
static nl_settings_value_t sDefaults[nl_settings_keyMax] = { "1", "2", "3", "4", "5", "6" };

static nl_settings_change_event_t sChangeKey =
{
    NL_INIT_SETTINGS_CHANGE_EVENT_STATIC(0, nl_settings_change_eventhandler, NULL, NULL, nl_settings_keyTest3)
};

static nl_settings_change_event_t sChangeAll =
{
    NL_INIT_SETTINGS_CHANGE_EVENT_STATIC(0, nl_settings_change_eventhandler, NULL, NULL, nl_settings_keyInvalid)
};

static bool task_is_testing(volatile const taskData_t *aData)
{
    bool retval;

    retval = (!aData->mFailed && !aData->mSucceeded);

    return (retval);
}

static void taskEntryA(void *aParams)
{
    nltask_t                 *curtask = nltask_get_current();
    const char               *name = nltask_get_name(curtask);
    volatile taskData_t      *data = (volatile taskData_t *)aParams;
    int                       idx = 0;

    (void)name;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' (%p)\n", name, aParams);

    while (task_is_testing(data))
    {
        int oldvalue;
        int status;

        status = nl_settings_get_value_as_int(idx % nl_settings_keyMax, &oldvalue);
        NL_LOG_CRIT(lrTEST, "get value result: %d\n", status);
        NLER_ASSERT(status == NLER_SUCCESS);

        status = nl_settings_set_value_from_int(idx % nl_settings_keyMax, (idx % nl_settings_keyMax) + oldvalue);
        NL_LOG_CRIT(lrTEST, "set value result: %d\n", status);
        NLER_ASSERT(status == NLER_SUCCESS);

        idx++;
    }

    NL_LOG_CRIT(lrTEST, "'%s' exiting\n", name);
}

static int nl_settings_change_eventhandler(nl_event_t *aEvent, void *aClosure)
{
    nl_settings_change_event_t  *event = (nl_settings_change_event_t *)aEvent;
    int                         retval = NLER_SUCCESS;

    NL_LOG_CRIT(lrTEST, "got settings_change_event: key: %d, value: '%s', change count: %d\n", event->mKey, event->mNewValue, event->mChangeCount);

    if (event->mKey == nl_settings_keyInvalid)
    {
        retval = nl_settings_subscribe_to_changes(&sChangeAll);
    }
    else
    {
        retval = nl_settings_subscribe_to_changes(&sChangeKey);
    }

    return (retval);
}

static int writer(void *aData, int aDataLength, void *aClosure)
{
    NL_LOG_CRIT(lrTEST, "data: %p, length: %d, closure: %p\n", aData, aDataLength, aClosure);

    return NLER_SUCCESS;
}

static void enumerator(nl_settings_entry_t *aEntry, void *aClosure)
{
    if (aEntry != NULL)
    {
        NL_LOG_CRIT(lrTEST, "enumerating: key %d, default: '%s', value: '%s', flags: %08x, change count: %d, subscribers %p\n",
                    aEntry->mKey, aEntry->mDefaultValue, aEntry->mCurrentValue, aEntry->mFlags, aEntry->mChangeCount, aEntry->mSubscribers);
    }
}

static void taskEntryB(void *aParams)
{
    nltask_t                 *curtask = nltask_get_current();
    const char               *name = nltask_get_name(curtask);
    volatile taskData_t      *data = (volatile taskData_t *)aParams;
    int                       idx = 0;
    int                       err;

    (void)name;
    (void)err;

    NL_LOG_CRIT(lrTEST, "from the task: '%s' (%p)\n", name, aParams);

    sChangeKey.mReturnQueue = data->mQueue;
    sChangeAll.mReturnQueue = data->mQueue;

    err = nl_settings_subscribe_to_changes(&sChangeKey);
    NL_LOG_DEBUG(lrTEST, "subscribe to key changes result: %d\n", err);
    NLER_ASSERT(err == NLER_SUCCESS);

    err = nl_settings_subscribe_to_changes(&sChangeAll);
    NL_LOG_DEBUG(lrTEST, "subscribe to all changes result: %d\n", err);
    NLER_ASSERT(err == NLER_SUCCESS);

    while (task_is_testing(data))
    {
        nl_event_t          *ev;
        nl_settings_value_t value;

        ev = nleventqueue_get_event(data->mQueue);

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

    NL_LOG_CRIT(lrTEST, "'%s' exiting\n", name);
}

static bool is_testing(volatile const taskData_t *aSubscriber,
                       volatile const taskData_t *aPublisher)
{
    bool retval = false;

    if ((!aSubscriber->mFailed && !aPublisher->mFailed) &&
        (!aSubscriber->mSucceeded && !aPublisher->mSucceeded))
    {
        retval = true;
    }

    return retval;
}

static bool was_successful(volatile const taskData_t *aSubscriber,
                           volatile const taskData_t *aPublisher)
{
    bool retval = false;

    if (aSubscriber->mFailed || aPublisher->mFailed)
    {
        retval = false;
    }
    else if (aSubscriber->mSucceeded && aPublisher->mSucceeded)
    {
        retval = true;
    }

    return retval;
}

static bool test_unthreaded(void)
{
    int   idx;
    int   status;
    bool  retval = true;

    NLER_STATIC_ASSERT(sizeof (sDefaults) == nl_settings_keyMax * sizeof (nl_settings_value_t), "unexpected default settings size");
    NLER_STATIC_ASSERT(sizeof (sValues) == nl_settings_keyMax * sizeof (nl_settings_value_t), "unexpected value settings size");

    NL_LOG_DEBUG(lrTEST, "sizeof (sValues): %d, sizeof (sDefaults): %d\n",
                 sizeof (sValues),
                 sizeof (sDefaults));

    for (idx = 0; idx < nl_settings_keyMax; idx++)
    {
        const char buffer[2] = { idx + 0x31, '\0' };

        status = strcmp(sDefaults[idx], buffer);
        NL_LOG_CRIT(lrTEST, "sDefaults[%d]: '%s'\n", idx, sDefaults[idx]);
        NLER_ASSERT(status == 0);
    }

    status = nl_settings_init(sDefaults, nl_settings_keyMax, sValues, nl_settings_keyMax);
    NL_LOG_CRIT(lrTEST, "settings initialized: %d\n", retval);
    NLER_ASSERT(status == NLER_SUCCESS);

    status = nl_settings_reset_to_defaults();
    NL_LOG_CRIT(lrTEST, "settings reset to defaults: %d\n", retval);
    NLER_ASSERT(status == NLER_SUCCESS);

    for (idx = 0; idx < nl_settings_keyMax; idx++)
    {
        const char buffer[1] = { '\0' };

        status = strcmp(sValues[idx], buffer);
        NL_LOG_CRIT(lrTEST, "sValues[%d]: '%s'\n", idx, sValues[idx]);
        NLER_ASSERT(status == 0);
    }

    return (retval);
}

static bool test_threaded(void)
{
    nl_event_t      *lQueueMem[50];
    nleventqueue_t   lQueue;
    taskData_t       taskDataA;
    taskData_t       taskDataB;
    int              status;
    bool             retval;

    status = nleventqueue_create(lQueueMem, sizeof(lQueueMem), &lQueue);
    NLER_ASSERT(status == NLER_SUCCESS);

    NL_LOG_DEBUG(lrTEST, "event queue: %p\n", lQueue);

    taskDataA.mQueue     = NULL;
    taskDataA.mFailed    = false;
    taskDataA.mSucceeded = false;

    taskDataB.mQueue     = &lQueue;
    taskDataB.mFailed    = false;
    taskDataB.mSucceeded = false;

    nltask_create(taskEntryA, kTaskNameA, sStackA, sizeof(sStackA), 10, &taskDataA, &sTaskA);
    nltask_create(taskEntryB, kTaskNameB, sStackB, sizeof(sStackB), 10, &taskDataB, &sTaskB);

    while (is_testing(&taskDataA, &taskDataB))
    {
        nltask_sleep_ms(kTHREAD_MAIN_SLEEP_MS);
    }

    retval = was_successful(&taskDataA, &taskDataB);

    nleventqueue_destroy(&lQueue);

    return (retval);
}

bool nler_settings_test(void)
{
    bool  retval;

    retval = test_unthreaded();

    if (retval == true)
    {
        retval = test_threaded();
    }

    return (retval);
}

int main(int argc, char **argv)
{
    bool status;

    nl_er_init();

    NL_LOG_CRIT(lrTEST, "start main\n");

    nl_er_start_running();

    status = nler_settings_test();

    nl_er_cleanup();

    NL_LOG_CRIT(lrTEST, "end main\n");

    return (status ? EXIT_SUCCESS : EXIT_FAILURE);
}
