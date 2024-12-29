############################################################################
# apps/testing/cmocka/Makefile
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

CSRCS += $(wildcard cmocka/src/*.c)
CFLAGS += -Dprint_error=nx_print_error
CFLAGS += -DCMOCKA_PLATFORM_INCLUDE
CFLAGS += -DHAVE_CLOCK_REALTIME=1
CFLAGS += -DHAVE_STRUCT_TIMESPEC=1
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/testing/cmocka

PROGNAME  = $(CONFIG_TESTING_CMOCKA_PROGNAME)
PRIORITY  = $(CONFIG_TESTING_CMOCKA_PRIORITY)
STACKSIZE = $(CONFIG_TESTING_CMOCKA_STACKSIZE)
MODULE    = $(CONFIG_TESTING_CMOCKA)

MAINSRC = $(CURDIR)/cmocka_main.c

# Download and unpack tarball if no git repo found
ifeq ($(wildcard cmocka/.git),)
VERSION ?= 1.1.5
cmocka.zip:
	$(Q) curl -L https://github.com/clibs/cmocka/archive/$(VERSION).zip -o cmocka.zip
	$(Q) unzip -o cmocka.zip
	$(Q) mv cmocka-$(VERSION) cmocka
	$(Q) patch -p0 < 0001-cmocka.c-Reduce-the-call-stack-consumption-of-printf.patch
	$(Q) patch -p0 < 0002-cmocka-feature-to-forwarding-cmocka-log-message-to-c.patch
	$(Q) patch -p0 < 0003-cmocka-update-method-for-strmatch-to-regex-and-add-list-all-testcases-function.patch
	$(Q) patch -p0 < 0004-cmocka-xml-report.patch
	$(Q) patch -p0 < 0005-cmocka-cmocka_private-fix-warning-in-cmocka_private.patch
	$(Q) patch -p0 < 0006-fix-linux-risc-v-compile-error-list_initialize.patch

context:: cmocka.zip

distclean::
	$(call DELDIR, cmocka)
	$(call DELFILE, cmocka.zip)

endif

include $(APPDIR)/Application.mk
