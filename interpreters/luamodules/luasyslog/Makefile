############################################################################
# apps/interpreters/luamodules/luasyslog/Makefile
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

LSYSLOG_VERSION  = $(patsubst "%",%,$(strip $(CONFIG_LUA_LSYSLOG_VERSION)))
LSYSLOG_TARBALL  = $(LSYSLOG_VERSION).tar.gz
LSYSLOG_UNPACK   = lsyslog
LSYSLOG_URL_BASE = https://github.com/lunarmodules/luasyslog/archive/refs/tags
LSYSLOG_URL      = $(LSYSLOG_URL_BASE)/$(LSYSLOG_TARBALL)
LSYSLOG_SRC      = $(LSYSLOG_UNPACK)

VPATH += $(LSYSLOG_SRC)
CSRCS = lsyslog.c

CFLAGS += -D'openlog(a,b,c)={(void)a; (void)c;}'
CFLAGS += -D'closelog()={}'

$(LSYSLOG_TARBALL):
	$(Q) echo "Downloading $(LSYSLOG_TARBALL) from $(LSYSLOG_URL)"
	$(Q) curl -O -L $(LSYSLOG_URL)

$(LSYSLOG_UNPACK): $(LSYSLOG_TARBALL)
	$(Q) echo "Unpacking $(LSYSLOG_TARBALL) to $(LSYSLOG_UNPACK)"
	$(Q) tar -xvzf $(LSYSLOG_TARBALL)
	$(Q) mv luasyslog-$(LSYSLOG_VERSION) $(LSYSLOG_UNPACK)

$(LSYSLOG_UNPACK)/.patch: $(LSYSLOG_UNPACK)
	touch $(LSYSLOG_UNPACK)/.patch

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LSYSLOG_UNPACK)/.git),)
distclean::
	$(call DELDIR, $(LSYSLOG_UNPACK))
	$(call DELFILE, $(LSYSLOG_TARBALL))

context:: $(LSYSLOG_UNPACK)/.patch
endif

# Set LUAMODNAME and include Module.mk to add this module to the list of
# builtin modules for the Lua interpreter. LUAMODNAME should match the
# module's luaopen function.

LUAMODNAME = lsyslog

include $(APPDIR)/interpreters/lua/Module.mk

include $(APPDIR)/Application.mk
