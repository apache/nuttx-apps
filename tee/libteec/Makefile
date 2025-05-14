############################################################################
# apps/tee/libteec/Makefile
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

LIBTEEC_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBTEEC_VERSION)))
LIBTEEC_URL ?= "https://github.com/OP-TEE/optee_client/archive/refs/tags"
LIBTEEC_ZIP = $(LIBTEEC_VERSION).zip
LIBTEEC_UNPACKNAME = optee_client
UNPACK ?= unzip -q -o

CSRCS += optee_client/libteec/src/tee_client_api.c
CSRCS += optee_client/libteec/src/teec_trace.c

$(LIBTEEC_ZIP):
	@echo "Downloading: $(LIBTEEC_URL)/$(LIBTEEC_ZIP)"
	$(Q) $(call DOWNLOAD,$(LIBTEEC_URL),$(LIBTEEC_ZIP))

$(LIBTEEC_UNPACKNAME): $(LIBTEEC_ZIP)
	@echo "Unpacking: $(LIBTEEC_ZIP) -> $(LIBTEEC_UNPACKNAME)"
	$(Q) $(UNPACK) $(LIBTEEC_ZIP)
	$(Q) mv $(LIBTEEC_UNPACKNAME)-$(LIBTEEC_VERSION) $(LIBTEEC_UNPACKNAME)
	$(Q) echo "Patching $(LIBTEEC_UNPACKNAME)"
	$(Q) patch -p1 -d $(LIBTEEC_UNPACKNAME) < 0001-libteec-NuttX.patch
	$(Q) touch $(LIBTEEC_UNPACKNAME)

ifeq ($(wildcard $(LIBTEEC_UNPACKNAME)/.git),)
context:: $(LIBTEEC_UNPACKNAME)

distclean::
	$(Q) rm -rf $(LIBTEEC_UNPACKNAME)
endif

include $(APPDIR)/Application.mk
