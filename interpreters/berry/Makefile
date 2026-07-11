############################################################################
# apps/interpreters/berry/Makefile
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

BERRY_COMMIT_ID  = 4db341475df9e9ad18e6741ef978b4c467690e09
BERRY_UNPACK     = berry
BERRY_ARCHIVE    = berry-$(BERRY_COMMIT_ID).zip
BERRY_URL_BASE   = https://github.com/berry-lang/berry/archive/
BERRY_URL        = $(BERRY_URL_BASE)/$(BERRY_COMMIT_ID).zip
BERRY_CONFIG     = include$(DELIM)berry_conf.h
BERRY_COC        = $(BERRY_UNPACK)$(DELIM)tools$(DELIM)coc$(DELIM)coc
BERRY_GENERATE   = $(BERRY_UNPACK)$(DELIM)generate
BERRY_GEN_STAMP  = $(BERRY_GENERATE)$(DELIM).generated

BERRY_SRCS = be_api.c be_baselib.c be_bytecode.c be_byteslib.c
BERRY_SRCS += be_class.c be_code.c be_debug.c be_debuglib.c be_exec.c
BERRY_SRCS += be_filelib.c be_func.c be_gc.c be_gclib.c be_globallib.c
BERRY_SRCS += be_introspectlib.c be_jsonlib.c be_lexer.c be_libs.c
BERRY_SRCS += be_list.c be_listlib.c be_map.c be_maplib.c be_mathlib.c
BERRY_SRCS += be_mem.c be_module.c be_object.c be_oslib.c be_parser.c
BERRY_SRCS += be_rangelib.c be_repl.c be_solidifylib.c be_strictlib.c
BERRY_SRCS += be_string.c be_strlib.c be_syslib.c be_timelib.c
BERRY_SRCS += be_undefinedlib.c be_var.c be_vector.c be_vm.c

CSRCS = $(BERRY_SRCS)
CSRCS += be_modtab.c be_port.c

MAINSRC = berry.c

VPATH += $(BERRY_UNPACK)$(DELIM)src
VPATH += $(BERRY_UNPACK)$(DELIM)default
DEPPATH += --dep-path $(BERRY_UNPACK)$(DELIM)src
DEPPATH += --dep-path $(BERRY_UNPACK)$(DELIM)default

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)$(DELIM)interpreters$(DELIM)berry$(DELIM)include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)$(DELIM)interpreters$(DELIM)berry$(DELIM)$(BERRY_UNPACK)$(DELIM)src
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)$(DELIM)interpreters$(DELIM)berry$(DELIM)$(BERRY_UNPACK)$(DELIM)default
CFLAGS += -Wno-unused-but-set-variable
CFLAGS += -Wno-shadow
CFLAGS += -DUSE_NUTTX_CONSOLE

PROGNAME  = $(CONFIG_INTERPRETERS_BERRY_PROGNAME)
PRIORITY  = $(CONFIG_INTERPRETERS_BERRY_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_BERRY_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_BERRY)

$(BERRY_ARCHIVE):
	$(Q) echo "Downloading $(BERRY_ARCHIVE)"
	$(Q) curl -L $(BERRY_URL) -o $(BERRY_ARCHIVE)

$(BERRY_UNPACK): $(BERRY_ARCHIVE)
	$(Q) echo "Unpacking $(BERRY_ARCHIVE) to $(BERRY_UNPACK)"
	$(Q) unzip -q -o $(BERRY_ARCHIVE)
	$(call DELDIR, $(BERRY_UNPACK))
	$(Q) mv berry-$(BERRY_COMMIT_ID) $(BERRY_UNPACK)
	$(Q) patch -l -d $(BERRY_UNPACK) -p1 < 0001-Fix-Berry-default-port-for-NuttX.patch
	$(Q) patch -l -d $(BERRY_UNPACK) -p1 < 0002-Use-NuttX-console-reader.patch

$(BERRY_GEN_STAMP): $(BERRY_UNPACK) $(BERRY_CONFIG)
	$(Q) echo "Generating Berry constant objects"
	$(Q) mkdir -p $(BERRY_GENERATE)
	$(Q) python3 $(BERRY_COC) -o $(BERRY_GENERATE) \
		$(BERRY_UNPACK)$(DELIM)src \
		$(BERRY_UNPACK)$(DELIM)default \
		-c $(BERRY_CONFIG)
	$(Q) touch $(BERRY_GEN_STAMP)

ifeq ($(wildcard $(BERRY_UNPACK)/.git),)
context:: $(BERRY_GEN_STAMP)
endif

depend:: $(BERRY_GEN_STAMP)

distclean::
ifeq ($(wildcard $(BERRY_UNPACK)/.git),)
	$(call DELDIR, $(BERRY_UNPACK))
	$(call DELFILE, $(BERRY_ARCHIVE))
endif

include $(APPDIR)/Application.mk
