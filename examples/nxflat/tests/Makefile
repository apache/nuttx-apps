############################################################################
# apps/examples/nxflat/tests/Makefile
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

# Most of these do no build yet
#SUBDIRS	= errno hello hello++ longjmp mutex pthread signal task struct
SUBDIRS		= errno hello struct

ifneq ($(CONFIG_DISABLE_PTHREAD),y)
BUILD_SUBDIRS += mutex pthread
endif

ifneq ($(CONFIG_ARCH_ADDRENV),y)
BUILD_SUBDIRS += task
endif

NXFLAT_DIR	= $(APPDIR)/examples/nxflat
TESTS_DIR	= $(NXFLAT_DIR)/tests
ROMFS_DIR	= $(TESTS_DIR)/romfs
ROMFS_IMG	= $(TESTS_DIR)/romfs.img
ROMFS_SRC	= $(TESTS_DIR)/romfs.c
DIRLIST_SRC	= $(TESTS_DIR)/dirlist.c
SYMTAB_SRC	= $(TESTS_DIR)/symtab.c

define DIR_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) $(2) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" ROMFS_DIR="$(ROMFS_DIR)" CROSSDEV=$(CROSSDEV)
endef

all: $(ROMFS_SRC) $(DIRLIST_SRC) $(SYMTAB_SRC)
.PHONY: all clean install

$(foreach DIR, $(SUBDIRS), $(eval $(call DIR_template,$(DIR),clean)))
$(foreach DIR, $(SUBDIRS), $(eval $(call DIR_template,$(DIR),install)))

# Install each program in the romfs directory

install: $(foreach DIR, $(SUBDIRS), $(DIR)_install)

# Create the romfs.img file from the populated romfs directory

$(ROMFS_IMG): install
	$(Q) genromfs -f $@.tmp -d $(ROMFS_DIR) -V "NXFLATTEST"
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Create the romfs.c file from the romfs.img file

$(ROMFS_SRC): $(ROMFS_IMG)
	$(Q) (cd $(TESTS_DIR) && echo "#include <nuttx/compiler.h>" >$@ && \
		xxd -i romfs.img | sed -e "s/^unsigned char/const unsigned char aligned_data(4)/g" >>$@)

# Create the dirlist.c file from the romfs directory

$(DIRLIST_SRC): install
	$(Q) $(TESTS_DIR)/mkdirlist.sh $(ROMFS_DIR) >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Create the exported symbol table list from the derived *-thunk.S files

$(SYMTAB_SRC): install
	$(Q) $(APPDIR)/tools/mksymtab.sh $(TESTS_DIR) g_nxflat >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Clean each subdirectory

clean: $(foreach DIR, $(SUBDIRS), $(DIR)_clean)
	$(Q) rm -f $(ROMFS_SRC) $(DIRLIST_SRC) $(ROMFS_IMG) $(SYMTAB_SRC)
	$(Q) rm -rf $(ROMFS_DIR)
