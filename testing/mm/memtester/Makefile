############################################################################
# apps/testing/mm/memtester/Makefile
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

PROGNAME  = $(CONFIG_UTILS_MEMTESTER_PROGNAME)
PRIORITY  = $(CONFIG_UTILS_MEMTESTER_PRIORITY)
STACKSIZE = $(CONFIG_UTILS_MEMTESTER_STACKSIZE)
MODULE    = $(CONFIG_UTILS_MEMTESTER)

CSRCS    += memtester/tests.c
CSRCS    += memtester/output.c

MAINSRC   = memtester/memtester.c

ifeq ($(CONFIG_UTILS_MEMTESTER_NARROW),y)
  CFLAGS += -DTEST_NARROW_WRITES
endif

ifeq ($(wildcard memtester/.git),)
VERSION ?= master
memtester.zip:
	$(Q) curl -L https://github.com/jnavila/memtester/archive/refs/heads/master.zip -o memtester.zip
	$(Q) unzip -o memtester.zip
	$(Q) mv memtester-$(VERSION) memtester

context:: memtester.zip

distclean::
	$(call DELDIR, memtester)
	$(call DELFILE, memtester.zip)

endif

include $(APPDIR)/Application.mk
