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
 *      Initialization.
 *
 */

#ifndef NL_ER_INIT_H
#define NL_ER_INIT_H

#ifdef __cplusplus
extern "C" {
#endif


/** Initialize the runtime. The runtime must be initialized before certain
 * system functions (logging, tasks, timers) are functional.
 *
 * @return NLER_SUCCESS if the runtime was successfully started.
 */
int nl_er_init(void);

/** Cleanup the runtime when execution is complete. In practice it may not be
 * possible to shut the runtime down once nl_er_start_running() has been
 * called.
 */
void nl_er_cleanup(void);

/** Begin running the tasks in the runtime. This function never returns. As
 * such, all task initialization must take place before starting the runtime.
 */
void nl_er_start_running(void);

#ifdef __cplusplus
}
#endif

#endif /* NL_ER_INIT_H */
