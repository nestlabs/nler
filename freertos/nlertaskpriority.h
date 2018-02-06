/*
 *
 *    Copyright (c) 2015-2016 Nest Labs, Inc.
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
 *      This file defines FreeRTOS-specific task priorities.
 *
 */

#ifndef NL_ER_TASK_PRIORITY_H
#define NL_ER_TASK_PRIORITY_H

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

/* these are the relative priorities of
 * tasks. see nlertask.h for more information.
 */
typedef int nl_task_priority_t;

#define NLER_TASK_PRIORITY_HIGHEST  (nl_task_priority_t)(configMAX_PRIORITIES - 1)
#define NLER_TASK_PRIORITY_HIGH     (nl_task_priority_t)(configMAX_PRIORITIES - 3)
#define NLER_TASK_PRIORITY_NORMAL   (nl_task_priority_t)(configMAX_PRIORITIES - 6)
#define NLER_TASK_PRIORITY_LOW      (nl_task_priority_t)(configMAX_PRIORITIES - 9)

#endif /*  NL_ER_TASK_PRIORITY_H */
