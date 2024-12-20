#############################################################################
# apps/netutils/cjson/Makefile
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
#############################################################################

# Standard includes

include $(APPDIR)/Make.defs

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

CONFIG_NETUTILS_CJSON_URL ?= "https://github.com/DaveGamble/cJSON/archive"
CONFIG_NETUTILS_CJSON_VERSION ?= "1.7.10"
CJSON_VERSION = $(patsubst "%",%,$(strip $(CONFIG_NETUTILS_CJSON_VERSION)))

CJSON_TARBALL = v$(CJSON_VERSION).tar.gz
CJSON_UNPACKNAME = cJSON

UNPACK ?= tar -zxf

CJSON_UNPACKDIR =  $(WD)/$(CJSON_UNPACKNAME)
CJSON_SRCDIR = $(CJSON_UNPACKNAME)

APPS_INCDIR = $(APPDIR)$(DELIM)include$(DELIM)netutils

CFLAGS += ${INCDIR_PREFIX}$(APPS_INCDIR)
CFLAGS += -DCJSON_INCLUDE_CONFIG_H

CSRCS = $(CJSON_SRCDIR)$(DELIM)cJSON.c
CSRCS += $(CJSON_SRCDIR)$(DELIM)cJSON_Utils.c

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(CJSON_UNPACKNAME)/.git),)
$(CJSON_TARBALL):
	@echo "Downloading: $(CJSON_TARBALL)"
	$(Q) curl -O -L $(CONFIG_NETUTILS_CJSON_URL)/$(CJSON_TARBALL)

$(CJSON_UNPACKNAME): $(CJSON_TARBALL)
	@echo "Unpacking: $(CJSON_TARBALL) -> $(CJSON_UNPACKNAME)"
	$(Q) $(UNPACK) $(CJSON_TARBALL)
	$(Q) mv cJSON-$(CJSON_VERSION) $(CJSON_UNPACKNAME)
	$(Q) touch $(CJSON_UNPACKNAME)
endif

ifneq ($(CONFIG_NETUTILS_CJSON_TEST),)
PROGNAME += cjson_test
MAINSRC += $(CJSON_SRCDIR)$(DELIM)test.c

UNITY_CJSONTESTS := parse_examples parse_number parse_hex4 parse_string parse_array \
	parse_object parse_value print_string print_number print_array print_object \
	print_value cjson_add parse_with_opts compare_tests readme_examples minify_tests \
	misc_tests json_patch_tests old_utils_tests misc_utils_tests
UNITY_CJSONTESTCS := $(foreach n, $(UNITY_CJSONTESTS), $(CJSON_SRCDIR)$(DELIM)tests$(DELIM)$(n).c)
CJSONTEST_MAIN := $(foreach n, $(UNITY_CJSONTESTS), cjson_$(n))
PROGNAME += $(CJSONTEST_MAIN)
MAINSRC += $(UNITY_CJSONTESTCS)

CSRCS += $(CJSON_SRCDIR)/tests/unity_setup.c
CFLAGS += -Wno-unused-function
CFLAGS += -DCJSON_NESTING_LIMIT=CONFIG_CJSON_NESTING_LIMIT

PRIORITY  = $(CONFIG_CJSON_TEST_PRIORITY)
STACKSIZE = $(CONFIG_CJSON_TEST_STACKSIZE)
endif

$(CJSON_SRCDIR)$(DELIM)cJSON.h: $(CJSON_UNPACKNAME)

$(CJSON_SRCDIR)$(DELIM)cJSON_Utils.h: $(CJSON_UNPACKNAME)

$(APPS_INCDIR)$(DELIM)cJSON.h: $(CJSON_SRCDIR)$(DELIM)cJSON.h
	$(Q) cp $< $@

$(APPS_INCDIR)$(DELIM)cJSON_Utils.h: $(CJSON_SRCDIR)$(DELIM)cJSON_Utils.h
	$(Q) cp $< $@

context:: $(APPS_INCDIR)$(DELIM)cJSON.h $(APPS_INCDIR)$(DELIM)cJSON_Utils.h

distclean::
ifeq ($(wildcard $(CJSON_UNPACKNAME)/.git),)
	$(call DELDIR, $(CJSON_UNPACKNAME))
	$(call DELFILE, $(CJSON_TARBALL))
endif
	$(call DELFILE, $(APPDIR)/include/netutils/cJSON.h)
	$(call DELFILE, $(APPDIR)/include/netutils/cJSON_Utils.h)

include $(APPDIR)/Application.mk
