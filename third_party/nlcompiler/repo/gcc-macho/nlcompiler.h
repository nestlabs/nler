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
 *      This file defines compatibility macros for GCC/MACHO (typically OSX)
 *
 */

#ifndef NLCOMPILER_GCCMACHO_H
#define NLCOMPILER_GCCMACHO_H
 
#define NL_SYMBOL_IN_NOINIT(symbolName)  __attribute__((section("__DATA, .noinit")))
#define NL_SYMBOL_AT_SECTION(sectionName) __attribute__((section("__DATA, " sectionName)))
#define NL_SYMBOL_AT_PLATFORM_DATA_SECTION(sectionName) __attribute__((section("__DATA, " sectionName)))
#define NL_WEAK_ATTRIBUTE __attribute__((weak))
#define NL_WEAK_ALIAS(target) __attribute__((weak, alias(#target)))

#endif
