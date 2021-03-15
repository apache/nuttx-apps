############################################################################
# interpreters/quickjs/Makefile
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

QUICKJS_VERSION  = 2020-11-08
QUICKJS_UNPACK   = quickjs
QUICKJS_TARBALL  = quickjs-$(QUICKJS_VERSION).tar.xz
QUICKJS_URL_BASE = https://bellard.org/quickjs/
QUICKJS_URL      = $(QUICKJS_URL_BASE)/$(QUICKJS_TARBALL)

CSRCS = quickjs.c libregexp.c libbf.c libunicode.c cutils.c

ifeq ($(CONFIG_INTERPRETERS_QUICKJS_MINI),y)
MAINSRC = qjsmini.c
else
CSRCS += quickjs-libc.c repl.c
MAINSRC = qjs.c
endif

VERSION=\"$(QUICKJS_VERSION)\"

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

PROGNAME  = qjs
PRIORITY  = $(CONFIG_INTERPRETERS_QUICKJS_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_QUICKJS_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_QUICKJS)

$(QUICKJS_TARBALL):
	$(Q) echo "Downloading $(QUICKJS_TARBALL)"
	$(Q) curl -O -L $(QUICKJS_URL)

$(QUICKJS_UNPACK): $(QUICKJS_TARBALL)
	$(Q) echo "Unpacking $(QUICKJS_TARBALL) to $(QUICKJS_UNPACK)"
	$(Q) tar -Jxf $(QUICKJS_TARBALL)
	$(Q) mv quickjs-$(QUICKJS_VERSION) $(QUICKJS_UNPACK)
	$(Q) patch -d $(QUICKJS_UNPACK) -p1 < 0001-Disabled-unsupported-feature-on-NuttX.patch

$(QUICKJS_UNPACK)/.patch: $(QUICKJS_UNPACK)
	$(Q) touch $(QUICKJS_UNPACK)/.patch

build_host: $(QUICKJS_UNPACK)/.patch
	make -C $(QUICKJS_UNPACK) \
		CONFIG_BIGNUM=$(CONFIG_INTERPRETERS_QUICKJS_BIGNUM)

context:: build_host

clean::
	$(Q) test ! -d $(QUICKJS_UNPACK) || make -C $(QUICKJS_UNPACK) clean

distclean::
	$(call DELDIR, $(QUICKJS_UNPACK))
	$(call DELFILE, $(QUICKJS_TARBALL))

include $(APPDIR)/Application.mk
