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
 *      This file defines compatibility macros for GCC Linux ELF
 *
 */

#ifndef NLCOMPILER_GCC_ELF_H
#define NLCOMPILER_GCC_ELF_H

#define NL_SYMBOL_IN_NOINIT(symbolName) __attribute__((section(".noinit." symbolName)))
#define NL_SYMBOL_AT_SECTION(sectionName) __attribute__((section(sectionName)))
#define NL_SYMBOL_AT_PLATFORM_DATA_SECTION(sectionName) __attribute__((section(sectionName)))
#define NL_WEAK_ATTRIBUTE __attribute__((weak))
#define NL_WEAK_ALIAS(target) __attribute__((weak, alias(#target)))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __BIG_ENDIAN__
#else
#error Endianess undefined
#endif

#endif // NLCOMPILER_GCC_ELF_H
