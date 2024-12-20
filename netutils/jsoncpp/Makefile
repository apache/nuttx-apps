############################################################################
# apps/netutils/jsoncpp/Makefile
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

JSONCPP_URL ?= "https://github.com/open-source-parsers/jsoncpp/archive/refs/tags"

JSONCPP_VERSION = $(patsubst "%",%,$(strip $(CONFIG_JSONCPP_VERSION)))
JSONCPP_ZIP = $(CONFIG_JSONCPP_VERSION).zip

JSONCPP_UNPACKNAME = jsoncpp
UNPACK ?= unzip -q -o

$(JSONCPP_ZIP):
	@echo "Downloading: $(JSONCPP_URL)/$(JSONCPP_ZIP)"
	$(Q) curl -O -L $(JSONCPP_URL)/$(JSONCPP_ZIP)

$(JSONCPP_UNPACKNAME): $(JSONCPP_ZIP)
	@echo "Unpacking: $(JSONCPP_ZIP) -> $(JSONCPP_UNPACKNAME)"
	$(Q) $(UNPACK) $(JSONCPP_ZIP)
	$(Q) mv jsoncpp-$(JSONCPP_VERSION) $(JSONCPP_UNPACKNAME)
	$(Q) touch $(JSONCPP_UNPACKNAME)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(JSONCPP_UNPACKNAME)),)
context:: $(JSONCPP_UNPACKNAME)

distclean::
	$(call DELDIR, $(JSONCPP_UNPACKNAME))
	$(call DELFILE, $(JSONCPP_ZIP))
endif

CXXEXT = .cpp

CXXFLAGS += -DJSON_USE_EXCEPTION=0

CXXSRCS += jsoncpp/src/lib_json/json_writer.cpp
CXXSRCS += jsoncpp/src/lib_json/json_value.cpp
CXXSRCS += jsoncpp/src/lib_json/json_reader.cpp

include $(APPDIR)/Application.mk
