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

ADB_DIR := $(APPDIR)/system/adb
CONFIG_ADBD_URL ?= "https://github.com/spiriou/microADB.git"
CONFIG_ADBD_VERSION ?= b7025c67b866925d1e64c016a844a6a3392557a4

ADB_UNPACKNAME := microADB
ADB_UNPACKDIR := $(ADB_DIR)/$(ADB_UNPACKNAME)

$(ADB_UNPACKDIR):
	@echo "Downloading: $(ADB_UNPACKNAME)"
	$(call DELDIR, "$@")
	$(Q) mkdir "$@"
	$(Q) cd "$@" && git init && \
	git remote add origin "$(CONFIG_ADBD_URL)" && \
	git fetch origin $(CONFIG_ADBD_VERSION) --depth=1 && \
	git reset --hard FETCH_HEAD

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

CFLAGS += -D__NUTTX__=1
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
CSRCS += shell_service.c
CSRCS += shell_pipe.c
endif

context:: $(ADB_UNPACKDIR)

clean::
	$(call DELFILE, $(OBJS))

distclean::
	$(call DELDIR, $(ADB_UNPACKDIR))

include $(APPDIR)/Application.mk
