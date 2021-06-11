############################################################################
# apps/fsutils/inih/Makefile
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

CP = cp -R
UNPACK = tar -xzf
PACKEXT = .tar.gz
NXTOOLSDIR = $(APPDIR)/tools

INIH_GITHUB_NAME = benhoyt/inih
INIH_VERSION = r42
INIH_SRC_SHA256 = 53ac3591be39a004ddc7874d0a57e7b9bc92be14192808bbef510c5c533cb1d8
INIH_EXT = tar.gz
INIH_SOURCES = inih-$(INIH_VERSION)
INIH_TARBALL = $(INIH_VERSION).$(INIH_EXT)
INIH_URL = https://github.com/$(INIH_GITHUB_NAME)/archive

CSRCS = $(INIH_SOURCES)/ini.c

# inih configuration from Kconfig

ifeq ($(CONFIG_INIH_MULTI_LINE_ENTRIES),y)
	CFLAGS += -DINI_ALLOW_MULTILINE=1
else
	CFLAGS += -DINI_ALLOW_MULTILINE=0
endif

ifeq ($(CONFIG_INIH_USE_MALLOC),y)
	CFLAGS += -DINI_USE_STACK=0
	CFLAGS += -DINI_ALLOW_REALLOC=1
	CFLAGS += -DINI_INITIAL_ALLOC=$(CONFIG_INIH_INITIAL_ALLOC)
else
	CFLAGS += -DINI_USE_STACK=1
	CFLAGS += -DINI_ALLOW_REALLOC=0
endif

CFLAGS += -DINI_MAX_LINE=$(CONFIG_INIH_MAX_LINE)

# compile-time configuration of inih

CFLAGS += -DINI_ALLOW_BOM=0
CFLAGS += -DINI_ALLOW_INLINE_COMMENTS=1
CFLAGS += -DINI_STOP_ON_FIRST_ERROR=0
CFLAGS += -DINI_HANDLER_LINENO=1

# building of inih

$(INIH_TARBALL):
	@echo "Downloading: $@"
	$(Q) curl -L -o $@ $(INIH_URL)/$@
	${Q} $(NXTOOLSDIR)/check-hash.sh sha256 $(INIH_SRC_SHA256) $@

$(INIH_SOURCES): $(INIH_TARBALL)
	@echo "Unpacking $< -> $@"
	$(Q) $(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

create_includes:
	$(Q) $(CP) $(INIH_SOURCES)/ini.h $(APPDIR)/include/fsutils

context:: $(INIH_SOURCES)
	$(Q) $(MAKE) create_includes

clean::
	$(Q) $(call DELFILE, $(APPDIR)/include/fsutils/ini.h)
	$(Q) $(foreach COBJ, $(COBJS), $(call DELFILE, $(COBJ)))

distclean::
	$(Q) $(call DELDIR, $(INIH_SOURCES))
	$(Q) $(call DELDIR, $(INIH_TARBALL))

include $(APPDIR)/Application.mk
