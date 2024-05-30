############################################################################
# apps/examples/sotest/lib/Makefile
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

ALL_SUBDIRS = sotest
BUILD_SUBDIRS = sotest

ifneq ($(CONFIG_MODLIB_MAXDEPEND),0)
ALL_SUBDIRS += modprint
BUILD_SUBDIRS += modprint
endif

SOTEST_DIR = $(APPDIR)/examples/sotest
LIB_DIR = $(SOTEST_DIR)/lib
FSROOT_DIR = $(LIB_DIR)/fsroot
SYMTAB_SRC = $(LIB_DIR)/sot_symtab.c

ifeq ($(CONFIG_EXAMPLES_SOTEST_BUILTINFS),y)
  ROMFS_IMG = $(LIB_DIR)/romfs.img
  ROMFS_SRC = $(LIB_DIR)/romfs.c
endif

define DIR_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) $(2) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" FSROOT_DIR="$(FSROOT_DIR)" CROSSDEV=$(CROSSDEV)
endef

all: $(ROMFS_SRC) $(SYMTAB_SRC)
.PHONY: all clean install

$(foreach DIR, $(ALL_SUBDIRS), $(eval $(call DIR_template,$(DIR),clean)))
$(foreach DIR, $(BUILD_SUBDIRS), $(eval $(call DIR_template,$(DIR),install)))

# Install each program in the fsroot directory

install: $(foreach DIR, $(BUILD_SUBDIRS), $(DIR)_install)

ifeq ($(CONFIG_EXAMPLES_SOTEST_BUILTINFS),y)
# Create the romfs.img file from the populated fsroot directory

$(ROMFS_IMG): install
	$(Q) genromfs -f $@.tmp -d $(FSROOT_DIR) -V "SOTESTTEST"
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Create the romfs.c file from the romfs.img file

$(ROMFS_SRC): $(ROMFS_IMG)
	$(Q) (cd $(LIB_DIR) && echo "#include <nuttx/compiler.h>" >$@ && \
		xxd -i romfs.img | sed -e "s/^unsigned char/const unsigned char aligned_data(4)/g" >>$@)
endif

# Create the exported symbol table

$(SYMTAB_SRC): install
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(FSROOT_DIR) g_sot | sed "/modprint/d" >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Clean each subdirectory

clean: $(foreach DIR, $(ALL_SUBDIRS), $(DIR)_clean)
	$(Q) rm -f $(ROMFS_SRC) $(ROMFS_IMG) $(SYMTAB_SRC)
	$(Q) rm -rf $(FSROOT_DIR)
