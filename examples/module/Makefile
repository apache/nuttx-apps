############################################################################
# apps/examples/module/Makefile
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

# Module example built-in application info

PROGNAME = module
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_MODULE)

# Module Example

ifeq ($(CONFIG_EXAMPLES_MODULE_ROMFS),y)
CSRCS = romfs.c
endif
ifeq ($(CONFIG_EXAMPLES_MODULE_CROMFS),y)
CSRCS = cromfs.c
endif
CSRCS += mod_symtab.c
drivers$(DELIM)mod_symtab.c_CFLAGS = -fno-builtin
drivers$(DELIM)mod_symtab.c_CELFFLAGS = -fno-builtin
MAINSRC = module_main.c

VPATH += drivers
DEPPATH += --dep-path drivers

# Build targets

ifeq ($(CONFIG_EXAMPLES_MODULE_ROMFS),y)
drivers/romfs.c: build
endif
ifeq ($(CONFIG_EXAMPLES_MODULE_CROMFS),y)
drivers/cromfs.c: build
endif
drivers/mod_symtab.c: build

.PHONY: build
build:
	+$(Q) $(MAKE) -C drivers TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV)

clean::
	+$(Q) $(MAKE) -C drivers TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV) clean

include $(APPDIR)/Application.mk
