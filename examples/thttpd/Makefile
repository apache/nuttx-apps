############################################################################
# apps/examples/thttpd/Makefile
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

# THTTPD Web Server Example

CSRCS = romfs.c
MAINSRC = thttpd_main.c

CONTENT_MAKE = $(MAKE) -C content
ifeq ($(CONFIG_THTTPD_BINFS),y)
  CONTENT_MAKE += -f Makefile.binfs
else
  CONTENT_MAKE += -f Makefile.nxflat
  CSRCS += symtab.c
  content$(DELIM)symtab.c_CFLAGS = -fno-builtin
  content$(DELIM)symtab.c_CELFFLAGS = -fno-builtin
endif

PROGNAME = thttp
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_THTTPD)

VPATH += content
DEPPATH += --dep-path content

content/romfs.c: build
ifeq ($(CONFIG_THTTPD_NXFLAT),y)
content/symtab.c: build
endif

.PHONY: build
build:
	+$(Q) $(CONTENT_MAKE) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV)

# Common build

context::
	+$(Q) $(CONTENT_MAKE) context TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV)

clean::
	+$(Q) $(CONTENT_MAKE) clean TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV)

distclean::
	+$(Q) $(CONTENT_MAKE) distclean TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" CROSSDEV=$(CROSSDEV)

include $(APPDIR)/Application.mk
