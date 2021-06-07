############################################################################
# apps/Makefile
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

# Symbol table for loadable apps.

SYMTABSRC = symtab_apps.c
SYMTABOBJ = $(SYMTABSRC:.c=$(OBJEXT))

# Build targets

# We first remove libapps.a before letting the other rules add objects to it
# so that we ensure libapps.a does not contain objects from prior build

all:
	$(RM) $(BIN)
	$(MAKE) $(BIN)

.PHONY: import install dirlinks export .depdirs preconfig depend clean distclean
.PHONY: context clean_context context_all register register_all
.PRECIOUS: $(BIN)

$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),all)))
$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),install)))
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

# In the KERNEL build, we must build and install all of the modules.  No
# symbol table is needed

ifeq ($(CONFIG_BUILD_KERNEL),y)

install: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_install)

.import: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_all)
	$(Q) $(MAKE) install

import: $(IMPORT_TOOLS)
	$(Q) $(MAKE) context TOPDIR="$(APPDIR)$(DELIM)import"
	$(Q) $(MAKE) register TOPDIR="$(APPDIR)$(DELIM)import"
	$(Q) $(MAKE) depend TOPDIR="$(APPDIR)$(DELIM)import"
	$(Q) $(MAKE) .import TOPDIR="$(APPDIR)$(DELIM)import"

else

# In FLAT and protected modes, the modules have already been created.  A
# symbol table is required.

ifeq ($(CONFIG_BUILD_LOADABLE),)

$(BIN): $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_all)
	$(Q) for app in ${CONFIGURED_APPS}; do \
		$(MAKE) -C "$${app}" archive ; \
	done

else

$(SYMTABSRC): $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_all)
	$(Q) for app in ${CONFIGURED_APPS}; do \
		$(MAKE) -C "$${app}" archive ; \
	done
	$(Q) $(MAKE) install
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(BINDIR) >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

$(SYMTABOBJ): %$(OBJEXT): %.c
	$(call COMPILE, -fno-lto $<, $@)

$(BIN): $(SYMTABOBJ)
ifeq ($(CONFIG_CYGWIN_WINTOOL),y)
	$(call ARCHIVE_ADD, "${shell cygpath -w $(BIN)}", $^)
else
	$(call ARCHIVE_ADD, $(BIN), $^)
endif

endif # !CONFIG_BUILD_LOADABLE

install: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_install)

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

context:
	$(Q) $(MAKE) context_all
	$(Q) $(MAKE) register_all

Kconfig:
	$(foreach SDIR, $(CONFIGDIRS), $(call MAKE_template,$(SDIR),preconfig))
	$(Q) $(MKKCONFIG)

preconfig: Kconfig

export:
ifneq ($(EXPORTDIR),)
ifneq ($(BUILTIN_REGISTRY),)
	$(Q) mkdir -p "${EXPORTDIR}"/registry || exit 1; \
	for f in "${BUILTIN_REGISTRY}"/*.bdat "${BUILTIN_REGISTRY}"/*.pdat ; do \
		[ -f "$${f}" ] && cp -f "$${f}" "${EXPORTDIR}"/registry ; \
	done
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
	$(call CLEAN)

distclean: $(foreach SDIR, $(CLEANDIRS), $(SDIR)_distclean)
ifeq ($(CONFIG_WINDOWS_NATIVE),y)
	$(Q) ( if exist  external ( \
		echo ********************************************************" \
		echo * The external directory/link must be removed manually *" \
		echo ********************************************************" \
	)
else
	$(Q) ( if [ -e external ]; then \
		echo "********************************************************"; \
		echo "* The external directory/link must be removed manually *"; \
		echo "********************************************************"; \
		fi; \
	)
endif
	$(call DELFILE, .depend)
	$(call DELFILE, $(SYMTABSRC))
	$(call DELFILE, $(SYMTABOBJ))
	$(call DELFILE, $(BIN))
	$(call DELFILE, Kconfig)
	$(call DELDIR, $(BINDIR))
	$(call CLEAN)
