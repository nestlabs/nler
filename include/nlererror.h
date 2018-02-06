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
 *      Error Codes.
 *
 */
#ifndef NL_ER_ERROR_H
#define NL_ER_ERROR_H

#define NLER_SUCCESS                (0)     /**< A non-error */
#define NLER_ERROR_FAILURE          (-1)    /**< Non specific failure error */
#define NLER_ERROR_BAD_INPUT        (-1000) /**< Bad input error */
#define NLER_ERROR_NO_RESOURCE      (-1001) /**< No resource error */
#define NLER_ERROR_BAD_STATE        (-1002) /**< Bad state error */
#define NLER_ERROR_NO_MEMORY        (-1003) /**< No memory error */
#define NLER_ERROR_INIT             (-1004) /**< Initialization error */
#define NLER_ERROR_NOT_IMPLEMENTED  (-1005) /**< Not Implemented error */

#define NLER_FIRST_APP_ERROR    (-2000) /**< Base all application level errors from this */

#endif
