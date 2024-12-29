############################################################################
# apps/Makefile
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

export APPDIR = $(CURDIR)
include $(APPDIR)/Make.defs
include $(APPDIR)/tools/Wasm.mk

# The GNU make CURDIR will always be a POSIX-like path with forward slashes
# as path segment separators.  This is fine for the above inclusions but
# will cause problems later for the native build.  If we know that this is
# a native build, then we need to fix up the APPDIR path for subsequent
# use

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  export APPDIR = $(subst /,\,$(CURDIR))
endif

# Symbol table for loadable apps.
#   SYMTABEXT: Extra arguments for mksymtab.sh

SYMTABSRC = symtab_apps.c
SYMTABOBJ = $(SYMTABSRC:.c=$(OBJEXT))

# Build targets

# We first remove libapps.a before letting the other rules add objects to it
# so that we ensure libapps.a does not contain objects from prior build

all: $(BIN)

.PHONY: import install dirlinks export .depdirs preconfig depend clean distclean
.PHONY: context postinstall clean_context context_all postinstall_all register register_all
.PRECIOUS: $(BIN)

$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),all)))
$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),install)))
$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),postinstall)))
$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),context)))
$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),register)))
$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),depend)))
$(foreach SDIR, $(CLEANDIRS), $(eval $(call SDIR_template,$(SDIR),clean)))
$(foreach SDIR, $(CLEANDIRS), $(eval $(call SDIR_template,$(SDIR),distclean)))

$(MKDEP): $(TOPDIR)/tools/mkdeps.c
	$(HOSTCC) $(HOSTINCLUDES) $(HOSTCFLAGS) $< -o $@

$(INCDIR): $(TOPDIR)/tools/incdir.c
	$(HOSTCC) $(HOSTINCLUDES) $(HOSTCFLAGS) $< -o $@

IMPORT_TOOLS = $(MKDEP) $(INCDIR)

ifeq ($(CONFIG_TOOLS_WASM_BUILD),y)

configure_wasm:
	$(Q) cmake -B$(APPDIR)$(DELIM)tools$(DELIM)Wasm$(DELIM)build \
		$(APPDIR)$(DELIM)tools$(DELIM)Wasm \
		-DAPPDIR=$(APPDIR) -DTOPDIR=$(TOPDIR) \
		-DWASI_SDK_PATH=$(WASI_SDK_PATH) \
		-DKCONFIG_FILE_PATH=$(TOPDIR)$(DELIM).config

context_wasm: configure_wasm
	$(Q) cmake --build $(APPDIR)$(DELIM)tools$(DELIM)Wasm$(DELIM)build

else

context_wasm:

endif


# In the KERNEL build, we must build and install all of the modules.  No
# symbol table is needed

ifeq ($(CONFIG_BUILD_KERNEL),y)

install: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_install)

$(BIN): $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_all)

.import: $(BIN)
	$(Q) install libapps.a $(APPDIR)$(DELIM)import$(DELIM)libs
	$(Q) $(MAKE) install
	$(Q) $(MAKE) postinstall

import: $(IMPORT_TOOLS)
	$(Q) $(MAKE) context TOPDIR="$(APPDIR)$(DELIM)import"
	$(Q) $(MAKE) register TOPDIR="$(APPDIR)$(DELIM)import"
	$(Q) $(MAKE) depend TOPDIR="$(APPDIR)$(DELIM)import"
	$(Q) $(MAKE) .import TOPDIR="$(APPDIR)$(DELIM)import"

else

# In FLAT and protected modes, the modules have already been created.  A
# symbol table is required.

ifeq ($(CONFIG_MODULES),)
ifeq ($(CONFIG_WINDOWS_NATIVE),y)
$(BIN): $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_all)
else
$(BIN): $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_all)
	$(call LINK_WASM)
endif

else

$(SYMTABSRC): $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_all)
	$(Q) $(MAKE) install
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(BINDIR) $(SYMTABEXT) >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

ifneq ($(CONFIG_ARM_TOOLCHAIN_GHS),y)
$(SYMTABOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@, -fno-lto -fno-builtin)
else
$(SYMTABOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@, -Onolink)
endif

$(BIN): $(SYMTABOBJ)
	$(call ARLOCK, $(call CONVERT_PATH,$(BIN)), $^)
	$(call LINK_WASM)

endif # !CONFIG_MODULES

install: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_install)
	$(Q) $(MAKE) postinstall_all

# Link nuttx

HEAD_OBJ += $(wildcard $(APPDIR)$(DELIM)import$(DELIM)startup$(DELIM)*$(OBJEXT))
HEAD_OBJ += $(wildcard $(APPDIR)$(DELIM)builtin$(DELIM)*$(OBJEXT))

.import: $(BIN) install
	$(Q) echo "LD: nuttx"
	$(Q) $(LD) --entry=__start $(LDFLAGS) $(LDLIBPATH) $(EXTRA_LIBPATHS) \
	  -L$(APPDIR)$(DELIM)import$(DELIM)scripts -T$(LDNAME) \
	  -o nuttx$(EXEEXT) $(HEAD_OBJ) $(EXTRA_OBJS) $(LDSTARTGROUP) \
	  $(BIN) $(LDLIBS) $(EXTRA_LIBS) $(LDENDGROUP)
ifeq ($(CONFIG_INTELHEX_BINARY),y)
	$(Q) echo "CP: nuttx.hex"
	$(Q) $(OBJCOPY) $(OBJCOPYARGS) -O ihex nuttx$(EXEEXT) nuttx.hex
endif
ifeq ($(CONFIG_RAW_BINARY),y)
	$(Q) echo "CP: nuttx.bin"
	$(Q) $(OBJCOPY) $(OBJCOPYARGS) -O binary nuttx$(EXEEXT) nuttx.bin
endif
	$(call POSTBUILD, $(APPDIR))

import: $(IMPORT_TOOLS)
	$(Q) $(MAKE) context TOPDIR="$(APPDIR)$(DELIM)import"
	$(Q) $(MAKE) register TOPDIR="$(APPDIR)$(DELIM)import"
	$(Q) $(MAKE) depend TOPDIR="$(APPDIR)$(DELIM)import"
	$(Q) $(MAKE) .import TOPDIR="$(APPDIR)$(DELIM)import"

endif # CONFIG_BUILD_KERNEL

dirlinks:
	$(Q) $(MAKE) -C platform dirlinks

context_all: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_context)
register_all: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_register)
postinstall_all: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_postinstall)

staging:
	$(Q) mkdir -p $@

context: | staging
	$(Q) $(MAKE) context_all
	$(Q) $(MAKE) register_all
	$(Q) $(MAKE) context_wasm

Kconfig:
	$(foreach SDIR, $(CONFIGDIRS), $(call MAKE_template,$(SDIR),preconfig))
	$(Q) $(MKKCONFIG)

preconfig: Kconfig

export:
ifneq ($(EXPORTDIR),)
	$(Q) mkdir -p "${EXPORTDIR}"$(DELIM)registry || exit 1;
ifneq ($(CONFIG_BUILD_KERNEL),y)
ifneq ($(BUILTIN_REGISTRY),)
	for f in "${BUILTIN_REGISTRY}"$(DELIM)*.bdat "${BUILTIN_REGISTRY}"$(DELIM)*.pdat ; do \
		if [ -f "$${f}" ]; then \
			cp -f "$${f}" "${EXPORTDIR}"$(DELIM)registry ; \
		fi \
	done
endif
endif
endif

.depdirs: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_depend)

.depend: Makefile .depdirs
	$(Q) touch $@

depend: .depend

clean_context:
	$(Q) $(MAKE) -C platform clean_context
	$(Q) $(MAKE) -C builtin clean_context

clean: $(foreach SDIR, $(CLEANDIRS), $(SDIR)_clean)
	$(call DELFILE, $(SYMTABSRC))
	$(call DELFILE, $(SYMTABOBJ))
	$(call DELFILE, $(BIN))
	$(call DELFILE, Kconfig)
	$(call DELDIR, $(BINDIR))
	$(call DELDIR, $(BINDIR_DEBUG))
	$(call CLEAN)

distclean: $(foreach SDIR, $(CLEANDIRS), $(SDIR)_distclean)
	$(call DELFILE, *.lock)
	$(call DELFILE, .depend)
	$(call DELFILE, $(SYMTABSRC))
	$(call DELFILE, $(SYMTABOBJ))
	$(call DELFILE, $(BIN))
	$(call DELFILE, Kconfig)
	$(call DELDIR, $(BINDIR))
	$(call DELDIR, staging)
	$(call DELDIR, wasm)
	$(call DELDIR, $(APPDIR)$(DELIM)tools$(DELIM)Wasm$(DELIM)build)
	$(call CLEAN)
