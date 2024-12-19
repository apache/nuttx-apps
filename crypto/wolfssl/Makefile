# ##############################################################################
# apps/crypto/wolfssl/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

include $(APPDIR)/Make.defs

# wolfSSL SSL/TLS crypto library

WOLFSSL_VERSION = $(patsubst "%",%,$(strip $(CONFIG_WOLFSSL_VERSION)))

WOLFSSL_UNPACKNAME = wolfssl
WOLFSSL_EXAMPLESNAME = wolfssl-examples

# WOLFSSL TLS FILES
CSRCS += $(WOLFSSL_UNPACKNAME)/src/crl.c
CSRCS += $(WOLFSSL_UNPACKNAME)/src/internal.c
CSRCS += $(WOLFSSL_UNPACKNAME)/src/keys.c
CSRCS += $(WOLFSSL_UNPACKNAME)/src/ocsp.c
CSRCS += $(WOLFSSL_UNPACKNAME)/src/sniffer.c
CSRCS += $(WOLFSSL_UNPACKNAME)/src/ssl.c
CSRCS += $(WOLFSSL_UNPACKNAME)/src/tls.c
CSRCS += $(WOLFSSL_UNPACKNAME)/src/tls13.c
CSRCS += $(WOLFSSL_UNPACKNAME)/src/wolfio.c

# wolfCrypt Core (FIPS validated crypto available contact fips@wolfssl.com)
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/aes.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/cmac.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/des3.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/dh.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/ecc.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/hmac.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/random.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/rsa.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sha.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sha256.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sha512.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sha3.c

# wolfCrypt Additional
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/asm.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/asn.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/blake2s.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/chacha.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/chacha20_poly1305.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/coding.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/compress.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/cpuid.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/cryptocb.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/curve25519.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/curve448.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/ecc_fp.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/eccsi.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/ed25519.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/ed448.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/error.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/fe_448.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/fe_low_mem.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/fe_operations.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/ge_448.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/ge_low_mem.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/ge_operations.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/hash.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/kdf.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/integer.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/logging.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/md5.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/memory.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/pkcs12.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/pkcs7.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/poly1305.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/pwdbased.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/rc2.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sakke.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/signature.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/srp.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sp_arm32.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sp_arm64.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sp_armthumb.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sp_c32.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sp_c64.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sp_cortexm.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sp_dsp32.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sp_int.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/sp_x86_64.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/tfm.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/wc_dsp.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/wc_encrypt.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/wc_pkcs11.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/wc_port.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/wolfevent.c
CSRCS += $(WOLFSSL_UNPACKNAME)/wolfcrypt/src/wolfmath.c

CFLAGS += -DWOLFSSL_USER_SETTINGS

ifneq ($(CONFIG_WOLFCRYPT_APPS),)

MODULE = $(CONFIG_WOLFSSL_APPS)

ifeq ($(CONFIG_WOLFCRYPT_APP_BENCHMARK),y)

PROGNAME  += $(CONFIG_WOLFCRYPT_APP_BENCHMARK_PROGNAME)
PRIORITY  += $(CONFIG_WOLFCRYPT_APP_BENCHMARK_PRIORITY)
STACKSIZE += $(CONFIG_WOLFCRYPT_APP_BENCHMARK_STACKSIZE)

CFLAGS += -I$(WOLFSSL_UNPACKNAME)
CFLAGS += -I$(WOLFSSL_UNPACKNAME)/../ # To reach user_settings.h
MAINSRC += $(WOLFSSL_UNPACKNAME)/wolfcrypt/benchmark/benchmark.c

endif

ifeq ($(CONFIG_WOLFCRYPT_APP_SELFTEST),y)

PROGNAME  += $(CONFIG_WOLFCRYPT_APP_SELFTEST_PROGNAME)
PRIORITY  += $(CONFIG_WOLFCRYPT_APP_SELFTEST_PRIORITY)
STACKSIZE += $(CONFIG_WOLFCRYPT_APP_SELFTEST_STACKSIZE)

CFLAGS += -I$(WOLFSSL_UNPACKNAME)
CFLAGS += -I$(WOLFSSL_UNPACKNAME)/../ # To reach user_settings.h
MAINSRC += $(WOLFSSL_UNPACKNAME)/wolfcrypt/test/test.c

endif

ifeq ($(CONFIG_WOLFSSL_APP_CLIENT_SERVER),y)

PROGNAME  += $(CONFIG_WOLFSSL_APP_CLIENT_SERVER_PROGNAME)
PRIORITY  += $(CONFIG_WOLFSSL_APP_CLIENT_SERVER_PRIORITY)
STACKSIZE += $(CONFIG_WOLFSSL_APP_CLIENT_SERVER_STACKSIZE)

CFLAGS += -I$(WOLFSSL_UNPACKNAME)
CFLAGS += -I$(WOLFSSL_UNPACKNAME)/../ # To reach user_settings.h
MAINSRC += $(WOLFSSL_EXAMPLESNAME)/embedded/tls-client-server.c

endif

endif

ifeq ($(CONFIG_WOLFSSL_APP_CLIENT_SERVER),y)
wolfssl:
	git clone git@github.com:wolfSSL/wolfssl.git $(WOLFSSL_UNPACKNAME)
	git clone git@github.com:wolfSSL/wolfssl-examples.git $(WOLFSSL_EXAMPLESNAME)
else
wolfssl:
	git clone git@github.com:wolfSSL/wolfssl.git $(WOLFSSL_UNPACKNAME)
endif

context:: wolfssl

ifeq ($(CONFIG_WOLFSSL_APP_CLIENT_SERVER),y)
distclean::
	$(call DELDIR, $(WOLFSSL_UNPACKNAME))
	$(call DELDIR, $(WOLFSSL_EXAMPLESNAME))
else
distclean:
	$(call DELDIR, $(WOLFSSL_UNPACKNAME))
endif

include $(APPDIR)/Application.mk
