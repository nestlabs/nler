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
#      This file is the GNU automake template for the Nest Labs
#      Embedded Runtime architecture-independent library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

lib_LIBRARIES                   = \
    libnlershared.a               \
    $(NULL)

libnlershared_a_CPPFLAGS        = \
    -I$(top_srcdir)/include       \
    $(NULL)

libnlershared_a_SOURCES         = \
    nlerevent.c                   \
    nlerlog.c                     \
    nlerlogmanager.c              \
    nlermathutil.c                \
    nlertime.c                    \
    nlertimer.c                   \
    nlertimer_sim.c               \
    nleventqueue_sim.c            \
    $(NULL)

if NLER_BUILD_EVENT_TIMER
libnlershared_a_SOURCES        += \
    nlerevent_timer.c             \
    $(NULL)
endif # NLER_BUILD_EVENT_TIMER

if NLER_BUILD_FLOW_TRACER
libnlershared_a_SOURCES        += \
    nlerflowtracer.c              \
    $(NULL)
endif # NLER_BUILD_FLOW_TRACER

include $(abs_top_nlbuild_autotools_dir)/automake/post.am

