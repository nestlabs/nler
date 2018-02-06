#
#    Copyright 2015-2017 Nest Labs Inc. All Rights Reserved.
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
#      Runtime public headers.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

include_HEADERS             = \
    nlerassert.h              \
    nleratomicops.h           \
    nlercfg.h                 \
    nlererror.h               \
    nlerevent.h               \
    nlereventpooled.h         \
    nlereventqueue.h          \
    nlereventqueue_sim.h      \
    nlereventtypes.h          \
    nlerinit.h                \
    nlerlock.h                \
    nlerlog.h                 \
    nlerlogmanager.h          \
    nlerlogregion.h           \
    nlerlogtoken.h            \
    nlermacros.h              \
    nlermathutil.h            \
    nlertask.h                \
    nlertime.h                \
    nlertimer.h               \
    nlertimer_sim.h           \
    $(NULL)

if NLER_BUILD_EVENT_TIMER
include_HEADERS            += \
    nlerevent_timer.h         \
    $(NULL)
endif # NLER_BUILD_EVENT_TIMER

if NLER_BUILD_FLOW_TRACER
include_HEADERS            += \
    nlerflowtrace-enum.h      \
    nlerflowtracer.h          \
    $(NULL)
endif # NLER_BUILD_FLOW_TRACER

if NLER_BUILD_UTILITIES
include_HEADERS            += \
    nllist.h                  \
    nlresendabletimer.h       \
    nlsettings.h              \
    $(NULL)
endif # NLER_BUILD_UTILITIES

install-headers: install-data

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
