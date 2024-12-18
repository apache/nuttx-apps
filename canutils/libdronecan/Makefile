############################################################################
# apps/canutils/libdronecan/Makefile
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

LIBDRONECAN_URL = $(patsubst "%",%,$(strip $(CONFIG_LIBDRONECAN_URL)))
LIBDRONECAN_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBDRONECAN_VERSION)))

LIBDRONECAN_PACKNAME = $(LIBDRONECAN_UNPACKNAME)$(PACKEXT)
LIBDRONECAN_UNPACKNAME = libcanard-$(LIBDRONECAN_VERSION)
LIBDRONECAN_SRCNAME = libcanard

LIBDRONECAN_SRCDIR = $(WD)/$(LIBDRONECAN_SRCNAME)
LIBDRONECAN_DRVDIR = $(LIBDRONECAN_SRCDIR)$(DELIM)drivers$(DELIM)nuttx
LIBDRONECAN_SOCKETCANDIR = $(LIBDRONECAN_SRCDIR)$(DELIM)drivers$(DELIM)socketcan

# Conflict with Cyphal's libcanard
ifeq ($(CONFIG_CANUTILS_LIBOPENCYPHAL),y)
CFLAGS += -DcanardInit=dronecanardInit
endif

CFLAGS += -std=c99 -DCANARD_ASSERT=DEBUGASSERT
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/canutils/libdronecan/libcanard

ifeq ($(CONFIG_LIBDRONECAN_CANFD),y)
CFLAGS += -DCANARD_ENABLE_CANFD=1
endif

CSRCS = $(LIBDRONECAN_SRCDIR)$(DELIM)canard.c

ifeq ($(CONFIG_NET_CAN),y)
CSRCS += $(LIBDRONECAN_SOCKETCANDIR)$(DELIM)socketcan.c
else
CSRCS += $(LIBDRONECAN_DRVDIR)$(DELIM)canard_nuttx.c
endif

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LIBDRONECAN_SRCNAME)/.git),)
$(LIBDRONECAN_PACKNAME):
	@echo "Downloading: $@"
	$(Q) curl -o $@ -L $(LIBDRONECAN_URL)$(DELIM)$(LIBDRONECAN_VERSION)$(PACKEXT)

$(LIBDRONECAN_SRCNAME): $(LIBDRONECAN_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) mv $(LIBDRONECAN_UNPACKNAME) $(LIBDRONECAN_SRCNAME)
	$(Q) touch $@
	
$(LIBDRONECAN_SRCDIR)$(DELIM)canard.c: $(LIBDRONECAN_SRCNAME)
endif

context:: $(LIBDRONECAN_SRCNAME)

distclean::
ifeq ($(wildcard $(LIBDRONECAN_SRCNAME)/.git),)
	$(call DELDIR, $(LIBDRONECAN_SRCNAME))
	$(call DELFILE, $(LIBDRONECAN_PACKNAME))
endif

include $(APPDIR)/Application.mk
