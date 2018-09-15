############################################################################
# apps/examples/elf/tests/Makefile
#
#   Copyright (C) 2012, 2014, 2018 Gregory Nutt. All rights reserved.
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

ALL_SUBDIRS = errno hello helloxx longjmp mutex pthread signal task struct
BUILD_SUBDIRS = errno hello struct

ifeq ($(CONFIG_HAVE_CXX),y)
BUILD_SUBDIRS += helloxx
endif

ifeq ($(CONFIG_EXAMPLES_ELF_LONGJMP),y)
BUILD_SUBDIRS += longjmp
endif

ifneq ($(CONFIG_DISABLE_PTHREAD),y)
BUILD_SUBDIRS += mutex pthread
endif

ifneq ($(CONFIG_DISABLE_SIGNALS),y)
BUILD_SUBDIRS += signal
endif

ifneq ($(CONFIG_ARCH_ADDRENV),y)
BUILD_SUBDIRS += task
endif

ELF_DIR = $(APPDIR)/examples/elf
TESTS_DIR = $(ELF_DIR)/tests
DIRLIST_HDR = $(TESTS_DIR)/dirlist.h
SYMTAB_SRC = $(TESTS_DIR)/symtab.c

ifeq ($(CONFIG_EXAMPLES_ELF_ROMFS),y)
  FSIMG_SUBDIR = romfs
  FSIMG_DIR = $(TESTS_DIR)/$(FSIMG_SUBDIR)
  ROMFS_IMG = $(TESTS_DIR)/romfs.img
  FSIMG_HDR = $(TESTS_DIR)/romfs.h
else
  NXTOOLDIR = $(TOPDIR)/tools
  GENCROMFSSRC = gencromfs.c
  GENCROMFSEXE = gencromfs$(EXEEXT)

  FSIMG_SUBDIR = cromfs
  FSIMG_DIR = $(TESTS_DIR)/$(FSIMG_SUBDIR)
  FSIMG_HDR = $(TESTS_DIR)/cromfs.h
endif

define DIR_template
$(1)_$(2):
	$(Q) $(MAKE) -C $(1) $(3) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" FSIMG_DIR="$(FSIMG_DIR)" CROSSDEV=$(CROSSDEV)
endef

all: $(FSIMG_HDR) $(DIRLIST_HDR) $(SYMTAB_SRC)
.PHONY: all build clean install populate
.PRECIOUS: ../../../libapps$(LIBEXT)

$(foreach DIR, $(BUILD_SUBDIRS), $(eval $(call DIR_template,$(DIR),build, all)))
$(foreach DIR, $(ALL_SUBDIRS), $(eval $(call DIR_template,$(DIR),clean,clean)))
$(foreach DIR, $(BUILD_SUBDIRS), $(eval $(call DIR_template,$(DIR),install,install)))

# Build program(s) in each sud-directory

build: $(foreach DIR, $(BUILD_SUBDIRS), $(DIR)_build)

# Install each program in the file system image directory

install: $(foreach DIR, $(BUILD_SUBDIRS), $(DIR)_install)
ifneq ($(STRIP),)
ifneq ($(CONFIG_EXAMPLES_ELF_NOSTRIP),y)
	$(Q) $(STRIP) $(wildcard $(FSIMG_SUBDIR)/*)
endif
endif

# Create the file system image directory

$(FSIMG_DIR):
	$(Q) mkdir $(FSIMG_DIR)

# Populate the file system image directory

populate:  $(FSIMG_DIR) build install

ifeq ($(CONFIG_EXAMPLES_ELF_ROMFS),y)
# Create the romfs.img file from the populated romfs directory

$(ROMFS_IMG): populate
	$(Q) genromfs -f $@ -d $(FSIMG_DIR) -V "ELFTEST"

# Create the romfs.h header file from the romfs.img file

$(FSIMG_HDR) : $(ROMFS_IMG)
	$(Q) (cd $(TESTS_DIR); xxd -i romfs.img | sed -e "s/^unsigned/static const unsigned/g" >$@)

else
# Make sure that the NuttX gencromfs tool has been built

$(NXTOOLDIR)/$(GENCROMFSEXE): $(NXTOOLDIR)/$(GENCROMFSSRC)
	$(Q) $(MAKE) -C $(NXTOOLDIR) -f Makefile.host $(GENCROMFSEXE)

# Create the cromfs.h header file from the populated cromfs directory

$(FSIMG_HDR) : populate $(NXTOOLDIR)/$(GENCROMFSEXE)
	$(Q) $(NXTOOLDIR)/$(GENCROMFSEXE) $(FSIMG_DIR) $(FSIMG_HDR)

endif

# Create the dirlist.h header file from the file system image directory

$(DIRLIST_HDR) : populate
	$(Q) $(TESTS_DIR)/mkdirlist.sh $(FSIMG_DIR) >$@

# Create the exported symbol table

$(SYMTAB_SRC): build
	$(Q) $(TESTS_DIR)/mksymtab.sh $(FSIMG_DIR) >$@

# Clean each subdirectory

clean: $(foreach DIR, $(ALL_SUBDIRS), $(DIR)_clean)
	$(Q) rm -f $(FSIMG_HDR) $(DIRLIST_HDR) $(ROMFS_IMG) $(SYMTAB_SRC)
	$(Q) rm -rf $(FSIMG_DIR)
