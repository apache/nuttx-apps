############################################################################
# apps/testing/drivers/crypto/Makefile
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

# Generic file system stress test application info
ifeq ($(CONFIG_TESTING_CRYPTO_3DES_CBC),y)
PROGNAME += des3cbc
MAINSRC  += 3descbc.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_AES_XTS),y)
PROGNAME += aesxts
MAINSRC  += aesxts.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_AES_CTR),y)
PROGNAME += aesctr
MAINSRC  += aesctr.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_AES_CBC),y)
PROGNAME += aescbc
MAINSRC  += aescbc.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_HMAC),y)
PROGNAME += hmac
MAINSRC  += hmac.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_HASH),y)
PROGNAME += hash
MAINSRC +=  hash.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_CRC32),y)
PROGNAME += crc32
MAINSRC +=  crc32.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_AES_CMAC),y)
PROGNAME += aescmac
MAINSRC +=  aescmac.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_ECDSA),y)
PROGNAME += ecdsa
MAINSRC +=  ecdsa.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_ECDH),y)
PROGNAME += dhm
MAINSRC +=  dhm.c
endif

ifeq ($(CONFIG_TESTING_CRYPTO_RSA),y)
PROGNAME += rsa
MAINSRC +=  rsa.c
endif

PRIORITY = $(CONFIG_TESTING_CRYPTO_PRIORITY)
STACKSIZE = $(CONFIG_TESTING_CRYPTO_STACKSIZE)
MODULE = $(CONFIG_TESTING_CRYPTO)

# Generic file system stress test

include $(APPDIR)/Application.mk
