############################################################################
# apps/testing/nist-sts/Makefile
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

PROGNAME  = $(CONFIG_TESTING_NIST_STS_PROGNAME)
PRIORITY  = $(CONFIG_TESTING_NIST_STS_PRIORITY)
STACKSIZE = $(CONFIG_TESTING_NIST_STS_STACKSIZE)
MODULE    = $(CONFIG_TESTING_NIST_STS)

NIST_URL ?= "https://csrc.nist.gov/CSRC/media/Projects/Random-Bit-Generation/documents/"

NIST_UNPACKNAME = nist-sts
NIST_PACKNAME   = sts-2.1.2
NIST_ZIP        = sts-2_1_2.zip
UNPACK         ?= unzip -q -o

$(NIST_ZIP):
	@echo "Downloading: $(NIST_ZIP)"
	$(Q) $(call DOWNLOAD,$(NIST_URL),$(NIST_ZIP))

$(NIST_UNPACKNAME): $(NIST_ZIP)
	@echo "Unpacking: $(NIST_ZIP) -> $(NIST_UNPACKNAME)"
	$(Q) $(UNPACK) $(NIST_ZIP)
	$(Q) mv $(NIST_PACKNAME) $(NIST_UNPACKNAME)
	$(Q) mv $(NIST_UNPACKNAME)/$(NIST_PACKNAME) $(NIST_UNPACKNAME)/sts
	$(Q) echo "Patching $(NIST_UNPACKNAME)"
	$(Q) patch -p1 -d $(NIST_UNPACKNAME) < 0001-Solve-the-memory-out-of-bounds-problem-in-sts.patch
	$(Q) patch -p1 -d $(NIST_UNPACKNAME) < 0002-Fix-bug-that-crash-in-running-a-single-test-after-ru.patch
	$(Q) touch $(NIST_UNPACKNAME)

MAINSRC   = nist-sts/sts/src/assess.c
CSRCS     = $(shell find nist-sts/sts/src -name "*.c" ! -name "assess.c")
CFLAGS   += -Wno-misleading-indentation -Wno-unused-but-set-variable \
            -Wno-strict-prototypes -Wno-undef -Wno-shadow -Wno-unused-variable \
            -Wno-maybe-uninitialized

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(NIST_UNPACKNAME)/.git),)
context:: $(NIST_UNPACKNAME)

distclean::
	$(call DELDIR, $(NIST_UNPACKNAME))
	$(call DELFILE, $(NIST_ZIP))
endif

include $(APPDIR)/Application.mk
