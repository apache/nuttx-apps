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

QUICKJS_VERSION  = 20200705
QUICKJS_UNPACK   = quickjs
QUICKJS_TARBALL  = $(QUICKJS_VERSION).tar.gz
QUICKJS_URL_BASE = https://github.com/horhof/quickjs/archive
QUICKJS_URL      = $(QUICKJS_URL_BASE)/$(QUICKJS_TARBALL)

CSRCS = quickjs.c libregexp.c jscompress.c libbf.c libunicode.c
CSRCS += cutils.c quickjs-libc.c repl.c

MAINSRC = qjs.c

VERSION=\"$(shell cat $(QUICKJS_UNPACK)/VERSION)\"

CFLAGS += -DCONFIG_VERSION=$(VERSION) -Wno-shadow -D__NuttX__
CFLAGS += -Wno-array-bounds -I$(QUICKJS_UNPACK)
ifneq ($(CONFIG_INTERPRETERS_QUICKJS_BIGNUM),)
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
	$(Q) wget $(QUICKJS_URL)

$(QUICKJS_UNPACK): $(QUICKJS_TARBALL)
	$(Q) echo "Unpacking $(QUICKJS_TARBALL) to $(QUICKJS_UNPACK)"
	$(Q) tar xzvf $(QUICKJS_TARBALL)
	$(Q) mv quickjs-$(QUICKJS_VERSION) $(QUICKJS_UNPACK)
	$(Q) patch -p0 < quickjs.patch

$(QUICKJS_UNPACK)/.patch: $(QUICKJS_UNPACK)
	$(Q) make -C $(QUICKJS_UNPACK) \
		CONFIG_BIGNUM=$(CONFIG_INTERPRETERS_QUICKJS_BIGNUM)
	$(Q) touch $(QUICKJS_UNPACK)/.patch

context:: $(QUICKJS_UNPACK)/.patch

clean::
	$(Q) test ! -d $(QUICKJS_UNPACK) || make -C $(QUICKJS_UNPACK) clean

distclean::
	$(call DELDIR, $(QUICKJS_UNPACK))
	$(call DELFILE, $(QUICKJS_TARBALL))

include $(APPDIR)/Application.mk
