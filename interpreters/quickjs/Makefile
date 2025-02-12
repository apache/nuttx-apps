############################################################################
# apps/interpreters/quickjs/Makefile
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

QUICKJS_VERSION     = 2024-02-14
QUICKJS_COMMIT_ID   = 6e2e68fd0896957f92eb6c242a2e048c1ef3cae0
QUICKJS_UNPACK      = quickjs
QUICKJS_ARCHIVE     = $(QUICKJS_COMMIT_ID).zip
QUICKJS_URL         = https://github.com/bellard/quickjs/archive/$(QUICKJS_ARCHIVE)

CSRCS = quickjs.c libregexp.c libbf.c libunicode.c cutils.c

VERSION=\"$(QUICKJS_VERSION)\"

CFLAGS += -Dmp_add=qjs_mp_add -Dmp_sub=qjs_mp_sub -Dmp_mul=qjs_mp_mul
CFLAGS += -DCONFIG_VERSION=$(VERSION) -Wno-shadow
CFLAGS += -Wno-array-bounds -I$(QUICKJS_UNPACK)
CFLAGS += -D__linux__ -include alloca.h
CFLAGS += -Wno-incompatible-pointer-types
CFLAGS += -Wno-implicit-function-declaration
CFLAGS += -Wno-unused-function
CFLAGS += -Wno-format

ifeq ($(CONFIG_ARCH_ARM),y)
CFLAGS += -DFE_TONEAREST=0x00000000
CFLAGS += -DFE_UPWARD=0x00400000
CFLAGS += -DFE_DOWNWARD=0x00800000
CFLAGS += -DFE_TOWARDZERO=0x00c00000
endif

ifeq ($(CONFIG_INTERPRETERS_QUICKJS_BIGNUM),y)
CFLAGS += -DCONFIG_BIGNUM
CSRCS  += qjscalc.c
endif

VPATH += $(QUICKJS_UNPACK)
DEPPATH += --dep-path $(QUICKJS_UNPACK)

ifneq ($(CONFIG_INTERPRETERS_QUICKJS_NONE),y)
PROGNAME  = qjs
PRIORITY  = $(CONFIG_INTERPRETERS_QUICKJS_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_QUICKJS_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_QUICKJS)
endif

$(QUICKJS_ARCHIVE):
	$(Q) echo "Downloading $(QUICKJS_ARCHIVE)"
	$(Q) curl -L $(QUICKJS_URL) -o quickjs-$(QUICKJS_ARCHIVE)

$(QUICKJS_UNPACK): $(QUICKJS_ARCHIVE)
	$(Q) echo "Unpacking quickjs-$(QUICKJS_ARCHIVE) to $(QUICKJS_UNPACK)"
	$(Q) unzip -q -o quickjs-$(QUICKJS_ARCHIVE)
	$(Q) mv quickjs-$(QUICKJS_COMMIT_ID) $(QUICKJS_UNPACK)
	$(Q) patch -d $(QUICKJS_UNPACK) -p1 < 0001-Disabled-unsupported-feature-on-NuttX.patch

ifeq ($(CONFIG_INTERPRETERS_QUICKJS_MINI),y)
MAINSRC = qjsmini.c
endif

ifeq ($(CONFIG_INTERPRETERS_QUICKJS_FULL),y)
CSRCS += quickjs-libc.c repl.c
MAINSRC = qjs.c
endif

# Download and unpack zipball if no archive found
ifeq ($(wildcard $(QUICKJS_UNPACK)/.*),)
ifeq ($(CONFIG_INTERPRETERS_QUICKJS_FULL),y)
context:: $(QUICKJS_UNPACK)
	$(MAKE) -C $(QUICKJS_UNPACK) \
		CONFIG_BIGNUM=$(CONFIG_INTERPRETERS_QUICKJS_BIGNUM)
else
context:: $(QUICKJS_UNPACK)
endif
endif

distclean::
	$(call DELDIR, $(QUICKJS_UNPACK))
	$(call DELFILE, $(QUICKJS_ARCHIVE))

include $(APPDIR)/Application.mk
