############################################################################
# apps/netutils/nanopb/Makefile
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

NANOPB_VERSION = $(CONFIG_NETUTILS_NANOPB_VERSION)
ifeq ($(CONFIG_HOST_WINDOWS),y)
	NANOPB_NAME = nanopb-$(NANOPB_VERSION)-windows-x86
else ifeq ($(CONFIG_HOST_LINUX),y)
	NANOPB_NAME = nanopb-$(NANOPB_VERSION)-linux-x86
else
	NANOPB_NAME = nanopb-$(NANOPB_VERSION)-macosx-x86
endif

NANOPB_TARBALL	= $(NANOPB_NAME).tar.gz
NANOPB_UNPACK	= nanopb
NANOPB_SRCDIR	= $(NANOPB_UNPACK)

$(NANOPB_TARBALL):
	$(Q) echo "Downloading $(NANOPB_TARBALL)"
	$(Q) curl -O -L https://jpa.kapsi.fi/nanopb/download/$(NANOPB_TARBALL)

$(NANOPB_UNPACK): $(NANOPB_TARBALL)
	$(Q) tar zxf $(NANOPB_TARBALL)
	$(Q) mv nanopb-$(NANOPB_VERSION)-linux-x86 $(NANOPB_UNPACK)
	$(Q) mv $(NANOPB_TARBALL) $(NANOPB_UNPACK)

ifeq ($(wildcard $(NANOPB_UNPACK)),)
context:: $(NANOPB_UNPACK)
endif

CSRCS = $(NANOPB_SRCDIR)$(DELIM)pb_common.c
CSRCS += $(NANOPB_SRCDIR)$(DELIM)pb_decode.c
CSRCS += $(NANOPB_SRCDIR)$(DELIM)pb_encode.c

ifneq ($(CONFIG_NETUTILS_NANOPB_EXAMPLE),)
PROGNAME  = nanopb_example
PRIORITY  = $(CONFIG_NETUTILS_NANOPB_EXAMPLE_PRIORITY)
STACKSIZE = $(CONFIG_NETUTILS_NANOPB_EXAMPLE_STACKSIZE)
MODULE    = $(CONFIG_NETUTILS_NANOPB_EXAMPLE)

EXAMPLE_DIR = $(NANOPB_UNPACK)$(DELIM)examples$(DELIM)simple
MAINSRC = $(EXAMPLE_DIR)$(DELIM)simple.c
CSRCS += $(EXAMPLE_DIR)$(DELIM)simple.pb.c	
CFLAGS += "-I$(NANOPB_UNPACK)"

# The context below generates the simple.pb.c and simple.pb.h from the .proto
# file using the nanobpb_generator executable inside nanopb/generator-bin.
GENERATOR_EXECUTABLE = $(NANOPB_UNPACK)$(DELIM)generator-bin$(DELIM)nanopb_generator
ifeq ($(CONFIG_HOST_WINDOWS),y)
GENERATOR_EXECUTABLE += .exe
endif

context::
	$(GENERATOR_EXECUTABLE)	--output-dir=$(EXAMPLE_DIR) --options-path=$(EXAMPLE_DIR) $(EXAMPLE_DIR)$(DELIM)simple.proto
endif

distclean::
	$(call DELDIR, $(NANOPB_UNPACK))

include $(APPDIR)/Application.mk
