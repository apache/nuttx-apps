############################################################################
# apps/examples/lgvldemo/Makefile
#
#   Copyright (C) 2018 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

include $(APPDIR)/Make.defs

# LittleVGL demo built-in application info

CONFIG_LV_EXAMPLES_URL ?= https://github.com/lvgl/lv_examples/archive

LVGL_EXAMPLES_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LVGL_VERSION)))
LVGL_EXAMPLES_TARBALL = v$(LVGL_EXAMPLES_VERSION).zip

WGET ?= wget
LVGL_EXAMPLES_UNPACKNAME = lv_examples
UNPACK ?= unzip -o

PROGNAME = lvgldemo
PRIORITY = $(CONFIG_EXAMPLES_LVGLDEMO_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_LVGLDEMO_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_LVGLDEMO)

# LittleVGL demo Example

CSRCS += fbdev.c tp.c tp_cal.c

# static common assets used in mutiple example

LV_EXAMPLE_ASSETS = $(wildcard ./lv_examples/assets/*.c)
CSRCS += $(notdir $(LV_EXAMPLE_ASSETS))
VPATH += lv_examples/assets

ifneq ($(CONFIG_EXAMPLES_LVGLDEMO_BENCHMARK),)
CSRCS += lv_demo_benchmark.c
VPATH += lv_examples/src/lv_demo_benchmark
endif

ifneq ($(CONFIG_EXAMPLES_LVGLDEMO_PRINTER),)
LV_PRINTER_IMAGES = $(wildcard ./lv_examples/src/lv_demo_printer/images/*.c)
CSRCS += lv_demo_printer.c lv_demo_printer_theme.c
CSRCS += $(notdir $(LV_PRINTER_IMAGES))
VPATH += lv_examples/src/lv_demo_printer
VPATH += lv_examples/src/lv_demo_printer/images
endif

ifneq ($(CONFIG_EXAMPLES_LVGLDEMO_STRESS),)
CSRCS += lv_demo_stress.c
VPATH += lv_examples/src/lv_demo_stress
endif

ifneq ($(CONFIG_EXAMPLES_LVGLDEMO_WIDGETS),)
CSRCS += lv_demo_widgets.c
VPATH += lv_examples/src/lv_demo_widgets
endif

MAINSRC = lvgldemo.c

CFLAGS += ${shell $(DEFINE) "$(CC)" LV_LVGL_H_INCLUDE_SIMPLE}
CXXFLAGS += ${shell $(DEFINE) "$(CC)" LV_LVGL_H_INCLUDE_SIMPLE}

$(LVGL_EXAMPLES_TARBALL):
	@echo "Downloading: $(LVGL_EXAMPLES_TARBALL)"
	$(Q) $(WGET) $(CONFIG_LV_EXAMPLES_URL)/$(LVGL_EXAMPLES_TARBALL)

$(LVGL_EXAMPLES_UNPACKNAME): $(LVGL_EXAMPLES_TARBALL)
	@echo "Unpacking: $(LVGL_EXAMPLES_TARBALL) -> $(LVGL_EXAMPLES_UNPACKNAME)"
	$(Q) $(UNPACK) $(LVGL_EXAMPLES_TARBALL)
	$(Q) mv	lv_examples-$(LVGL_EXAMPLES_VERSION) $(LVGL_EXAMPLES_UNPACKNAME)
	$(Q) touch $(LVGL_EXAMPLES_UNPACKNAME)

context:: $(LVGL_EXAMPLES_UNPACKNAME)

distclean::
	$(call DELDIR, $(LVGL_EXAMPLES_UNPACKNAME))
	$(call DELFILE, $(LVGL_EXAMPLES_TARBALL))

include $(APPDIR)/Application.mk
