############################################################################
# apps/benchmarks/rt-tests/Makefile
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

############################################################################
# Targets
############################################################################


include $(APPDIR)/Make.defs

PROGNAME  = cyclictest
PRIORITY  = $(CONFIG_BENCHMARK_RTTESTS_PRIORITY)
STACKSIZE = $(CONFIG_BENCHMARK_RTTESTS_STACKSIZE)
MODULE    = $(CONFIG_BENCHMARK_RTTESTS)

RTTESTS_PATH    = $(APPDIR)/benchmarks/rt-tests/rt-tests
RTTESTS_URL     = https://git.kernel.org/pub/scm/utils/rt-tests/rt-tests.git
RTTESTS_VERSION = cadd661f984c0e6717e681fdaca1ce589b0ed964
RTTESTS_GIT     = rt-tests

ifeq ($(wildcard rt-tests/.git),)
rt-tests:
	@echo "Git Cloning: $(RTTESTS_URL)"
	$(Q) git clone $(RTTESTS_URL)
	@echo "Checkout commit: $(RTTESTS_VERSION)"
	$(Q) cd $(RTTESTS_PATH) && \
	git checkout -q $(RTTESTS_VERSION)
	@echo "Patching: Applying patch"
	$(Q) cd $(RTTESTS_PATH) && \
	patch -p1 < ../0001-rt-tests-cyclictest-Port-for-NuttX.patch

context:: rt-tests
endif

distclean::
	$(call DELDIR, $(RTTESTS_GIT))

MAINSRC   = rt-tests/src/cyclictest/cyclictest.c
CSRCS     = rt-tests/src/lib/rt-error.c rt-tests/src/lib/histogram.c
CSRCS    += rt-tests/src/lib/rt-utils.c rt-tests/src/lib/rt-numa.c

CFLAGS   += -Irt-tests/src/include -I. -DVERSION=0.1
CFLAGS   += -DSYS_gettid=0 -Dgettid=rttest_gettid -D'syscall(x)=((pid_t)(x))'
CFLAGS   += -w -DRTTESTS_PRIORITY=$(CONFIG_BENCHMARK_RTTESTS_PRIORITY)+1

include $(APPDIR)/Application.mk
