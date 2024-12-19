############################################################################
# apps/fsutils/libtinycbor/Makefile
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

TINYCBOR_URL ?= "https://github.com/intel/tinycbor/archive"
TINYCBOR_VERSION ?= 3cba6b11aaa0f6f674cd56ebaa573c4b65f71ee7

TINYCBOR_UNPACKNAME = tinycbor

$(TINYCBOR_UNPACKNAME):
	@echo "Downloading: $(TINYCBOR_UNPACKNAME)"
	$(Q) curl -O -L $(TINYCBOR_URL)/$(TINYCBOR_VERSION).zip
	$(Q) mkdir $(TINYCBOR_UNPACKNAME)
	$(Q) unzip -o -j $(TINYCBOR_VERSION).zip -d $(TINYCBOR_UNPACKNAME)
	$(Q) patch -Np1 < fix_open_memstream.patch
	$(call DELFILE, $(TINYCBOR_VERSION).zip)

# Files

CSRCS += tinycbor/cborencoder.c
CSRCS += tinycbor/cborencoder_close_container_checked.c
CSRCS += tinycbor/cborencoder_float.c
CSRCS += tinycbor/cborerrorstrings.c
CSRCS += tinycbor/cborparser.c
CSRCS += tinycbor/cborparser_dup_string.c
CSRCS += tinycbor/cborparser_float.c
CSRCS += tinycbor/cborpretty.c
CSRCS += tinycbor/cborpretty_stdio.c
CSRCS += tinycbor/cbortojson.c
CSRCS += tinycbor/cborvalidation.c
CSRCS += tinycbor/open_memstream.c

CFLAGS += -std=c99

clean::
	$(call DELFILE, $(OBJS))

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(TINYCBOR_UNPACKNAME)/.git),)
context:: $(TINYCBOR_UNPACKNAME)

distclean::
	$(call DELDIR, $(TINYCBOR_UNPACKNAME))
endif

include $(APPDIR)/Application.mk
