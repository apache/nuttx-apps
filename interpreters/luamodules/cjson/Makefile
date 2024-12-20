############################################################################
# apps/interpreters/luamodules/cjson/Makefile
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

LUACJSON_VERSION  = $(patsubst "%",%,$(strip $(CONFIG_LUA_CJSON_VERSION)))
LUACJSON_TARBALL  = $(LUACJSON_VERSION).tar.gz
LUACJSON_UNPACK   = lua-cjson
LUACJSON_URL_BASE = https://github.com/openresty/lua-cjson/archive/refs/tags
LUACJSON_URL      = $(LUACJSON_URL_BASE)/$(LUACJSON_TARBALL)
LUACJSON_SRC      = $(LUACJSON_UNPACK)

VPATH += lua-cjson
CSRCS = lua_cjson.c strbuf.c fpconv.c

$(LUACJSON_TARBALL):
	$(Q) echo "Downloading $(LUACJSON_TARBALL) from $(LUACJSON_URL)"
	$(Q) curl -O -L $(LUACJSON_URL)

$(LUACJSON_UNPACK): $(LUACJSON_TARBALL)
	$(Q) echo "Unpacking $(LUACJSON_TARBALL) to $(LUACJSON_UNPACK)"
	$(Q) tar -xvzf $(LUACJSON_TARBALL)
	$(Q) mv lua-cjson-$(LUACJSON_VERSION) $(LUACJSON_UNPACK)
	$(Q) patch -d $(LUACJSON_UNPACK) -p1 < 0001-fix-compile-warnings.patch
	$(Q) patch -d $(LUACJSON_UNPACK) -p1 < 0002-bugfix-Fix-implicit-declaration-warning-for-strncase.patch

$(LUACJSON_UNPACK)/.patch: $(LUACJSON_UNPACK)
	touch $(LUACJSON_UNPACK)/.patch

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LUACJSON_UNPACK)/.git),)
distclean::
	$(call DELDIR, $(LUACJSON_UNPACK))
	$(call DELFILE, $(LUACJSON_TARBALL))

context:: $(LUACJSON_UNPACK)/.patch
endif

# Set LUAMODNAME and include Module.mk to add this module to the list of
# builtin modules for the Lua interpreter. LUAMODNAME should match the
# module's luaopen function.

LUAMODNAME = cjson

include $(APPDIR)/interpreters/lua/Module.mk

include $(APPDIR)/Application.mk
