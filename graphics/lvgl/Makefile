############################################################################
# apps/graphics/lvgl/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# LVGL graphic library

LVGL_DIR = .
LVGL_DIR_NAME = lvgl

-include ./lvgl/lvgl.mk

ifneq ($(CONFIG_LV_ASSERT_HANDLER_INCLUDE), "")
CFLAGS += "-DLV_ASSERT_HANDLER=ASSERT(0);"
endif

ifeq ($(and $(CONFIG_LV_USE_PERF_MONITOR),$(CONFIG_SCHED_CPULOAD)),y)
CFLAGS += "-DLV_SYSMON_GET_IDLE=lv_nuttx_get_idle"
endif

ifeq ($(and $(CONFIG_SCHED_INSTRUMENTATION),$(CONFIG_LV_USE_PROFILER)),y)
CFLAGS += "-DLV_PROFILER_BEGIN=sched_note_beginex(NOTE_TAG_GRAPHICS, __func__)"
CFLAGS += "-DLV_PROFILER_END=sched_note_endex(NOTE_TAG_GRAPHICS, __func__)"

CFLAGS += "-DLV_PROFILER_BEGIN_TAG(str)=sched_note_beginex(NOTE_TAG_GRAPHICS, str)"
CFLAGS += "-DLV_PROFILER_END_TAG(str)=sched_note_endex(NOTE_TAG_GRAPHICS, str)"
endif

ifneq ($(CONFIG_LV_OPTLEVEL), "")
# Since multiple options need to be supported, subst needs to be used here to remove
# the redundant double quotes, otherwise it will cause command parsing errors.
CFLAGS   += $(subst ",, $(CONFIG_LV_OPTLEVEL))
CXXFLAGS += $(subst ",, $(CONFIG_LV_OPTLEVEL))
endif

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

CONFIG_GRAPH_LVGL_URL ?= "https://github.com/lvgl/lvgl/archive/refs/tags"

LVGL_VERSION = 9.2.1
LVGL_TARBALL = v$(LVGL_VERSION).zip

LVGL_UNPACKNAME = lvgl
UNPACK ?= unzip -o $(if $(V),,-q)
CURL ?= curl -L -O $(if $(V),,-Ss)

LVGL_UNPACKDIR =  $(WD)/$(LVGL_UNPACKNAME)

$(LVGL_TARBALL):
	$(ECHO_BEGIN)"Downloading: $(LVGL_TARBALL)"
	$(Q) $(CURL) $(CONFIG_GRAPH_LVGL_URL)/$(LVGL_TARBALL)
	$(ECHO_END)

$(LVGL_UNPACKNAME): $(LVGL_TARBALL)
	$(ECHO_BEGIN)"Unpacking: $(LVGL_TARBALL) -> $(LVGL_UNPACKNAME)"
	$(Q) $(UNPACK) $(LVGL_TARBALL)
	$(Q) mv	lvgl-$(LVGL_VERSION) $(LVGL_UNPACKNAME)
	$(Q) touch $(LVGL_UNPACKNAME)
	$(ECHO_END)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LVGL_UNPACKNAME)/.git),)
context:: $(LVGL_UNPACKNAME)
endif

include $(APPDIR)/Application.mk

ifeq ($(wildcard $(LVGL_UNPACKNAME)/.git),)
distclean::
	$(call DELDIR, $(LVGL_UNPACKNAME))
	$(call DELFILE, $(LVGL_TARBALL))
endif
