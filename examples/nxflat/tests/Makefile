############################################################################
# apps/examples/nxflat/tests/Makefile
#
#   Copyright (C) 2009, 2011, 2014 Gregory Nutt. All rights reserved.
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
	$(Q) (cd $(TESTS_DIR); xxd -i romfs.img | sed -e "s/^unsigned/const unsigned/g" >$@)

# Create the dirlist.c file from the romfs directory

$(DIRLIST_SRC): install
	$(Q) $(TESTS_DIR)/mkdirlist.sh $(ROMFS_DIR) >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Create the exported symbol table list from the derived *-thunk.S files

$(SYMTAB_SRC): install
	$(Q) $(APPDIR)/tools/mksymtab.sh $(ROMFS_DIR) g_nxflat >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Clean each subdirectory

clean: $(foreach DIR, $(SUBDIRS), $(DIR)_clean)
	$(Q) rm -f $(ROMFS_SRC) $(DIRLIST_SRC) $(ROMFS_IMG) $(SYMTAB_SRC)
	$(Q) rm -rf $(ROMFS_DIR)
