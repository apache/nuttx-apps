############################################################################
# apps/system/adb/Makefile
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

ADB_URL ?= "https://github.com/spiriou/microADB/archive"
ADB_VERSION ?= master

ADB_DIR := $(APPDIR)/system/adb
ADB_UNPACKNAME := microADB
ADB_UNPACKDIR := $(ADB_DIR)/$(ADB_UNPACKNAME)

$(ADB_UNPACKDIR):
	@echo "Downloading: $(ADB_UNPACKNAME)"
	$(Q) curl -O -L $(ADB_URL)/$(ADB_VERSION).zip
	$(Q) unzip -o $(ADB_VERSION).zip
	$(call DELFILE, $(ADB_VERSION).zip)
	$(call MOVEFILE, $(ADB_UNPACKNAME)-$(ADB_VERSION), $(ADB_UNPACKDIR))

# adb server app

PROGNAME  := $(CONFIG_ADBD_PROGNAME)
PRIORITY  := $(CONFIG_ADBD_PRIORITY)
STACKSIZE := $(CONFIG_ADBD_STACKSIZE)
MODULE    := $(CONFIG_SYSTEM_ADBD)

# Files

MAINSRC := adb_main.c
CSRCS += adb_banner.c
CSRCS += $(ADB_UNPACKNAME)/adb_client.c
CSRCS += $(ADB_UNPACKNAME)/adb_frame.c

CSRCS += $(ADB_UNPACKNAME)/hal/hal_uv.c
CSRCS += $(ADB_UNPACKNAME)/hal/hal_uv_packet.c

CFLAGS += -I$(ADB_UNPACKNAME)

ifeq ($(CONFIG_ADBD_TCP_SERVER),y)
CSRCS += $(ADB_UNPACKNAME)/hal/hal_uv_client_tcp.c
endif

ifeq ($(CONFIG_ADBD_USB_SERVER),y)
CSRCS += $(ADB_UNPACKNAME)/hal/hal_uv_client_usb.c
endif

ifeq ($(CONFIG_ADBD_QEMU_SERVER),y)
CSRCS += $(ADB_UNPACKNAME)/hal/hal_uv_client_qemu.c
endif

ifeq ($(CONFIG_ADBD_AUTHENTICATION),y)
CSRCS += $(ADB_UNPACKNAME)/adb_auth_key.c
endif

ifeq ($(CONFIG_ADBD_FILE_SERVICE),y)
CSRCS += $(ADB_UNPACKNAME)/file_sync_service.c
endif

ifeq ($(CONFIG_ADBD_LOGCAT_SERVICE),y)
CSRCS += logcat_service.c
endif

ifeq ($(CONFIG_ADBD_SHELL_SERVICE),y)
CSRCS += $(ADB_UNPACKNAME)/hal/shell_service_uv.c
endif

ifeq ($(CONFIG_ADBD_SOCKET_SERVICE),y)
CSRCS += $(ADB_UNPACKNAME)/tcp_service.c
CSRCS += $(ADB_UNPACKNAME)/hal/hal_uv_socket.c
endif

clean::
	$(call DELFILE, $(OBJS))

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(ADB_UNPACKDIR)/.git),)
context:: $(ADB_UNPACKDIR)

distclean::
	$(call DELDIR, $(ADB_UNPACKDIR))
endif

include $(APPDIR)/Application.mk
