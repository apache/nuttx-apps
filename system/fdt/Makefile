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

ifneq ($(CONFIG_SYSTEM_FDTDUMP),)
  MAINSRC += fdtdump.c
  MODULE += $(CONFIG_SYSTEM_FDTDUMP)
  PROGNAME += fdtdump
  STACKSIZE += $(CONFIG_SYSTEM_FDTDUMP_STACKSIZE)
  PRIORITY += $(CONFIG_SYSTEM_FDTDUMP_PRIORITY)
endif

ifneq ($(CONFIG_SYSTEM_FDTGET),)
  MAINSRC += fdtget.c
  MODULE += $(CONFIG_SYSTEM_FDTGET)
  PROGNAME += fdtget
  STACKSIZE += $(CONFIG_SYSTEM_FDTGET_STACKSIZE)
  PRIORITY += $(CONFIG_SYSTEM_FDTGET_PRIORITY)
endif

ifneq ($(CONFIG_SYSTEM_FDTPUT),)
  MAINSRC += fdtput.c
  MODULE += $(CONFIG_SYSTEM_FDTPUT)
  PROGNAME += fdtput
  STACKSIZE += $(CONFIG_SYSTEM_FDTPUT_STACKSIZE)
  PRIORITY += $(CONFIG_SYSTEM_FDTPUT_PRIORITY)
endif

ifneq ($(CONFIG_SYSTEM_FDTOVERLAY),)
  MAINSRC += fdtoverlay.c
  MODULE += $(CONFIG_SYSTEM_FDTOVERLAY)
  PROGNAME += fdtoverlay
  STACKSIZE += $(CONFIG_SYSTEM_FDTOVERLAY_STACKSIZE)
  PRIORITY += $(CONFIG_SYSTEM_FDTOVERLAY_PRIORITY)
endif

CFLAGS += ${INCDIR_PREFIX}$(TOPDIR)/libs/libc/fdt/
CFLAGS += ${INCDIR_PREFIX}$(TOPDIR)/libs/libc/fdt/dtc/libfdt/

VPATH += $(TOPDIR)/libs/libc/fdt/dtc/
DEPPATH += --dep-path $(TOPDIR)/libs/libc/fdt/dtc/

include $(APPDIR)/Application.mk
