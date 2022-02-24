############################################################################
# apps/interpreters/lua/Module.mk
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

LUAMOD_REGISTRY = $(APPDIR)$(DELIM)interpreters$(DELIM)lua$(DELIM)registry

define LUAMOD_REGISTER
	$(Q) echo Register Lua Module: $1
	$(Q) echo { \"$1\", $2 }, > "$(LUAMOD_REGISTRY)$(DELIM)$1.bdat"
	$(Q) echo "int $2(lua_State *L);" > "$(LUAMOD_REGISTRY)$(DELIM)$1.pdat"
	$(Q) touch "$(LUAMOD_REGISTRY)$(DELIM).updated"
endef

ifneq ($(LUAMODNAME),)
LUAMODLIST := $(addprefix $(LUAMOD_REGISTRY)$(DELIM),$(addsuffix .bdat,$(LUAMODNAME)))
$(LUAMODLIST): $(DEPCONFIG) Makefile
	$(call LUAMOD_REGISTER,$(LUAMODNAME),luaopen_$(LUAMODNAME))

register:: $(LUAMODLIST)
endif
