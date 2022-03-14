############################################################################
# apps/system/adb/Makefile
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

ADBD_URL ?= "https://github.com/spiriou/microADB/archive"
ADBD_VERSION ?= 5b3c9229e70f74fedfbbf1a0b9000836c9a34789

ADB_DIR := $(APPDIR)/system/adb
ADB_UNPACKNAME := microADB
ADB_UNPACKDIR := $(ADB_DIR)/$(ADB_UNPACKNAME)

$(ADB_UNPACKDIR):
	@echo "Downloading: $(ADB_UNPACKNAME)"
	$(Q) curl -O -L $(ADBD_URL)/$(ADBD_VERSION).zip
	$(Q) unzip -o $(ADBD_VERSION).zip
	$(call DELFILE, $(ADBD_VERSION).zip)
	$(call MOVEFILE, $(ADB_UNPACKNAME)-$(ADBD_VERSION), $(ADB_UNPACKDIR))

# adb server app

PROGNAME  := $(CONFIG_ADBD_PROGNAME)
PRIORITY  := $(CONFIG_ADBD_PRIORITY)
STACKSIZE := $(CONFIG_ADBD_STACKSIZE)
MODULE    := $(CONFIG_ADB_SERVER)

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

context:: $(ADB_UNPACKDIR)

clean::
	$(call DELFILE, $(OBJS))

distclean::
	$(call DELDIR, $(ADB_UNPACKDIR))

include $(APPDIR)/Application.mk
