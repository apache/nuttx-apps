############################################################################
# apps/system/fdt/Makefile
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
############################################################################

include $(APPDIR)/Make.defs

# Utility Tools

CSRCS = util.c

ifeq ($(CONFIG_SYSTEM_FDTDUMP),y)
  MAINSRC += fdtdump.c
  PROGNAME += fdtdump
  STACKSIZE += $(CONFIG_SYSTEM_FDTDUMP_STACKSIZE)
  PRIORITY += $(CONFIG_SYSTEM_FDTDUMP_PRIORITY)
endif

ifeq ($(CONFIG_SYSTEM_FDTGET),y)
  MAINSRC += fdtget.c
  PROGNAME += fdtget
  STACKSIZE += $(CONFIG_SYSTEM_FDTGET_STACKSIZE)
  PRIORITY += $(CONFIG_SYSTEM_FDTGET_PRIORITY)
endif

ifeq ($(CONFIG_SYSTEM_FDTPUT),y)
  MAINSRC += fdtput.c
  PROGNAME += fdtput
  STACKSIZE += $(CONFIG_SYSTEM_FDTPUT_STACKSIZE)
  PRIORITY += $(CONFIG_SYSTEM_FDTPUT_PRIORITY)
endif

ifeq ($(CONFIG_SYSTEM_FDTOVERLAY),y)
  MAINSRC += fdtoverlay.c
  PROGNAME += fdtoverlay
  STACKSIZE += $(CONFIG_SYSTEM_FDTOVERLAY_STACKSIZE)
  PRIORITY += $(CONFIG_SYSTEM_FDTOVERLAY_PRIORITY)
endif

CFLAGS += ${INCDIR_PREFIX}$(TOPDIR)/libs/libc/fdt/
CFLAGS += ${INCDIR_PREFIX}$(TOPDIR)/libs/libc/fdt/dtc/libfdt/

VPATH += $(TOPDIR)/libs/libc/fdt/dtc/
DEPPATH += --dep-path $(TOPDIR)/libs/libc/fdt/dtc/

include $(APPDIR)/Application.mk
