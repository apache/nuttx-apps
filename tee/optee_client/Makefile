############################################################################
# apps/tee/optee_client/Makefile
#
# SPDX-License-Identifier: Apache-2.0
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

OPTEE_CLIENT_VERSION = $(patsubst "%",%,$(strip $(CONFIG_OPTEE_CLIENT_VERSION)))
OPTEE_CLIENT_URL ?= "https://github.com/OP-TEE/optee_client/archive/refs/tags"
OPTEE_CLIENT_ZIP = $(OPTEE_CLIENT_VERSION).zip
OPTEE_CLIENT_UNPACKNAME = optee_client
UNPACK ?= unzip -q -o

DEPPATH += --dep-path $(OPTEE_CLIENT_UNPACKNAME)/libteec/src
VPATH += $(OPTEE_CLIENT_UNPACKNAME)/libteec/src

CSRCS += tee_client_api.c
CSRCS += teec_trace.c

ifneq ($(CONFIG_OPTEE_SUPPLICANT),)
	DEPPATH += --dep-path $(OPTEE_CLIENT_UNPACKNAME)/tee-supplicant/src
	VPATH += $(OPTEE_CLIENT_UNPACKNAME)/tee-supplicant/src

	PROGNAME  += ${CONFIG_OPTEE_SUPPLICANT_PROGNAME}
	PRIORITY  += $(CONFIG_OPTEE_SUPPLICANT_PRIORITY)
	STACKSIZE += $(CONFIG_OPTEE_SUPPLICANT_STACKSIZE)
	MODULE    += $(CONFIG_OPTEE_SUPPLICANT)
	MAINSRC   += tee_supplicant.c

	CSRCS += handle.c
	CSRCS += hmac_sha2.c
	CSRCS += sha2.c
	CSRCS += sd_notify.c
	CSRCS += teec_ta_load.c
	CSRCS += tee_supp_fs.c
	CSRCS += rpmb.c

	ifeq ($(CONFIG_GP_SOCKETS),1)
		CSRCS += tee_socket.c
		FLAGS += ${DEFINE_PREFIX}CFG_GP_SOCKETS=1
	endif

	FLAGS += ${INCDIR_PREFIX}optee_client/tee-supplicant/src
	FLAGS += ${DEFINE_PREFIX}TEEC_LOAD_PATH=\"/bin\"
	FLAGS += ${DEFINE_PREFIX}TEE_FS_PARENT_PATH=\"/data/tee\"
	FLAGS += ${DEFINE_PREFIX}DEBUGLEVEL_$(CONFIG_OPTEE_SUPPLICANT_LOG_LEVEL)
endif

AFLAGS += $(FLAGS)
CFLAGS += $(FLAGS)
CXXFLAGS += $(FLAGS)

$(OPTEE_CLIENT_ZIP):
	@echo "Downloading: $(OPTEE_CLIENT_URL)/$(OPTEE_CLIENT_ZIP)"
	$(Q) $(call DOWNLOAD,$(OPTEE_CLIENT_URL),$(OPTEE_CLIENT_ZIP))

$(OPTEE_CLIENT_UNPACKNAME): $(OPTEE_CLIENT_ZIP)
	@echo "Unpacking: $(OPTEE_CLIENT_ZIP) -> $(OPTEE_CLIENT_UNPACKNAME)"
	$(Q) $(UNPACK) $(OPTEE_CLIENT_ZIP)
	$(Q) mv $(OPTEE_CLIENT_UNPACKNAME)-$(OPTEE_CLIENT_VERSION) $(OPTEE_CLIENT_UNPACKNAME)
	$(Q) echo "Patching $(OPTEE_CLIENT_UNPACKNAME)"
	$(Q) patch -p1 -d $(OPTEE_CLIENT_UNPACKNAME) < 0001-libteec-NuttX.patch
	$(Q) patch -p1 -d $(OPTEE_CLIENT_UNPACKNAME) < 0002-tee-supplicant-port-to-nuttx.patch
	$(Q) touch $(OPTEE_CLIENT_UNPACKNAME)

ifeq ($(wildcard $(OPTEE_CLIENT_UNPACKNAME)/.git),)
context:: $(OPTEE_CLIENT_UNPACKNAME)

distclean::
	$(call DELDIR, $(OPTEE_CLIENT_UNPACKNAME))
	$(call DELFILE, $(OPTEE_CLIENT_ZIP))
endif

include $(APPDIR)/Application.mk
