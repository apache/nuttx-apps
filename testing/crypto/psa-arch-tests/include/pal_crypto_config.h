/****************************************************************************
 * apps/testing/crypto/psa-arch-tests/include/pal_crypto_config.h
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

#ifndef _PAL_CRYPTO_CONFIG_H_
#define _PAL_CRYPTO_CONFIG_H_

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "pal_crypto_config_check.h"
#include <psa/crypto.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* \file pal_crypto_config.h
 *
 * \brief Configuration options for crypto tests (set of defines)
 *
 *  This set of compile-time options may be used to enable
 *  or disable features selectively for crypto test suite
 */

#define PSA_PAKE_STEP_CONFIRM                  ((psa_pake_step_t)0x04)
#define PSA_PAKE_CONFIRMED_KEY                 0
#define PSA_ALG_IS_JPAKE(alg)                  0
#define PSA_ALG_IS_SPAKE2P(alg)                0
#define PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(curve)   0
#define PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(curve) 0

/**
 * @brief A key confirmation value that indicates an unconfirmed key
 *        in a PAKE cipher suite.
 */

#define PSA_PAKE_UNCONFIRMED_KEY               1

/**
 * \def ARCH_TEST_RSA
 *
 * Enable the RSA public-key cryptosystem.
 * By default all supported keys are enabled.
 *
 * Comment macros to disable the types
 */

#define ARCH_TEST_RSA
#define ARCH_TEST_RSA_1024
#define ARCH_TEST_RSA_2048
#define ARCH_TEST_RSA_3072

/**
 * \def  ARCH_TEST_ECC
 * \def  ARCH_TEST_ECC_CURVE_SECPXXXR1
 *
 * Enable the elliptic curve
 * Enable specific curves within the Elliptic Curve
 * module.  By default all supported curves are enabled.
 *
 * Requires: ARCH_TEST_ECC
 * Comment macros to disable the curve
 */

#define ARCH_TEST_ECC
#define ARCH_TEST_ECC_CURVE_SECP192R1
#define ARCH_TEST_ECC_CURVE_SECP224R1
#define ARCH_TEST_ECC_CURVE_SECP256R1
#define ARCH_TEST_ECC_CURVE_SECP384R1

/**
 * \def ARCH_TEST_AES
 *
 * Enable the AES block cipher.
 * By default all supported keys are enabled.
 *
 * Comment macros to disable the types
 */

#define ARCH_TEST_AES
#define ARCH_TEST_AES_128
#define ARCH_TEST_AES_192
#define ARCH_TEST_AES_256
#define ARCH_TEST_AES_512

/**
 * \def  ARCH_TEST_DES
 *
 * Enable the DES block cipher.
 * By default all supported keys are enabled.
 *
 * Comment macros to disable the types
 */

#define ARCH_TEST_DES
#define ARCH_TEST_DES_1KEY
#define ARCH_TEST_DES_2KEY
#define ARCH_TEST_DES_3KEY

/**
 * \def  ARCH_TEST_RAW
 *
 * A "key" of this type cannot be used for any cryptographic operation.
 * Applications may use this type to store arbitrary data in the keystore.
 */

#define ARCH_TEST_RAW

/**
 * \def ARCH_TEST_CIPHER
 *
 * Enable the generic cipher layer.
 */

#define ARCH_TEST_CIPHER

/**
 * \def ARCH_TEST_ARC4
 *
 * Enable the ARC4 key type.
 */

#define ARCH_TEST_ARC4

/**
 * \def ARCH_TEST_CHACHA20
 *
 * Enable CHACHA20 key type
 */

#define ARCH_TEST_CHACHA20

/**
 * \def ARCH_TEST_XCHACHA20
 *
 * Enable XCHACHA20 key type
 */

/* #define ARCH_TEST_XCHACHA20 */

/**
 * \def ARCH_TEST_CIPHER_MODE_CTR
 *
 * Enable Counter Block Cipher mode (CTR) for symmetric ciphers.
 *
 * Requires: ARCH_TEST_CIPHER
 */

#define ARCH_TEST_CIPHER_MODE_CTR

/**
 * \def ARCH_TEST_CIPHER_MODE_CFB
 *
 * Enable Cipher Feedback mode (CFB) for symmetric ciphers.
 *
 * Requires: ARCH_TEST_CIPHER
 */

#define ARCH_TEST_CIPHER_MODE_CFB

/**
 * \def ARCH_TEST_CIPHER_MODE_CBC
 *
 * Enable Cipher Block Chaining mode (CBC) for symmetric ciphers.
 *
 * Requires: ARCH_TEST_CIPHER
 */

#define ARCH_TEST_CIPHER_MODE_CBC

/**
 * \def ARCH_TEST_CTR_AES
 *
 * Requires: ARCH_TEST_CIPHER, ARCH_TEST_AES, ARCH_TEST_CIPHER_MODE_CTR
 */

#define ARCH_TEST_CTR_AES

/**
 * \def ARCH_TEST_CBC_AES
 *
 * Requires: ARCH_TEST_CIPHER, ARCH_TEST_AES, ARCH_TEST_CIPHER_MODE_CBC
 *
 * Comment macros to disable the types
 */

#define ARCH_TEST_CBC_AES
#define ARCH_TEST_CBC_AES_NO_PADDING

/**
 * \def ARCH_TEST_CBC_NO_PADDING
 *
 * Requires: ARCH_TEST_CIPHER, ARCH_TEST_CIPHER_MODE_CBC
 *
 * Comment macros to disable the types
 */

#define ARCH_TEST_CBC_NO_PADDING

/**
 * \def ARCH_TEST_CFB_AES
 *
 * Requires: ARCH_TEST_CIPHER, ARCH_TEST_AES, ARCH_TEST_CIPHER_MODE_CFB
 */

#define ARCH_TEST_CFB_AES

/**
 * \def ARCH_TEST_PKCS1V15_*
 *
 * Enable support for PKCS#1 v1.5 encoding.
 * Enable support for PKCS#1 v1.5 operations.
 * Enable support for RSA-OAEP
 *
 * Requires: ARCH_TEST_RSA, ARCH_TEST_PKCS1V15
 *
 * Comment macros to disable the types
 */

#define ARCH_TEST_PKCS1V15
#define ARCH_TEST_RSA_PKCS1V15_SIGN
#define ARCH_TEST_RSA_PKCS1V15_SIGN_RAW
#define ARCH_TEST_RSA_PKCS1V15_CRYPT
#define ARCH_TEST_RSA_OAEP

/**
 * \def ARCH_TEST_CBC_PKCS7
 *
 * Requires: ARCH_TEST_CIPHER_MODE_CBC
 *
 * Comment macros to disable the types
 */

#define ARCH_TEST_CBC_PKCS7

/**
 * \def ARCH_TEST_ASYMMETRIC_ENCRYPTION
 *
 * Enable support for Asymmetric encryption algorithms
 */

#define ARCH_TEST_ASYMMETRIC_ENCRYPTION

/**
 * \def ARCH_TEST_HASH
 *
 * Enable the hash algorithm.
 */

#define ARCH_TEST_HASH

/**
 * \def  ARCH_TEST_HMAC
 *
 * The key policy determines which underlying hash algorithm the key can be
 * used for.
 *
 * Requires: ARCH_TEST_HASH
 */

#define ARCH_TEST_HMAC

/**
 * Enable ZIGBEE MMO HASH algorithm
 * Requires : ARCH_TEST_AES
 *
 */

/* #define ARCH_TEST_AES_MMO_ZIGBEE */

/**
 * \def ARCH_TEST_MDX
 * \def ARCH_TEST_SHAXXX
 *
 * Enable the MDX algorithm.
 * Enable the SHAXXX algorithm.
 *
 * Requires: ARCH_TEST_HASH
 *
 * Comment macros to disable the types
 */

/* #define ARCH_TEST_MD2
 * #define ARCH_TEST_MD4
 */

#define ARCH_TEST_MD5
#define ARCH_TEST_RIPEMD160
#define ARCH_TEST_SHA1
#define ARCH_TEST_SHA224
#define ARCH_TEST_SHA256
#define ARCH_TEST_SHA384
#define ARCH_TEST_SHA512
/* #define ARCH_TEST_SHA512_224
 * #define ARCH_TEST_SHA512_256
 * #define ARCH_TEST_SHA3_224
 * #define ARCH_TEST_SHA3_256
 * #define ARCH_TEST_SHA3_384
 * #define ARCH_TEST_SHA3_512
 */

/**
 * \def ARCH_TEST_HKDF
 *
 * Enable the HKDF algorithm (RFC 5869).
 *
 * Requires: ARCH_TEST_HASH
 */

#define ARCH_TEST_HKDF
#define ARCH_TEST_HKDF_EXTRACT
#define ARCH_TEST_HKDF_EXPAND

/**
 * \def ARCH_TEST_TLS12_PRF
 *
 * Enable the TLS-1.2 PRF algorithm (RFC 5246).
 *
 * Requires: ARCH_TEST_HASH
 */

#define ARCH_TEST_TLS12_PRF
#define ARCH_TEST_TLS12_PSK_TO_MS

/**
 * \def ARCH_TEST_SP800_108
 *
 * Enable the NIST SP800-108 Counter mode KDF algorithm
 */

/* #define ARCH_TEST_SP800_108_COUNTER_HMAC
 * #define ARCH_TEST_SP800_108_COUNTER_CMAC
 */

/**
 * \def ARCH_TEST_xMAC
 *
 * Enable the xMAC (Cipher/Hash/G-based Message Authentication Code)
 * mode for block ciphers.
 * Requires: ARCH_TEST_AES or ARCH_TEST_DES
 *
 * Comment macros to disable the types
 */

#define ARCH_TEST_CMAC
#define ARCH_TEST_HMAC

/**
 * \def ARCH_TEST_CCM
 *
 * Enable the Counter with CBC-MAC (CCM) mode for 128-bit block cipher.
 *
 * Requires: ARCH_TEST_AES
 */

#define ARCH_TEST_CCM

/**
 * \def ARCH_TEST_GCM
 *
 * Enable the Galois/Counter Mode (GCM) for AES.
 *
 * Requires: ARCH_TEST_AES
 *
 */

/**
 * \def ARCH_TEST_CCM_STAR_NO_TAG
 *
 * Enable CBC-MAC (CCM) mode without authentication.
 *
 * Requires: ARCH_TEST_AES
 */

#define ARCH_TEST_CCM_STAR_NO_TAG
#define ARCH_TEST_CCM_STAR_ANY_TAG
#define ARCH_TEST_GCM

/**
 * \def ARCH_TEST_TRUNCATED_MAC
 *
 * Enable support for RFC 6066 truncated HMAC in SSL.
 *
 * Comment this macro to disable support for truncated HMAC in SSL
 */

#define ARCH_TEST_TRUNCATED_MAC

/**
 * \def ARCH_TEST_ECDH
 *
 * Enable the elliptic curve Diffie-Hellman library.
 *
 * Requires: ARCH_TEST_ECC
 */

#define ARCH_TEST_ECDH

/**
 * \def ARCH_TEST_ECDSA
 *
 * Enable the elliptic curve DSA library.
 * Requires: ARCH_TEST_ECC
 */

#define ARCH_TEST_ECDSA

/**
 * \def ARCH_TEST_DETERMINISTIC_ECDSA
 *
 * Enable deterministic ECDSA (RFC 6979).
 */

#define ARCH_TEST_DETERMINISTIC_ECDSA
/* #define ARCH_TEST_TWISTED_EDWARDS */

/**
 * \def ARCH_TEST_ECC_ASYMMETRIC_API_SUPPORT
 *
 * Enable ECC support for asymmetric API.
 */

/* #define ARCH_TEST_ECC_ASYMMETRIC_API_SUPPORT
 */

/**
 * \def ARCH_TEST_PBKDF2
 *
 * Enable the Password-based Key derivation functions - 2.
 */

/* #define ARCH_TEST_PBKDF2 */

/**
 * \def ARCH_TEST_JAPKE
 *
 * Enable support for balanced PAKE: ECJPAKE algorithm
 */

/* #define ARCH_TEST_JPAKE
 */

/**
 * \def ARCH_TEST_SPAKE2P
 *
 * Enable support for augmented PAKE: SPAKE2P algorithm
 */

/* #define ARCH_TEST_SPAKE2P
 * #define ARCH_TEST_SPAKE2P_HMAC
 * #define ARCH_TEST_SPAKE2P_CMAC
 * #define ARCH_TEST_SPAKE2P_MATTER
 */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

psa_status_t psa_key_agreement(psa_key_id_t private_key,
                               FAR const uint8_t *peer_key,
                               size_t peer_key_length,
                               psa_algorithm_t alg,
                               FAR const psa_key_attributes_t *attributes,
                               FAR psa_key_id_t *key);

psa_status_t psa_pake_set_context(FAR psa_pake_operation_t *operation,
                                  FAR const uint8_t *context,
                                  size_t context_len);

psa_status_t psa_pake_get_shared_key(
                     FAR psa_pake_operation_t *operation,
                     FAR const psa_key_attributes_t *attributes,
                     FAR psa_key_id_t *key);

void psa_pake_cs_set_key_confirmation(
                     FAR psa_pake_cipher_suite_t *cipher_suite,
                     uint32_t key_confirmation);

psa_status_t psa_key_derivation_verify_bytes(
                     FAR psa_key_derivation_operation_t *operation,
                     FAR const uint8_t *expected_output,
                     size_t output_length);

psa_status_t
psa_key_derivation_verify_key(FAR psa_key_derivation_operation_t *operation,
                              psa_key_id_t expected);

#endif /* _PAL_CRYPTO_CONFIG_H_ */
