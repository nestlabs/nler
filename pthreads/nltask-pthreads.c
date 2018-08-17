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
 *      @note
 *        Even though the NLER task API accepts a user-specified
 *        stack, this implementation does not use it and instead
 *        relies on pthreads to allocate a stack of the user-specified
 *        size because stacks allocated off the heap generally do not
 *        work on non-embedded pthreads systems.
 *
 */

#define _GNU_SOURCE

#include "nler-config.h"

#include <nlerassert.h>
#include <nlertask.h>
#include <nlererror.h>
#include <nlerlog.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 * Preprocessor Definitions
 */

#define _nlMASK(aSize)               (~((aSize) - 1))
#define _nlALIGN_UP(aValue, aSize)   (((aValue) + ((aSize) - 1)) & _nlMASK(aSize))
#define _nlALIGN_DOWN(aValue, aSize) ((aValue) & _nlMASK(aSize))

#define _NLER_TASK_PTHREADS_PRIORITY_INVALID -1

/*
 * Type Defintions
 */

/**
 *  Scheduler policy-specific priority limit indices.
 */
enum
{
    kSchedOther      = 0,  /**< Index into SCHED_OTHER priority limits */
    kSchedRoundRobin = 1,  /**< Index into SCHED_RR priority limits */ 

    kSchedPoliciesMax,
};

/**
 *  Minimum and maximum scheduler priority to a particular scheduler
 *  policy.
 */
typedef struct nltask_pthreads_prio_limits_s
{
    int                            mPriorityMinimum; /**< minimum priority */
    int                            mPriorityMaximum; /**< maximum priority */
} nltask_pthreads_prio_limits_t;

/**
 *  Global state for maintaining thread local storage and
 *  scheduler-policy specific priority limits for priority mapping.
 */
typedef struct nltask_globals_pthreads_s
{
    size_t                         mPageSize;
    pthread_key_t                  mThreadLocalStorageKey;
    nltask_pthreads_prio_limits_t  mPriorityLimits[kSchedPoliciesMax];
} nltask_globals_pthreads_t;

/*
 * Forward Declarations
 */
extern int nltask_pthreads_init(void);
extern void nltask_pthreads_destroy(void);

/*
 * Global Variables
 */

/**
 *  Global, somewhat "faked" task structure for the main, parent
 *  thread to ensure that nltask_get_current(), etc. work correctly.
 */
static nltask_t sMainTask;

/**
 *  File scope global state for maintaining thread local storage and
 *  scheduler-policy specific priority limits for priority mapping.
 */
static nltask_globals_pthreads_t sGlobals;

/**
 *  Scheduler policy preferences
 *
 *  We'll try to allocate threads, using this policy order, until
 *  successful.
 *
 *  Since it most closely effects the scheduling policy used by most
 *  embedded, RTOS systems, prefer the scheduler policy SCHED_RR
 *  first. If that fails due to lack of support or permission, we'll
 *  fall back to SCHED_OTHER.
 */
static const int sSchedPolicies[kSchedPoliciesMax] = { SCHED_RR, SCHED_OTHER };

/**
 *  Get scheduler priority limits for the specified scheduler policy.
 *
 *  @param[in]     aPolicy  the scheduler policy to get priority limits for.
 *
 *  @param[inout]  aLimits  a pointer to storage for minimum and maximum
 *                          scheduler priorities for the given policy.
 *
 *  @retval  #NLER_SUCCESS          on success.
 *  @retval  #NLER_ERROR_BAD_INPUT  if the scheduler policy is not SCHED_OTHE
 *                                  or SCHED_RR.
 *  @retval  #NLER_ERROR_FAILURE    on all other errors.
 *
 */
static int nltask_pthreads_get_prio_limits(int aPolicy, nltask_pthreads_prio_limits_t *aLimits)
{
    int retval = NLER_SUCCESS;
    int max_priority, min_priority;

    if ((aPolicy != SCHED_OTHER) && (aPolicy != SCHED_RR))
    {
        retval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    max_priority = sched_get_priority_max(aPolicy);
    if (max_priority == _NLER_TASK_PTHREADS_PRIORITY_INVALID)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    min_priority = sched_get_priority_min(aPolicy);
    if (min_priority == _NLER_TASK_PTHREADS_PRIORITY_INVALID)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    aLimits->mPriorityMinimum = min_priority;
    aLimits->mPriorityMaximum = max_priority;

 done:
    return (retval);
}

/**
 *  Initialize scheduler policy-specific priority limits.
 *
 *  @retval  #NLER_SUCCESS          on success.
 *  @retval  #NLER_ERROR_FAILURE    on all other errors.
 *
 */
static int nltask_pthreads_prio_init(void)
{
    int retval = NLER_SUCCESS;

    retval = nltask_pthreads_get_prio_limits(SCHED_OTHER, &sGlobals.mPriorityLimits[kSchedOther]);
    if (retval != NLER_SUCCESS)
    {
        goto done;
    }

    retval = nltask_pthreads_get_prio_limits(SCHED_RR, &sGlobals.mPriorityLimits[kSchedRoundRobin]);
    if (retval != NLER_SUCCESS)
    {
        goto done;
    }

 done:
    return (retval);
}

/**
 *  Map the specified NLER task priority to an implementation and
 *  scheduler policy-specific priority.
 *
 *  @param[in]  aPolicy     The system scheduler policy by which to map the
 *                          priority
 *
 *  @param[in]  aPriority   The NLER task priority to map.
 *
 *  @return the mapped priority on success; otherwise
 *          #_NLER_TASK_PTHREADS_PRIORITY_INVALID on failure.
 *
 */
static int nltask_pthreads_map_priority_to_native(int aPolicy, nltask_priority_t aPriority)
{
    int                                   retval = _NLER_TASK_PTHREADS_PRIORITY_INVALID;
    const nltask_pthreads_prio_limits_t *prio_limits = NULL;

    /* Attempt to find an entry in the global mapping table based on
     * the supported scheduler policies.
     */

    switch (aPolicy)
    {
    case SCHED_OTHER:
        prio_limits = &sGlobals.mPriorityLimits[kSchedOther];
        break;

    case SCHED_RR:
        prio_limits = &sGlobals.mPriorityLimits[kSchedRoundRobin];
        break;

    default:
        break;
    }

    /* If we successfully found a mapping table, map the priority.
     */
    
    if (prio_limits != NULL)
    {
        retval = prio_limits->mPriorityMinimum +
            ((aPriority * (prio_limits->mPriorityMaximum -
                           prio_limits->mPriorityMinimum))
             / NLER_TASK_PRIORITY_HIGHEST);
    }
        
    return (retval);
}

/**
 *  Estabish a "faked" but sane task structure for the main thread
 *  such that functions such as nltask_get_current() work correctly
 *  when called from it.
 *
 *  @param[out]  aOutTask  A pointer to the task structure to populate
 *                         for use with the main thread.
 *
 *  @retval  #NLER_SUCCESS          on success.
 *  @retval  #NLER_ERROR_FAILURE    on all other errors.
 */
static int nltask_pthreads_main_init(nltask_t *aOutTask)
{
    const pthread_t    lThread = pthread_self();
    int                retval = NLER_SUCCESS;
    int                status;

    aOutTask->mStackTop              = 0;

    aOutTask->mNativeTaskObj.mEntry  = NULL;
    aOutTask->mNativeTaskObj.mParams = NULL;
    aOutTask->mNativeTaskObj.mName   = NULL;
    aOutTask->mNativeTaskObj.mThread = lThread;

    status = pthread_setspecific(sGlobals.mThreadLocalStorageKey, aOutTask);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

 done:
    return (retval);
}

/**
 *  Initialize POSIX threads (pthreads) task support.
 *
 *  @retval  #NLER_SUCCESS          on success.
 *  @retval  #NLER_ERROR_FAILURE    on all other errors.
 *
 */
int nltask_pthreads_init(void)
{
    int  retval = NLER_SUCCESS;
    int  status;
    long page_size;

    /* Determine the system page size required for aligning
     * user-specified stacks.
     */

    page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    sGlobals.mPageSize = page_size;

    /* Initialize runtime scheduler priorities used for priority mapping.
     */

    retval = nltask_pthreads_prio_init();
    if (retval != NLER_SUCCESS)
    {
        goto done;
    }

    /* Establish a key to use for storing the thread context data in
     * thread local storage.
     */

    status = pthread_key_create(&sGlobals.mThreadLocalStorageKey, NULL);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    /* Initialize the "faked" main thread task structure
     */

    retval = nltask_pthreads_main_init(&sMainTask);
    if (retval != NLER_SUCCESS)
    {
        goto done;
    }

 done:
    return (retval);
}

/**
 *  De-initialize POSIX threads (pthreads) task support.
 *
 */
void nltask_pthreads_destroy(void)
{
    pthread_key_delete(sGlobals.mThreadLocalStorageKey);
}

/**
 *  Check task-creation arguments for validity.
 *
 *  @param[in]  aEntry      A pointer to the function entry point for the task.
 *
 *  @param[in]  aName       A non-NULL pointer to a NULL-terminated C string
 *                          of the task name.
 *
 *  @param[in]  aStack      A pointer to the task stack (unused).
 *
 *  @param[in]  aStackSize  The size, in bytes, of the task stack, which must
 *                          be NLER_REQUIRED_STACK_ALIGNMENT aligned, at
 *                          minimum, and larger than PTHREAD_STACK_MIN.
 *
 *  @param[in]  aPriority   The task priority.
 *
 *  @param[in]  aParams     The initial parameters passed to the task entry
 *                          point.
 *
 *  @param[in]  aOutTask    A non-NULL pointer to the task structure.
 *
 *  @retval  #NLER_SUCCESS          on success.
 *  @retval  #NLER_ERROR_BAD_INPUT  if any of the supplied arugments are
 *                                  invalid.
 *  @retval  #NLER_ERROR_FAILURE    on all other errors.
 *
 */
static int nltask_pthreads_check_args(nltask_entry_point_t aEntry, const char *aName, const void *aStack, size_t aStackSize, nltask_priority_t aPriority, void *aParams, const nltask_t *aOutTask)
{
    int retval = NLER_SUCCESS;

    if (aOutTask == NULL)
    {
        retval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    if (aName == NULL)
    {
        retval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    if (aStackSize % NLER_REQUIRED_STACK_ALIGNMENT != 0)
    {
        retval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

    if (aStackSize < PTHREAD_STACK_MIN)
    {
        retval = NLER_ERROR_BAD_INPUT;
        goto done;
    }

 done:
    return (retval);
}

/**
 *  Set the specified POSIX threads (pthreads) scheduler attributes
 *  based on the specified scheduler policy and scheduler
 *  policy-specific priority.
 *
 *  @param[in]     aPolicy      The system scheduler policy to attempt to
 *                              create the task with.
 *
 *  @param[in]     aPriority    The NLER task priority to map and set
 *                              according to the specified scheduler policy.
 *
 *  @param[inout]  aAttributes  A pointer to the POSIX threads (pthreads)
 *                              attributes to store scheduler-specific
 *                              attributes into.
 *
 *  @return 0 on success; otherwise, non-zero on failure.
 *
 */
static int nltask_pthreads_sched_set(int aPolicy, nltask_priority_t aPriority, pthread_attr_t *aAttributes)
{
    int                retval = 0;
    struct sched_param schedparam;
    int                schedpriority;

    /* We will be setting explicit scheduler attributes so set the
     * scheduler inheritance polict to explicit-provided rather than
     * inherited from the parent process or thread.
     */

    retval = pthread_attr_setinheritsched(aAttributes, PTHREAD_EXPLICIT_SCHED);
    if (retval != 0)
    {
        goto done;
    }

    /* Since it most closely effects the scheduling policy used by
     * most embedded, RTOS systems, attempt to set the scheduler
     * policy to SCHED_RR first. If that fails due to lack of support
     * or permission, then fall back to SCHED_OTHER.
     */

    retval = pthread_attr_setschedpolicy(aAttributes, aPolicy);
    if (retval != 0)
    {
        goto done;
    }

    /* Attempt to map the specified priority based on the scheduler
     * policy just set.
     */

    schedpriority = nltask_pthreads_map_priority_to_native(aPolicy, aPriority);
    if (schedpriority == _NLER_TASK_PTHREADS_PRIORITY_INVALID)
    {
        retval = ENOTSUP;
        goto done;
    }

    schedparam.sched_priority = schedpriority;

    /* Save the scheduler parameters to the thread attributes.
     */

    retval = pthread_attr_setschedparam(aAttributes, &schedparam);
    if (retval != 0)
    {
        goto done;
    }

 done:
    return (retval);
}

/**
 *  Set the specified POSIX threads (pthreads) stack and scheduler attributes.
 *
 *  @param[in]     aStack       A pointer to the task stack (unused).
 *
 *  @param[in]     aStackSize   The size, in bytes, of the task stack, which
 *                              must be NLER_REQUIRED_STACK_ALIGNMENT aligned
 *                              and larger than PTHREAD_STACK_MIN.
 *
 *  @param[inout]  aAttributes  A pointer to the POSIX threads (pthreads)
 *                              attributes to store scheduler-specific
 *                              attributes into.
 *
 *  @retval  #NLER_SUCCESS          on success.
 *  @retval  #NLER_ERROR_FAILURE    on all other errors.
 *
 */
static int nltask_pthreads_attr_set(void *aStack, size_t aStackSize, pthread_attr_t *aAttributes)
{
    int retval = NLER_SUCCESS;
    int status;

    /* Create joinable, rather than detached, threads.
     */

    status = pthread_attr_setdetachstate(aAttributes, PTHREAD_CREATE_DETACHED);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    /* Try the narrowest scope, PROCESS, first and then fall back if
     * that is not supported.
     */

    status = pthread_attr_setscope(aAttributes, PTHREAD_SCOPE_PROCESS);
    if (status == ENOTSUP)
    {
        status = pthread_attr_setscope(aAttributes, PTHREAD_SCOPE_SYSTEM);
        if (status != 0)
        {
            retval = NLER_ERROR_FAILURE;
            goto done;
        }
    }
    else if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    /* Set the stack size.
     *
     * At this point, we have checked whether the stack size and
     * is aligned to NLER_REQUIRED_STACK_ALIGNMENT and is greater than
     * or equal to PTHREAD_STACK_MIN. This represents a minium imposed
     * alignment.
     *
     * However, POSIX further requires alignment to the system page
     * size, which we hide from the caller. This requires aligning up
     * the stack size to meet the alignment requirements.
     */

    aStackSize = _nlALIGN_UP(aStackSize, sGlobals.mPageSize);

    status = pthread_attr_setstacksize(aAttributes, aStackSize);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

 done:
    return (retval);
}

/**
 *  Attempt, if supported, to set the name for the current thread.
 *
 *  @param[in]  aThread     The identifier of the current thread.
 *
 *  @param[in]  aName       A non-NULL pointer to a NULL-terminated C string
 *                          of the task name.
 *
 *  @return 0 on success; otherwise, non-zero on failure.
 *
 */
static int nltask_pthreads_setname(pthread_t aThread, const char *aName)
{
    int status;

    /* Some platforms have a non-portable interface for setting the
     * name of the current (or any arbitirary) thread. Use it, if
     * available, preferrign the two argument form to the single
     * argument form.
     */

#if HAVE_PTHREAD_SETNAME_NP
#if PTHREAD_SETNAME_NP_ARGS == 2
    status = pthread_setname_np(aThread, aName);
#elif PTHREAD_SETNAME_NP_ARGS == 1
    status = pthread_setname_np(aName);
#else
    status = 0;
#endif /* PTHREAD_SETNAME_NP_ARGS == 2 */
#endif /* HAVE_PTHREAD_SETNAME_NP */

    return (status);
}

/**
 *  POSIX threads (pthreads)-specific entry point trampoline.
 *
 *  This is the non-user entry point called from pthread_create on
 *  successful thread creation. Several NLER-specific housekeeping
 *  tasks are performed prior to invoking the user-specified entry
 *  point.
 *
 *  Any failure in this trampoline will result in a call to
 *  pthread_exit with return status EXIT_FAILURE.
 *
 *  @param[inout]  aClosure  A pointer to the NLER-specific task structure.
 *
 *  @return EXIT_SUCCESS on successful thread termination.
 *
 */
static void *nltask_pthreads_entry(void *aClosure)
{
    void                  *retval = EXIT_SUCCESS;
    nltask_t              *lTask = (nltask_t *)aClosure;
    nltask_entry_point_t   lEntry = (nltask_entry_point_t)lTask->mNativeTaskObj.mEntry;
    const pthread_t        lThread = pthread_self();
    int                    status;

    lTask->mNativeTaskObj.mThread = lThread;

    /* Store the NLER-specific task structure in the POSIX threads
     * (pthreads) local storage for use in supporting nltask_get_current.
     */

    status = pthread_setspecific(sGlobals.mThreadLocalStorageKey, lTask);
    if (status != 0)
    {
        pthread_exit((void *)EXIT_FAILURE);
    }

    /* Attempt to set the name of the thread.
     */

    status = nltask_pthreads_setname(lThread, lTask->mNativeTaskObj.mName);
    if (status != 0)
    {
        pthread_exit((void *)EXIT_FAILURE);
    }

    /* Jump to the user-specific entry point. At this point, the task
     * is live from an end user perspective.
     */

    (*lEntry)(lTask->mNativeTaskObj.mParams);

    return (retval);
}

/**
 *  Attempt to create a task with the specified attributes, scheduler
 *  policy, scheduler policy-specific priority, and task structure.
 *
 *  @param[in]     aPolicy      The system scheduler policy to attempt to
 *                              create the task with.
 *
 *  @param[in]     aPriority    The NLER task priority to map and set
 *                              according to the specified scheduler policy.
 *
 *  @param[inout]  aAttributes  A pointer to the POSIX threads (pthreads)
 *                              attributes to store scheduler-specific
 *                              attributes into.
 *
 *  @param[inout]  aOutTask     A non-NULL pointer to the task structure.
 *
 *  @return 0 on success; otherwise, non-zero on failure.
 *
 */
static int nltask_pthreads_try_create(int aPolicy, int aPriority, pthread_attr_t *aAttributes, nltask_t *aOutTask)
{
    int       retval;
    pthread_t thread;

    /* Set scheduler-specific attributes.
     */

    retval = nltask_pthreads_sched_set(aPolicy, aPriority, aAttributes);
    if (retval != 0)
    {
        goto done;
    }

    /* Attempt to create the thread.
     */
    retval = pthread_create(&thread, aAttributes, nltask_pthreads_entry, aOutTask);
    if (retval != 0)
    {
        goto done;
    }

 done:
    return (retval);
}

int nltask_create(nltask_entry_point_t aEntry, const char *aName, void *aStack, size_t aStackSize, nltask_priority_t aPriority, void *aParams, nltask_t *aOutTask)
{
    int retval = NLER_SUCCESS;
    int status;
    pthread_attr_t threadattr;
    const int *sched_begin = &sSchedPolicies[0];
    const int *sched_end = &sSchedPolicies[kSchedPoliciesMax];

    retval = nltask_pthreads_check_args(aEntry, aName, aStack, aStackSize, aPriority, aParams, aOutTask);
    if (retval != NLER_SUCCESS)
    {
        goto done;
    }

    status = pthread_attr_init(&threadattr);
    if (status != 0)
    {
        retval = NLER_ERROR_FAILURE;
        goto done;
    }

    retval = nltask_pthreads_attr_set(aStack, aStackSize, &threadattr);
    if (retval != NLER_SUCCESS)
    {
        goto threadattr_destroy;
    }

    aOutTask->mStackTop                  = aStack + aStackSize;

    aOutTask->mNativeTaskObj.mName       = aName;
    aOutTask->mNativeTaskObj.mEntry      = aEntry;
    aOutTask->mNativeTaskObj.mParams     = aParams;

    while (sched_begin != sched_end)
    {
        status = nltask_pthreads_try_create(*sched_begin, aPriority, &threadattr, aOutTask);
        if (status == 0)
        {
            retval = NLER_SUCCESS;
            break;
        }
        else if ((status != ENOTSUP) && (status != EPERM))
        {
            retval = NLER_ERROR_FAILURE;
            break;
        }

        sched_begin++;
    }

 threadattr_destroy:
    pthread_attr_destroy(&threadattr);

 done:
    if (retval != NLER_SUCCESS)
    {
        NL_LOG_CRIT(lrERTASK, "failed to create task: %s (%d)\n", aName ? aName : "[No name specified]", retval);
    }

    return (retval);
}

void nltask_suspend(nltask_t *aTask)
{

}

void nltask_resume(nltask_t *aTask)
{

}

void nltask_set_priority(nltask_t *aTask, int aPriority)
{
    const pthread_t thread = aTask->mNativeTaskObj.mThread;
    struct sched_param schedparam;
    int schedpolicy;
    int schedpriority;
    int status;

    status = pthread_getschedparam(thread, &schedpolicy, &schedparam);
    if (status != 0)
    {
        goto done;
    }

    schedpriority = nltask_pthreads_map_priority_to_native(schedpolicy, aPriority);

    if ((schedpriority != _NLER_TASK_PTHREADS_PRIORITY_INVALID) &&
        (schedpriority != schedparam.sched_priority))
    {
        schedparam.sched_priority = schedpriority;

#if HAVE_PTHREAD_SETSCHEDPRIO
        status = pthread_setschedprio(thread, schedparam.sched_priority);
#elif HAVE_PTHREAD_SETSCHEDPARAM
        status = pthread_setschedparam(thread, schedpolicy, &schedparam);
#else
#error "unable to find either pthread_setschedprio or pthread_setschedparam to support nltask_set_priority"
#endif
        NLER_ASSERT(status == 0);
    }

 done:
    return;
}

nltask_t *nltask_get_current(void)
{
    nltask_t *retval;

    retval = pthread_getspecific(sGlobals.mThreadLocalStorageKey);

    return (retval);
}

void nltask_sleep_ms(nl_time_ms_t aDurationMS)
{
    struct timespec request, remain;
    int status;

    request.tv_sec = aDurationMS / 1000;
    request.tv_nsec = (aDurationMS % 1000) * 1000000;

    while ((status = nanosleep(&request, &remain)) != 0)
    {
        if (errno == EINTR)
        {
            request = remain;
            continue;
        }
        else
        {
            break;
        }
    }
}

void nltask_yield(void)
{
#if HAVE_PTHREAD_YIELD
    pthread_yield();
#elif HAVE_PTHREAD_YIELD_NP
    pthread_yield_np();
#else
#error "unable to find either pthread_yield or pthread_yield_np to support nltask_yield"
#endif
}

const char *nltask_get_name(const nltask_t *aTask)
{
    return ((aTask != NULL) ? aTask->mNativeTaskObj.mName : NULL);
}
