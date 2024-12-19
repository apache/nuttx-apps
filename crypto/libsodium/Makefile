############################################################################
# apps/crypto/libsodium/Makefile
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

LIBSODIUM_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBSODIUM_VERSION)))
LIBSODIUM_URL ?= "https://github.com/jedisct1/libsodium/archive"

LIBSODIUM_ZIP = $(LIBSODIUM_VERSION).zip

LIBSODIUM_UNPACKNAME = libsodium
UNPACK ?= unzip -q -o

LIBSODIUM_UNPACKLIBDIR = $(LIBSODIUM_UNPACKNAME)$(DELIM)src$(DELIM)$(LIBSODIUM_UNPACKNAME)
LIBSODIUM_UNPACKTESTDIR = $(LIBSODIUM_UNPACKNAME)$(DELIM)test$(DELIM)default

CSRCS += $(shell find $(LIBSODIUM_UNPACKLIBDIR) -name "*.c")
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/crypto/libsodium/libsodium/src/libsodium/include/sodium
CFLAGS += -DDEV_MODE -DCONFIGURED=1
CFLAGS += -Wno-unused-function -Wno-undef -Wno-unused-variable -Wno-deprecated-declarations \
          -Wno-shadow

ifneq ($(CONFIG_LIBSODIUM_TEST),)
MODULE = $(CONFIG_LIBSODIUM_TEST)
MAINSRC = $(wildcard $(LIBSODIUM_UNPACKTESTDIR)$(DELIM)*.c)
PROGNAME = $(patsubst $(LIBSODIUM_UNPACKTESTDIR)$(DELIM)%.c,%,$(MAINSRC))
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/crypto/libsodium/libsodium/test/quirks
PRIORITY = $(CONFIG_LIBSODIUM_TEST_PRIORITY)
STACKSIZE = $(CONFIG_LIBSODIUM_TEST_STACKSIZE)
endif

$(LIBSODIUM_ZIP):
	@echo "Downloading: $(LIBSODIUM_URL)/$(LIBSODIUM_ZIP)"
	$(Q) $(call DOWNLOAD,$(LIBSODIUM_URL),$(LIBSODIUM_ZIP))

$(LIBSODIUM_UNPACKNAME): $(LIBSODIUM_ZIP)
	@echo "Unpacking: $(LIBSODIUM_ZIP) -> $(LIBSODIUM_UNPACKNAME)"
	$(Q) $(UNPACK) $(LIBSODIUM_ZIP)
	$(Q) mv $(LIBSODIUM_UNPACKNAME)-$(LIBSODIUM_VERSION) $(LIBSODIUM_UNPACKNAME)
	$(Q) echo "Patching $(LIBSODIUM_UNPACKNAME)"
	$(Q) patch -p1 -d $(LIBSODIUM_UNPACKNAME) < 0001-fix-multiple-definition-bug-in-libsodium-test.patch
	$(Q) patch -p1 -d $(LIBSODIUM_UNPACKNAME) < 0002-fix-cannot-find-file-sodium-version.h.patch
	$(Q) touch $(LIBSODIUM_UNPACKNAME)

ifeq ($(wildcard $(LIBSODIUM_UNPACKNAME)/.git),)
context:: $(LIBSODIUM_UNPACKNAME)

distclean::
	$(Q) rm -rf $(LIBSODIUM_UNPACKNAME)
endif

include $(APPDIR)/Application.mk