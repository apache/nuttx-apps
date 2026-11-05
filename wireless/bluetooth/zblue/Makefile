############################################################################
# apps/wireless/bluetooth/zblue/Makefile
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

ZBLUE_DIR = $(APPDIR)/wireless/bluetooth/zblue

# Zephyr source, downloaded from the release tag

ZEPHYR_REF := $(patsubst "%",%,$(strip $(CONFIG_ZBLUE_REF)))
ZEPHYR_SHA256 := $(patsubst "%",%,$(strip $(CONFIG_ZBLUE_SHA256)))
ZEPHYR_URL ?= https://github.com/zephyrproject-rtos/zephyr/archive
ZEPHYR_TAR = zephyr-$(ZEPHYR_REF).tar.gz
ZEPHYR_UNPACK = zephyr
ZEPHYR = $(ZBLUE_DIR)/$(ZEPHYR_UNPACK)

# Only the parts of the tree used by the Bluetooth host are unpacked

ZEPHYR_PATHS = LICENSE Kconfig.constants include lib/net_buf lib/utils \
               lib/uuid modules/mbedtls scripts/kconfig subsys/bluetooth \
               subsys/crc subsys/logging subsys/settings

$(ZEPHYR_TAR):
	@echo "Downloading: $(ZEPHYR_URL)/$(ZEPHYR_REF).tar.gz"
	$(Q) curl -fL $(ZEPHYR_URL)/$(ZEPHYR_REF).tar.gz -o $(ZEPHYR_TAR)

$(ZEPHYR_UNPACK): $(ZEPHYR_TAR)
	$(Q) $(APPDIR)/tools/check-hash.sh sha256 "$(ZEPHYR_SHA256)" \
	     $(ZEPHYR_TAR)
	@echo "Unpacking: $(ZEPHYR_TAR) -> $(ZEPHYR_UNPACK)"
	$(Q) tar xzf $(ZEPHYR_TAR) \
	     $(addprefix zephyr-$(ZEPHYR_REF:v%=%)/,$(ZEPHYR_PATHS))
	$(Q) mv zephyr-$(ZEPHYR_REF:v%=%) $(ZEPHYR_UNPACK)
	$(Q) touch $(ZEPHYR_UNPACK)

ifeq ($(wildcard $(ZEPHYR_UNPACK)/.git),)
context:: $(ZEPHYR_UNPACK)

distclean::
	$(call DELFILE,$(ZEPHYR_TAR))
	$(call DELDIR,$(ZEPHYR_UNPACK))
endif

# Zephyr configuration: resolved by the Zephyr Kconfig after download

ZBLUE_GEN = $(ZBLUE_DIR)/gen
ZBLUE_CONF := $(patsubst "%",%,$(strip $(CONFIG_ZBLUE_CONF)))
ifeq ($(filter /%,$(ZBLUE_CONF)),)
ZBLUE_CONF := $(ZBLUE_DIR)/$(ZBLUE_CONF)
endif

$(ZBLUE_GEN)/zblue.config: $(ZBLUE_CONF) $(TOPDIR)/.config \
                           Kconfig.zephyr tools/zkconfig.py $(ZEPHYR_UNPACK)
	$(Q) python3 tools/zkconfig.py --zephyr $(ZEPHYR) \
	     --kconfig Kconfig.zephyr --nuttx-config $(TOPDIR)/.config \
	     --outdir $(ZBLUE_GEN) $(ZBLUE_CONF)
	$(Q) python3 tools/gen_syscalls.py $(ZEPHYR)/include \
	     $(ZBLUE_GEN)/zephyr/syscalls

context:: $(ZBLUE_GEN)/zblue.config

distclean::
	$(call DELDIR,$(ZBLUE_GEN))

-include $(ZBLUE_GEN)/zblue.config

# Compiler flags for the Zephyr sources and the port

ZBLUE_FLAGS  = ${INCDIR_PREFIX}$(ZBLUE_DIR)/port/include
ZBLUE_FLAGS += ${INCDIR_PREFIX}$(ZBLUE_GEN)
ZBLUE_FLAGS += ${INCDIR_PREFIX}$(ZEPHYR)/include
ZBLUE_FLAGS += ${INCDIR_PREFIX}$(ZEPHYR)/subsys/bluetooth
ZBLUE_FLAGS += -imacros $(ZBLUE_GEN)/zephyr/autoconf.h
ZBLUE_FLAGS += -include $(ZBLUE_DIR)/port/include/zblue_prefix.h
ZBLUE_FLAGS += -D__ZEPHYR__=1 -std=gnu11
ZBLUE_FLAGS += -Wno-undef -Wno-shadow -Wno-unused-but-set-variable

# The host relies on the compiler removing code under IS_ENABLED(n)

ifeq ($(CONFIG_DEBUG_NOOPT),y)
ZBLUE_FLAGS += -Og
endif

CFLAGS += $(ZBLUE_FLAGS)

# Porting layer

ZSRCS += $(wildcard port/src/*.c)

# Upstream sources, selected as in the upstream CMakeLists.txt files

BT = $(ZEPHYR_UNPACK)/subsys/bluetooth

ZSRCS += $(BT)/common/addr.c $(BT)/common/dummy.c $(BT)/common/bt_str.c
ZSRCS += $(BT)/common/hci_pkt.c $(BT)/common/hci_lockstep.c
ifeq ($(ZCONFIG_BT_HCI_SET_PUBLIC_ADDR),y)
ZSRCS += $(BT)/common/hci_public_addr.c
endif
ifeq ($(ZCONFIG_BT_RPA),y)
ZSRCS += $(BT)/common/rpa.c
endif
ifeq ($(ZCONFIG_BT_LONG_WQ),y)
ZSRCS += $(BT)/common/long_wq.c
endif

ifeq ($(ZCONFIG_BT_HCI_HOST),y)
ZSRCS += $(BT)/host/uuid.c $(BT)/host/addr.c $(BT)/host/buf.c
ZSRCS += $(BT)/host/data.c $(BT)/host/hci_core.c $(BT)/host/hci_common.c
ZSRCS += $(BT)/host/id.c
ifeq ($(ZCONFIG_BT_BROADCASTER),y)
ZSRCS += $(BT)/host/adv.c
endif
ifeq ($(ZCONFIG_BT_OBSERVER),y)
ZSRCS += $(BT)/host/scan.c
endif
ifeq ($(ZCONFIG_BT_HOST_CRYPTO),y)
ZSRCS += $(BT)/host/crypto_psa.c
endif
ifeq ($(ZCONFIG_BT_ECC),y)
ZSRCS += $(BT)/host/ecc.c
endif
ifeq ($(ZCONFIG_BT_CONN),y)
ZSRCS += $(BT)/host/conn.c $(BT)/host/l2cap.c $(BT)/host/att.c
ZSRCS += $(BT)/host/gatt.c
ifeq ($(ZCONFIG_BT_GATT_GAP_SVC_VALIDATE),y)
ZSRCS += $(BT)/host/gatt_gap_svc_validate.c
endif
ifeq ($(ZCONFIG_BT_SMP),y)
ZSRCS += $(BT)/host/smp.c $(BT)/host/keys.c
else
ZSRCS += $(BT)/host/smp_null.c
endif
endif
endif

ifeq ($(ZCONFIG_BT_HOST_CCM),y)
ZSRCS += $(BT)/host/aes_ccm.c
endif

ifeq ($(ZCONFIG_BT_CRYPTO),y)
ZSRCS += $(BT)/crypto/bt_crypto.c $(BT)/crypto/bt_crypto_psa.c
endif

ifeq ($(ZCONFIG_BT_GAP_SVC),y)
ZSRCS += $(BT)/services/gap_svc.c
endif

ifeq ($(ZCONFIG_NET_BUF),y)
ZSRCS += $(ZEPHYR_UNPACK)/lib/net_buf/buf.c
ZSRCS += $(ZEPHYR_UNPACK)/lib/net_buf/buf_simple.c
endif

ZSRCS += $(ZEPHYR_UNPACK)/subsys/crc/crc8_sw.c
ZSRCS += $(ZEPHYR_UNPACK)/subsys/crc/crc16_sw.c
ZSRCS += $(ZEPHYR_UNPACK)/lib/utils/hex.c
ifeq ($(ZCONFIG_UUID),y)
ZSRCS += $(ZEPHYR_UNPACK)/lib/uuid/uuid.c
endif

# The zblue objects are linked into one relocatable object.  Some of them
# (e.g. services/gap_svc.c) only add entries to an iterable section and
# have no symbol referenced from elsewhere, so they would not be pulled out
# of libapps.a.

ZOBJDIR = $(ZBLUE_GEN)/obj
ZOBJS = $(patsubst %.c,$(ZOBJDIR)/%.o,$(ZSRCS))

$(ZOBJDIR)/%.o: %.c $(ZBLUE_GEN)/zblue.config
	$(Q) mkdir -p $(dir $@)
	$(call COMPILE, $<, $@)

$(ZBLUE_GEN)/zblue.o: $(ZOBJS)
	@echo "LD: zblue.o"
	$(Q) $(LD) -r -o $@ $(ZOBJS)

EXTOBJS += $(ZBLUE_GEN)/zblue.o

clean::
	$(call DELDIR,$(ZOBJDIR))
	$(call DELFILE,$(ZBLUE_GEN)/zblue.o)

include $(APPDIR)/Application.mk
