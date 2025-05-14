############################################################################
# apps/interpreters/python/Makefile
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

CPYTHON_URL ?= "https://github.com/python/cpython/archive"
CPYTHON_VERSION = $(patsubst "%",%,$(strip $(CONFIG_INTERPRETER_CPYTHON_VERSION)))
CPYTHON_VERSION_MINOR=$(basename $(CPYTHON_VERSION))
CPYTHON_ZIP = v$(CPYTHON_VERSION).zip

CPYTHON_UNPACKNAME = Python
UNPACK ?= unzip -q -o

MACHDEP=nuttx
CONFIG_SITE=${CURDIR}/config.site
SETUP_LOCAL=${CURDIR}/Setup.local
CPYTHON_PATH=$(CURDIR)/$(CPYTHON_UNPACKNAME)

BUILDIR=$(CURDIR)/build
INSTALLDIR=$(CURDIR)/install
HOSTBUILD=$(BUILDIR)/host
HOSTINSTALL=$(INSTALLDIR)/host
HOSTPYTHON=$(HOSTINSTALL)/bin/python3
TARGETBUILD=$(BUILDIR)/target
TARGETINSTALL=$(INSTALLDIR)/target
TARGETLIBPYTHON=$(TARGETINSTALL)/libpython$(CPYTHON_VERSION_MINOR).a
TARGETMODULESPACK=$(TARGETBUILD)/lib/python$(shell echo $(CPYTHON_VERSION_MINOR) | tr -d .).zip
TARGETMODULES=$(TARGETINSTALL)/lib/

CFLAGS += ${INCDIR_PREFIX}$(CPYTHON_PATH)$(DELIM)Include
CFLAGS += ${INCDIR_PREFIX}$(CPYTHON_PATH)$(DELIM)Test
CFLAGS += ${INCDIR_PREFIX}$(CPYTHON_PATH)$(DELIM)Include$(DELIM)internal
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)$(DELIM)system
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)$(DELIM)system$(DELIM)zlib$(DELIM)zlib
CFLAGS += ${INCDIR_PREFIX}$(TARGETBUILD)
CFLAGS += -Wno-shadow -Wno-undef -Wno-format -Wno-builtin-macro-redefined
CFLAGS += -Wno-type-limits -Wno-implicit-fallthrough -Wno-char-subscripts
CFLAGS += -Wno-sign-compare -Wno-unused-const-variable -Wno-unused-function
CFLAGS += -Wno-unused-variable -Wno-overflow -Wno-unused-but-set-variable
CFLAGS += -Wno-strict-prototypes -nostdlib

DEPPATH += --dep-path $(CPYTHON_UNPACKNAME)$(DELIM)Programs
VPATH   += :$(CPYTHON_UNPACKNAME)$(DELIM)Programs

$(CPYTHON_ZIP):
	@echo "Downloading: $(CPYTHON_URL)/$(CPYTHON_ZIP)"
	$(Q) $(call DOWNLOAD,$(CPYTHON_URL),$(CPYTHON_ZIP))

$(CPYTHON_UNPACKNAME): $(CPYTHON_ZIP)
	@echo "Unpacking: $(CPYTHON_ZIP) -> $(CPYTHON_UNPACKNAME)"
	$(Q) $(UNPACK) $(CPYTHON_ZIP)
	$(Q) mv	cpython-$(CPYTHON_VERSION) $(CPYTHON_UNPACKNAME)
	@echo "Patching $(CPYTHON_UNPACKNAME)"
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0001-workaround-newlib-resource.h-limitations.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0002-fix-various-uint32_t-unsigned-int-type-mismatch-issu.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0003-reuse-wasm_assets.py-for-generating-an-archive-of-py.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0004-recognize-nuttx-as-a-supported-OS.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0005-gh-122907-Fix-Builds-Without-HAVE_DYNAMIC_LOADING-Se.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0006-change-var-name-to-avoid-conflict-with-nuttx-unused_.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0007-undef-atexit_register.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0008-declare-struct-timeval.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0009-include-nuttx-sys-select-header-to-define-FD_SETSIZE.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0010-check-for-the-d_ino-member-of-the-structure-dirent.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0011-avoid-redefinition-warning-if-UNUSED-is-already-defi.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0012-hack-place-_PyRuntime-structure-into-PSRAM-bss-regio.patch
	$(Q) patch -p1 -d $(CPYTHON_UNPACKNAME) < patch$(DELIM)0013-transform-functions-used-by-NuttX-to-lowercase.patch

$(HOSTPYTHON):
	mkdir -p $(HOSTBUILD)
	mkdir -p $(HOSTINSTALL)
	$(Q) ( \
			cd $(HOSTBUILD) && $(CPYTHON_PATH)/configure \
			 --with-pydebug \
			 --prefix=$(HOSTINSTALL) \
		 )
	$(MAKE) -C $(HOSTBUILD) install

# The `config.site` file contains settings that override the configuration
# settings provided by the `configure` script. Depending on the features
# enabled on NuttX, this file may need to be adjusted.

$(CONFIG_SITE):
	$(Q) ( cp $(CONFIG_SITE).in $(CONFIG_SITE))
ifeq ($(CONFIG_ARCH_HAVE_FORK),y)
	@echo "export ac_cv_func_fork=\"yes\"" >> $@
else
	@echo "export ac_cv_func_fork=\"no\"" >> $@
endif
ifeq ($(CONFIG_SYSTEM_SYSTEM),y)
	@echo "export ac_cv_func_system=\"yes\"" >> $@
else
	@echo "export ac_cv_func_system=\"no\"" >> $@
endif

# The `Setup.local` file enables or disables Python modules.
# Depending on the features enabled on NuttX, this file may need to be
# adjusted. Please note that the base `Setup.local.in` file  only contains
# a section to disable Python modules. Inserting lines to it will disable
# such modules.

$(SETUP_LOCAL):
	$(Q) ( cp $(SETUP_LOCAL).in $(SETUP_LOCAL))
ifneq ($(CONFIG_ARCH_HAVE_FORK),y)
	@echo "_posixsubprocess" >> $@
endif

# For the Python's `configure` script, please consider the following
# when building for NuttX:
#
# Use sed to remove optimization flags from NuttX's CFLAGS because
# Python's configure script requires them in OPT. Having the flags in
# both places causes a conflict.
#
# Also, use -O0 for OPT because -Os is causing problems in
# Python/Modules/getpath.c (issue will be filed soon to track this
# problem).

$(TARGETBUILD)/Makefile: $(HOSTPYTHON) $(CONFIG_SITE) $(SETUP_LOCAL)
	$(Q) mkdir -p $(TARGETBUILD)/Modules
	$(Q) mkdir -p $(TARGETMODULES)/python$(CPYTHON_VERSION_MINOR)
	$(Q) ( cp Setup.local $(TARGETBUILD)/Modules/Setup.local )
	$(Q) ( \
			cd $(TARGETBUILD); \
			CFLAGS="$(CFLAGS)"; \
			ARCH=$(CONFIG_ARCH); \
			ARCH_CHIP=$(CONFIG_ARCH_CHIP); \
			ARCH="$${ARCH//-/}"; \
			ARCH_CHIP="$${ARCH_CHIP//-/}"; \
			CFLAGS="$$(echo "$${CFLAGS}" | sed 's/-Os //')" \
			CC="$(CC)" \
			CXX="$(CXX)" \
			AR="$(AR)" \
			ARFLAGS=" " \
			MACHDEP="$(MACHDEP)" \
			OPT="-g -O0 -Wall" \
			CONFIG_SITE="$(CONFIG_SITE)" \
			$(CPYTHON_PATH)/configure \
			 --prefix=${TARGETINSTALL} \
			 --disable-shared \
			 --host=$${ARCH}-$${ARCH_CHIP}-nuttx \
			 --build=$(shell $(CPYTHON_PATH)/config.guess) \
			 --with-build-python=${HOSTPYTHON} \
			 --without-mimalloc \
			 --without-pymalloc \
			 --disable-test-modules \
		)

$(TARGETLIBPYTHON): $(TARGETBUILD)/Makefile
	$(MAKE) -C $(TARGETBUILD) regen-frozen
	$(MAKE) -C $(TARGETBUILD) libpython$(CPYTHON_VERSION_MINOR).a wasm_stdlib
	$(Q) ( cp $(TARGETBUILD)/libpython$(CPYTHON_VERSION_MINOR).a $(TARGETLIBPYTHON) )
	$(Q) $(UNPACK) $(TARGETMODULESPACK) -d $(TARGETMODULES)/python$(CPYTHON_VERSION_MINOR)

MODULE    = $(CONFIG_INTERPRETER_CPYTHON)

PROGNAME  += $(CONFIG_INTERPRETER_CPYTHON_PROGNAME)
PRIORITY  += $(CONFIG_INTERPRETER_CPYTHON_PRIORITY)
STACKSIZE += $(CONFIG_INTERPRETER_CPYTHON_STACKSIZE)

MAINSRC += python_wrapper.c

checkgenromfs:
	@genromfs -h 1>/dev/null 2>&1 || { \
 echo "Host executable genromfs not available in PATH"; \
 echo "You may need to download in from https://romfs.sourceforge.net/"; \
 exit 1; \
	}

romfs_cpython_modules.img : $(TARGETLIBPYTHON) checkgenromfs
	@genromfs -f $@ -d $(TARGETMODULES) -V "ROMFS_Test" || { echo "genromfs failed" ; exit 1 ; }

romfs_cpython_modules.h : romfs_cpython_modules.img
	@xxd -i $< | sed -e "s/^unsigned/static const unsigned/g" >$@ || { echo "xxd of $< failed" ; exit 1 ; }

context:: $(CPYTHON_UNPACKNAME)

depend:: romfs_cpython_modules.h

distclean::
	$(call DELDIR, $(BUILDIR))
	$(call DELDIR, $(INSTALLDIR))
	$(call DELDIR, $(CPYTHON_UNPACKNAME))
	$(call DELFILE, $(CPYTHON_ZIP))
	$(call DELFILE, romfs_cpython_modules.img)
	$(call DELFILE, romfs_cpython_modules.h)
	$(call DELFILE, config.site)
	$(call DELFILE, Setup.local)

include $(APPDIR)/Application.mk
