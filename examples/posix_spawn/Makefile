############################################################################
# apps/examples/posix_spawn/Makefile
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

CSRCS = romfs.c
CSRCS += symtab.c
filesystem$(DELIM)symtab.c_CFLAGS = -fno-builtin
filesystem$(DELIM)symtab.c_CELFFLAGS = -fno-builtin
MAINSRC = spawn_main.c

PROGNAME = posix_spawn
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_POSIXSPAWN)

DEPPATH	+= --dep-path filesystem

# Build targets

VPATH += filesystem

filesystem/romfs.c: build
filesystem/symtab.c: build

.PHONY: build
build:
	+$(Q) $(MAKE) -C filesystem TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV)

clean::
	+$(Q) $(MAKE) -C filesystem TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV) clean

include $(APPDIR)/Application.mk
