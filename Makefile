############################################################################
# apps/Makefile
#
#   Copyright (C) 2011 Uros Platise. All rights reserved.
#   Copyright (C) 2011-2014, 2018 Gregory Nutt. All rights reserved.
#   Authors: Uros Platise <uros.platise@isotel.eu>
#            Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

APPDIR = ${shell pwd}
TOPDIR ?= $(APPDIR)/import

-include $(TOPDIR)/Make.defs
-include $(APPDIR)/Make.defs

# Application Directories

# BUILDIRS is the list of top-level directories containing Make.defs files
# CLEANDIRS is the list of all top-level directories containing Makefiles.
#   It is used only for cleaning.

BUILDIRS   := $(dir $(filter-out import/Make.defs,$(wildcard */Make.defs)))
CLEANDIRS  := $(dir $(wildcard */Makefile))

# CONFIGURED_APPS is the application directories that should be built in
#   the current configuration.

CONFIGURED_APPS =

define Add_Application
  include $(1)Make.defs
endef

$(foreach BDIR, $(BUILDIRS), $(eval $(call Add_Application,$(BDIR))))

# Library path

LIBPATH ?= $(TOPDIR)$(DELIM)staging

# The install path

EXE_DIR = $(APPDIR)$(DELIM)exe
BIN_DIR = $(EXE_DIR)$(DELIM)system$(DELIM)bin

# The final build target

BIN = libapps$(LIBEXT)

# Symbol table for loadable apps.

SYMTABSRC = $(EXE_DIR)$(DELIM)symtab_apps.c
SYMTABOBJ = $(SYMTABSRC:.c=$(OBJEXT))

# Build targets

all: $(BIN)
.PHONY: import install dirlinks context context_serialize clean_context context_rest .depdirs preconfig depend clean distclean
.PRECIOUS: libapps$(LIBEXT)

define MAKE_template
	$(Q) cd $(1) && $(MAKE) $(2) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" BIN_DIR="$(BIN_DIR)"

endef

define SDIR_template
$(1)_$(2):
	$(Q) cd $(1) && $(MAKE) $(2) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" BIN_DIR="$(BIN_DIR)"

endef

$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),all)))
$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),install)))
$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),context)))
$(foreach SDIR, $(CONFIGURED_APPS), $(eval $(call SDIR_template,$(SDIR),depend)))
$(foreach SDIR, $(CLEANDIRS), $(eval $(call SDIR_template,$(SDIR),clean)))
$(foreach SDIR, $(CLEANDIRS), $(eval $(call SDIR_template,$(SDIR),distclean)))

ifeq ($(CONFIG_BUILD_LOADABLE),)
$(BIN): $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_all)
else
$(SYMTABSRC): $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_all)
	$(Q) $(MAKE) install TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(EXE_DIR)$(DELIM)system $(SYMTABSRC)

$(SYMTABOBJ): %$(OBJEXT): %.c
	$(call COMPILE, -fno-lto $<, $@)

$(BIN): $(SYMTABOBJ)
	$(call ARCHIVE, $(BIN), $^)
endif

.install: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_install)

$(BIN_DIR):
	$(Q) mkdir -p $(BIN_DIR)

install: $(BIN_DIR) .install

.import: $(BIN) install

import:
	$(Q) $(MAKE) .import TOPDIR="$(APPDIR)$(DELIM)import"

dirlinks:
	$(Q) $(MAKE) -C platform dirlinks TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"

context_rest: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_context)

context_serialize:
	$(Q) $(MAKE) -C builtin context TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"
	$(Q) $(MAKE) context_rest

context: context_serialize

Kconfig:
	$(foreach SDIR, $(BUILDIRS), $(call MAKE_template,$(SDIR),preconfig))
	$(Q) $(MKKCONFIG)

preconfig: Kconfig

.depdirs: $(foreach SDIR, $(CONFIGURED_APPS), $(SDIR)_depend)

.depend: context Makefile .depdirs
	$(Q) touch $@

depend: .depend

clean_context:
	$(Q) $(MAKE) -C platform clean_context TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"

clean: $(foreach SDIR, $(CLEANDIRS), $(SDIR)_clean)
	$(call DELFILE, $(SYMTABSRC))
	$(call DELFILE, $(BIN))
	$(call DELFILE, Kconfig)
	$(call DELDIR, $(BIN_DIR))
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
	$(call DELFILE, $(BIN))
	$(call DELFILE, Kconfig)
	$(call DELDIR, $(BIN_DIR))
	$(call CLEAN)
