############################################################################
# apps/examples/sotest/Makefile
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

# Shared library example built-in application info

PROGNAME = sotest
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_SOTEST)

# Shared Library Example

ifeq ($(CONFIG_EXAMPLES_SOTEST_BUILTINFS),y)
CSRCS = romfs.c
endif
CSRCS += sot_symtab.c
lib$(DELIM)sot_symtab.c_CFLAGS = -fno-builtin
lib$(DELIM)sot_symtab.c_CELFFLAGS = -fno-builtin
MAINSRC = sotest_main.c

DEPPATH += --dep-path lib

# Build targets

VPATH = lib

ifeq ($(CONFIG_EXAMPLES_SOTEST_BUILTINFS),y)
lib/romfs.c: build
endif
lib/sot_symtab.c: build

.PHONY: build
build:
	+$(Q) $(MAKE) -C lib TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV)

clean::
	+$(Q) $(MAKE) -C lib TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV) clean

include $(APPDIR)/Application.mk
