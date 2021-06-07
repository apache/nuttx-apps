############################################################################
# apps/platform/Makefile
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

CONFIG_ARCH_BOARD ?= dummy

# Directories

PLATFORMDIR = board
DUMMYDIR    = $(APPDIR)/platform/dummy

ifeq ($(CONFIG_ARCH_BOARD_CUSTOM),y)
  LINKDIR   = $(DUMMYDIR)
else
  BOARDDIR  = $(APPDIR)$(DELIM)platform$(DELIM)$(CONFIG_ARCH_BOARD)
  LINKDIR   = $(if $(wildcard $(BOARDDIR)$(DELIM)Make.defs),$(BOARDDIR),$(DUMMYDIR))
endif

VPATH       += board
DEPPATH += --obj-path bin
DEPPATH += --obj-suffix $(OBJEXT)
DEPPATH += --dep-path $(PLATFORMDIR)

# Binaries

-include $(PLATFORMDIR)$(DELIM)Make.defs

# Build context setup

$(TOPDIR)$(DELIM).config:

$(PLATFORMDIR): $(TOPDIR)$(DELIM).config
	@echo "LN: platform$(DELIM)board to $(LINKDIR)"
	$(Q) $(DIRLINK) $(LINKDIR) $(PLATFORMDIR)

dirlinks: $(PLATFORMDIR)

context:: dirlinks

clean_context:
	$(Q) $(DIRUNLINK) $(PLATFORMDIR)

clean::
	$(Q) $(MAKE) -C bin TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" clean

include $(APPDIR)/Application.mk
