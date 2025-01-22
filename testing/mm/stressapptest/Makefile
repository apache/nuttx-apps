############################################################################
# apps/testing/mm/stressapptest/Makefile
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

STRESSAPPTEST_VERSION=$(patsubst "%",%,$(CONFIG_TESTING_STRESSAPPTEST_VERSION))

v$(STRESSAPPTEST_VERSION).tar.gz:
	$(call DOWNLOAD,https://github.com/stressapptest/stressapptest/archive/refs/tags/,$@)

stressapptest: v$(STRESSAPPTEST_VERSION).tar.gz
	$(Q) tar -xf $<
	$(Q) mv stressapptest-$(STRESSAPPTEST_VERSION) stressapptest

context:: stressapptest

distclean::
	$(Q) $(DELFILE) v$(STRESSAPPTEST_VERSION).tar.gz
	$(call DELDIR, stressapptest)

VPATH   += stressapptest
VPATH   += stressapptest/src
DEPPATH += --dep-path stressapptest
DEPPATH += --dep-path stressapptest/src

CXXEXT   := .cc
CXXFLAGS += -DHAVE_CONFIG_H -fpermissive

CXXSRCS += adler32memcpy.cc disk_blocks.cc error_diag.cc finelock_queue.cc logger.cc
CXXSRCS += os.cc os_factory.cc pattern.cc queue.cc sat.cc sat_factory.cc worker.cc

MAINSRC  += main.cc
PROGNAME += stressapptest

PRIORITY  = $(CONFIG_TESTING_STRESSAPPTEST_PRIORITY)
STACKSIZE = $(CONFIG_TESTING_STRESSAPPTEST_STACKSIZE)
MODULE    = $(CONFIG_TESTING_STRESSAPPTEST)

include $(APPDIR)/Application.mk
