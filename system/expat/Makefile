############################################################################
# apps/system/expat/Makefile
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

EXPAT_VERSION = $(patsubst "%",%,$(CONFIG_LIB_EXPAT_VERSION))
EXPAT_TAG = R_$(subst .,_,$(EXPAT_VERSION))
EXPAT_ARCHIVE = $(EXPAT_TAG).tar.gz
EXPAT_ARCHIVES = $(wildcard R_*.tar.gz)
EXPAT_SRCDIR = libexpat
EXPAT_URL ?= https://codeload.github.com/libexpat/libexpat/tar.gz/refs/tags

CSRCS += $(EXPAT_SRCDIR)/expat/lib/xmlparse.c
CSRCS += $(EXPAT_SRCDIR)/expat/lib/xmlrole.c
CSRCS += $(EXPAT_SRCDIR)/expat/lib/xmltok.c
CSRCS += $(EXPAT_SRCDIR)/expat/lib/random_arc4random_buf.c

CFLAGS += ${INCDIR_PREFIX}$(CURDIR)
CFLAGS += ${INCDIR_PREFIX}$(EXPAT_SRCDIR)/expat/lib
CFLAGS += -DXML_STATIC -Wno-shadow

ifeq ($(CONFIG_LIB_EXPAT_DTD),y)
CFLAGS += -DXML_DTD
endif

ifeq ($(wildcard $(EXPAT_SRCDIR)/.git),)
$(EXPAT_ARCHIVE):
	@echo "Downloading: $(EXPAT_ARCHIVE)"
	$(Q) curl -o $(EXPAT_ARCHIVE) -L \
		$(EXPAT_URL)/$(EXPAT_TAG)

$(EXPAT_SRCDIR): $(EXPAT_ARCHIVE)
	$(Q) tar -xf $(EXPAT_ARCHIVE)
	$(Q) mv libexpat-$(EXPAT_TAG) $(EXPAT_SRCDIR)

context:: $(EXPAT_SRCDIR)

distclean::
	$(call DELDIR, $(EXPAT_SRCDIR))
	$(call DELFILE, $(EXPAT_ARCHIVES))
endif

MODULE = $(CONFIG_LIB_EXPAT)

include $(APPDIR)/Application.mk
