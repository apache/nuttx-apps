############################################################################
# apps/wireless/bluetooth/nimble/Makefile
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

NIMBLE_UNPACKDIR = mynewt-nimble
NIMBLE_ROOT = $(APPDIR)/wireless/bluetooth/nimble/$(NIMBLE_UNPACKDIR)
CONFIG_NIMBLE_REF := $(patsubst "%",%,$(strip $(CONFIG_NIMBLE_REF)))
NIMBLE_TAR := $(CONFIG_NIMBLE_REF).tar.gz
NIMBLE_URL := https://github.com/apache/mynewt-nimble/archive/$(NIMBLE_TAR)

ifneq ($(CONFIG_NIMBLE_PORTING_EXAMPLE),)
# NimBLE porting example built-in application

-include $(NIMBLE_ROOT)/porting/examples/nuttx/Make.defs
PRIORITY  = 255
STACKSIZE = $(CONFIG_NIMBLE_PORTING_EXAMPLE_STACKSIZE)

# NimBLE assumes this flag since it expects undefined macros to be zero value

CFLAGS += -Wno-pointer-to-int-cast -Wno-undef
endif

$(NIMBLE_TAR):
	$(Q) curl -L $(NIMBLE_URL) -o $(NIMBLE_TAR)

$(NIMBLE_UNPACKDIR): $(NIMBLE_TAR)
	$(Q) tar zxf $(NIMBLE_TAR)
	$(Q) mv mynewt-nimble-$(CONFIG_NIMBLE_REF) $(NIMBLE_UNPACKDIR)
	$(Q) touch $(NIMBLE_UNPACKDIR)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(NIMBLE_UNPACKDIR)/.git),)
context:: $(NIMBLE_UNPACKDIR)

distclean::
	$(call DELFILE,$(NIMBLE_TAR))
	$(call DELDIR,$(NIMBLE_UNPACKDIR))
endif

include $(APPDIR)/Application.mk
