############################################################################
# apps/benchmarks/tinymembench/Makefile
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

PROGNAME    = tinymembench
PRIORITY    = $(CONFIG_BENCHMARK_TINY_MEMBENCH_PRIORITY)
STACKSIZE   = $(CONFIG_BENCHMARK_TINY_MEMBENCH_STACKSIZE)
MODULE      = $(CONFIG_BENCHMARK_TINY_MEMBENCH)

TINYMB_UNPACK   = tinymembench
TINYMB_GIT      = github.com/ssvb/tinymembench
TINYMB_URL      = https://github.com/ssvb/tinymembench/archive
TINYMB_VERSION  = master
TINYMB_ZIP      = $(TINYMB_UNPACK)-$(TINYMB_VERSION).zip
UNPACK         ?= unzip -q -o

$(TINYMB_ZIP):
	@echo "Downloading: $(TINYMB_URL)"
	$(Q) curl -L $(TINYMB_URL)/$(TINYMB_VERSION).zip -o $(TINYMB_UNPACK)-$(TINYMB_VERSION).zip

$(TINYMB_UNPACK): $(TINYMB_ZIP)
	@echo "Unpacking: $(TINYMB_ZIP) -> $(TINYMB_UNPACK)"
	$(Q) $(UNPACK) $(TINYMB_ZIP)
	$(Q) mv	tinymembench-$(TINYMB_VERSION) $(TINYMB_UNPACK)
	$(Q) touch $(TINYMB_UNPACK)
	@echo "Patching: Applying patch"
	$(Q) cd tinymembench && patch -p1 < ../0001-tinymembench-fix-compiling-error-for-NuttX.patch

ifeq ($(wildcard $(TINYMB_UNPACK)/.git),)
context:: $(TINYMB_UNPACK)

distclean::
	$(call DELDIR, $(TINYMB_UNPACK))
	$(call DELFILE, $(TINYMB_ZIP))
endif

ASRCS      += $(TINYMB_UNPACK)/aarch64-asm.S
ASRCS      += $(TINYMB_UNPACK)/arm-neon.S
ASRCS      += $(TINYMB_UNPACK)/x86-sse2.S
ASRCS      += $(TINYMB_UNPACK)/mips-32.S

CSRCS      += $(TINYMB_UNPACK)/asm-opt.c
CSRCS      += $(TINYMB_UNPACK)/util.c

MAINSRC     = $(TINYMB_UNPACK)/main.c

CFLAGS     += -Wno-unused-but-set-variable -Wno-unused-variable
CFLAGS     += -Wno-strict-prototypes

include $(APPDIR)/Application.mk
