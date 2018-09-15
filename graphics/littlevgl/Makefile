############################################################################
# apps/graphics/littlevgl/Makefile
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

-include $(TOPDIR)/Make.defs

# LVGL graphic library

ASRCS =
CSRCS =
VPATH =

# LVGL Libraries

-include ./lvgl/lv_core/lv_core.mk
-include ./lvgl/lv_hal/lv_hal.mk
-include ./lvgl/lv_objx/lv_objx.mk
-include ./lvgl/lv_misc/lv_fonts/lv_fonts.mk
-include ./lvgl/lv_misc/lv_misc.mk
-include ./lvgl/lv_themes/lv_themes.mk
-include ./lvgl/lv_draw/lv_draw.mk

# Set up build configuration and environment

WD := ${shell pwd | sed -e 's/ /\\ /g'}

CONFIG_GRAPH_LVGL_URL ?= "https://littlevgl.com/download/"

LVGL_VERSION ?= v5_1_1
LVGL_TARBALL = lvgl_$(LVGL_VERSION).tar.gz
WGET ?= wget

LVGL_UNPACKNAME = lvgl
UNPACK ?= tar -zxf

LVGL_UNPACKDIR =  $(WD)/$(LVGL_UNPACKNAME)

CFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/graphics/littlevgl}

$(LVGL_TARBALL):
	@echo "Downloading: $(LVGL_TARBALL)"
	$(Q) $(WGET) $(CONFIG_GRAPH_LVGL_URL)/$(LVGL_TARBALL)

$(LVGL_UNPACKNAME): $(LVGL_TARBALL)
	@echo "Unpacking: $(LVGL_TARBALL) -> $(LVGL_UNPACKNAME)"
	$(Q) $(UNPACK) $(LVGL_TARBALL)
	$(Q) touch $(LVGL_UNPACKNAME)

$(APPDIR)/include/graphics/lvgl.h: $(LVGL_UNPACKNAME) lvgl/lvgl.h
	@echo "CP: lvgl/lvgl.h"
	$(Q) cp lvgl/lvgl.h $(APPDIR)/include/graphics/lvgl.h

context:: $(LVGL_UNPACKNAME) $(APPDIR)/include/graphics/lvgl.h

clean::
	$(call DELDIR, build)

distclean::
	$(call DELDIR, $(LVGL_UNPACKNAME))
	$(call DELFILE, $(LVGL_TARBALL))
	$(call DELFILE, $(APPDIR)/include/graphics/lvgl.h)

include $(APPDIR)/Application.mk
