############################################################################
# apps/examples/elf/Makefile
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

# ELF Example

ifeq ($(CONFIG_EXAMPLES_ELF_ROMFS),y)
CSRCS = romfs.c
endif
ifeq ($(CONFIG_EXAMPLES_ELF_CROMFS),y)
CSRCS = cromfs.c
endif
CSRCS += dirlist.c
CSRCS += symtab.c
MAINSRC = elf_main.c

PROGNAME = elf
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_ELF)

DEPPATH := --dep-path tests

# Build targets

VPATH += :tests

ifeq ($(CONFIG_EXAMPLES_ELF_ROMFS),y)
tests/romfs.c: build
endif
ifeq ($(CONFIG_EXAMPLES_ELF_CROMFS),y)
tests/cromfs.c: build
endif
tests/dirlist.c: build
tests/symtab.c: build

.PHONY: build
build:
	+$(Q) $(MAKE) -C tests TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV)

clean::
	+$(Q) $(MAKE) -C tests TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV) clean

include $(APPDIR)/Application.mk
