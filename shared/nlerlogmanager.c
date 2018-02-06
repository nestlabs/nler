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
 *      This file implements NLER build platform-independent log
 *      management interfaces.
 *
 */
#include <stdlib.h>
#include "nlerlog.h"
#include "nlerlogmanager.h"

nl_log_printer_t gLogger = NULL;
void             *gLoggerClosure = NULL;

nl_log_token_printer_t gTokenLogger = NULL;
void                   *gTokenLoggerClosure = NULL;

void nl_set_logging_function(nl_log_printer_t aPrinter, void *aClosure)
{
    gLogger = aPrinter;
    gLoggerClosure = aClosure;
}

void nl_set_token_logging_function(nl_log_token_printer_t aPrinter, void *aClosure)
{
    gTokenLogger = aPrinter;
    gTokenLoggerClosure = aClosure;
}
