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

ifneq ($(CONFIG_MCUBOOT_BOOTLOADER),)
MAINSRC   += $(MCUBOOT_UNPACK)/boot/nuttx/main.c

PROGNAME  += mcuboot_loader
PRIORITY  += SCHED_PRIORITY_DEFAULT
STACKSIZE += $(CONFIG_DEFAULT_TASK_STACKSIZE)
endif

CFLAGS += -Wno-undef

CSRCS := $(MCUBOOT_UNPACK)/boot/bootutil/src/boot_record.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/bootutil_misc.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/bootutil_public.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/caps.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/encrypted.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/fault_injection_hardening.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/image_ec.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/image_ec256.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/image_ed25519.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/image_rsa.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/image_validate.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/loader.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/swap_misc.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/swap_move.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/swap_scratch.c \
         $(MCUBOOT_UNPACK)/boot/bootutil/src/tlv.c \
         $(MCUBOOT_UNPACK)/boot/nuttx/src/flash_map_backend/flash_map_backend.c

ifneq ($(CONFIG_MCUBOOT_WATCHDOG),)
CSRCS += $(MCUBOOT_UNPACK)/boot/nuttx/src/watchdog/watchdog.c
endif

ifneq ($(CONFIG_MCUBOOT_USE_TINYCRYPT),)
CSRCS += $(MCUBOOT_UNPACK)/ext/tinycrypt/lib/source/aes_encrypt.c \
         $(MCUBOOT_UNPACK)/ext/tinycrypt/lib/source/aes_decrypt.c \
         $(MCUBOOT_UNPACK)/ext/tinycrypt/lib/source/ctr_mode.c \
         $(MCUBOOT_UNPACK)/ext/tinycrypt/lib/source/hmac.c \
         $(MCUBOOT_UNPACK)/ext/tinycrypt/lib/source/ecc_dh.c \
         $(MCUBOOT_UNPACK)/ext/tinycrypt/lib/source/sha256.c \
         $(MCUBOOT_UNPACK)/ext/tinycrypt/lib/source/utils.c
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
