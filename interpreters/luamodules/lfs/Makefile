############################################################################
# apps/interpreters/luamodules/lfs/Makefile
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

LFS_VERSION  = $(patsubst "%",%,$(strip $(CONFIG_LUA_LFS_VERSION)))
LFS_TARBALL  = v$(LFS_VERSION).tar.gz
LFS_UNPACK   = lfs
LFS_URL_BASE = https://github.com/lunarmodules/luafilesystem/archive/refs/tags
LFS_URL      = $(LFS_URL_BASE)/$(LFS_TARBALL)
LFS_SRC      = $(LFS_UNPACK)$(DELIM)src

VPATH += $(LFS_SRC)
CSRCS = lfs.c

$(LFS_TARBALL):
	$(Q) echo "Downloading $(LFS_TARBALL) from $(LFS_URL)"
	$(Q) curl -O -L $(LFS_URL)

$(LFS_UNPACK): $(LFS_TARBALL)
	$(Q) echo "Unpacking $(LFS_TARBALL) to $(LFS_UNPACK)"
	$(Q) tar -xvzf $(LFS_TARBALL)
	$(Q) mv luafilesystem-$(LFS_VERSION) $(LFS_UNPACK)
	$(Q) patch -d $(LFS_UNPACK) -p1 < 0001-call-symlink-or-link-directly.patch

$(LFS_UNPACK)/.patch: $(LFS_UNPACK)
	touch $(LFS_UNPACK)/.patch

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LFS_UNPACK)/.git),)
distclean::
	$(call DELDIR, $(LFS_UNPACK))
	$(call DELFILE, $(LFS_TARBALL))

context:: $(LFS_UNPACK)/.patch
endif

# Set LUAMODNAME and include Module.mk to add this module to the list of
# builtin modules for the Lua interpreter. LUAMODNAME should match the
# module's luaopen function.

LUAMODNAME = lfs

include $(APPDIR)/interpreters/lua/Module.mk

include $(APPDIR)/Application.mk
