############################################################################
# apps/examples/posix_spawn/filesystem/Makefile
#
#   Copyright (C) 2013 Gregory Nutt. All rights reserved.
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

SPAWN_DIR = $(APPDIR)$(DELIM)examples$(DELIM)posix_spawn
FILESYSTEM_DIR = $(SPAWN_DIR)$(DELIM)filesystem
ROMFS_DIR = $(FILESYSTEM_DIR)$(DELIM)romfs
ROMFS_IMG = $(FILESYSTEM_DIR)$(DELIM)romfs.img
ROMFS_SRC = $(FILESYSTEM_DIR)$(DELIM)romfs.c
SYMTAB_SRC = $(FILESYSTEM_DIR)$(DELIM)symtab.c

all: $(ROMFS_SRC) $(SYMTAB_SRC)
.PHONY: all hello/hello redirect/redirect clean

# Build the hello test program

hello/hello:
	$(Q) $(MAKE) -C hello hello TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" ROMFS_DIR="$(ROMFS_DIR)"

# Build the redirection test program

redirect/redirect:
	$(Q) $(MAKE) -C redirect redirect TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" ROMFS_DIR="$(ROMFS_DIR)"

# Create the romfs.img file from the romfs directory
# REVISIT: ELF binaries should be stripped so that they are not so big

$(ROMFS_IMG): hello/hello redirect/redirect testdata.txt
	$(Q) $(MAKE) -C hello install TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" ROMFS_DIR="$(ROMFS_DIR)"
	$(Q) $(MAKE) -C redirect install TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" ROMFS_DIR="$(ROMFS_DIR)"
	$(Q) install -m 0644 testdata.txt $(ROMFS_DIR)/testdata.txt
	$(Q) genromfs -f $@.tmp -d $(ROMFS_DIR) -V "POSIXSPAWN"
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Create the romfs.c file from the romfs.img file

$(ROMFS_SRC): $(ROMFS_IMG)
	$(Q) (cd $(FILESYSTEM_DIR); xxd -i romfs.img | sed -e "s/^unsigned/const unsigned/g" >$@)

# Create the exported symbol table

$(SYMTAB_SRC): $(ROMFS_IMG)
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(ROMFS_DIR) g_spawn >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# Clean each subdirectory

clean:
	$(Q) $(MAKE) -C hello clean TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" ROMFS_DIR="$(ROMFS_DIR)"
	$(Q) $(MAKE) -C redirect clean TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" ROMFS_DIR="$(ROMFS_DIR)"
	$(Q) rm -f $(ROMFS_SRC) $(ROMFS_IMG) $(SYMTAB_SRC)
	$(Q) rm -rf $(ROMFS_DIR)
