############################################################################
# apps/crypto/tinycrypt/Makefile
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

TINYCRYPT_VERSION = $(patsubst "%",%,$(strip $(CONFIG_TINYCRYPT_VERSION)))
TINYCRYPT_URL ?= "https://github.com/intel/tinycrypt/archive/refs/tags/v$(TINYCRYPT_VERSION).zip"

TINYCRYPT_ZIP = v$(TINYCRYPT_VERSION).zip

TINYCRYPT_UNPACKNAME = tinycrypt
UNPACK ?= unzip -q -o

$(TINYCRYPT_ZIP):
	@echo "Downloading: $(TINYCRYPT_URL)/$(TINYCRYPT_ZIP)"
	$(Q) curl -O -L $(TINYCRYPT_URL)

$(TINYCRYPT_UNPACKNAME): $(TINYCRYPT_ZIP)
	@echo "Unpacking: $(TINYCRYPT_ZIP) -> $(TINYCRYPT_UNPACKNAME)"
	$(Q) $(UNPACK) $(TINYCRYPT_ZIP)
	$(Q) mv	tinycrypt-$(TINYCRYPT_VERSION) $(TINYCRYPT_UNPACKNAME)
	$(Q) touch $(TINYCRYPT_UNPACKNAME)
	$(Q) patch -p0 -d $(TINYCRYPT_UNPACKNAME) < 0001-TinyCrypt-test-resolve-compile-error.patch

ifeq ($(wildcard $(TINYCRYPT_UNPACKNAME)/.git),)
context:: $(TINYCRYPT_UNPACKNAME)

distclean::
	$(Q) rm -rf $(TINYCRYPT_UNPACKNAME)
endif

CSRCS += tinycrypt/lib/source/utils.c

ifeq ($(CONFIG_TINYCRYPT_ECC),y)
CSRCS += tinycrypt/lib/source/ecc.c
CSRCS += tinycrypt/lib/source/ecc_platform_specific.c
endif

ifeq ($(CONFIG_TINYCRYPT_ECC_DH),y)
	CSRCS += tinycrypt/lib/source/ecc_dh.c
endif

ifeq ($(CONFIG_TINYCRYPT_ECC_DSA),y)
	CSRCS += tinycrypt/lib/source/ecc_dsa.c
endif

ifeq ($(CONFIG_TINYCRYPT_AES),y)
	CSRCS += tinycrypt/lib/source/aes_decrypt.c
endif

ifeq ($(CONFIG_TINYCRYPT_AES),y)
	CSRCS += tinycrypt/lib/source/aes_encrypt.c
endif

ifeq ($(CONFIG_TINYCRYPT_AES_CBC),y)
	CSRCS += tinycrypt/lib/source/cbc_mode.c
endif

ifeq ($(CONFIG_TINYCRYPT_AES_CTR),y)
	CSRCS += tinycrypt/lib/source/ctr_mode.c
endif

ifeq ($(CONFIG_TINYCRYPT_AES_CCM),y)
	CSRCS += tinycrypt/lib/source/ccm_mode.c
endif

ifeq ($(CONFIG_TINYCRYPT_AES_CMAC),y)
	CSRCS += tinycrypt/lib/source/cmac_mode.c
endif

ifeq ($(CONFIG_TINYCRYPT_SHA256),y)
	CSRCS += tinycrypt/lib/source/sha256.c
endif

ifeq ($(CONFIG_TINYCRYPT_SHA256_HMAC),y)
	CSRCS += tinycrypt/lib/source/hmac.c
endif

ifeq ($(CONFIG_TINYCRYPT_SHA256_HMAC_PRNG),y)
	CSRCS += tinycrypt/lib/source/hmac_prng.c
endif

ifeq ($(CONFIG_TINYCRYPT_CTR_PRNG),y)
	CSRCS += tinycrypt/lib/source/ctr_prng.c
endif

ifeq ($(CONFIG_TINYCRYPT_TEST),y)
	CFLAGS    += ${INCDIR_PREFIX}$(APPDIR)/crypto/tinycrypt/tinycrypt/tests/include
	CFLAGS    +=-Dhex2bin=ltp_hex2bin
	CFLAGS    +=-DENABLE_TESTS
	CSRCS     += tinycrypt/tests/test_ecc_utils.c
	PRIORITY   = $(CONFIG_TINYCRYPT_TEST_PRIORITY)
	STACKSIZE  = $(CONFIG_TINYCRYPT_TEST_STACKSIZE)

	MAINSRC  += tinycrypt/tests/test_aes.c
	MAINSRC  += tinycrypt/tests/test_cbc_mode.c
	MAINSRC  += tinycrypt/tests/test_ccm_mode.c
	MAINSRC  += tinycrypt/tests/test_cmac_mode.c
	MAINSRC  += tinycrypt/tests/test_ctr_mode.c
	MAINSRC  += tinycrypt/tests/test_ctr_prng.c
	MAINSRC  += tinycrypt/tests/test_ecc_dh.c
	MAINSRC  += tinycrypt/tests/test_ecc_dsa.c
	MAINSRC  += tinycrypt/tests/test_hmac.c
	MAINSRC  += tinycrypt/tests/test_hmac_prng.c
	MAINSRC  += tinycrypt/tests/test_sha256.c
	PROGNAME += tinycrypt_test_aes
	PROGNAME += tinycrypt_test_cbc_mode
	PROGNAME += tinycrypt_test_ccm_mode
	PROGNAME += tinycrypt_test_cmac_mode
	PROGNAME += tinycrypt_test_ctr_mode
	PROGNAME += tinycrypt_test_ctr_prng
	PROGNAME += tinycrypt_test_ecc_dh
	PROGNAME += tinycrypt_test_ecc_dsa
	PROGNAME += tinycrypt_test_hmac
	PROGNAME += tinycrypt_test_hmac_prng
	PROGNAME += tinycrypt_test_sha256
endif

include $(APPDIR)/Application.mk
