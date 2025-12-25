############################################################################
# apps/interpreters/mquickjs/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this to you under the Apache License, Version 2.0 (the
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

MQJS_UNPACK = mquickjs
MQJS_ARCHIVE = mquickjs-master.zip
MQJS_URL = https://github.com/bellard/mquickjs/archive/refs/heads/master.zip

# Determine target word size for header generation
ifeq ($(CONFIG_ARCH_64BIT),y)
MQJS_BUILD_FLAGS = -m64
else
MQJS_BUILD_FLAGS = -m32
endif

CSRCS = $(MQJS_UNPACK)/mquickjs.c \
        $(MQJS_UNPACK)/dtoa.c \
        $(MQJS_UNPACK)/libm.c \
        $(MQJS_UNPACK)/cutils.c \
        $(MQJS_UNPACK)/readline.c \
        $(MQJS_UNPACK)/readline_tty.c

PROGNAME = mqjs
PRIORITY = $(CONFIG_INTERPRETERS_MQJS_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_MQJS_STACKSIZE)
MODULE = $(CONFIG_INTERPRETERS_MQJS)
MAINSRC = $(MQJS_UNPACK)/mqjs.c

# Download archive
$(MQJS_ARCHIVE):
	$(Q) echo "Downloading $(MQJS_ARCHIVE)"
	$(Q) curl -L $(MQJS_URL) -o $(MQJS_ARCHIVE)

# Unpack archive
$(MQJS_UNPACK): $(MQJS_ARCHIVE)
	$(Q) echo "Unpacking $(MQJS_ARCHIVE) to $(MQJS_UNPACK)"
	$(Q) unzip -q -o $(MQJS_ARCHIVE)
	$(Q) mv bellard-mquickjs-* $(MQJS_UNPACK)
	$(Q) touch $(MQJS_UNPACK)

# Build host tool via upstream Makefile
$(MQJS_UNPACK)/mqjs_stdlib:
	$(Q) echo "Building mquickjs host tool via upstream Makefile"
	$(Q) rm -f $(MQJS_UNPACK)/mqjs_stdlib
	$(Q) $(MAKE) -C $(MQJS_UNPACK) HOSTCC=$(HOSTCC) mqjs_stdlib

# Generate stdlib header using built host tool
$(MQJS_UNPACK)/mqjs_stdlib.h: $(MQJS_UNPACK)/mqjs_stdlib
	$(Q) echo "Generating mquickjs stdlib header"
	$(MQJS_UNPACK)/mqjs_stdlib $(MQJS_BUILD_FLAGS) > $@

# Generate atom header using built host tool
$(MQJS_UNPACK)/mquickjs_atom.h: $(MQJS_UNPACK)/mqjs_stdlib
	$(Q) echo "Generating mquickjs atom header"
	$(MQJS_UNPACK)/mqjs_stdlib -a $(MQJS_BUILD_FLAGS) > $@

# Download and unpack only if not a git repository
context:: $(MQJS_UNPACK)/mqjs_stdlib.h $(MQJS_UNPACK)/mquickjs_atom.h

# Clean up on distclean
distclean::
ifneq ($(wildcard $(MQJS_UNPACK)/.git),)
	$(Q) $(MAKE) -C $(MQJS_UNPACK) clean
else
	$(call DELDIR, $(MQJS_UNPACK))
endif
	$(call DELFILE, $(MQJS_ARCHIVE))

# Dependencies
$(MQJS_UNPACK)/mquickjs.c: $(MQJS_UNPACK)/mquickjs_atom.h

include $(APPDIR)/Application.mk
