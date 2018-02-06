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
 *      This file defines POSIX threads (pthreads)-specific task stack
 *      space requirements.
 *
 */

#ifndef NL_ER_TASK_STACK_H
#define NL_ER_TASK_STACK_H

/**
 *  Stack size to give POSIX threads (pthreads) in addition to what
 *  application and runtime require.
 */
#define NLER_TASK_STACK_BASE  32768

#endif /* NL_ER_TASK_STACK_H */
