############################################################################
# netutils/mdns/Makefile
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

# mDNS application

############################################################################
# Flags
############################################################################

MDNS_URL ?= "https://github.com/mjansson/mdns/archive"

MDNS_VERSION = main
MDNS_ZIP = $(MDNS_VERSION).zip

MDNS_UNPACKNAME = mdns
UNPACK ?= unzip -q -o

VPATH += $(MDNS_UNPACKNAME)
VPATH += $(MDNS_UNPACKNAME)$(DELIM)posix
DEPPATH += --dep-path $(MDNS_UNPACKNAME)
DEPPATH += --dep-path $(MDNS_UNPACKNAME)$(DELIM)posix

CFLAGS += -Wno-strict-prototypes -Wno-undef -Wno-format

APPS_INCDIR = $(APPDIR)$(DELIM)include$(DELIM)netutils

############################################################################
# Targets
############################################################################

$(MDNS_ZIP):
	@echo "Downloading: $(MDNS_URL)/$(MDNS_ZIP)"
	$(Q) curl -O -L $(MDNS_URL)/$(MDNS_ZIP)

$(MDNS_UNPACKNAME): $(MDNS_ZIP)
	@echo "Unpacking: $(MDNS_ZIP) -> $(MDNS_UNPACKNAME)"
	$(Q) $(UNPACK) $(MDNS_ZIP)
	$(Q) mv	mdns-$(MDNS_VERSION) $(MDNS_UNPACKNAME)
	$(Q) cp $(MDNS_UNPACKNAME)$(DELIM)mdns.h $(APPS_INCDIR)
	$(Q) patch -p0 < verbose_option.patch # Update to enable non-verbose mode
	$(Q) touch $(MDNS_UNPACKNAME)

clean::
	$(call DELFILE, $(OBJS))

# Download and unpack tarball if no git repo found

ifeq ($(wildcard $(MDNS_UNPACKNAME)/.git),)
context:: $(MDNS_UNPACKNAME)

distclean::
	$(call DELFILE, $(OBJS))
	$(call DELDIR, $(MDNS_UNPACKNAME))
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)mdns.h)
	$(call DELFILE, $(MDNS_ZIP))
endif

############################################################################
# Applications Configuration
############################################################################

include $(APPDIR)/Make.defs

ifneq ($(CONFIG_NETUTILS_MDNS),)
PROGNAME  = $(CONFIG_NETUTILS_MDNS_PROGNAME)
PRIORITY  = $(CONFIG_NETUTILS_MDNS_PRIORITY)
STACKSIZE = $(CONFIG_NETUTILS_MDNS_STACKSIZE)
MODULE    = $(CONFIG_NETUTILS_MDNS)

MAINSRC = mdns/mdns.c
else
CSRCS += mdns/mdns.c
endif

ifneq ($(CONFIG_NETUTILS_MDNS_DAEMON),)
CSRCS += mdnsd.c
endif

# This is an external library so we accept NuttX style violations

CFLAGS += -Wno-undef -Wno-strict-prototypes -Wno-unused-variable \
          -Wno-pointer-sign -Wno-unused-but-set-variable -Wno-shadow \
		  -Wno-format

include $(APPDIR)/Application.mk