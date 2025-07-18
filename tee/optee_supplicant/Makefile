############################################################################
# apps/tee/optee_supplicant/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2023 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = ${CONFIG_OPTEE_SUPPLICANT_PROGNAME}
PRIORITY  = $(CONFIG_OPTEE_SUPPLICANT_PRIORITY)
STACKSIZE = $(CONFIG_OPTEE_SUPPLICANT_STACKSIZE)
MODULE    = $(CONFIG_OPTEE_SUPPLICANT)
MAINSRC   = optee_client/tee-supplicant/src/tee_supplicant.c

LIBTEEC_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBTEEC_VERSION)))
OPTEE_SUPPLICANT_VERSION = $(patsubst "%",%,$(strip $(CONFIG_OPTEE_SUPPLICANT_VERSION)))
PATCH_FILE = 0001-tee-supplicant-port-to-nuttx.patch
OPTEE_SUPPLICANT_ZIP = ../libteec/$(LIBTEEC_VERSION).zip
OPTEE_SUPPLICANT_UNPACKNAME = optee_client
UNPACK ?= unzip -q -o

ifeq ($(strip $(OPTEE_SUPPLICANT_VERSION)),$(strip $(LIBTEEC_VERSION)))
  # Versions match, continue.
else
  $(error \
    The supplicant patch version ($(OPTEE_SUPPLICANT_VERSION)) does not match LIBTEEC_VERSION ($(LIBTEEC_VERSION)). \
    Build aborted. Change LIBTEEC_VERSION version to match OPTEE_SUPPLICANT_VERSION. \
  )
endif

CSRCS += optee_client/tee-supplicant/src/handle.c
CSRCS += optee_client/tee-supplicant/src/hmac_sha2.c
CSRCS += optee_client/tee-supplicant/src/sha2.c
CSRCS += optee_client/tee-supplicant/src/sd_notify.c
CSRCS += optee_client/tee-supplicant/src/teec_ta_load.c
CSRCS += optee_client/tee-supplicant/src/tee_supp_fs.c
CSRCS += optee_client/tee-supplicant/src/rpmb.c

ifeq ($(CONFIG_GP_SOCKETS),1)
CSRCS += optee_client/tee-supplicant/src/tee_socket.c
CFLAGS += ${DEFINE_PREFIX}CFG_GP_SOCKETS=1
endif

CFLAGS += ${INCDIR_PREFIX}optee_client/tee-supplicant/src
CFLAGS += ${DEFINE_PREFIX}TEEC_LOAD_PATH=\"/bin\"
CFLAGS += ${DEFINE_PREFIX}TEE_FS_PARENT_PATH=\"/data/tee\"
CFLAGS += ${DEFINE_PREFIX}DEBUGLEVEL_$(CONFIG_OPTEE_SUPPLICANT_LOG_LEVEL)

$(OPTEE_SUPPLICANT_UNPACKNAME): $(OPTEE_SUPPLICANT_ZIP)
	@echo "Unpacking: $(OPTEE_SUPPLICANT_ZIP) -> $(OPTEE_SUPPLICANT_UNPACKNAME)"
	$(Q) $(UNPACK) $(OPTEE_SUPPLICANT_ZIP)
	$(Q) mv $(OPTEE_SUPPLICANT_UNPACKNAME)-$(OPTEE_SUPPLICANT_VERSION) $(OPTEE_SUPPLICANT_UNPACKNAME)
	$(Q) echo "Patching $(OPTEE_SUPPLICANT_UNPACKNAME)"
	$(Q) patch -p1 -d $(OPTEE_SUPPLICANT_UNPACKNAME) < $(PATCH_FILE)
	$(Q) touch $(OPTEE_SUPPLICANT_UNPACKNAME)

ifeq ($(wildcard $(OPTEE_SUPPLICANT_UNPACKNAME)/.git),)
context:: $(OPTEE_SUPPLICANT_UNPACKNAME)

distclean::
	$(Q) rm -rf $(OPTEE_SUPPLICANT_UNPACKNAME)
endif

include $(APPDIR)/Application.mk
