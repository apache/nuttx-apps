############################################################################
# apps/testing/arch/x86-64-ABI/Makefile
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

X86_64_ABI_COMMIT ?= ab2062ad5653913c39124548943b1177330e34c8

# Download and unpack tarball if no git repo found

ifeq ($(wildcard x86-64-ABI/.git),)
x86-64-ABI.zip:
	$(call DOWNLOAD,https://gitlab.com/x86-psABIs/x86-64-ABI/-/archive/$(X86_64_ABI_COMMIT),x86-64-ABI-$(X86_64_ABI_COMMIT).zip,x86-64-ABI.zip)
	$(Q) unzip -o x86-64-ABI.zip
	mv x86-64-ABI-$(X86_64_ABI_COMMIT) x86-64-ABI
	$(Q) patch -p1 -d x86-64-ABI < 0001-app-testing-Resolve-the-issue-of-compilation-failure.patch

context:: x86-64-ABI.zip
endif

distclean::
ifeq ($(wildcard x86-64-ABI/.git),)
	$(call DELDIR, x86-64-ABI)
	$(call DELFILE, x86-64-ABI.zip)
endif

SUBDIR = x86-64-ABI/abitest

$(SUBDIR)/test_3_element_struct_and_unions.c: $(SUBDIR)/make_3esau.c $(SUBDIR)/typelist.h \
	$(SUBDIR)/typelist.c $(SUBDIR)/defines.h $(SUBDIR)/args.h
	cd $(SUBDIR) && \
	gcc -o make_3esau make_3esau.c typelist.c && \
	./make_3esau > test_3_element_struct_and_unions.c

$(SUBDIR)/test_passing_integers.c $(SUBDIR)/test_passing_floats.c: $(SUBDIR)/make_passing_tests.c \
	$(SUBDIR)/typelist.h $(SUBDIR)/typelist.c $(SUBDIR)/defines.h $(SUBDIR)/args.h
	cd $(SUBDIR) && \
	gcc -o make_passing_tests make_passing_tests.c typelist.c && \
	./make_passing_tests

$(SUBDIR)/test_basic_returning.c: $(SUBDIR)/make_returning.c $(SUBDIR)/typelist.h $(SUBDIR)/typelist.c \
	$(SUBDIR)/defines.h $(SUBDIR)/args.h
	cd $(SUBDIR) && \
	gcc -o make_returning make_returning.c typelist.c && \
	./make_returning > test_basic_returning.c

CSRCS += $(SUBDIR)/test_basic_returning.c  $(SUBDIR)/test_passing_integers.c $(SUBDIR)/test_passing_floats.c \
         $(SUBDIR)/test_3_element_struct_and_unions.c

ASRCS = $(SUBDIR)/asm-support.S

define add_benchmark
  PROGNAME += $1
  MAINSRC += $(SUBDIR)/$1.c
  PRIORITY += 100
  STACKSIZE += $(CONFIG_DEFAULT_TASK_STACKSIZE)
endef

# Add test cases with compilation errors
EXCLUDE = make_3esau make_passing_tests make_returning medium-test typelist
ALL_CASES := $(wildcard x86-64-ABI/abitest/*.c)

# Filter unnecessary files
BENCHMARKS := $(filter-out $(EXCLUDE), $(patsubst %.c,%,$(notdir $(ALL_CASES))))

$(foreach benchmark,$(BENCHMARKS),$(eval $(call add_benchmark,$(benchmark))))

include $(APPDIR)/Application.mk

