############################################################################
# apps/testing/crypto/psa-arch-test/Makefile
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

PROGNAME  = $(CONFIG_PSA_ARCH_TESTS_PROGNAME)
PRIORITY  = $(CONFIG_PSA_ARCH_TESTS_PRIORITY)
STACKSIZE = $(CONFIG_PSA_ARCH_TESTS_STACKSIZE)
MODULE    = $(CONFIG_PSA_ARCH_TESTS)

PSA_ARCH_TESTS_URL ?= "https://github.com/ARM-software/psa-arch-tests/archive"

PSA_ARCH_TESTS_UNPACKNAME  = psa-arch-tests
PSA_ARCH_TESTS_PACKNAME    = psa-arch-tests-main
PSA_ARCH_TESTS_ZIP         = psa-arch-tests-main.zip
UNPACK                    ?= unzip -q -o
PSA_API_DIR = $(PSA_ARCH_TESTS_UNPACKNAME)$(DELIM)api-tests

$(PSA_ARCH_TESTS_ZIP):
	@echo "Downloading: $(PSA_ARCH_TESTS_ZIP)"
	$(Q) $(call DOWNLOAD,${PSA_ARCH_TESTS_URL},${PSA_ARCH_TESTS_ZIP})

$(PSA_ARCH_TESTS_UNPACKNAME): $(PSA_ARCH_TESTS_ZIP)
	@echo "Unpacking: $(PSA_ARCH_TESTS_ZIP) -> $(PSA_ARCH_TESTS_UNPACKNAME)"
	${Q} $(UNPACK) $(PSA_ARCH_TESTS_ZIP)
	${Q} mv $(PSA_ARCH_TESTS_PACKNAME) $(PSA_ARCH_TESTS_UNPACKNAME)
	$(Q) touch $(PSA_ARCH_TESTS_UNPACKNAME)

MAINSRC   = ${PSA_API_DIR}/platform/targets/tgt_dev_apis_linux/nspe/main.c
CSRCS    += $(shell find psa-arch-tests/api-tests/val/nspe -name "*.c")
CSRCS    += $(shell find psa-arch-tests/api-tests/val/common -name "*.c")
CSRCS    += ${PSA_API_DIR}/platform/targets/tgt_dev_apis_linux/nspe/pal_driver_intf.c
CSRCS    += ${PSA_API_DIR}/platform/targets/common/nspe/pal_weak.c
CFLAGS   += ${INCDIR_PREFIX}${PSA_API_DIR}/val/common
CFLAGS   += ${INCDIR_PREFIX}${PSA_API_DIR}/val/nspe
CFLAGS   += ${INCDIR_PREFIX}${PSA_API_DIR}/platform/targets/common/nspe
CFLAGS   += ${INCDIR_PREFIX}${PSA_API_DIR}/dev_apis/crypto/common
CFLAGS   += ${INCDIR_PREFIX}${PSA_API_DIR}/dev_apis/storage/common
CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/crypto/mbedtls/mbedtls/include
CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/crypto/mbedtls/mbedtls/library
CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/testing/crypto/psa-arch-tests/include
CFLAGS   += -Dmd5_hash=psa_md5_hash
CFLAGS   += -Div=psa_iv -Dplaintext=psa_plaintext
CFLAGS   += -Wno-undef -Wno-shadow -Wno-unused-variable -Wno-misleading-indentation -Wno-uninitialized

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(PSA_ARCH_TESTS_UNPACKNAME)/.git),)
context:: $(PSA_ARCH_TESTS_UNPACKNAME)

distclean::
    $(call DELDIR, $(PSA_ARCH_TESTS_UNPACKNAME))
    $(call DELFILE, $(PSA_ARCH_TESTS_ZIP))
endif

ifeq ($(CONFIG_PSA_ARCH_CRYPTO_TESTS),y)
CSRCS  += $(shell find psa-arch-tests/api-tests/dev_apis/crypto  -name "*.c")
CSRCS  += $(shell find psa-arch-tests/api-tests/platform/targets/common/nspe/crypto -name "*.c")
CSRCS  += $(APPDIR)/testing/crypto/psa-arch-tests/src/pal_crypto.c
CFLAGS += ${INCDIR_PREFIX}${PSA_API_DIR}/platform/targets/common/nspe/crypto
CFLAGS += -DCRYPTO
# testcase 078 conflicts with psa crypto api
CSRCS  := $(filter-out psa-arch-tests/api-tests/dev_apis/crypto/test_c078/test_c078.c, $(CSRCS))
endif

ifeq ($(CONFIG_PSA_ARCH_ITS_TESTS),y)
CSRCS  += $(shell find psa-arch-tests/api-tests/dev_apis/storage -name "*.c")
CSRCS  += $(shell find psa-arch-tests/api-tests/platform/targets/common/nspe/internal_trusted_storage -name "*.c")
CFLAGS += -DINTERNAL_TRUSTED_STORAGE
CSRCS  := $(filter-out psa-arch-tests/api-tests/dev_apis/storage/test_s016/test_s016.c, $(CSRCS))
CSRCS  := $(filter-out psa-arch-tests/api-tests/dev_apis/storage/test_s017/test_s017.c, $(CSRCS))
endif

include $(APPDIR)/Application.mk
