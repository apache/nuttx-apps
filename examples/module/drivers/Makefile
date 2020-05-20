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

include $(APPDIR)/Make.defs

ALL_SUBDIRS = chardev
BUILD_SUBDIRS = chardev
FSIMG_SUBDIR = fsroot

MODULE_DIR = $(APPDIR)/examples/module
DRIVER_DIR = $(MODULE_DIR)/drivers
FSROOT_DIR = $(DRIVER_DIR)/fsroot
SYMTAB_SRC = $(DRIVER_DIR)/mod_symtab.c

ifneq ($(CONFIG_BUILD_FLAT),y)
  PASS1_SYMTAB = $(TOPDIR)/pass1/mod_symtab.c
endif

ifeq ($(CONFIG_EXAMPLES_MODULE_BUILTINFS),y)
ifeq ($(CONFIG_EXAMPLES_MODULE_ROMFS),y)
  ROMFS_IMG = $(DRIVER_DIR)/romfs.img
  FSIMG_SRC = $(DRIVER_DIR)/romfs.c
else ifeq ($(CONFIG_EXAMPLES_MODULE_CROMFS),y)
  NXTOOLDIR = $(TOPDIR)/tools
  GENCROMFSSRC = gencromfs.c
  GENCROMFSEXE = gencromfs$(HOSTEXEEXT)

  FSIMG_SRC = $(DRIVER_DIR)/cromfs.c
endif
endif

define DIR_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) $(2) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" FSROOT_DIR="$(FSROOT_DIR)" CROSSDEV=$(CROSSDEV)
endef

all: $(FSIMG_SRC) $(SYMTAB_SRC) $(PASS1_SYMTAB)
.PHONY: all clean install

$(foreach DIR, $(ALL_SUBDIRS), $(eval $(call DIR_template,$(DIR),clean)))
$(foreach DIR, $(BUILD_SUBDIRS), $(eval $(call DIR_template,$(DIR),install)))

# Install each program in the fsroot directory

install: $(foreach DIR, $(BUILD_SUBDIRS), $(DIR)_install)

ifeq ($(CONFIG_EXAMPLES_MODULE_BUILTINFS),y)
ifeq ($(CONFIG_EXAMPLES_MODULE_ROMFS),y)
# Create the romfs.img file from the populated fsroot directory

$(ROMFS_IMG): install
	$(Q) genromfs -f $@.tmp -d $(FSROOT_DIR) -V "MODULETEST"
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Create the romfs.c file from the romfs.img file

$(FSIMG_SRC): $(ROMFS_IMG)
	$(Q) (cd $(DRIVER_DIR); xxd -i romfs.img | sed -e "s/^unsigned/const unsigned/g" >$@)

else ifeq ($(CONFIG_EXAMPLES_MODULE_CROMFS),y)
# Make sure that the NuttX gencromfs tool has been built

$(NXTOOLDIR)/$(GENCROMFSEXE): $(NXTOOLDIR)/$(GENCROMFSSRC)
	$(Q) $(MAKE) -C $(NXTOOLDIR) -f Makefile.host $(GENCROMFSEXE)

# Create the cromfs.c file from the populated cromfs directory

$(FSIMG_SRC): install $(NXTOOLDIR)/$(GENCROMFSEXE)
	$(Q) $(NXTOOLDIR)/$(GENCROMFSEXE) $(FSIMG_DIR) $@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

endif
endif

# Create the exported symbol table

$(SYMTAB_SRC): install
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(FSROOT_DIR) g_mod >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Copy the symbol table into the kernel pass1/ build directory

ifneq ($(CONFIG_BUILD_FLAT),y)
$(PASS1_SYMTAB): $(SYMTAB_SRC)
	$(Q) install -m 0644 $(SYMTAB_SRC) $(PASS1_SYMTAB)
endif

# Clean each subdirectory

clean: $(foreach DIR, $(ALL_SUBDIRS), $(DIR)_clean)
	$(Q) rm -f $(FSIMG_SRC) $(ROMFS_IMG) $(SYMTAB_SRC)
	$(Q) rm -rf $(FSROOT_DIR)
