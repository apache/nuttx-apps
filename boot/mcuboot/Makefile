############################################################################
# apps/boot/mcuboot/Makefile
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

MCUBOOT_VERSION := $(patsubst "%",%,$(CONFIG_MCUBOOT_VERSION))
MCUBOOT_TARBALL  = $(MCUBOOT_VERSION).tar.gz
MCUBOOT_UNPACK   = mcuboot
MCUBOOT_SRCDIR   = $(MCUBOOT_UNPACK)$(DELIM)boot$(DELIM)bootutil$(DELIM)src

DEPPATH += --dep-path $(MCUBOOT_UNPACK)$(DELIM)src
DEPPATH += --dep-path $(MCUBOOT_SRCDIR)
VPATH   += :$(MCUBOOT_UNPACK)$(DELIM)src
VPATH   += :$(MCUBOOT_SRCDIR)

ifneq ($(CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE),)
MAINSRC   = mcuboot_agent_main.c

PROGNAME  = mcuboot_agent
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
else ifneq ($(CONFIG_MCUBOOT_SLOT_CONFIRM_EXAMPLE),)
MAINSRC   = mcuboot_confirm_main.c

PROGNAME  = mcuboot_confirm
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
else ifneq ($(CONFIG_MCUBOOT_BOOTLOADER),)
MAINSRC   = mcuboot/boot/nuttx/main.c

PROGNAME  = mcuboot_loader
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
endif

CFLAGS += -Wno-undef

CSRCS := mcuboot/boot/bootutil/src/boot_record.c \
         mcuboot/boot/bootutil/src/bootutil_misc.c \
         mcuboot/boot/bootutil/src/bootutil_public.c \
         mcuboot/boot/bootutil/src/caps.c \
         mcuboot/boot/bootutil/src/encrypted.c \
         mcuboot/boot/bootutil/src/fault_injection_hardening.c \
         mcuboot/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c \
         mcuboot/boot/bootutil/src/image_ec.c \
         mcuboot/boot/bootutil/src/image_ec256.c \
         mcuboot/boot/bootutil/src/image_ed25519.c \
         mcuboot/boot/bootutil/src/image_rsa.c \
         mcuboot/boot/bootutil/src/image_validate.c \
         mcuboot/boot/bootutil/src/loader.c \
         mcuboot/boot/bootutil/src/swap_misc.c \
         mcuboot/boot/bootutil/src/swap_move.c \
         mcuboot/boot/bootutil/src/swap_scratch.c \
         mcuboot/boot/bootutil/src/tlv.c \
         mcuboot/boot/nuttx/src/flash_map_backend/flash_map_backend.c

ifneq ($(CONFIG_MCUBOOT_WATCHDOG),)
CSRCS += mcuboot/boot/nuttx/src/watchdog/watchdog.c
endif

ifneq ($(CONFIG_MCUBOOT_USE_TINYCRYPT),)
CSRCS += mcuboot/ext/tinycrypt/lib/source/aes_encrypt.c \
         mcuboot/ext/tinycrypt/lib/source/aes_decrypt.c \
         mcuboot/ext/tinycrypt/lib/source/ctr_mode.c \
         mcuboot/ext/tinycrypt/lib/source/hmac.c \
         mcuboot/ext/tinycrypt/lib/source/ecc_dh.c \
         mcuboot/ext/tinycrypt/lib/source/sha256.c \
         mcuboot/ext/tinycrypt/lib/source/utils.c
endif

$(MCUBOOT_TARBALL):
	$(Q) echo "Downloading MCUboot-$(MCUBOOT_VERSION)"
	$(Q) curl -O -L https://github.com/mcu-tools/mcuboot/archive/$(MCUBOOT_TARBALL)

$(MCUBOOT_UNPACK): $(MCUBOOT_TARBALL)
	$(Q) echo "Unpacking: $(MCUBOOT_TARBALL) -> $(MCUBOOT_UNPACK)"
	$(Q) tar zxf $(MCUBOOT_TARBALL)
	$(Q) mv mcuboot-$(MCUBOOT_VERSION) $(MCUBOOT_UNPACK)
	$(Q) touch $(MCUBOOT_UNPACK)

context:: $(MCUBOOT_UNPACK)

distclean::
	$(call DELFILE, $(MCUBOOT_TARBALL))
	$(call DELDIR, $(MCUBOOT_UNPACK))

include $(APPDIR)/Application.mk
