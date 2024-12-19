/****************************************************************************
 * apps/crypto/wolfssl/user_settings.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 ****************************************************************************/

#include <nuttx/config.h>

/* Library */
#define SINGLE_THREADED
#define WOLFSSL_SMALL_STACK

/* Environment */
#define NO_FILESYSTEM
#define HAVE_STRINGS_H
#define WOLF_C99

/* Math */
#define WOLFSSL_SP_MATH_ALL

/* Crypto */
#define HAVE_ECC
#define ECC_TIMING_RESISTANT
#define WC_RSA_BLINDING
#undef  RSA_LOW_MEM
#define NO_MD4
#define NO_DSA

/* RNG */
#define WOLFSSL_GENSEED_FORTEST

/* Applications */
#define NO_MAIN_FUNCTION
#define BENCH_EMBEDDED
#define WOLFSSL_BENCHMARK_FIXED_UNITS_MB

/* Development */
/*#define DEBUG_WOLFSSL*/

#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_ENCRYPT_THEN_MAC
#define HAVE_EXTENDED_MASTER
#define WOLFSSL_TLS13
#define HAVE_AESGCM
#define HAVE_HKDF
#define HAVE_DH
#define HAVE_FFDHE_2048
#define HAVE_DH_DEFAULT_PARAMS
#define WC_RSA_PSS
#define HAVE_AEAD
#define WOLFSSL_SHA224
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512
#define WOLFSSL_SHA3
#define HAVE_POLY1305
#define HAVE_CHACHA
#define HAVE_ENCRYPT_THEN_MAC
#define NO_OLD_TLS
