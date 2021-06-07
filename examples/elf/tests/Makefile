############################################################################
# apps/examples/elf/tests/Makefile
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

ALL_SUBDIRS = errno hello helloxx longjmp mutex pthread signal task struct
BUILD_SUBDIRS = errno hello struct signal

ifeq ($(CONFIG_HAVE_CXX),y)
BUILD_SUBDIRS += helloxx
endif

ifeq ($(CONFIG_EXAMPLES_ELF_LONGJMP),y)
BUILD_SUBDIRS += longjmp
endif

ifneq ($(CONFIG_DISABLE_PTHREAD),y)
BUILD_SUBDIRS += mutex pthread
endif

ifneq ($(CONFIG_ARCH_ADDRENV),y)
BUILD_SUBDIRS += task
endif

ELF_DIR = $(APPDIR)/examples/elf
TESTS_DIR = $(ELF_DIR)/tests
DIRLIST_SRC = $(TESTS_DIR)/dirlist.c
SYMTAB_SRC = $(TESTS_DIR)/symtab.c

ifeq ($(CONFIG_EXAMPLES_ELF_ROMFS),y)
  FSIMG_SUBDIR = romfs
  FSIMG_DIR = $(TESTS_DIR)/$(FSIMG_SUBDIR)
  ROMFS_IMG = $(TESTS_DIR)/romfs.img
  FSIMG_SRC = $(TESTS_DIR)/romfs.c
else
  NXTOOLDIR = $(TOPDIR)/tools
  GENCROMFSSRC = gencromfs.c
  GENCROMFSEXE = gencromfs$(HOSTEXEEXT)

  FSIMG_SUBDIR = cromfs
  FSIMG_DIR = $(TESTS_DIR)/$(FSIMG_SUBDIR)
  FSIMG_SRC = $(TESTS_DIR)/cromfs.c
endif

define DIR_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) $(2) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" FSIMG_DIR="$(FSIMG_DIR)" CROSSDEV=$(CROSSDEV)
endef

all: $(FSIMG_SRC) $(DIRLIST_SRC) $(SYMTAB_SRC)
.PHONY: all clean install

$(foreach DIR, $(ALL_SUBDIRS), $(eval $(call DIR_template,$(DIR),clean)))
$(foreach DIR, $(BUILD_SUBDIRS), $(eval $(call DIR_template,$(DIR),install)))

# Install each program in the file system image directory

install: $(foreach DIR, $(BUILD_SUBDIRS), $(DIR)_install)

ifeq ($(CONFIG_EXAMPLES_ELF_ROMFS),y)
# Create the romfs.img file from the populated romfs directory

$(ROMFS_IMG): install
	$(Q) genromfs -f $@.tmp -d $(FSIMG_DIR) -V "ELFTEST"
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Create the romfs.c file from the romfs.img file

$(FSIMG_SRC): $(ROMFS_IMG)
	$(Q) (cd $(TESTS_DIR); xxd -i romfs.img | sed -e "s/^unsigned/const unsigned/g" >$@)

else
# Make sure that the NuttX gencromfs tool has been built

$(NXTOOLDIR)/$(GENCROMFSEXE): $(NXTOOLDIR)/$(GENCROMFSSRC)
	$(Q) $(MAKE) -C $(NXTOOLDIR) -f Makefile.host $(GENCROMFSEXE)

# Create the cromfs.h header file from the populated cromfs directory

$(FSIMG_SRC): install $(NXTOOLDIR)/$(GENCROMFSEXE)
	$(Q) $(NXTOOLDIR)/$(GENCROMFSEXE) $(FSIMG_DIR) $@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

endif

# Create the dirlist.h header file from the file system image directory

$(DIRLIST_SRC): install
	$(Q) $(TESTS_DIR)/mkdirlist.sh $(FSIMG_DIR) >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Create the exported symbol table

$(SYMTAB_SRC): install
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(FSIMG_DIR) g_elf >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Clean each subdirectory

clean: $(foreach DIR, $(ALL_SUBDIRS), $(DIR)_clean)
	$(Q) rm -f $(FSIMG_SRC) $(DIRLIST_SRC) $(ROMFS_IMG) $(SYMTAB_SRC)
	$(Q) rm -rf $(FSIMG_DIR)
