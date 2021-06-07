############################################################################
# apps/examples/posix_spawn/filesystem/Makefile
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
