############################################################################
# apps/examples/module/drivers/Makefile
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
