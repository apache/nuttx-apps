############################################################################
# apps/graphics/uGLy/Makefile
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

# uGLy graphic library

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

CONFIG_GRAPH_UGLY_URL ?= "https://github.com/TheFakeMontyOnTheRun/uGLy/archive/refs/tags"

UGLY_PACKAGE := test-for-0.1
UGLY_TARBALL := $(UGLY_PACKAGE).zip

UGLY_UNPACKNAME = uGLy
UNPACK ?= unzip -o $(if $(V),,-q)
CURL ?= curl -L -O $(if $(V),,-Ss)

UGLY_UNPACKDIR =  $(WD)/$(UGLY_UNPACKNAME)

# Patches applied on top of the uGLy release, in name order

UGLY_PATCHES := $(sort $(wildcard patches/*.patch))

$(UGLY_TARBALL):
	$(ECHO_BEGIN)"Downloading: $(UGLY_TARBALL)"
	$(Q) $(CURL) $(CONFIG_GRAPH_UGLY_URL)/$(UGLY_TARBALL)
	$(ECHO_END)

$(UGLY_UNPACKNAME): $(UGLY_TARBALL) $(UGLY_PATCHES)
	$(ECHO_BEGIN)"Unpacking: $(UGLY_TARBALL) -> $(UGLY_UNPACKNAME)"
	$(Q) $(UNPACK) $(UGLY_TARBALL)
	$(call DELDIR, $(UGLY_UNPACKNAME))
	$(Q) mv uGLy-$(UGLY_PACKAGE) $(UGLY_UNPACKNAME)
	$(Q) $(foreach p,$(UGLY_PATCHES),patch -s -p1 -d $(UGLY_UNPACKNAME) < $(p) &&) true
	$(Q) touch $(UGLY_UNPACKNAME)
	$(Q) cp Make_uGLy.defs uGLy/Make.defs
	$(ECHO_END)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(UGLY_UNPACKNAME)/.git),)
context:: $(UGLY_UNPACKNAME)
endif

# only includes this file next time when it was copied in the above step
-include uGLy/Make.defs

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/graphics/uGLy/uGLy/third_party/fpsqrt
CFLAGS += -DBPP24 -DDISABLE_STENCIL_BUFFER
CFLAGS += -DXRES_FRAMEBUFFER=$(CONFIG_GRAPHICS_UGLY_XRES)
CFLAGS += -DYRES_FRAMEBUFFER=$(CONFIG_GRAPHICS_UGLY_YRES)

include $(APPDIR)/Application.mk

ifeq ($(wildcard $(UGLY_UNPACKNAME)/.git),)
distclean::
	$(call DELDIR, $(UGLY_UNPACKNAME))
	$(call DELFILE, $(UGLY_TARBALL))
endif
