############################################################################
# apps/graphics/tinygl/Makefile
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

# TinyGL software OpenGL library

TINYGL_URL      = $(patsubst "%",%,$(strip $(CONFIG_TINYGL_URL)))
TINYGL_VERSION  = $(patsubst "%",%,$(strip $(CONFIG_TINYGL_VERSION)))
TINYGL_TARBALL  = tinygl-$(TINYGL_VERSION).tar.gz
TINYGL_UNPACK   = tinygl
TINYGL_PATCHES  = $(sort $(wildcard $(CURDIR)/[0-9][0-9][0-9][0-9]-*.patch))

SRC = $(TINYGL_UNPACK)/src

CSRCS  = $(SRC)/accum.c
CSRCS += $(SRC)/api.c
CSRCS += $(SRC)/arrays.c
CSRCS += $(SRC)/clear.c
CSRCS += $(SRC)/clip.c
CSRCS += $(SRC)/get.c
CSRCS += $(SRC)/image_util.c
CSRCS += $(SRC)/init.c
CSRCS += $(SRC)/light.c
CSRCS += $(SRC)/list.c
CSRCS += $(SRC)/matrix.c
CSRCS += $(SRC)/memory.c
CSRCS += $(SRC)/misc.c
CSRCS += $(SRC)/msghandling.c
CSRCS += $(SRC)/select.c
CSRCS += $(SRC)/specbuf.c
CSRCS += $(SRC)/texture.c
CSRCS += $(SRC)/vertex.c
CSRCS += $(SRC)/zbuffer.c
CSRCS += $(SRC)/zline.c
CSRCS += $(SRC)/zmath.c
CSRCS += $(SRC)/zpostprocess.c
CSRCS += $(SRC)/zraster.c
CSRCS += $(SRC)/ztext.c
CSRCS += $(SRC)/ztriangle.c

# TinyGL is third party code, silence the warnings that the NuttX build
# enables but that upstream does not care about.

CFLAGS += -Wno-shadow -Wno-undef -Wno-unused-function -Wno-unknown-pragmas
CFLAGS += -Wno-unused-variable -DNDEBUG

$(TINYGL_TARBALL):
	$(ECHO_BEGIN)"Downloading: $(TINYGL_TARBALL)"
	$(Q) curl -L $(if $(V),,-Ss) -o $(TINYGL_TARBALL) \
		$(TINYGL_URL)/$(TINYGL_VERSION).tar.gz
	$(ECHO_END)

# The stamp file marks a complete unpack + patch, so that a partially
# created tree (for example an object directory) is never mistaken for
# the sources.

TINYGL_STAMP = $(TINYGL_UNPACK)/.nuttx_patched

$(TINYGL_STAMP): $(TINYGL_TARBALL)
	$(ECHO_BEGIN)"Unpacking: $(TINYGL_TARBALL) -> $(TINYGL_UNPACK)"
	$(Q) rm -rf $(TINYGL_UNPACK) tinygl-$(TINYGL_VERSION)
	$(Q) tar zxf $(TINYGL_TARBALL)
	$(Q) mv tinygl-$(TINYGL_VERSION) $(TINYGL_UNPACK)
	$(Q) for p in $(TINYGL_PATCHES); do \
		patch -s -N -p1 -d $(TINYGL_UNPACK) < $$p || exit 1; \
	done
	$(Q) touch $@
	$(ECHO_END)

# Download and unpack tarball if no git repo found

ifeq ($(wildcard $(TINYGL_UNPACK)/.git),)
context:: $(TINYGL_STAMP)

distclean::
	$(call DELDIR, $(TINYGL_UNPACK))
	$(call DELFILE, $(TINYGL_TARBALL))
endif

include $(APPDIR)/Application.mk
