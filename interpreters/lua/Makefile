############################################################################
# apps/interpreters/lua/Makefile
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

# Lua built-in application info

PROGNAME  = lua
PRIORITY  = $(CONFIG_INTERPRETER_LUA_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETER_LUA_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_LUA)

# Lua library

LUA_VERSION  = $(patsubst "%",%,$(strip $(CONFIG_INTERPRETER_LUA_VERSION)))
LUA_TARBALL  = lua-$(LUA_VERSION).tar.gz
LUA_UNPACK   = lua-$(LUA_VERSION)
LUA_URL_BASE = http://www.lua.org/ftp
LUA_URL      = $(LUA_URL_BASE)/$(LUA_TARBALL)
LUA_SRC      = $(LUA_UNPACK)$(DELIM)src

MAINSRC       = $(LUA_SRC)$(DELIM)lua.c
CORELIBS_SRCS = $(filter-out $(LUA_SRC)$(DELIM)lauxlib.c,$(wildcard $(LUA_SRC)$(DELIM)*lib.c))
EXCLUDE_SRCS  = $(MAINSRC) $(CORELIBS_SRCS) $(LUA_SRC)$(DELIM)luac.c $(LUA_SRC)$(DELIM)linit.c
CSRCS         = $(filter-out $(EXCLUDE_SRCS),$(wildcard $(LUA_SRC)$(DELIM)*.c))
CSRCS        += nuttx_linit.c

CFLAGS += -DLUA_MAXINPUT=$(CONFIG_INTERPRETER_LUA_IOBUFSIZE)
CFLAGS += -DLUA_PROGNAME=\"$(PROGNAME)\"

ifeq ($(CONFIG_INTERPRETER_LUA_32BITS),y)
CFLAGS += -DLUA_32BITS
endif

ifneq ($(CONFIG_INTERPRETER_LUA_PATH),"")
CFLAGS += -DLUA_PATH_DEFAULT=\"$(CONFIG_INTERPRETER_LUA_PATH)\"
endif

ifneq ($(CONFIG_INTERPRETER_LUA_CPATH),"")
CFLAGS += -DLUA_CPATH_DEFAULT=\"$(CONFIG_INTERPRETER_LUA_CPATH)\"
endif

ifeq ($(CONFIG_SYSTEM_READLINE),y)
CFLAGS += -include "system/readline.h"
CFLAGS += -D'lua_initreadline(L)=((void)L)'
CFLAGS += -D'lua_readline(L,b,p)=((void)L,fputs(p,stdout),fflush(stdout),readline(b,LUA_MAXINPUT,stdin,stdout))'
CFLAGS += -D'lua_saveline(L,line)={(void)L;(void)line;}'
CFLAGS += -D'lua_freeline(L,line)={(void)L;(void)b;}'
endif

# Lua download and unpack

$(LUA_TARBALL):
	$(Q) echo "Downloading $(LUA_TARBALL)"
	$(Q) curl -O -L $(LUA_URL)

$(LUA_UNPACK): $(LUA_TARBALL)
	$(Q) echo "Unpacking $(LUA_TARBALL) to $(LUA_UNPACK)"
	$(Q) tar -xvzf $(LUA_TARBALL)

context:: $(LUA_UNPACK)

# Register core modules

ifeq ($(CONFIG_INTERPRETER_LUA_CORELIBS),y)
CSRCS += $(CORELIBS_SRCS)

register::
	# collect core module names from defines in lualib.h
	$(Q) while read -r line; do \
		name=$$(expr "$$line" : '#define LUA_[[:upper:]]\+LIBNAME[[:space:]]\+"\([[:lower:]]\+\)"'); \
		if [ ! -z $$name ]; then \
			printf '{ "%s", luaopen_%s },\n' $$name $$name > registry$(DELIM)$$name.bdat; \
		fi done < $(LUA_SRC)$(DELIM)lualib.h
endif

# Lua builtin module registry

PDATLIST = $(strip $(call RWILDCARD, registry, *.pdat))
BDATLIST = $(strip $(call RWILDCARD, registry, *.bdat))

lua_main.c: luamod_list.h luamod_proto.h

registry$(DELIM).updated:
	$(Q) touch registry$(DELIM).updated

luamod_list.h: registry$(DELIM).updated
ifeq ($(BDATLIST),)
	$(call DELFILE, luamod_list.h)
	$(Q) touch luamod_list.h
else
	$(call CATFILE, luamod_list.h, $(BDATLIST))
endif

luamod_proto.h: registry$(DELIM).updated
ifeq ($(PDATLIST),)
	$(call DELFILE, luamod_proto.h)
	$(Q) touch luamod_proto.h
else
	$(call CATFILE, luamod_proto.h, $(PDATLIST))
endif

depend:: luamod_list.h luamod_proto.h

clean::
	$(call DELFILE, luamod_list.h)
	$(call DELFILE, luamod_proto.h)

clean_context::
	$(call DELFILE, $(PDATLIST))
	$(call DELFILE, $(BDATLIST))

distclean:: clean_context clean
	$(call DELFILE, registry$(DELIM).updated)
	$(call DELDIR, $(LUA_UNPACK))
	$(call DELFILE, $(LUA_TARBALL))

include $(APPDIR)/Application.mk
