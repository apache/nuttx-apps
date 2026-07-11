############################################################################
# apps/examples/sotest/main/Makefile
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

# Shared library example built-in application info

PROGNAME = sotest
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_SOTEST)

# Shared Library Example

MAINSRC = sotest_main.c

SYMTABSRC = sotest_symtab.c
SYMTABOBJ = $(SYMTABSRC:.c=$(OBJEXT))

$(SYMTABSRC):
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(BINDIR) g_sot >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)


$(SYMTABOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@, -fno-lto -fno-builtin)


ifeq ($(CONFIG_EXAMPLES_SOTEST_BUILTINFS),y)

ROMFSIMG = sotest_romfs.img
ROMFSSRC = sotest_romfs.c
ROMFSOBJ = $(ROMFSSRC:.c=$(OBJEXT))
ifdef CONFIG_SYSTEM_NXPKG
HOSTOBJSEXT ?= hobj
PKGSHAREDINDEX = $(BINDIR)/shared-index.json
PKGSHAREDTEST = $(BINDIR)/pkgsotest.nsh
PKG_FIXTURE_GEN = mk_pkg_fixture_shared$(HOSTEXEEXT)
PKG_FIXTURE_SRCS = mk_pkg_fixture_shared.c
PKG_FIXTURE_OBJS = $(PKG_FIXTURE_SRCS:.c=.$(HOSTOBJSEXT))
PKG_FIXTURE_OUTPUTS = $(PKGSHAREDINDEX) $(PKGSHAREDTEST)
PKG_FIXTURE_STAMP = $(BINDIR)/.pkgsotest.stamp
PKG_FIXTURE_COMPAT = $(if $(CONFIG_ARCH_BOARD),$(CONFIG_ARCH_BOARD),$(CONFIG_ARCH_BOARD_CUSTOM_NAME))

$(PKG_FIXTURE_OBJS): %.$(HOSTOBJSEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@

$(PKG_FIXTURE_GEN): $(PKG_FIXTURE_OBJS)
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(PKG_FIXTURE_OBJS) -o $@

$(PKG_FIXTURE_OUTPUTS): $(PKG_FIXTURE_STAMP)

$(PKG_FIXTURE_STAMP): $(BINDIR)/modprint $(BINDIR)/sotest $(PKG_FIXTURE_GEN)
	$(Q) ./$(PKG_FIXTURE_GEN) $(BINDIR)/modprint $(BINDIR)/sotest \
		$(PKGSHAREDINDEX) $(PKGSHAREDTEST) $(CONFIG_ARCH) \
		$(PKG_FIXTURE_COMPAT)
	$(Q) touch $@
endif

$(ROMFSIMG): $(PKG_FIXTURE_OUTPUTS)
	$(Q) genromfs -d $(BINDIR) -f $@

$(ROMFSSRC): $(ROMFSIMG)
	$(Q) (echo "#include <nuttx/compiler.h>" >$@ && \
		xxd -i $(ROMFSIMG) | sed -e "s/^unsigned char/const unsigned char aligned_data(4)/g" >>$@)

$(ROMFSOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@, -fno-lto -fno-builtin)

endif

postinstall:: $(ROMFSOBJ) $(SYMTABOBJ) $(FSIMG_OBJ)
	$(call ARLOCK, $(call CONVERT_PATH,$(BIN)), $^)

distclean::
	$(Q) $(call DELFILE, $(SYMTABSRC) $(SYMTABOBJ) $(ROMFSSRC) $(ROMFSIMG) $(ROMFSOBJ) $(PKG_FIXTURE_GEN) $(PKG_FIXTURE_OBJS) $(PKG_FIXTURE_STAMP))

include $(APPDIR)/Application.mk
