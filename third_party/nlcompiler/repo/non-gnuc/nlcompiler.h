/*
 *
 *    Copyright (c) 2010-2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 */

/**
 *
 *    @file
 *      This file defines compatibility macros for non-GNU C compilers, by
 *      defining all the compiler-specific macros as empty/void.
 *
 */

#ifndef NLCOMPILER_NON_GNUC_H
#define NLCOMPILER_NON_GNUC_H

#define NL_SYMBOL_IN_NOINIT(symbolName)
#define NL_SYMBOL_AT_SECTION(sectionName)
#define NL_RETAIN_SYMBOL
#define NL_SYMBOL_AT_PLATFORM_DATA_SECTION(sectionName)
#define NL_WEAK_ALIAS(target)
#define NL_WEAK_ATTRIBUTE

#define NL_SECTION_BEGIN_ADDRESS(sec)
#define NL_SECTION_END_ADDRESS(sec)

#define NL_ATTR_UNUSED

#endif // NLCOMPILER_NON_GNUC_H
