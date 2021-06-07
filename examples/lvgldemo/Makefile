############################################################################
# apps/examples/lgvldemo/Makefile
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

# LittleVGL demo built-in application info

CONFIG_LV_EXAMPLES_URL ?= https://github.com/lvgl/lv_demos/archive

LVGL_EXAMPLES_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LVGL_VERSION)))
LVGL_EXAMPLES_TARBALL = v$(LVGL_EXAMPLES_VERSION).zip

LVGL_EXAMPLES_UNPACKNAME = lv_demos
UNPACK ?= unzip -o

PROGNAME = lvgldemo
PRIORITY = $(CONFIG_EXAMPLES_LVGLDEMO_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_LVGLDEMO_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_LVGLDEMO)

# LittleVGL demo Example

CSRCS += fbdev.c lcddev.c

ifneq ($(CONFIG_INPUT_TOUCHSCREEN)$(CONFIG_INPUT_MOUSE),)
CSRCS += tp.c tp_cal.c
endif

# static common assets used in multiple examples

LV_EXAMPLE_ASSETS = $(wildcard ./lv_demos/assets/*.c)
CSRCS += $(notdir $(LV_EXAMPLE_ASSETS))
VPATH += lv_demos/assets

ifneq ($(CONFIG_EXAMPLES_LVGLDEMO_BENCHMARK),)
CSRCS += lv_demo_benchmark.c
VPATH += lv_demos/src/lv_demo_benchmark
endif

ifneq ($(CONFIG_EXAMPLES_LVGLDEMO_PRINTER),)
LV_PRINTER_IMAGES = $(wildcard ./lv_demos/src/lv_demo_printer/images/*.c)
CSRCS += lv_demo_printer.c lv_demo_printer_theme.c
CSRCS += $(notdir $(LV_PRINTER_IMAGES))
VPATH += lv_demos/src/lv_demo_printer
VPATH += lv_demos/src/lv_demo_printer/images
endif

ifneq ($(CONFIG_EXAMPLES_LVGLDEMO_STRESS),)
CSRCS += lv_demo_stress.c
VPATH += lv_demos/src/lv_demo_stress
endif

ifneq ($(CONFIG_EXAMPLES_LVGLDEMO_WIDGETS),)
CSRCS += lv_demo_widgets.c
VPATH += lv_demos/src/lv_demo_widgets
endif

MAINSRC = lvgldemo.c

CFLAGS += ${shell $(DEFINE) "$(CC)" LV_LVGL_H_INCLUDE_SIMPLE} -Wno-format
CXXFLAGS += ${shell $(DEFINE) "$(CC)" LV_LVGL_H_INCLUDE_SIMPLE} -Wno-format

$(LVGL_EXAMPLES_TARBALL):
	$(Q) echo "Downloading: $(LVGL_EXAMPLES_TARBALL)"
	$(Q) curl -O -L $(CONFIG_LV_EXAMPLES_URL)/$(LVGL_EXAMPLES_TARBALL)

$(LVGL_EXAMPLES_UNPACKNAME): $(LVGL_EXAMPLES_TARBALL)
	$(Q) echo "Unpacking: $(LVGL_EXAMPLES_TARBALL) -> $(LVGL_EXAMPLES_UNPACKNAME)"
	$(Q) $(UNPACK) $(LVGL_EXAMPLES_TARBALL)
	$(Q) mv	lv_demos-$(LVGL_EXAMPLES_VERSION) $(LVGL_EXAMPLES_UNPACKNAME)
	$(Q) touch $(LVGL_EXAMPLES_UNPACKNAME)

context:: $(LVGL_EXAMPLES_UNPACKNAME)

distclean::
	$(call DELDIR, $(LVGL_EXAMPLES_UNPACKNAME))
	$(call DELFILE, $(LVGL_EXAMPLES_TARBALL))

include $(APPDIR)/Application.mk
