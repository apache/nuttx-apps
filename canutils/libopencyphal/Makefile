############################################################################
# apps/canutils/libopencyphal/Makefile
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

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

UNPACK = unzip
PACKEXT = .zip

LIBOPENCYPHAL_URL = $(patsubst "%",%,$(strip $(CONFIG_LIBOPENCYPHAL_URL)))
LIBOPENCYPHAL_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBOPENCYPHAL_VERSION)))

LIBOPENCYPHAL_PACKNAME = $(LIBOPENCYPHAL_UNPACKNAME)$(PACKEXT)
LIBOPENCYPHAL_UNPACKNAME = libcanard-$(LIBOPENCYPHAL_VERSION)
LIBOPENCYPHAL_SRCNAME = libcanard

LIBOPENCYPHAL_SRCDIR = $(WD)/$(LIBOPENCYPHAL_SRCNAME)
LIBOPENCYPHAL_DRVDIR = $(LIBOPENCYPHAL_SRCDIR)$(DELIM)libcanard

O1HEAP_URL = $(patsubst "%",%,$(strip $(CONFIG_O1HEAP_URL)))
O1HEAP_VERSION = $(patsubst "%",%,$(strip $(CONFIG_O1HEAP_VERSION)))

O1HEAP_PACKNAME = $(O1HEAP_UNPACKNAME)$(PACKEXT)
O1HEAP_UNPACKNAME = o1heap-$(O1HEAP_VERSION)
O1HEAP_SRCNAME = o1heap

O1HEAP_SRCDIR = $(WD)/$(O1HEAP_SRCNAME)
O1HEAP_DRVDIR = $(O1HEAP_SRCDIR)$(DELIM)o1heap

CFLAGS += -std=c11 -DCANARD_ASSERT=DEBUGASSERT -DCANARD_DSDL_CONFIG_LITTLE_ENDIAN=1

CSRCS  = $(LIBOPENCYPHAL_DRVDIR)$(DELIM)canard.c $(LIBOPENCYPHAL_DRVDIR)$(DELIM)canard_dsdl.c
CSRCS += $(O1HEAP_DRVDIR)$(DELIM)o1heap.c

ifeq ($(wildcard $(LIBOPENCYPHAL_SRCNAME)/.git),)
$(LIBOPENCYPHAL_PACKNAME):
	@echo "Downloading: $@"
	$(Q) curl -o $@ -L $(LIBOPENCYPHAL_URL)$(DELIM)$(LIBOPENCYPHAL_VERSION)$(PACKEXT)

$(LIBOPENCYPHAL_SRCNAME): $(LIBOPENCYPHAL_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) mv $(LIBOPENCYPHAL_UNPACKNAME) $(LIBOPENCYPHAL_SRCNAME)
	$(Q) touch $@
	
$(LIBOPENCYPHAL_DRVDIR)$(DELIM)canard.c: $(LIBOPENCYPHAL_SRCNAME)
endif

ifeq ($(wildcard $(O1HEAP_SRCNAME)/.git),)
$(O1HEAP_PACKNAME):
	@echo "Downloading: $@"
	$(Q) curl -o $@ -L $(O1HEAP_URL)$(DELIM)$(O1HEAP_VERSION)$(PACKEXT)

$(O1HEAP_SRCNAME): $(O1HEAP_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) mv $(O1HEAP_UNPACKNAME) $(O1HEAP_SRCNAME)
	$(Q) touch $@

$(O1HEAP_DRVDIR)$(DELIM)o1heap.c: $(O1HEAP_SRCNAME)
endif

context:: $(LIBOPENCYPHAL_SRCNAME) $(O1HEAP_SRCNAME)

distclean::
	$(call DELDIR, $(LIBOPENCYPHAL_SRCNAME))
	$(call DELFILE, $(LIBOPENCYPHAL_PACKNAME))
	$(call DELDIR, $(O1HEAP_SRCNAME))
	$(call DELFILE, $(O1HEAP_PACKNAME))

include $(APPDIR)/Application.mk
