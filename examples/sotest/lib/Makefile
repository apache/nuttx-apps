############################################################################
# apps/examples/sotest/lib/Makefile
#
#   Copyright (C) 2015, 2017 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
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

-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs
include $(APPDIR)/Make.defs

ALL_SUBDIRS = sotest
BUILD_SUBDIRS = sotest

ifneq ($(CONFIG_MODLIB_MAXDEPEND),0)
ALL_SUBDIRS += modprint
BUILD_SUBDIRS += modprint
endif

SOTEST_DIR = $(APPDIR)/examples/sotest
LIB_DIR = $(SOTEST_DIR)/lib
FSROOT_DIR = $(LIB_DIR)/fsroot
ROMFS_IMG = $(LIB_DIR)/romfs.img
ROMFS_HDR = $(LIB_DIR)/romfs.h
SYMTAB_SRC = $(LIB_DIR)/sot_symtab.c

define DIR_template
$(1)_$(2):
	$(Q) $(MAKE) -C $(1) $(3) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" FSROOT_DIR="$(FSROOT_DIR)" CROSSDEV=$(CROSSDEV)
endef

all: $(ROMFS_HDR) $(DIRLIST_HDR) $(SYMTAB_SRC)
.PHONY: all build clean install populate
.PRECIOUS: ../../../libapps$(LIBEXT)

$(foreach DIR, $(BUILD_SUBDIRS), $(eval $(call DIR_template,$(DIR),build, all)))
$(foreach DIR, $(ALL_SUBDIRS), $(eval $(call DIR_template,$(DIR),clean,clean)))
$(foreach DIR, $(BUILD_SUBDIRS), $(eval $(call DIR_template,$(DIR),install,install)))

# Build program(s) in each sud-directory

build: $(foreach DIR, $(BUILD_SUBDIRS), $(DIR)_build)

# Install each program in the fsroot directory

install: $(foreach DIR, $(BUILD_SUBDIRS), $(DIR)_install)

# Create the fsroot directory

$(FSROOT_DIR):
	$(Q) mkdir $(FSROOT_DIR)

# Populate the fsroot directory

populate:  $(FSROOT_DIR) build install

# Create the romfs.img file from the populated fsroot directory

$(ROMFS_IMG): populate
ifeq ($(CONFIG_EXAMPLES_SOTEST_BUILTINFS),y)
	$(Q) genromfs -f $@ -d $(FSROOT_DIR) -V "SOTESTTEST"
endif

# Create the romfs.h header file from the romfs.img file

$(ROMFS_HDR) : $(ROMFS_IMG)
ifeq ($(CONFIG_EXAMPLES_SOTEST_BUILTINFS),y)
	$(Q) (cd $(LIB_DIR); xxd -i romfs.img | sed -e "s/^unsigned/static const unsigned/g" >$@)
endif

# Create the exported symbol table

$(SYMTAB_SRC): build
	$(Q) $(LIB_DIR)/mksymtab.sh $(FSROOT_DIR) >$@

# Clean each subdirectory

clean: $(foreach DIR, $(ALL_SUBDIRS), $(DIR)_clean)
	$(Q) rm -f $(ROMFS_HDR) $(DIRLIST_HDR) $(ROMFS_IMG) $(SYMTAB_SRC)
	$(Q) rm -rf $(FSROOT_DIR)
