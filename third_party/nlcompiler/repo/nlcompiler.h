/*
 *
 *    Copyright (c) 2010-2015 Nest Labs, Inc.
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
 *
 */

#ifndef NLCOMPILER_H
#define NLCOMPILER_H

#if NL_SIMULATOR
    #if defined(__GNUC__)
        #if defined(__MACH__)
            #if defined(__llvm__)
                #include "llvm-macho/nlcompiler.h"
            #else
                #include "gcc-macho/nlcompiler.h"
            #endif
        #elif defined(__linux__)
            #include "gcc-linux/nlcompiler.h"
        #endif
    #endif
#else
    #if defined(__GNUC__)
        #if defined(__llvm__)
            #include "llvm-elf/nlcompiler.h"
        #else
            #include "gcc-elf/nlcompiler.h"
        #endif
    #endif
#endif

/* RETAIN is only used by breadcrumbs sEraseNeeded right now.
 * Since RAM is always retained in EM358, we don't need a special section
 * for it.  The intention is that this value be kept across reboots
 * so .noinit is the right section to put it.
 */
#define NL_RETAIN_SYMBOL NL_SYMBOL_AT_SECTION(".noinit")

#ifdef BUILD_FEATURE_RESET_INFO_IN_TEMP_RAM
/* RESETINFO is a special section that is temporary.  It is only
 * valid between the time where we're about to reboot and to the
 * early part of the next boot.  It is shared/overlaid with other
 * memory (EMHEAP) so must not be used outside of those times.
 */
#define NL_RESETINFO_SYMBOL NL_SYMBOL_AT_SECTION(".nlresetinfo")
#endif

#endif
