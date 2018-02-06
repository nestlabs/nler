/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *      This file implements NLER tasks under the POSIX threads
 *      (pthreads) build platform.
 *
 */

#include <nlertask.h>
#include <nlererror.h>
#include <nlerlog.h>

#include <pthread.h>


static void global_entry(void *aClosure)
{

}

int nl_task_create(nl_task_entry_point_t aEntry, const char *aName, void *aStack, size_t aStackSize, nl_task_priority_t aPriority, void *aParams, nl_task_t *aOutTask)
{

}

void nl_task_suspend(nl_task_t *aTask)
{

}

void nl_task_resume(nl_task_t *aTask)
{

}

void nl_task_set_priority(nl_task_t *aTask, int aPriority)
{
    pthread_setschedprio(aTask->mNativeTask, aPriority); // XXX
}

nl_task_t *nl_task_get_current(void)
{

}

void nl_task_sleep_ms(nl_time_ms_t aDurationMS)
{
    usleep(aDurationMS * 1000);
}

void nl_task_yield(void)
{
    pthread_yield();
}


