############################################################################
# apps/examples/module/drivers/Makefile
#
#   Copyright (C) 2015, 2017-2018 Gregory Nutt. All rights reserved.
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

ALL_SUBDIRS = chardev
BUILD_SUBDIRS = chardev
FSIMG_SUBDIR = fsroot

MODULE_DIR = $(APPDIR)/examples/module
DRIVER_DIR = $(MODULE_DIR)/drivers
FSROOT_DIR = $(DRIVER_DIR)/fsroot
SYMTAB_SRC = $(DRIVER_DIR)/mod_symtab.c
PASS1_SYMTAB = $(TOPDIR)/pass1/mod_symtab.c

ifeq ($(CONFIG_EXAMPLES_MODULE_ROMFS),y)
  ROMFS_IMG = $(DRIVER_DIR)/romfs.img
  FSIMG_HDR = $(DRIVER_DIR)/romfs.h
else ifeq ($(CONFIG_EXAMPLES_MODULE_CROMFS),y)
  NXTOOLDIR = $(TOPDIR)/tools
  GENCROMFSSRC = gencromfs.c
  GENCROMFSEXE = gencromfs$(EXEEXT)

  FSIMG_HDR = $(DRIVER_DIR)/cromfs.h
endif

define DIR_template
$(1)_$(2):
	$(Q) $(MAKE) -C $(1) $(3) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" FSROOT_DIR="$(FSROOT_DIR)" CROSSDEV=$(CROSSDEV)
endef

all: $(FSIMG_HDR) $(DIRLIST_HDR) $(SYMTAB_SRC) $(PASS1_SYMTAB)
.PHONY: all build clean install populate
.PRECIOUS: ../../../libapps$(LIBEXT)

$(foreach DIR, $(BUILD_SUBDIRS), $(eval $(call DIR_template,$(DIR),build, all)))
$(foreach DIR, $(ALL_SUBDIRS), $(eval $(call DIR_template,$(DIR),clean,clean)))
$(foreach DIR, $(BUILD_SUBDIRS), $(eval $(call DIR_template,$(DIR),install,install)))

# Build program(s) in each sud-directory

build: $(foreach DIR, $(BUILD_SUBDIRS), $(DIR)_build)

# Install each program in the fsroot directory

install: $(foreach DIR, $(BUILD_SUBDIRS), $(DIR)_install)
ifneq ($(STRIP),)
ifneq ($(CONFIG_EXAMPLES_MODULE_NOSTRIP),y)
	$(Q) $(STRIP) $(wildcard $(FSIMG_SUBDIR)$(DELIM)*)
endif
endif

# Create the fsroot directory

$(FSROOT_DIR):
	$(Q) mkdir $(FSROOT_DIR)

# Populate the fsroot directory

populate:  $(FSROOT_DIR) build install

ifeq ($(CONFIG_EXAMPLES_MODULE_BUILTINFS),y)
ifeq ($(CONFIG_EXAMPLES_MODULE_ROMFS),y)
# Create the romfs.img file from the populated fsroot directory

$(ROMFS_IMG): populate
ifeq ($(CONFIG_EXAMPLES_MODULE_BUILTINFS),y)
	$(Q) genromfs -f $@ -d $(FSROOT_DIR) -V "MODULETEST"
endif

# Create the romfs.h header file from the romfs.img file

$(FSIMG_HDR) : $(ROMFS_IMG)
ifeq ($(CONFIG_EXAMPLES_MODULE_BUILTINFS),y)
	$(Q) (cd $(DRIVER_DIR); xxd -i romfs.img | sed -e "s/^unsigned/static const unsigned/g" >$@)
endif

else ifeq ($(CONFIG_EXAMPLES_MODULE_CROMFS),y)
# Make sure that the NuttX gencromfs tool has been built

$(NXTOOLDIR)/$(GENCROMFSEXE): $(NXTOOLDIR)/$(GENCROMFSSRC)
       $(Q) $(MAKE) -C $(NXTOOLDIR) -f Makefile.host $(GENCROMFSEXE)

# Create the cromfs.h header file from the populated cromfs directory

$(FSIMG_HDR) : populate $(NXTOOLDIR)/$(GENCROMFSEXE)
       $(Q) $(NXTOOLDIR)/$(GENCROMFSEXE) $(FSIMG_DIR) $(FSIMG_HDR)

endif

# Create the exported symbol table

$(SYMTAB_SRC): build
	$(Q) $(DRIVER_DIR)/mksymtab.sh $(FSROOT_DIR) >$@

else

# Create the exported symbol table

$(SYMTAB_SRC): build populate
	$(Q) $(DRIVER_DIR)/mksymtab.sh $(FSROOT_DIR) >$@

endif

# Copy the symbol table into the kernel pass1/ build directory

$(PASS1_SYMTAB): $(SYMTAB_SRC)
ifneq ($(CONFIG_BUILD_FLAT),y)
	$(Q) install -m 0644 $(SYMTAB_SRC) $(PASS1_SYMTAB)
endif

# Clean each subdirectory

clean: $(foreach DIR, $(ALL_SUBDIRS), $(DIR)_clean)
	$(Q) rm -f $(FSIMG_HDR) $(DIRLIST_HDR) $(ROMFS_IMG) $(SYMTAB_SRC)
	$(Q) rm -rf $(FSROOT_DIR)
