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
 *      Tasks.
 *
 */

#ifndef NL_ER_TASK_H
#define NL_ER_TASK_H

#include <stddef.h>
#include <stdint.h>
#include "nlercfg.h"
#include "nlcompiler.h"
#include "nlertaskstack.h"
#include "nlertaskpriority.h"
#include "nlertime.h"
#include "nlernative.h"

#ifdef __cplusplus
extern "C" {
#endif

#if NLER_FEATURE_TASK_LOCAL_STORAGE
typedef NLER_TASK_STORAGE_TYPE nltask_storage_t;
#endif

#if NLER_FEATURE_STACK_ALIGNMENT
#define NLER_REQUIRED_STACK_ALIGNMENT NLER_FEATURE_STACK_ALIGNMENT
#else
/* To be ARM AAPCS compliant, stacks should have a minimum of 8-byte alignment */
#define NLER_REQUIRED_STACK_ALIGNMENT 8
#endif

/* Memory section to put stacks in.  A product can set a preference by defining
 * NLER_PRODUCT_STACK_MEM_SECTION or else the default is ".stack"
 */
#ifdef NLER_PRODUCT_STACK_MEM_SECTION
#define NLER_STACK_MEM_SECTION NLER_PRODUCT_STACK_MEM_SECTION
#else
#define NLER_STACK_MEM_SECTION ".stack"
#endif

/* Macro to define a stack and have it be properly aligned and in the
 * preferred noload ".stack" section.
 */
#define DEFINE_STACK(stackName, stackSize) uint8_t (stackName)[(stackSize)] __attribute__((aligned(NLER_REQUIRED_STACK_ALIGNMENT))) NL_SYMBOL_AT_PLATFORM_DATA_SECTION(NLER_STACK_MEM_SECTION)

/** Task priority. Priority values are implementation specific.
 * Implementations must ensure it is ok to take any of these values and add or
 * subtract one to set the relative priorities of the tasks in your
 * application. Values out side of these definitions (+1/-1) will yield
 * undefined results and possible run time failures.
 *
 * The following three priorities are safe to add or subtract one in order to
 * obtain relative priorities within an application:
 *
 *   - @c NLER_TASK_PRIORITY_LOW
 *   - @c NLER_TASK_PRIORITY_NORMAL
 *   - @c NLER_TASK_PRIORITY_HIGH
 *
 * The highest priority is @c NLER_TASK_PRIORITY_HIGHEST. It is *not* safe to
 * add or subtract one from this.
 */

/** Task entry point.
 *
 * @param aParams Data passed to task upon startup.
 */
typedef void (*nltask_entry_point_t)(void *aParams);

/** Task structure. It need not be initialized and will be filled in when
 * created using nltask_create. Callers provide the storage.
 */
typedef struct nltask_s
{
    nltask_obj_t          mNativeTaskObj; /**< Task buffer/control block */
    void                 *mStackTop;      /**< StackTop, useful for backtracing */
#if NLER_FEATURE_TASK_LOCAL_STORAGE
    nl_task_storage_t     mStorage;       /**< Task storage */
#endif
} nltask_t;

/** Create a new task
 *
 * @param[in] aEntry function to call when new task
 * is started. will be called with a single
 * argument of aParams
 *
 * @param[in] aName pointer to a string used to identify the task. Must not be
 * NULL.
 *
 * @param[in] aStack pointer to memory to use as the stack for the new task. It
 * is up to the caller to size the stack appropriately.  aStack must be aligned
 * to either 8-bytes (default) or possibly a larger value (like 32-bytes if
 * stack guards are in use) defined by NLER_FEATURE_STACK_ALIGNMENT.
 *
 * @param[in] aStackSize size in bytes of the stack pointed to by aStack
 *
 * @param[in] aPriority priority of this task. Higher numbers are higher
 * priority.
 *
 * @param[in] aParams Argument passed to aEntry upon task startup
 *
 * @param[in] aTask points to an nltask_t structure managed by the caller.
 *
 * @return NLER_SUCCESS if the task was able to be created
 */
int nltask_create(nltask_entry_point_t aEntry,
                  const char *aName,
                  void *aStack,
                  size_t aStackSize,
                  nltask_priority_t aPriority,
                  void *aParams,
                  nltask_t *aTask);

/** Suspend execution of a task. This request is a best-effort by the runtime
 * implementation. Has no effect if the task is already suspended.
 *
 * @param[in, out] aTask task to prevent from being considered a candidate to
 * resume execution at the next task reschedule.
 */
void nltask_suspend(nltask_t *aTask);

/** Resume execution of a suspended task. This is a best-effort by the runtime
 * implementation. Has no effect on tasks that are not suspended.
 *
 * @param[in] aTask suspended task to resume executing
 * at the scheduler's convenience.
 */
void nltask_resume(nltask_t *aTask);

/** Alter the scheduling priority of a task.
 *
 * @param[in, out] aTask task that will have it's scheduling priority changed.
 *
 * @param[in] aPriority new priority. the priority will be altered at the
 * convenience of the scheduler.
 */
void nltask_set_priority(nltask_t *aTask, nltask_priority_t aPriority);

/** Get the scheduling priority of a task.
 *
 * @param[in] aTask task to get priority of
 *
 * @return Priority of aTask
 */
nltask_priority_t nltask_get_priority(const nltask_t *aTask);

/** Get the currently executing task.
 *
 * @return Pointer to the currently running task
 */
nltask_t *nltask_get_current(void);

/** Pause execution of the current task for at least aDurationMS milliseconds.
 *
 * @param[in] aDurationMS time in milliseconds to sleep
 */
void nltask_sleep_ms(nl_time_ms_t aDurationMS);

/** Yield current task.  This asks the scheduler to run and possibly schedule
 * another task to run.
 */
void nltask_yield(void);

/** Get name of a task.
 */
const char *nltask_get_name(const nltask_t *aTask);

#ifdef __cplusplus
}
#endif

/** @example test-task.c
 * A simple example illustrating bringing up two tasks.
 */
#endif /* NL_ER_TASK_H */
