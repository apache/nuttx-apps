############################################################################
# apps/benchmarks/test-tlb/Makefile
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

PROGNAME  = testtlb
PRIORITY  = $(CONFIG_BENCHMARK_TESTTLB_PRIORITY)
STACKSIZE = $(CONFIG_BENCHMARK_TESTTLB_STACKSIZE)
MODULE    = $(CONFIG_BENCHMARK_TESTTLB)

TESTTLB_UNPACK   = test-tlb
TESTTLB_GIT      = github.com/torvalds/test-tlb
TESTTLB_URL      = https://github.com/torvalds/test-tlb/archive
TESTTLB_VERSION  = master
TESTTLB_ZIP      = $(TESTTLB_UNPACK)-$(TESTTLB_VERSION).zip
UNPACK         ?= unzip -q -o

$(TESTTLB_ZIP):
	@echo "Downloading: $(TESTTLB_URL)"
	$(Q) curl -L $(TESTTLB_URL)/$(TESTTLB_VERSION).zip -o $(TESTTLB_UNPACK)-$(TESTTLB_VERSION).zip

$(TESTTLB_UNPACK): $(TESTTLB_ZIP)
	@echo "Unpacking: $(TESTTLB_ZIP) -> $(TESTTLB_UNPACK)"
	$(Q) $(UNPACK) $(TESTTLB_ZIP)
	$(Q) mv	test-tlb-$(TESTTLB_VERSION) $(TESTTLB_UNPACK)
	$(Q) touch $(TESTTLB_UNPACK)
	@echo "Patching: Applying patch"
	$(Q) cd test-tlb && patch -p1 < ../0001-test-tlb-port-for-NuttX.patch

ifeq ($(wildcard $(TESTTLB_UNPACK)/.git),)
context:: $(TESTTLB_UNPACK)

distclean::
	$(call DELDIR, $(TESTTLB_UNPACK))
	$(call DELFILE, $(TESTTLB_ZIP))
endif

MAINSRC   = test-tlb/test-tlb.c

include $(APPDIR)/Application.mk
