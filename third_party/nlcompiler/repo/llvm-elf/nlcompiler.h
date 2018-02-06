/*
 *
 *    Copyright (c) 2010-2012 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 *    Description:
 *      This file defines compatibility macros for LLVM
 */

#ifndef NLCOMPILER_LLVM_H
#define NLCOMPILER_LLVM_H

#define NL_SYMBOL_IN_NOINIT(symbolName) NL_SYMBOL_AT_PLATFORM_DATA_SECTION(".noinit." symbolName)
#define NL_SYMBOL_AT_SECTION(sectionName) __attribute__((section(sectionName)))
#define NL_SYMBOL_AT_PLATFORM_DATA_SECTION(sectionName) NL_SYMBOL_AT_SECTION(sectionName)
#define NL_WEAK_ATTRIBUTE __attribute__((weak_import))

/* No alias support in llvm, just ignore the target for clang static analysis purposes */
#define NL_WEAK_ALIAS(target) __attribute__((weak_import))

#endif
