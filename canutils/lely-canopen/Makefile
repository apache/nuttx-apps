############################################################################
# apps/canutils/lely-canopen/Makefile
#
# SPDX-License-Identifier: Apache-2.0
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
###########################################################################

# Standard includes

include $(APPDIR)/Make.defs

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

LELYCANOPEN_VERSION = $(patsubst "%",%,$(strip $(CONFIG_CANUTILS_LELYCANOPEN_VERSION)))

LELYCANOPEN_TARBALL = lely-core-$(LELYCANOPEN_VERSION).tar.gz
LELYCANOPEN_UNPACKNAME = lely-core-master-$(LELYCANOPEN_VERSION)
LELYCANOPEN_SRCNAME = lely-core
UNPACK ?= tar -zxf
LELYCANOPEN_SRCDIR = $(WD)/$(LELYCANOPEN_SRCNAME)

# CAN network object

CSRCS += $(LELYCANOPEN_SRCDIR)/src/can/buf.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/can/msg.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/can/vci.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/can/net.c

# CANopen library

CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/crc.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/dev.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/nmt.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/nmt_hb.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/nmt_srv.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/obj.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/pdo.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/sdo.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/ssdo.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/type.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/val.c

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_TIME),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/time.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_CSDO),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/csdo.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_DCF),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/dcf.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_EMCY),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/emcy.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_GW),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/gw.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_GW_TXT),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/gw_txt.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_LSS),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/lss.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_NMTBOOT),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/nmt_boot.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_NMTCFG),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/nmt_cfg.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_RPDO),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/rpdo.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_SDEV),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/sdev.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_SYNC),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/sync.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_TPDO),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/tpdo.c
endif

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_WTM),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/co/wtm.c
endif

# utils

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_DIAG),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/diag.c
endif

CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/bits.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/bitset.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/cmp.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/config.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/config_ini.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/dllist.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/endian.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/errnum.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/frbuf.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/fwbuf.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/lex.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/membuf.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/pheap.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/print.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/rbtree.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/stop.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/time.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/util/ustring.c

# Lely IO lib

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_IOLIB),y)
CSRCS += $(LELYCANOPEN_SRCDIR)/src/io/handle.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/io/io.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/io/pipe.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/io/poll.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/io2/posix/poll.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/io2/ctx.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/io/can.c
CSRCS += $(LELYCANOPEN_SRCDIR)/src/can/socket.c
endif

# enable config.h

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/include/canutils/lely
CFLAGS += -DHAVE_CONFIG_H=1

CFLAGS += -Wno-shadow -Wno-undef

MODULE = $(CONFIG_CANUTILS_LELYCANOPEN)

# Lely CANopen tools

ifeq ($(CONFIG_CANUTILS_LELYCANOPEN_TOOLS_COCTL),y)
PROGNAME   = coctl
PRIORITY   = $(CONFIG_CANUTILS_LELYCANOPEN_TOOLS_COCTL_PRIORITY)
STACKSIZE  = $(CONFIG_CANUTILS_LELYCANOPEN_TOOLS_COCTL_STACKSIZE)
MAINSRC    = $(LELYCANOPEN_SRCDIR)/tools/coctl.c
endif

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LELYCANOPEN_SRCNAME)/.git),)
$(LELYCANOPEN_TARBALL):
	@echo "Downloading: $(LELYCANOPEN_TARBALL)"
	$(Q) curl -L -O $(CONFIG_CANUTILS_LELYCANOPEN_URL)/$(LELYCANOPEN_TARBALL)

$(LELYCANOPEN_SRCNAME): $(LELYCANOPEN_TARBALL)
	@echo "Unpacking: $(LELYCANOPEN_TARBALL) -> $(LELYCANOPEN_UNPACKNAME)"
	$(Q) $(UNPACK) $(LELYCANOPEN_TARBALL)

patch_src: $(LELYCANOPEN_SRCNAME)
	# Get the name of the directory created by the tar command
	$(eval LELYCANOPEN_UNPACKNAME := $(shell ls -d lely-core-master*))
	$(Q) mv $(LELYCANOPEN_UNPACKNAME) $(LELYCANOPEN_SRCNAME)
	$(Q) cat 0001-tools-eliminate-multiple-definitions-of-poll-compile.patch | patch -s -N -d $(LELYCANOPEN_SRCNAME) -p1
	$(Q) cat 0002-tools-coctl.c-fix-printf-issues.patch | patch -s -N -d $(LELYCANOPEN_SRCNAME) -p1
	$(Q) cat 0003-src-co-nmt.c-fix-compilation.patch | patch -s -N -d $(LELYCANOPEN_SRCNAME) -p1
	$(Q) cat 0004-tools-coctl.c-add-missing-mutex-init.patch | patch -s -N -d $(LELYCANOPEN_SRCNAME) -p1
	$(Q) cat 0005-add-NuttX-support.patch | patch -s -N -d $(LELYCANOPEN_SRCNAME) -p1
	$(Q) echo "Patching $(LELYCANOPEN_SRCNAME)"

context:: patch_src
else
context:: $(LELYCANOPEN_SRCNAME)
endif


distclean::
ifeq ($(wildcard $(LELYCANOPEN_SRCNAME)/.git),)
	$(call DELDIR, $(LELYCANOPEN_SRCNAME))
	$(call DELFILE, $(LELYCANOPEN_TARBALL))
endif

include $(APPDIR)/Application.mk
