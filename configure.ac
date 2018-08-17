#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#
#    Copyright (c) 2016-2018 Nest Labs Inc.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      This file is the GNU autoconf input source file for
#      Nest Labs Embedded Runtime.
#

#
# Declare autoconf version requirements
#
AC_PREREQ([2.68])

#
# Initialize autoconf for the package
#
AC_INIT([NLER],
        m4_esyscmd([third_party/nlbuild-autotools/repo/scripts/mkversion -b `cat .default-version` .]),
        [nler-users@google.com],
        [nler],
        [https://github.com/nestlabs/nler/])

# Tell the rest of the build system the absolute path where the
# nlbuild-autotools repository is rooted at.

AC_SUBST(nlbuild_autotools_stem,[third_party/nlbuild-autotools/repo])
AC_SUBST(abs_top_nlbuild_autotools_dir,[\${abs_top_srcdir}/\${nlbuild_autotools_stem}])

#
# NLER interface current, revision, and age versions.
#
# Maintainters: Please manage these fields as follows:
#
#   Interfaces removed:    CURRENT++, AGE = 0, REVISION = 0
#   Interfaces added:      CURRENT++, AGE++,   REVISION = 0
#   No interfaces changed:                     REVISION++
#
#
AC_SUBST(LIBNLER_VERSION_CURRENT,  [1])
AC_SUBST(LIBNLER_VERSION_AGE,      [0])
AC_SUBST(LIBNLER_VERSION_REVISION, [0])
AC_SUBST(LIBNLER_VERSION_INFO,     [${LIBNLER_VERSION_CURRENT}:${LIBNLER_VERSION_REVISION}:${LIBNLER_VERSION_AGE}])

#
# Check the sanity of the source directory by checking for the
# presence of a key watch file
#
AC_CONFIG_SRCDIR([include/nlercfg.h])

#
# Tell autoconf where to find auxilliary build tools (e.g. config.guess,
# install-sh, missing, etc.)
#
AC_CONFIG_AUX_DIR([third_party/nlbuild-autotools/repo/third_party/autoconf])

#
# Tell autoconf where to find auxilliary M4 macros
#
AC_CONFIG_MACRO_DIRS([third_party/nlbuild-autotools/repo/third_party/autoconf/m4 third_party/nlbuild-autotools/repo/autoconf/m4])

#
# Tell autoconf what file the package is using to aggregate C preprocessor
# defines.
#
AC_CONFIG_HEADERS([include/nler-config.h])

#
# Figure out what the canonical build, host and target tuples are.
#
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

#
# Mac OS X / Darwin ends up putting some versioning cruft on the end of its
# tuple that we don't care about in this script. Create "clean" variables
# devoid of it.
#

NL_FILTERED_CANONICAL_BUILD
NL_FILTERED_CANONICAL_HOST
NL_FILTERED_CANONICAL_TARGET

#
# Configure automake with the desired options, indicating that this is not
# a native GNU package, that we want "silent" build rules, and that we want
# objects built in the same subdirectory as their source rather than collapsed
# together at the top-level directory.
#
# Disable silent build rules by either passing --disable-silent-rules to
# configure or passing V=1 to make
#
AM_INIT_AUTOMAKE([1.14 foreign silent-rules subdir-objects tar-pax])

#
# Silent build rules requires at least automake-1.11. Employ
# techniques for not breaking earlier versions of automake.
#
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_SILENT_RULES([yes])

#
# Enable maintainer mode to prevent the package from constantly trying
# to rebuild configure, Makefile.in, etc. Rebuilding such files rarely,
# if ever, needs to be done "in the field".
#
# Use the included 'bootstrap' script instead when necessary.
#
AM_MAINTAINER_MODE


#
# Check for the target build platform
#
NLER_BUILD_PLATFORM=
NLER_BUILD_PLATFORM_FREERTOS=0
NLER_BUILD_PLATFORM_NSPR=0
NLER_BUILD_PLATFORM_PTHREADS=0

AC_MSG_CHECKING([for build platform])

# The user may have attempted to explicitly specify the build
# platform. Sanity check it or default to 'auto'.

AC_ARG_WITH(build-platform,
    [AS_HELP_STRING([--with-build-platform=TARGET],
        [Specify the build platform from one of: auto, freertos, nspr, or pthreads @<:@default=auto@:>@.])],
    [
        case "${with_build_platform}" in

        auto|freertos|nspr|pthreads)
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${with_build_platform} for --with-build-platform])
            ;;

        esac
    ],
    [with_build_platform=auto])

# At this point, the build platform is one of the allowed
# values. If it's 'auto' we autodetect it.

if test "${with_build_platform}" = "auto"; then
    case ${target_os} in

	*cygwin*|*darwin*|*linux*|*freebsd*|*netbsd*|*openbsd*)
            with_build_platform=pthreads
	    ;;

	*freertos*)
            with_build_platform=freertos
	    ;;

	*)
	    AC_MSG_RESULT([unknown])
	    AC_MSG_ERROR([Unsupported target OS ${target_os}])
	    ;;

    esac
fi

NLER_BUILD_PLATFORM=${with_build_platform}

case ${with_build_platform} in

    freertos)
        NLER_BUILD_PLATFORM_FREERTOS=1
        ;;

    nspr)
        NLER_BUILD_PLATFORM_NSPR=1
        ;;

    pthreads)
        NLER_BUILD_PLATFORM_PTHREADS=1
        ;;

esac

AC_MSG_RESULT(${NLER_BUILD_PLATFORM})

AC_SUBST(NLER_BUILD_PLATFORM)
AC_DEFINE_UNQUOTED([NLER_BUILD_PLATFORM],[${NLER_BUILD_PLATFORM}],[Embedded Runtime build platform])

AC_SUBST(NLER_BUILD_PLATFORM_FREERTOS)
AM_CONDITIONAL([NLER_BUILD_PLATFORM_FREERTOS], [test "${NLER_BUILD_PLATFORM}" = "freertos"])
AC_DEFINE_UNQUOTED([NLER_BUILD_PLATFORM_FREERTOS],[${NLER_BUILD_PLATFORM_FREERTOS}],[Define to 1 if you want to use Embedded Runtime with FreeRTOS])

AC_SUBST(NLER_BUILD_PLATFORM_NSPR)
AM_CONDITIONAL([NLER_BUILD_PLATFORM_NSPR], [test "${NLER_BUILD_PLATFORM}" = "nspr"])
AC_DEFINE_UNQUOTED([NLER_BUILD_PLATFORM_NSPR],[${NLER_BUILD_PLATFORM_NSPR}],[Define to 1 if you want to use Embedded Runtime with Netscape Portable Runtime (i.e., nspr)])

AC_SUBST(NLER_BUILD_PLATFORM_PTHREADS)
AM_CONDITIONAL([NLER_BUILD_PLATFORM_PTHREADS], [test "${NLER_BUILD_PLATFORM}" = "pthreads"])
AC_DEFINE_UNQUOTED([NLER_BUILD_PLATFORM_PTHREADS],[${NLER_BUILD_PLATFORM_PTHREADS}],[Define to 1 if you want to use Embedded Runtime with POSIX threads (i.e., pthreads)])

NLER_CPPFLAGS="-I\${abs_top_srcdir}/${NLER_BUILD_PLATFORM} ${NLER_CPPFLAGS}"

#
# Check for the target assembler ISA 
#

AC_MSG_CHECKING([for target instruction set architecture])

AC_ARG_WITH(target-isa,
    [AS_HELP_STRING([--with-target-isa=ARCH],
        [Specify the build platform from one of: auto or generic @<:@default=auto@:>@.])],
    [
        case "${with_target_isa}" in

        auto|generic)
            ;;

        *)
            AC_MSG_ERROR([Invalid or unsupported value ${with_target_isa} for --with-target-isa])
            ;;

        esac
    ],
    [with_target_isa=auto])

# At this point, the target ISA is one of the allowed
# values. If it's 'auto' we try to autodetect it.

if test "${with_target_isa}" = "auto"; then
    case ${target_cpu} in

	*)
            with_target_isa="generic"
	    ;;

    esac
fi

# At this point, the target ISA is an explicitly-supported
# architecture or 'generic'.

NLER_ASM_ISA=${with_target_isa}
NLER_ASM_ISA_GENERIC=0

case ${NLER_ASM_ISA} in

    generic)
        NLER_ASM_ISA_GENERIC=1
	;;

esac

AC_MSG_RESULT(${NLER_ASM_ISA})

AC_SUBST(NLER_ASM_ISA)
AC_SUBST(NLER_ASM_ISA_GENERIC)

AM_CONDITIONAL([NLER_ASM_ISA_GENERIC], [test "${NLER_ASM_ISA}" = "generic"])

AC_DEFINE_UNQUOTED([NLER_ASM_ISA],[${NLER_ASM_ISA}],[Embedded Runtime target CPU-specific instruction set architecture])

AC_DEFINE_UNQUOTED([NLER_ASM_ISA_GENERIC],[${NLER_ASM_ISA_GENERIC}],[Define to 1 if you want to use Embedded Runtime without target CPU-specific instruction set architecture (ISA) support])

#
# Checks for build host programs
#

# If we are cross-compiling and we are on an embedded target that
# doesn't support independent, standalone executables, then all
# compiler tests that attempt to create an executable will fail. In
# such circumstances, set AC_NO_EXECUTABLES (see http://sourceware.org/
# ml/newlib/2006/msg00353.html).

AC_MSG_CHECKING([whether to disable executable checking])
if test "$cross_compiling" = yes; then
    AC_NO_EXECUTABLES
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

# Passing -Werror to GCC-based or -compatible compilers breaks some
# autoconf tests (see
# http://lists.gnu.org/archive/html/autoconf-patches/2008-09/msg00014.html).
#
# If -Werror has been passed transform it into -Wno-error. We'll
# transform it back later with NL_RESTORE_WERROR.

NL_SAVE_WERROR

# Check for compilers.
#
# These should be checked BEFORE we check for and, implicitly,
# initialize libtool such that libtool knows what languages it has to
# work with.

AM_PROG_AS

AC_PROG_CPP
AC_PROG_CPP_WERROR

AC_PROG_CC
AC_PROG_CC_C_O

# Check for other compiler toolchain tools.

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib)
AC_CHECK_TOOL(OBJCOPY, objcopy)
AC_CHECK_TOOL(STRIP, strip)

# Check for other host tools.

AC_PROG_INSTALL
AC_PROG_LN_S

AC_PATH_PROG(CMP, cmp)
AC_PATH_PROG(PERL, perl)

#
# Checks for specific compiler characteristics
#

#
# Common compiler flags we would like to have.
#
#   -Wall                        CC
#   -Wchar-subscripts            CC
#   -Wformat                     CC
#   -Wimplicit                   CC
#   -Wparentheses                CC
#   -Wreturn-type                CC
#   -Wsequence-point             CC
#   -Wunused                     CC
#   -Wno-switch                  CC
#

PROSPECTIVE_CFLAGS="-Wall -Wchar-subscripts -Wformat -Wimplicit -Wparentheses -Wreturn-type -Wsequence-point -Wunused -Wno-switch"

AX_CHECK_COMPILER_OPTIONS([C],   ${PROSPECTIVE_CFLAGS})

# Check for and initialize libtool

LT_INIT

# Check whether the current compiler supports atomic built-ins
#
# Because these are compiler built-in intrinsics, the standard
# AC_CHECK_FUNCS will not work.

AC_MSG_NOTICE([checking for compiler atomic built-ins])

# __sync_add_and_fetch

AC_CACHE_CHECK([whether the target supports __sync_add_and_fetch],
	       [nler_cv_have__sync_add_and_fetch],
	       [
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
				]],[[
					volatile int val = 0;
					return __sync_add_and_fetch(&val, 1);
				]])],
				[nler_cv_have__sync_add_and_fetch=yes],
				[nler_cv_have__sync_add_and_fetch=no])
		])

if test "x${nler_cv_have__sync_add_and_fetch}" != "xno"; then
	AC_DEFINE(HAVE___SYNC_ADD_AND_FETCH, 1,
		  [Define this if your target compiler supports __sync_add_and_fetch])
fi

# __sync_fetch_and_and

AC_CACHE_CHECK([whether the target supports __sync_fetch_and_and],
	       [nler_cv_have__sync_fetch_and_and],
	       [
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
				]],[[
					volatile int val = 0;
					return __sync_fetch_and_and(&val, 1);
				]])],
				[nler_cv_have__sync_fetch_and_and=yes],
				[nler_cv_have__sync_fetch_and_and=no])
		])

if test "x${nler_cv_have__sync_fetch_and_and}" != "xno"; then
	AC_DEFINE(HAVE___SYNC_FETCH_AND_AND, 1,
		  [Define this if your target compiler supports __sync_fetch_and_and])
fi

# __sync_fetch_and_or

AC_CACHE_CHECK([whether the target supports __sync_fetch_and_or],
	       [nler_cv_have__sync_fetch_and_or],
	       [
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
				]],[[
					volatile int val = 0;
					return __sync_fetch_and_or(&val, 1);
				]])],
				[nler_cv_have__sync_fetch_and_or=yes],
				[nler_cv_have__sync_fetch_and_or=no])
		])

if test "x${nler_cv_have__sync_fetch_and_or}" != "xno"; then
	AC_DEFINE(HAVE___SYNC_FETCH_AND_OR, 1,
		  [Define this if your target compiler supports __sync_fetch_and_or])
fi

# __sync_sub_and_fetch

AC_CACHE_CHECK([whether the target supports __sync_sub_and_fetch],
	       [nler_cv_have__sync_sub_and_fetch],
	       [
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
				]],[[
					volatile int val = 0;
					return __sync_sub_and_fetch(&val, 1);
				]])],
				[nler_cv_have__sync_sub_and_fetch=yes],
				[nler_cv_have__sync_sub_and_fetch=no])
		])

if test "x${nler_cv_have__sync_sub_and_fetch}" != "xno"; then
	AC_DEFINE(HAVE___SYNC_SUB_AND_FETCH, 1,
		  [Define this if your target compiler supports __sync_sub_and_fetch])
fi

# __sync_lock_test_and_set

AC_CACHE_CHECK([whether the target supports __sync_lock_test_and_set],
	       [nler_cv_have__sync_lock_test_and_set],
	       [
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
				]],[[
					volatile int val = 0;
					return __sync_lock_test_and_set(&val, 1);
				]])],
				[nler_cv_have__sync_lock_test_and_set=yes],
				[nler_cv_have__sync_lock_test_and_set=no])
		])

if test "x${nler_cv_have__sync_lock_test_and_set}" != "xno"; then
	AC_DEFINE(HAVE___SYNC_LOCK_TEST_AND_SET, 1,
		  [Define this if your target compiler supports __sync_lock_test_and_set])
fi

# __sync_val_compare_and_swap

AC_CACHE_CHECK([whether the target supports __sync_val_compare_and_swap],
	       [nler_cv_have__sync_val_compare_and_swap],
	       [
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
				]],[[
					volatile int val = 0;
					return __sync_val_compare_and_swap(&val, 0, 1);
				]])],
				[nler_cv_have__sync_val_compare_and_swap=yes],
				[nler_cv_have__sync_val_compare_and_swap=no])
		])

if test "x${nler_cv_have__sync_val_compare_and_swap}" != "xno"; then
	AC_DEFINE(HAVE___SYNC_VAL_COMPARE_AND_SWAP, 1,
		  [Define this if your target compiler supports __sync_val_compare_and_swap])
fi

NLER_HAVE_ATOMIC_BUILTINS=0

if test "${nler_cv_have__sync_add_and_fetch}"        = "yes" && \
   test "${nler_cv_have__sync_fetch_and_and}"        = "yes" && \
   test "${nler_cv_have__sync_fetch_and_or}"         = "yes" && \
   test "${nler_cv_have__sync_sub_and_fetch}"        = "yes" && \
   test "${nler_cv_have__sync_lock_test_and_set}"    = "yes" && \
   test "${nler_cv_have__sync_val_compare_and_swap}" = "yes"; then
    NLER_HAVE_ATOMIC_BUILTINS=1
fi

AC_DEFINE_UNQUOTED([NLER_HAVE_ATOMIC_BUILTINS],
    [${NLER_HAVE_ATOMIC_BUILTINS}],
    [Define this if your target compiler supports the __sync* atomic built-in functions])

#
# Core build-specific features
#
#   * Assertions
#   * Default Logger
#   * Event Timer
#   * Flow Tracer
#   * Log Tokenization
#   * Simulateable Time
#   * Stack Alignment
#   * Task Local Storage
#   * Timer Using Software-based Timer
#   * Wake Timer
#

NLER_FEATURE_ASSERTS=0
NLER_FEATURE_DEFAULT_LOGGER=0
NLER_FEATURE_EVENT_TIMER=0
NLER_FEATURE_FLOW_TRACER=0
NLER_FEATURE_LOG_TOKENIZATION=0
NLER_FEATURE_SIMULATEABLE_TIME=0
NLER_FEATURE_STACK_ALIGNMENT=0
NLER_FEATURE_TASK_LOCAL_STORAGE=0
NLER_FEATURE_TIMER_USING_SWTIMER=0
NLER_FEATURE_WAKE_TIMER=0

#
# Assertions
#
AC_MSG_CHECKING([whether to build assertion support])
AC_ARG_ENABLE(asserts,
    [AS_HELP_STRING([--enable-asserts],[Enable building of assertion support @<:@default=yes@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_asserts=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-asserts])
            ;;

        esac
    ],
    [nler_build_asserts=yes])
AC_MSG_RESULT(${nler_build_asserts})
AM_CONDITIONAL([NLER_BUILD_ASSERTS], [test "${nler_build_asserts}" = "yes"])
if test "${nler_build_asserts}" = "yes"; then
    NLER_FEATURE_ASSERTS=1
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_ASSERTS=${NLER_FEATURE_ASSERTS}"
fi

#
# Default Logger
#
AC_MSG_CHECKING([whether to build default logger support])
AC_ARG_ENABLE(default-logger,
    [AS_HELP_STRING([--enable-default-logger],[Enable building of default logger (vprintf) support @<:@default=yes@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_default_logger=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-default-logger])
            ;;

        esac
    ],
    [nler_build_default_logger=yes])
AC_MSG_RESULT(${nler_build_default_logger})
AM_CONDITIONAL([NLER_BUILD_DEFAULT_LOGGER], [test "${nler_build_default_logger}" = "yes"])
if test "${nler_build_default_logger}" = "yes"; then
    NLER_FEATURE_DEFAULT_LOGGER=1
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_DEFAULT_LOGGER=${NLER_FEATURE_DEFAULT_LOGGER}"
fi

#
# Event Timer
#
AC_MSG_CHECKING([whether to build event timer support])
AC_ARG_ENABLE(event-timer,
    [AS_HELP_STRING([--enable-event-timer],[Enable building of event timer support @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_event_timer=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-event-timer])
            ;;

        esac
    ],
    [nler_build_event_timer=no])
AC_MSG_RESULT(${nler_build_event_timer})
AM_CONDITIONAL([NLER_BUILD_EVENT_TIMER], [test "${nler_build_event_timer}" = "yes"])
if test "${nler_build_event_timer}" = "yes"; then
    NLER_FEATURE_EVENT_TIMER=1
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_EVENT_TIMER=${NLER_FEATURE_EVENT_TIMER}"
fi

#
# Flow Tracer
#
AC_MSG_CHECKING([whether to build flow tracer support])
AC_ARG_ENABLE(flow-tracer,
    [AS_HELP_STRING([--enable-flow-tracer],[Enable building of flow tracer support @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_flow_tracer=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-flow-tracer])
            ;;

        esac
    ],
    [nler_build_flow_tracer=no])
AC_MSG_RESULT(${nler_build_flow_tracer})
AM_CONDITIONAL([NLER_BUILD_FLOW_TRACER], [test "${nler_build_flow_tracer}" = "yes"])
if test "${nler_build_flow_tracer}" = "yes"; then
    NLER_FEATURE_FLOW_TRACER=1
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_FLOW_TRACER=${NLER_FEATURE_FLOW_TRACER}"
fi

#
# Log Tokenization
#
AC_MSG_CHECKING([whether to build log tokenization support])
AC_ARG_ENABLE(log-tokenization,
    [AS_HELP_STRING([--enable-log-tokenization],[Enable building of log tokenization support @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_log_tokenization=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-log-tokenization])
            ;;

        esac
    ],
    [nler_build_log_tokenization=no])
AC_MSG_RESULT(${nler_build_log_tokenization})
AM_CONDITIONAL([NLER_BUILD_LOG_TOKENIZATION], [test "${nler_build_log_tokenization}" = "yes"])
if test "${nler_build_log_tokenization}" = "yes"; then
    NLER_FEATURE_LOG_TOKENIZATION=1
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_LOG_TOKENIZATION=${NLER_FEATURE_LOG_TOKENIZATION}"
fi

#
# Simulateable Time
#
AC_MSG_CHECKING([whether to build simulateable time support])
AC_ARG_ENABLE(simulateable-time,
    [AS_HELP_STRING([--enable-simulateable-time],[Enable building of simulateable time support @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_simulateable_time=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-simulateable-time])
            ;;

        esac
    ],
    [nler_build_simulateable_time=no])
AC_MSG_RESULT(${nler_build_simulateable_time})
AM_CONDITIONAL([NLER_BUILD_SIMULATEABLE_TIME], [test "${nler_build_simulateable_time}" = "yes"])
if test "${nler_build_simulateable_time}" = "yes"; then
    NLER_FEATURE_SIMULATEABLE_TIME=1
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_SIMULATEABLE_TIME=${NLER_FEATURE_SIMULATEABLE_TIME}"
fi

#
# Stack Alignment
#
AC_MSG_CHECKING([for required stack alignment])
AC_ARG_WITH(stack-alignment,
    [AS_HELP_STRING([--with-stack-alignment=ALIGNMENT],[Specify the required staack alignment, in bytes @<:@default=none@:>@.])],
    [
        case "${with_stack_alignment}" in

        *[!0-9]*|'')
            AC_MSG_ERROR([Invalid value ${with_stack_alignment} for --with-stack-alignment])
            ;;

        *)
            nler_build_stack_alignment=${with_stack_alignment}
            ;;

        esac
    ],
    [nler_build_stack_alignment=none])
AC_MSG_RESULT(${nler_build_stack_alignment})
AM_CONDITIONAL([NLER_BUILD_STACK_ALIGNMENT], [test "${nler_build_stack_alignment}" != "none"])
if test "${nler_build_stack_alignment}" != "none"; then
    NLER_FEATURE_STACK_ALIGNMENT=${nler_build_stack_alignment}
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_STACK_ALIGNMENT=${NLER_FEATURE_STACK_ALIGNMENT}"
fi

#
# Task Local Storage
#
AC_MSG_CHECKING([whether to build task local storage support])
AC_ARG_ENABLE(task-local-storage,
    [AS_HELP_STRING([--enable-task-local-storage],[Enable building of task local storage support @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_task_local_storage=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-task-local-storage])
            ;;

        esac
    ],
    [nler_build_task_local_storage=no])
AC_MSG_RESULT(${nler_build_task_local_storage})
AM_CONDITIONAL([NLER_BUILD_TASK_LOCAL_STORAGE], [test "${nler_build_task_local_storage}" = "yes"])
if test "${nler_build_task_local_storage}" = "yes"; then
    NLER_FEATURE_TASK_LOCAL_STORAGE=1
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_TASK_LOCAL_STORAGE=${NLER_FEATURE_TASK_LOCAL_STORAGE}"
fi

#
# Timer Using Software-based Timers
#
AC_MSG_CHECKING([whether to build software-based timer support])
AC_ARG_ENABLE(software-timers,
    [AS_HELP_STRING([--enable-software-timers],[Enable building of software-based timer support @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_software_timers=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-task-local-storage])
            ;;

        esac
    ],
    [nler_build_software_timers=no])
AC_MSG_RESULT(${nler_build_software_timers})
AM_CONDITIONAL([NLER_BUILD_TIMER_USING_SWTIMER], [test "${nler_build_software_timers}" = "yes"])
if test "${nler_build_software_timers}" = "yes"; then
    NLER_FEATURE_TIMER_USING_SWTIMER=1
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_TIMER_USING_SWTIMER=${NLER_FEATURE_TIMER_USING_SWTIMER}"
fi

#
# Wake Timer
#
AC_MSG_CHECKING([whether to build wake timer support])
AC_ARG_ENABLE(wake-timer,
    [AS_HELP_STRING([--enable-wake-timer],[Enable building of wake timer support @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_wake_timer=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-wake-timer])
            ;;

        esac
    ],
    [nler_build_wake_timer=no])
AC_MSG_RESULT(${nler_build_wake_timer})
AM_CONDITIONAL([NLER_BUILD_WAKE_TIMER], [test "${nler_build_wake_timer}" = "yes"])
if test "${nler_build_wake_timer}" = "yes"; then
    NLER_FEATURE_WAKE_TIMER=1
    NLER_CPPFLAGS="${NLER_CPPFLAGS} -DNLER_FEATURE_WAKE_TIMER=${NLER_FEATURE_WAKE_TIMER}"
fi

#
# Utilities
#

AC_MSG_CHECKING([whether to build utilities])
AC_ARG_ENABLE(utilities,
    [AS_HELP_STRING([--enable-utilities],[Enable building of utilties @<:@default=yes@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            nler_build_utilities=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-utilities])
            ;;

        esac
    ],
    [nler_build_utilities=yes])
AC_MSG_RESULT(${nler_build_utilities})
AM_CONDITIONAL([NLER_BUILD_UTILITIES], [test "${nler_build_utilities}" = "yes"])

#
# Debug instances
#

# Debug

NL_ENABLE_DEBUG([no])

AM_CONDITIONAL([NLER_BUILD_DEBUG], [test "${nl_cv_build_debug}" = "yes"])

#
# Code coverage and compiler optimization
#

# Coverage

NL_ENABLE_COVERAGE([no])

AM_CONDITIONAL([NLER_BUILD_COVERAGE], [test "${nl_cv_build_coverage}" = "yes"])

NL_ENABLE_COVERAGE_REPORTS([auto])

AM_CONDITIONAL([NLER_BUILD_COVERAGE_REPORTS], [test "${nl_cv_build_coverage_reports}" = "yes"])

# Optimization

NL_ENABLE_OPTIMIZATION([yes])

AM_CONDITIONAL([NLER_BUILD_OPTIMIZED], [test "${nl_cv_build_optimized}" = "yes"])

#
# Tests
#
AC_MSG_NOTICE([checking whether to build tests])

# Tests

NL_ENABLE_TESTS([yes])

AM_CONDITIONAL([NLER_BUILD_TESTS], [test "${nl_cv_build_tests}" = "yes"])

#
# Documentation
#

# Determine whether or not documentation (via Doxygen) should be built
# or not, with 'auto' as the default and establish a default support
# value for GraphViz 'dot' support.

NL_ENABLE_DOCS([auto],[NO])

AM_CONDITIONAL(NLER_BUILD_DOCS, [test "${nl_cv_build_docs}" = "yes"])


# Check if the build host has pkg-config

AC_PATH_PROG([PKG_CONFIG],[pkg-config])

#
# Checks for libraries and packages.
#
# At minimum, the following packages are required:
#
#   * nlcompiler
#
# At minimum, the following packages are optional, depending on
# configuration:
#
#   * freertos
#   * nlunit-test
#   * nlutilities
#   * nspr
#

AC_MSG_NOTICE([checking required and optional package dependencies])

#
# Nlcompiler
#

NL_WITH_PACKAGE(
    [Nlcompiler],
    [NLCOMPILER],
    [nlcompiler],
    [],
    [
        # At this point, the internal Nlcompiler package will be neither
        # configured nor built, so the normal checks we undertake for an
        # external package cannot be run here. Simply set the appropriate
        # variables and trust all will be well.

        NLCOMPILER_CPPFLAGS="-I\${abs_top_srcdir}/third_party/nlcompiler/repo"
        NLCOMPILER_LDFLAGS=
        NLCOMPILER_LIBS=
    ],
    [
        # Check for required Nlcompiler header.

        AC_CHECK_HEADERS([nlcompiler.h],
        [],
        [
            AC_MSG_ERROR(The nlcompiler header "$ac_header" is required but cannot be found.)
        ])
    ]
)

AM_CONDITIONAL([WEAVE_WITH_NLCOMPILER_INTERNAL], [test "${nl_with_nlcompiler}" = "internal"])

if test "${NLER_BUILD_PLATFORM}" == "freertos"; then
    NL_WITH_OPTIONAL_EXTERNAL_PACKAGE([FreeRTOS],
	[FREERTOS],
	[freertos],
	[-lcommon -lkernel],
	[
	    # Check for required FreeRTOS headers

	    AC_CHECK_HEADERS([FreeRTOS.h],
		[],
		[
		    AC_MSG_NOTICE(The FreeRTOS header "$ac_header" is required but cannot be found.)
	    ])

	    # The secondary FreeRTOS headers cannot be included without FreeRTOS.h

	    AC_CHECK_HEADERS([init.h queue.h semphr.h task.h],
		[],
		[
		    AC_MSG_NOTICE(The FreeRTOS header "$ac_header" is required but cannot be found.)
		],
		[
    #ifdef HAVE_FREERTOS_H
    #include <FreeRTOS.h>
    #endif
	    ])
	]
    )
fi

AM_CONDITIONAL([NLER_WITH_FREERTOS], [test "${nl_with_freertos}" != ""])

#
# Nlunit-test
#

if test "${nl_cv_build_tests}" = "yes" || test "${nler_build_event_timer}" = "yes"; then
    NL_WITH_OPTIONAL_EXTERNAL_PACKAGE([Nlunit-test],
        [NLUNIT_TEST],
        [nlunit_test],
        [-lnlunit-test],
        [
            # Check for required nlunit-test headers.

            AC_CHECK_HEADERS([nltest.h],
            [],
            [
                AC_MSG_ERROR(The nlunit-test header "$ac_header" is required but cannot be found.)
            ])
        ]
    )
fi

#
# Nlutilities
#

if test "${nler_build_log_tokenization}" = "yes" || test "${nler_build_event_timer}" = "yes"; then
    NL_WITH_OPTIONAL_EXTERNAL_PACKAGE([Nlutilities],
        [NLUTILITIES],
        [nlutilities],
        [-lnlutilities],
        [
            # Check for required nlutilities headers.

            AC_CHECK_HEADERS([nlutilities.h nlmacros.h],
            [],
            [
                AC_MSG_ERROR(The nlutilities header "$ac_header" is required but cannot be found.)
            ])
        ]
    )
fi

if test "${NLER_BUILD_PLATFORM}" == "nspr"; then
    NL_WITH_OPTIONAL_EXTERNAL_PACKAGE([Netscape Portable Runtime],
	[NSPR],
	[nspr],
	[-lnspr4],
	[
	    # Check for required NSPR headers

	    AC_CHECK_HEADERS([nspr/prinit.h nspr/prinrval.h nspr/prio.h nspr/prlock.h nspr/prthread.h],
		[],
		[
                    AC_MSG_NOTICE(The NSPR header "$ac_header" is required but cannot be found.)
	        ]
            )
	]
    )
fi

AM_CONDITIONAL([NLER_WITH_NSPR], [test "${nl_with_nspr}" != ""])

#
# Check for POSIX thread (pthread) support
#
AX_PTHREAD([], [])

AC_CHECK_DECL([PTHREAD_NULL],
              [],
              [AC_DEFINE([PTHREAD_NULL],
                         [0],
                         [Approximation of PTHREAD_NULL since pthread.h does not define one])],
              [[#include <pthread.h>]])

#
# Check for headers
#
AC_HEADER_STDBOOL
AC_HEADER_STDC

AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([stdarg.h])
AC_CHECK_HEADERS([stdbool.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])

#
# Check for types and structures
#
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

#
# Checks for library functions
#

if test "${ac_no_link}" != "yes"; then
    AC_CHECK_FUNCS([memcpy])

    if test "${NLER_BUILD_PLATFORM}" == "pthreads"; then

        # Check if pthread_setschedprio exists. If so, prefer it. If
        # not, fall back to pthread_setschedparam.

	AC_SEARCH_LIBS([pthread_setschedprio], [pthread])
	AC_CHECK_FUNCS([pthread_setschedprio])

	if test "${ac_cv_func_pthread_setschedprio}" == "no"; then
            AC_SEARCH_LIBS([pthread_setschedparam], [pthread])
            AC_CHECK_FUNCS([pthread_setschedparam])

            if test "${ac_cv_func_pthread_setschedparam}" == "no"; then
	        AC_MSG_ERROR([unable to find either pthread_setschedprio or pthread_setschedparam to support nltask_set_priority])
            fi
	fi


	# Check if pthread_yield (most systems) or pthread_yield_np
	# (Darwin) exists.

	AC_SEARCH_LIBS([pthread_yield], [pthread])
	AC_CHECK_FUNCS([pthread_yield])

	if test "${ac_cv_func_pthread_yield}" == "no"; then
            AC_SEARCH_LIBS([pthread_yield_np], [pthread])
            AC_CHECK_FUNCS([pthread_yield_np])

            if test "${ac_cv_func_pthread_yield_np}" == "no"; then
	        AC_MSG_ERROR([unable to find either pthread_yield or pthread_yield_np to support nltask_yield])
            fi
	fi

	# Check if the non-portable pthread_setname_np function exists.

	AC_SEARCH_LIBS([pthread_setname_np], [pthread])
	AC_CHECK_FUNCS([pthread_setname_np])

	# If the non-portable pthread_setname_np exists, determine
	# whether it takes one or two arguments.

	AS_IF([test "${ac_cv_func_pthread_setname_np}" = yes], [
	    AC_MSG_CHECKING([number of arguments to pthread_setname_np()])
	    AC_TRY_COMPILE([#include <pthread.h>
	        ],[pthread_t thread = 0;
	        const char *name = "";
	        pthread_setname_np(thread, name);],[
	            AC_MSG_RESULT([2])
	            AC_DEFINE(PTHREAD_SETNAME_NP_ARGS, 2, [Number of arguments to pthread_setname_np()])],[
	                AC_TRY_COMPILE([#include <pthread.h>
	                    ],[const char *name = "";
	                    pthread_setname_np(name);],[
	                        AC_MSG_RESULT([1])
	                        AC_DEFINE(PTHREAD_SETNAME_NP_ARGS, 1, [Number of arguments to pthread_setname_np()])],[
	                        AC_MSG_RESULT([unknown])
	                        AC_MSG_ERROR([unable to determine number of arguments to pthread_setname_np()])])])
	])

        # Check for clock_gettime and gettimeofday. In some traget
        # environments, clock_gettime exists in librt.

        AC_SEARCH_LIBS([clock_gettime], [rt])
        AC_CHECK_FUNCS([clock_gettime gettimeofday])
        AC_CHECK_HEADERS([time.h sys/time.h])
        AC_CHECK_TYPES([clockid_t], [], [], [[#include <time.h>]])

        # Note that different platforms have different APIs for monotonic
        # timer with sleep time compensation. CLOCK_BOOTTIME is available
        # in later versions of Linux.

        AC_CHECK_DECLS([CLOCK_MONOTONIC, CLOCK_BOOTTIME], [], [], [[#include <time.h>]])
    fi
fi

# Accumulate any build platform-specific CPPFLAGS, LDFLAGS, and LIBS

if test "${NLER_BUILD_PLATFORM}" == "frertos"; then
    NLER_CPPFLAGS="${NLER_CPPFLAGS} ${FREERTOS_CPPFLAGS}"
    NLER_LDFLAGS="${NLER_LDFLAGS} ${FREERTOS_LDFLAGS}"
    NLER_LIBS="${NLER_LIBS} ${FREERTOS_LIBS}"
elif test "${NLER_BUILD_PLATFORM}" == "nspr"; then
    NLER_CPPFLAGS="${NLER_CPPFLAGS} ${NSPR_CPPFLAGS}"
    NLER_LDFLAGS="${NLER_LDFLAGS} ${NSPR_LDFLAGS}"
    NLER_LIBS="${NLER_LIBS} ${NSPR_LIBS}"
elif test "${NLER_BUILD_PLATFORM}" == "pthreads"; then
    NLER_CPPFLAGS="${NLER_CPPFLAGS} ${PTHREAD_CFLAGS}"
    NLER_LDFLAGS="${NLER_LDFLAGS} ${PTHREAD_CFLAGS}"
    NLER_LIBS="${NLER_LIBS} ${PTHREAD_LIBS}"
fi

# Add any code coverage CPPFLAGS and LDFLAGS

CPPFLAGS="${CPPFLAGS} ${NL_COVERAGE_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NL_COVERAGE_LDFLAGS}"

# Add any build feature and platform CPPFLAGS

CPPFLAGS="${CPPFLAGS} ${NLER_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NLER_LDFLAGS}"
LIBS="${LIBS} ${NLER_LIBS}"

# Add any nlcompiler CPPFLAGS, LDFLAGS, and LIBS

CPPFLAGS="${CPPFLAGS} ${NLCOMPILER_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NLCOMPILER_LDFLAGS}"
LIBS="${LIBS} ${NLCOMPILER_LIBS}"

# Add any nlunit-test CPPFLAGS, LDFLAGS, and LIBS

CPPFLAGS="${CPPFLAGS} ${NLUNIT_TEST_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NLUNIT_TEST_LDFLAGS}"
LIBS="${LIBS} ${NLUNIT_TEST_LIBS}"

# Add any nlutilities CPPFLAGS, LDFLAGS, and LIBS

CPPFLAGS="${CPPFLAGS} ${NLUTILITIES_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NLUTILITIES_LDFLAGS}"
LIBS="${LIBS} ${NLUTILITIES_LIBS}"

# At this point, we can restore the compiler flags to whatever the
# user passed in, now that we're clear of any -Werror issues by
# transforming -Wno-error back to -Werror.

NL_RESTORE_WERROR

#
# Identify the various makefiles and auto-generated files for the package
#
AC_CONFIG_FILES([
Makefile
third_party/Makefile
include/Makefile
shared/Makefile
arch/Makefile
freertos/Makefile
nspr/Makefile
pthreads/Makefile
test/Makefile
utilities/Makefile
doc/Makefile
])

#
# Generate the auto-generated files for the package
#
AC_OUTPUT

#
# Summarize the package configuration
#

AC_MSG_NOTICE([

  Configuration Summary
  ---------------------
  Package                                   : ${PACKAGE_NAME}
  Version                                   : ${PACKAGE_VERSION}
  Interface                                 : ${LIBNLER_VERSION_INFO//:/.}
  Build system                              : ${build}
  Host system                               : ${host}
  Target system                             : ${target}
  Target architecture                       : ${target_cpu}
  Target OS                                 : ${target_os}
  Build platform                            : ${NLER_BUILD_PLATFORM}
  Build instruction set architecture        : ${NLER_ASM_ISA}
  Cross compiling                           : ${cross_compiling}
  Build shared libraries                    : ${enable_shared}
  Build static libraries                    : ${enable_static}
  Build debug libraries                     : ${nl_cv_build_debug}
  Build optimized libraries                 : ${nl_cv_build_optimized}
  Build coverage libraries                  : ${nl_cv_build_coverage}
  Build coverage reports                    : ${nl_cv_build_coverage_reports}
  Lcov                                      : ${LCOV:--}
  Genhtml                                   : ${GENHTML:--}
  Build tests                               : ${nl_cv_build_tests}
  Build asserts                             : ${nler_build_asserts}
  Build default logger                      : ${nler_build_default_logger}
  Build event timer                         : ${nler_build_event_timer}
  Build flow tracer                         : ${nler_build_flow_tracer}
  Build log tokenization                    : ${nler_build_log_tokenization}
  Build simulateable time                   : ${nler_build_simulateable_time}
  Build software timers                     : ${nler_build_software_timers}
  Build stack alignment                     : ${nler_build_stack_alignment}
  Build task local storage                  : ${nler_build_task_local_storage}
  Build wake timer                          : ${nler_build_wake_timer}
  Build utilities                           : ${nler_build_utilities}
  Prefix                                    : ${prefix}
  Documentation support                     : ${nl_cv_build_docs}
  Doxygen                                   : ${DOXYGEN:--}
  GraphViz dot                              : ${DOT:--}
  PERL                                      : ${PERL:--}
  FreeRTOS source                           : ${nl_with_freertos:--}
  FreeRTOS compile flags                    : ${FREERTOS_CPPFLAGS:--}
  FreeRTOS link flags                       : ${FREERTOS_LDFLAGS:--}
  FreeRTOS link libraries                   : ${FREERTOS_LIBS:--}
  Nlcompiler source                         : ${nl_with_nlcompiler:--}
  Nlcompiler compile flags                  : ${NLCOMPILER_CPPFLAGS:--}
  Nlcompiler link flags                     : ${NLCOMPILER_LDFLAGS:--}
  Nlcompiler link libraries                 : ${NLCOMPILER_LIBS:--}
  Nlunit-test source                        : ${nl_with_nlunittest:--}
  Nlunit-test compile flags                 : ${NLUNIT_TEST_CPPFLAGS:--}
  Nlunit-test link flags                    : ${NLUNIT_TEST_LDFLAGS:--}
  Nlunit-test link libraries                : ${NLUNIT_TEST_LIBS:--}
  Nlutilities source                        : ${nl_with_nlutilities:--}
  Nlutilities compile flags                 : ${NLUTILITIES_CPPFLAGS:--}
  Nlutilities link flags                    : ${NLUTILITIES_LDFLAGS:--}
  Nlutilities link libraries                : ${NLUTILITIES_LIBS:--}
  NSPR source                               : ${nl_with_nspr:--}
  NSPR compile flags                        : ${NSPR_CPPFLAGS:--}
  NSPR link flags                           : ${NSPR_LDFLAGS:--}
  NSPR link libraries                       : ${NSPR_LIBS:--}
  PThreads compile flags                    : ${PTHREAD_CFLAGS:--}
  PThreads link libraries                   : ${PTHREAD_LIBS:--}
  Assembler                                 : ${CCAS}
  C Preprocessor                            : ${CPP}
  C Compiler                                : ${CC}
  Archiver                                  : ${AR}
  Archive Indexer                           : ${RANLIB}
  Symbol Stripper                           : ${STRIP}
  Object Copier                             : ${OBJCOPY}
  Assembler flags                           : ${CCASFLAGS:--}
  C Preprocessor flags                      : ${CPPFLAGS:--}
  C Compile flags                           : ${CFLAGS:--}
  Link flags                                : ${LDFLAGS:--}
  Link libraries                            : ${LIBS}

])
