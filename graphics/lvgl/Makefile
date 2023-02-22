############################################################################
# apps/graphics/lvgl/Makefile
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

# Relax LVGL's format checking and unused variable checking to avoid errors

CFLAGS += -Wno-format -Wno-format-security -Wno-unused-variable

-include ./lvgl/lvgl.mk

CSRCS += port/lv_port.c
CSRCS += port/lv_port_tick.c

ifeq ($(CONFIG_LIBUV),y)
CSRCS += port/lv_port_libuv.c
endif

ifeq ($(CONFIG_LV_PORT_USE_LCDDEV),y)
CSRCS += port/lv_port_lcddev.c
endif

ifeq ($(CONFIG_LV_PORT_USE_FBDEV),y)
CSRCS += port/lv_port_fbdev.c
endif

ifeq ($(CONFIG_LV_PORT_USE_TOUCHPAD),y)
CSRCS += port/lv_port_touchpad.c
endif

ifeq ($(CONFIG_LV_PORT_USE_BUTTON),y)
CSRCS += port/lv_port_button.c
endif

ifeq ($(CONFIG_LV_PORT_USE_KEYPAD),y)
CSRCS += port/lv_port_keypad.c
endif

ifeq ($(CONFIG_LV_PORT_USE_ENCODER),y)
CSRCS += port/lv_port_encoder.c
endif

ifeq ($(CONFIG_LV_USE_LOG),y)
CSRCS += port/lv_port_syslog.c
endif

ifneq ($(CONFIG_LV_PORT_MEM_ATTRIBUTE_FAST_MEM_SECTION_NAME), "")
CFLAGS   += "-DLV_ATTRIBUTE_FAST_MEM=locate_data(CONFIG_LV_PORT_MEM_ATTRIBUTE_FAST_MEM_SECTION_NAME)"
CXXFLAGS += "-DLV_ATTRIBUTE_FAST_MEM=locate_data(CONFIG_LV_PORT_MEM_ATTRIBUTE_FAST_MEM_SECTION_NAME)"
endif

ifeq ($(CONFIG_LV_MEM_CUSTOM),y)
ifneq ($(CONFIG_LV_PORT_MEM_CUSTOM_SIZE), 0)
CFLAGS += "-DLV_MEM_CUSTOM_ALLOC=lv_port_mem_alloc"
CFLAGS += "-DLV_MEM_CUSTOM_FREE=lv_port_mem_free"
CFLAGS += "-DLV_MEM_CUSTOM_REALLOC=lv_port_mem_realloc"
CSRCS  += port/lv_port_mem.c
endif
endif

ifneq ($(CONFIG_LV_ASSERT_HANDLER_INCLUDE), "")
CFLAGS += "-DLV_ASSERT_HANDLER=ASSERT(0);"
endif

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

CONFIG_GRAPH_LVGL_URL ?= "https://github.com/lvgl/lvgl/archive/refs/tags"

LVGL_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LVGL_VERSION)))
LVGL_TARBALL = v$(LVGL_VERSION).zip

LVGL_UNPACKNAME = lvgl
UNPACK ?= unzip -o

LVGL_UNPACKDIR =  $(WD)/$(LVGL_UNPACKNAME)

$(LVGL_TARBALL):
	$(Q) echo "Downloading: $(LVGL_TARBALL)"
	$(Q) curl -O -L $(CONFIG_GRAPH_LVGL_URL)/$(LVGL_TARBALL)

$(LVGL_UNPACKNAME): $(LVGL_TARBALL)
	$(Q) echo "Unpacking: $(LVGL_TARBALL) -> $(LVGL_UNPACKNAME)"
	$(Q) $(UNPACK) $(LVGL_TARBALL)
	$(Q) mv	lvgl-$(LVGL_VERSION) $(LVGL_UNPACKNAME)
	$(Q) touch $(LVGL_UNPACKNAME)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LVGL_UNPACKNAME)/.git),)
context:: $(LVGL_UNPACKNAME)

distclean::
	$(call DELDIR, $(LVGL_UNPACKNAME))
	$(call DELFILE, $(LVGL_TARBALL))
endif

include $(APPDIR)/Application.mk
