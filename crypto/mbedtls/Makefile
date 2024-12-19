############################################################################
# apps/crypto/mbedtls/Makefile
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

# Mbed TLS crypto library

# Set up build configuration and environment

MBEDTLS_URL ?= "https://github.com/ARMmbed/mbedtls/archive"

MBEDTLS_VERSION = $(patsubst "%",%,$(strip $(CONFIG_MBEDTLS_VERSION)))
MBEDTLS_ZIP = v$(MBEDTLS_VERSION).zip

MBEDTLS_UNPACKNAME = mbedtls
UNPACK ?= unzip -q -o

MBEDTLS_UNPACKLIBDIR = $(MBEDTLS_UNPACKNAME)$(DELIM)library
MBEDTLS_UNPACKPROGDIR = $(MBEDTLS_UNPACKNAME)$(DELIM)programs

# This lets Mbed TLS better use some of the POSIX features we have
CFLAGS += ${DEFINE_PREFIX}unix

mbedtls/library/bignum.c_CFLAGS += -fno-lto

# Build break on Assemble compiler if -fno-omit-frame-pointer and -O3 enabled at same time
# {standard input}: Assembler messages:
# {standard input}:2560: Error: branch out of range
# make[2]: *** [apps/Application.mk:170: mbedtls/library/sha256.o] Error 1

ifeq ($(CONFIG_FRAME_POINTER),y)
  ifeq ($(CONFIG_DEBUG_OPTLEVEL),"-O3")
    mbedtls/library/sha256.c_CFLAGS += -O2
    mbedtls/library/cmac.c_CFLAGS += -O2
  endif
endif

ifeq ($(CONFIG_ARCH_SIM),y)
  CFLAGS += -O0
endif

CSRCS = $(wildcard $(MBEDTLS_UNPACKLIBDIR)$(DELIM)*.c)

$(MBEDTLS_ZIP):
	@echo "Downloading: $(MBEDTLS_URL)/$(MBEDTLS_ZIP)"
	$(Q) curl -O -L $(MBEDTLS_URL)/$(MBEDTLS_ZIP)

$(MBEDTLS_UNPACKNAME): $(MBEDTLS_ZIP)
	@echo "Unpacking: $(MBEDTLS_ZIP) -> $(MBEDTLS_UNPACKNAME)"
	$(Q) $(UNPACK) $(MBEDTLS_ZIP)
	$(Q) mv	mbedtls-$(MBEDTLS_VERSION) $(MBEDTLS_UNPACKNAME)
	$(Q) patch -p1 -d $(MBEDTLS_UNPACKNAME) < 0001-mbedtls-entropy_poll-use-getrandom-to-get-the-system.patch
	$(Q) patch -p1 -d $(MBEDTLS_UNPACKNAME) < 0002-mbedtls-add-mbedtls-x509-crt-pool.patch
	$(Q) patch -p1 -d $(MBEDTLS_UNPACKNAME) < 0003-Fix-MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT-warning.patch
	$(Q) touch $(MBEDTLS_UNPACKNAME)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(MBEDTLS_UNPACKNAME)/.git),)
context:: $(MBEDTLS_UNPACKNAME)

distclean::
	$(call DELDIR, $(MBEDTLS_UNPACKNAME))
	$(call DELFILE, $(MBEDTLS_ZIP))
endif

# Configuration Applications

ifneq ($(CONFIG_MBEDTLS_APPS),)

MODULE = $(CONFIG_MBEDTLS_APPS)

ifeq ($(CONFIG_MBEDTLS_APP_BENCHMARK),y)

PROGNAME  += $(CONFIG_MBEDTLS_APP_BENCHMARK_PROGNAME)
PRIORITY  += $(CONFIG_MBEDTLS_APP_BENCHMARK_PRIORITY)
STACKSIZE += $(CONFIG_MBEDTLS_APP_BENCHMARK_STACKSIZE)

MAINSRC += $(MBEDTLS_UNPACKPROGDIR)/test/benchmark.c
endif

ifeq ($(CONFIG_MBEDTLS_APP_SELFTEST),y)

PROGNAME  += $(CONFIG_MBEDTLS_APP_SELFTEST_PROGNAME)
PRIORITY  += $(CONFIG_MBEDTLS_APP_SELFTEST_PRIORITY)
STACKSIZE += $(CONFIG_MBEDTLS_APP_SELFTEST_STACKSIZE)

MAINSRC += $(MBEDTLS_UNPACKPROGDIR)/test/selftest.c
endif

endif

# Configuration alternative implementation

ifneq ($(CONFIG_MBEDTLS_ENTROPY_HARDWARE_ALT)$(CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG),)
CSRCS += $(APPDIR)/crypto/mbedtls/source/entropy_alt.c
endif

ifeq ($(CONFIG_MBEDTLS_ALT),y)

CSRCS += $(APPDIR)/crypto/mbedtls/source/dev_alt.c

ifeq ($(CONFIG_MBEDTLS_AES_ALT),y)
CSRCS += $(APPDIR)/crypto/mbedtls/source/aes_alt.c
endif

ifeq ($(CONFIG_MBEDTLS_CMAC_ALT),y)
CSRCS += $(APPDIR)/crypto/mbedtls/source/cmac_alt.c
endif

ifeq ($(CONFIG_MBEDTLS_MD5_ALT),y)
CSRCS += $(APPDIR)/crypto/mbedtls/source/md5_alt.c
endif

ifeq ($(CONFIG_MBEDTLS_POLY1305_ALT),y)
CSRCS += $(APPDIR)/crypto/mbedtls/source/poly1305_alt.c
endif

ifeq ($(CONFIG_MBEDTLS_RIPEMD160_ALT),y)
CSRCS += $(APPDIR)/crypto/mbedtls/source/ripemd160_alt.c
endif

ifeq ($(CONFIG_MBEDTLS_SHA1_ALT),y)
CSRCS += $(APPDIR)/crypto/mbedtls/source/sha1_alt.c
endif

ifeq ($(CONFIG_MBEDTLS_SHA256_ALT),y)
CSRCS += $(APPDIR)/crypto/mbedtls/source/sha256_alt.c
endif

ifeq ($(CONFIG_MBEDTLS_SHA512_ALT),y)
CSRCS += $(APPDIR)/crypto/mbedtls/source/sha512_alt.c
endif

ifeq ($(CONFIG_MBEDTLS_BIGNUM_ALT),y)
CSRCS += $(APPDIR)/crypto/mbedtls/source/bignum_alt.c
endif

endif

include $(APPDIR)/Application.mk
