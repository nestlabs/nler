#
#    Copyright (c) 2018 Nest Labs, Inc.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      This file is the GNU automake template for the Nest Labs Embedded
#      Runtime test applications
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                                     = \
    test-nlertimer.c                             \
    test-oneshot-timer.c                         \
    test-repeated-timer.c                        \
    $(NULL)

noinst_HEADERS                                 = \
    nltestlogregions.h                           \
    test-settings.h                              \
    $(NULL)

if NLER_BUILD_TESTS
lib_LIBRARIES                                  = \
    libnlertest.a                                \
    $(NULL)

# C preprocessor option flags that will apply to all compiled objects in this
# makefile.

AM_CPPFLAGS                                    = \
    -DNLER_APPLICATION_LOG_REGION_FILE=\"nltestlogregions.h\" \
    -I$(top_srcdir)/include                      \
    $(NULL)

libnlertest_a_SOURCES                          = \
    nlertimer-test.c                             \
    $(NULL)

NLER_LDADD                                     = \
    $(top_builddir)/$(NLER_BUILD_PLATFORM)/libnler$(NLER_BUILD_PLATFORM).a \
    $(top_builddir)/arch/libnlerarch.a           \
    $(top_builddir)/shared/libnlershared.a       \
    $(NULL)

COMMON_LDADD                                   = \
    $(COMMON_LDFLAGS)                            \
    $(NLER_LDADD)                                \
    libnlertest.a                                \
    -L$(top_builddir)/$(NLER_BUILD_PLATFORM) -lnler$(NLER_BUILD_PLATFORM) \
    $(NULL)

# Test applications that should be run when the 'check' target is run.

check_PROGRAMS                                 = \
    test-atomic                                  \
    test-earlyevent                              \
    test-event                                   \
    test-eventqueue                              \
    test-lock                                    \
    test-nlmathutil                              \
    test-pooledevent                             \
    test-task                                    \
    $(NULL)

if NLER_BUILD_FLOW_TRACER
check_PROGRAMS                                += \
    test-nlerflowtracer                          \
    $(NULL)
endif # NLER_BUILD_FLOW_TRACER

if NLER_BUILD_UTILITIES
check_PROGRAMS                                += \
    test-settings                                \
    $(NULL)
endif # NLER_BUILD_UTILITIES

if !NLER_BUILD_EVENT_TIMER
check_PROGRAMS                                += \
    test-subpub                                  \
    test-timer                                   \
    $(NULL)
endif # !NLER_BUILD_EVENT_TIMER

# Test applications and scripts that should be built and run when the
# 'check' target is run.

TESTS                                          = \
    $(check_PROGRAMS)                            \
    $(NULL)

# The additional environment variables and their values that will be
# made available to all programs and scripts in TESTS.

TESTS_ENVIRONMENT                              = \
    $(NULL)

# Source, compiler, and linker options for test programs.

test_atomic_SOURCES                      = test-atomic.c nltestlogregions.c
test_atomic_LDADD                        = $(COMMON_LDADD)

test_earlyevent_SOURCES                  = test-earlyevent.c nltestlogregions.c
test_earlyevent_LDADD                    = $(COMMON_LDADD)

test_event_SOURCES                       = test-event.c nltestlogregions.c
test_event_LDADD                         = $(COMMON_LDADD)

test_eventqueue_SOURCES                  = test-eventqueue.c nltestlogregions.c
test_eventqueue_LDADD                    = $(COMMON_LDADD)

test_lock_SOURCES                        = test-lock.c nltestlogregions.c
test_lock_LDADD                          = $(COMMON_LDADD)

test_nlerflowtracer_SOURCES              = test-nlerflowtracer.c nltestlogregions.c
test_nlerflowtracer_LDADD                = $(COMMON_LDADD)

test_nlmathutil_SOURCES                  = test-nlmathutil.c nltestlogregions.c
test_nlmathutil_LDADD                    = $(COMMON_LDADD)

test_pooledevent_SOURCES                 = test-pooledevent.c nltestlogregions.c
test_pooledevent_LDADD                   = $(COMMON_LDADD)

test_settings_SOURCES                    = test-settings.c nltestlogregions.c
test_settings_CPPFLAGS                   = $(AM_CPPFLAGS) -DHAVE_NLER_SETTINGS_APPLICATION_SETTINGS_KEYS -DNLER_SETTINGS_APPLICATION_SETTINGS_KEYS=\"test-settings.h\"
test_settings_LDADD                      = -L$(top_builddir)/utilities -lnlerutilities $(COMMON_LDADD)

test_subpub_SOURCES                      = test-subpub.c nltestlogregions.c
test_subpub_LDADD                        = $(COMMON_LDADD)

test_task_SOURCES                        = test-task.c nltestlogregions.c
test_task_LDADD                          = $(COMMON_LDADD)

test_timer_SOURCES                       = test-timer.c nltestlogregions.c
test_timer_LDADD                         = $(COMMON_LDADD)

if NLER_BUILD_COVERAGE
CLEANFILES                               = $(wildcard *.gcda *.gcno)

if NLER_BUILD_COVERAGE_REPORTS
# The bundle should positively be qualified with the absolute build
# path. Otherwise, VPATH will get auto-prefixed to it if there is
# already such a directory in the non-colocated source tree.

NLER_COVERAGE_BUNDLE                    = ${abs_builddir}/${PACKAGE}${NL_COVERAGE_BUNDLE_SUFFIX}
NLER_COVERAGE_INFO                      = ${NLER_COVERAGE_BUNDLE}/${PACKAGE}${NL_COVERAGE_INFO_SUFFIX}

$(NLER_COVERAGE_BUNDLE):
	$(call create-directory)

$(NLER_COVERAGE_INFO): check | $(NLER_COVERAGE_BUNDLE)
	$(call generate-coverage-report,${abs_top_builddir})

coverage: $(NLER_COVERAGE_INFO)

clean-local: clean-local-coverage

.PHONY: clean-local-coverage
clean-local-coverage:
	-$(AM_V_at)rm -rf $(NLER_COVERAGE_BUNDLE)
endif # NLER_BUILD_COVERAGE_REPORTS
endif # NLER_BUILD_COVERAGE
endif # NLER_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
