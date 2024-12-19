# ##############################################################################
# apps/crypto/libtomcrypt/CMakeLists.txt
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

if(CONFIG_CRYPTO_LIBTOMCRYPT)

  # ############################################################################
  # Config and Fetch libtomcrypt
  # ############################################################################
  set(LIBTOMCRYPT_DIR ${CMAKE_CURRENT_LIST_DIR}/libtomcrypt)

  if(NOT EXISTS ${LIBTOMCRYPT_DIR})
    set(CONFIG_LIBTOMCRYPT_URL https://github.com/libtom/libtomcrypt/archive)

    FetchContent_Declare(
      libtomcrypt_fetch
      URL ${CONFIG_LIBTOMCRYPT_URL}/v${CONFIG_LIBTOMCRYPT_VERSION}.zip
          SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/libtomcrypt BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/crypto/libtomcrypt/libtomcrypt
      PATCH_COMMAND
        patch -p0 -d ${CMAKE_CURRENT_LIST_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/digit-bit.patch && patch -p0 -d
        ${CMAKE_CURRENT_LIST_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/ltcrypt-flush.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(libtomcrypt_fetch)

    if(NOT libtomcrypt_fetch_POPULATED)
      FetchContent_Populate(libtomcrypt_fetch)
    endif()
  endif()

  nuttx_add_library(libtomcrypt STATIC)

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${LIBTOMCRYPT_DIR}/src/headers)

  # ############################################################################
  # Sources
  # ############################################################################

  set(CSRCS
      ${LIBTOMCRYPT_DIR}/src/ciphers/aes/aes.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/aes/aes_tab.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/anubis.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/blowfish.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/camellia.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/cast5.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/des.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/kasumi.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/khazad.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/kseed.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/multi2.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/noekeon.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/rc2.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/rc5.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/rc6.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/safer/safer.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/safer/saferp.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/skipjack.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/twofish/twofish.c
      ${LIBTOMCRYPT_DIR}/src/ciphers/xtea.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ccm/ccm_add_aad.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ccm/ccm_add_nonce.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ccm/ccm_done.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ccm/ccm_init.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ccm/ccm_memory.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ccm/ccm_process.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ccm/ccm_reset.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ccm/ccm_test.c
      ${LIBTOMCRYPT_DIR}/src/encauth/chachapoly/chacha20poly1305_add_aad.c
      ${LIBTOMCRYPT_DIR}/src/encauth/chachapoly/chacha20poly1305_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/chachapoly/chacha20poly1305_done.c
      ${LIBTOMCRYPT_DIR}/src/encauth/chachapoly/chacha20poly1305_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/chachapoly/chacha20poly1305_init.c
      ${LIBTOMCRYPT_DIR}/src/encauth/chachapoly/chacha20poly1305_memory.c
      ${LIBTOMCRYPT_DIR}/src/encauth/chachapoly/chacha20poly1305_setiv.c
      ${LIBTOMCRYPT_DIR}/src/encauth/chachapoly/chacha20poly1305_setiv_rfc7905.c
      ${LIBTOMCRYPT_DIR}/src/encauth/chachapoly/chacha20poly1305_test.c
      ${LIBTOMCRYPT_DIR}/src/encauth/eax/eax_addheader.c
      ${LIBTOMCRYPT_DIR}/src/encauth/eax/eax_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/eax/eax_decrypt_verify_memory.c
      ${LIBTOMCRYPT_DIR}/src/encauth/eax/eax_done.c
      ${LIBTOMCRYPT_DIR}/src/encauth/eax/eax_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/eax/eax_encrypt_authenticate_memory.c
      ${LIBTOMCRYPT_DIR}/src/encauth/eax/eax_init.c
      ${LIBTOMCRYPT_DIR}/src/encauth/eax/eax_test.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_add_aad.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_add_iv.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_done.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_gf_mult.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_init.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_memory.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_mult_h.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_process.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_reset.c
      ${LIBTOMCRYPT_DIR}/src/encauth/gcm/gcm_test.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_decrypt_verify_memory.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_done_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_done_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_encrypt_authenticate_memory.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_init.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_ntz.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_shift_xor.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/ocb_test.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb/s_ocb_done.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_add_aad.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_decrypt_last.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_decrypt_verify_memory.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_done.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_encrypt_authenticate_memory.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_encrypt_last.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_init.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_int_ntz.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_int_xor_blocks.c
      ${LIBTOMCRYPT_DIR}/src/encauth/ocb3/ocb3_test.c
      ${LIBTOMCRYPT_DIR}/src/hashes/blake2b.c
      ${LIBTOMCRYPT_DIR}/src/hashes/blake2s.c
      ${LIBTOMCRYPT_DIR}/src/hashes/chc/chc.c
      ${LIBTOMCRYPT_DIR}/src/hashes/helper/hash_file.c
      ${LIBTOMCRYPT_DIR}/src/hashes/helper/hash_filehandle.c
      ${LIBTOMCRYPT_DIR}/src/hashes/helper/hash_memory.c
      ${LIBTOMCRYPT_DIR}/src/hashes/helper/hash_memory_multi.c
      ${LIBTOMCRYPT_DIR}/src/hashes/md2.c
      ${LIBTOMCRYPT_DIR}/src/hashes/md4.c
      ${LIBTOMCRYPT_DIR}/src/hashes/md5.c
      ${LIBTOMCRYPT_DIR}/src/hashes/rmd128.c
      ${LIBTOMCRYPT_DIR}/src/hashes/rmd160.c
      ${LIBTOMCRYPT_DIR}/src/hashes/rmd256.c
      ${LIBTOMCRYPT_DIR}/src/hashes/rmd320.c
      ${LIBTOMCRYPT_DIR}/src/hashes/sha1.c
      ${LIBTOMCRYPT_DIR}/src/hashes/sha2/sha224.c
      ${LIBTOMCRYPT_DIR}/src/hashes/sha2/sha256.c
      ${LIBTOMCRYPT_DIR}/src/hashes/sha2/sha384.c
      ${LIBTOMCRYPT_DIR}/src/hashes/sha2/sha512.c
      ${LIBTOMCRYPT_DIR}/src/hashes/sha2/sha512_224.c
      ${LIBTOMCRYPT_DIR}/src/hashes/sha2/sha512_256.c
      ${LIBTOMCRYPT_DIR}/src/hashes/sha3.c
      ${LIBTOMCRYPT_DIR}/src/hashes/sha3_test.c
      ${LIBTOMCRYPT_DIR}/src/hashes/tiger.c
      ${LIBTOMCRYPT_DIR}/src/hashes/whirl/whirl.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2bmac.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2bmac_file.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2bmac_memory.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2bmac_memory_multi.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2bmac_test.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2smac.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2smac_file.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2smac_memory.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2smac_memory_multi.c
      ${LIBTOMCRYPT_DIR}/src/mac/blake2/blake2smac_test.c
      ${LIBTOMCRYPT_DIR}/src/mac/f9/f9_done.c
      ${LIBTOMCRYPT_DIR}/src/mac/f9/f9_file.c
      ${LIBTOMCRYPT_DIR}/src/mac/f9/f9_init.c
      ${LIBTOMCRYPT_DIR}/src/mac/f9/f9_memory.c
      ${LIBTOMCRYPT_DIR}/src/mac/f9/f9_memory_multi.c
      ${LIBTOMCRYPT_DIR}/src/mac/f9/f9_process.c
      ${LIBTOMCRYPT_DIR}/src/mac/f9/f9_test.c
      ${LIBTOMCRYPT_DIR}/src/mac/hmac/hmac_done.c
      ${LIBTOMCRYPT_DIR}/src/mac/hmac/hmac_file.c
      ${LIBTOMCRYPT_DIR}/src/mac/hmac/hmac_init.c
      ${LIBTOMCRYPT_DIR}/src/mac/hmac/hmac_memory.c
      ${LIBTOMCRYPT_DIR}/src/mac/hmac/hmac_memory_multi.c
      ${LIBTOMCRYPT_DIR}/src/mac/hmac/hmac_process.c
      ${LIBTOMCRYPT_DIR}/src/mac/hmac/hmac_test.c
      ${LIBTOMCRYPT_DIR}/src/mac/omac/omac_done.c
      ${LIBTOMCRYPT_DIR}/src/mac/omac/omac_file.c
      ${LIBTOMCRYPT_DIR}/src/mac/omac/omac_init.c
      ${LIBTOMCRYPT_DIR}/src/mac/omac/omac_memory.c
      ${LIBTOMCRYPT_DIR}/src/mac/omac/omac_memory_multi.c
      ${LIBTOMCRYPT_DIR}/src/mac/omac/omac_process.c
      ${LIBTOMCRYPT_DIR}/src/mac/omac/omac_test.c
      ${LIBTOMCRYPT_DIR}/src/mac/pelican/pelican.c
      ${LIBTOMCRYPT_DIR}/src/mac/pelican/pelican_memory.c
      ${LIBTOMCRYPT_DIR}/src/mac/pelican/pelican_test.c
      ${LIBTOMCRYPT_DIR}/src/mac/pmac/pmac_done.c
      ${LIBTOMCRYPT_DIR}/src/mac/pmac/pmac_file.c
      ${LIBTOMCRYPT_DIR}/src/mac/pmac/pmac_init.c
      ${LIBTOMCRYPT_DIR}/src/mac/pmac/pmac_memory.c
      ${LIBTOMCRYPT_DIR}/src/mac/pmac/pmac_memory_multi.c
      ${LIBTOMCRYPT_DIR}/src/mac/pmac/pmac_ntz.c
      ${LIBTOMCRYPT_DIR}/src/mac/pmac/pmac_process.c
      ${LIBTOMCRYPT_DIR}/src/mac/pmac/pmac_shift_xor.c
      ${LIBTOMCRYPT_DIR}/src/mac/pmac/pmac_test.c
      ${LIBTOMCRYPT_DIR}/src/mac/poly1305/poly1305.c
      ${LIBTOMCRYPT_DIR}/src/mac/poly1305/poly1305_file.c
      ${LIBTOMCRYPT_DIR}/src/mac/poly1305/poly1305_memory.c
      ${LIBTOMCRYPT_DIR}/src/mac/poly1305/poly1305_memory_multi.c
      ${LIBTOMCRYPT_DIR}/src/mac/poly1305/poly1305_test.c
      ${LIBTOMCRYPT_DIR}/src/mac/xcbc/xcbc_done.c
      ${LIBTOMCRYPT_DIR}/src/mac/xcbc/xcbc_file.c
      ${LIBTOMCRYPT_DIR}/src/mac/xcbc/xcbc_init.c
      ${LIBTOMCRYPT_DIR}/src/mac/xcbc/xcbc_memory.c
      ${LIBTOMCRYPT_DIR}/src/mac/xcbc/xcbc_memory_multi.c
      ${LIBTOMCRYPT_DIR}/src/mac/xcbc/xcbc_process.c
      ${LIBTOMCRYPT_DIR}/src/mac/xcbc/xcbc_test.c
      ${LIBTOMCRYPT_DIR}/src/math/fp/ltc_ecc_fp_mulmod.c
      ${LIBTOMCRYPT_DIR}/src/math/gmp_desc.c
      ${LIBTOMCRYPT_DIR}/src/math/ltm_desc.c
      ${LIBTOMCRYPT_DIR}/src/math/multi.c
      ${LIBTOMCRYPT_DIR}/src/math/radix_to_bin.c
      ${LIBTOMCRYPT_DIR}/src/math/rand_bn.c
      ${LIBTOMCRYPT_DIR}/src/math/rand_prime.c
      ${LIBTOMCRYPT_DIR}/src/math/tfm_desc.c
      ${LIBTOMCRYPT_DIR}/src/misc/adler32.c
      ${LIBTOMCRYPT_DIR}/src/misc/base64/base64_decode.c
      ${LIBTOMCRYPT_DIR}/src/misc/base64/base64_encode.c
      ${LIBTOMCRYPT_DIR}/src/misc/burn_stack.c
      ${LIBTOMCRYPT_DIR}/src/misc/compare_testvector.c
      ${LIBTOMCRYPT_DIR}/src/misc/crc32.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_argchk.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_cipher_descriptor.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_cipher_is_valid.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_constants.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_find_cipher.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_find_cipher_any.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_find_cipher_id.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_find_hash.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_find_hash_any.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_find_hash_id.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_find_hash_oid.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_find_prng.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_fsa.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_hash_descriptor.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_hash_is_valid.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_inits.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_ltc_mp_descriptor.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_prng_descriptor.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_prng_is_valid.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_prng_rng_descriptor.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_register_all_ciphers.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_register_all_hashes.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_register_all_prngs.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_register_cipher.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_register_hash.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_register_prng.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_sizes.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_unregister_cipher.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_unregister_hash.c
      ${LIBTOMCRYPT_DIR}/src/misc/crypt/crypt_unregister_prng.c
      ${LIBTOMCRYPT_DIR}/src/misc/error_to_string.c
      ${LIBTOMCRYPT_DIR}/src/misc/hkdf/hkdf.c
      ${LIBTOMCRYPT_DIR}/src/misc/hkdf/hkdf_test.c
      ${LIBTOMCRYPT_DIR}/src/misc/mem_neq.c
      ${LIBTOMCRYPT_DIR}/src/misc/pk_get_oid.c
      ${LIBTOMCRYPT_DIR}/src/misc/pkcs5/pkcs_5_1.c
      ${LIBTOMCRYPT_DIR}/src/misc/pkcs5/pkcs_5_2.c
      ${LIBTOMCRYPT_DIR}/src/misc/pkcs5/pkcs_5_test.c
      ${LIBTOMCRYPT_DIR}/src/misc/zeromem.c
      ${LIBTOMCRYPT_DIR}/src/modes/cbc/cbc_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/cbc/cbc_done.c
      ${LIBTOMCRYPT_DIR}/src/modes/cbc/cbc_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/cbc/cbc_getiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/cbc/cbc_setiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/cbc/cbc_start.c
      ${LIBTOMCRYPT_DIR}/src/modes/cfb/cfb_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/cfb/cfb_done.c
      ${LIBTOMCRYPT_DIR}/src/modes/cfb/cfb_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/cfb/cfb_getiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/cfb/cfb_setiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/cfb/cfb_start.c
      ${LIBTOMCRYPT_DIR}/src/modes/ctr/ctr_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/ctr/ctr_done.c
      ${LIBTOMCRYPT_DIR}/src/modes/ctr/ctr_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/ctr/ctr_getiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/ctr/ctr_setiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/ctr/ctr_start.c
      ${LIBTOMCRYPT_DIR}/src/modes/ctr/ctr_test.c
      ${LIBTOMCRYPT_DIR}/src/modes/ecb/ecb_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/ecb/ecb_done.c
      ${LIBTOMCRYPT_DIR}/src/modes/ecb/ecb_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/ecb/ecb_start.c
      ${LIBTOMCRYPT_DIR}/src/modes/f8/f8_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/f8/f8_done.c
      ${LIBTOMCRYPT_DIR}/src/modes/f8/f8_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/f8/f8_getiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/f8/f8_setiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/f8/f8_start.c
      ${LIBTOMCRYPT_DIR}/src/modes/f8/f8_test_mode.c
      ${LIBTOMCRYPT_DIR}/src/modes/lrw/lrw_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/lrw/lrw_done.c
      ${LIBTOMCRYPT_DIR}/src/modes/lrw/lrw_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/lrw/lrw_getiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/lrw/lrw_process.c
      ${LIBTOMCRYPT_DIR}/src/modes/lrw/lrw_setiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/lrw/lrw_start.c
      ${LIBTOMCRYPT_DIR}/src/modes/lrw/lrw_test.c
      ${LIBTOMCRYPT_DIR}/src/modes/ofb/ofb_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/ofb/ofb_done.c
      ${LIBTOMCRYPT_DIR}/src/modes/ofb/ofb_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/ofb/ofb_getiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/ofb/ofb_setiv.c
      ${LIBTOMCRYPT_DIR}/src/modes/ofb/ofb_start.c
      ${LIBTOMCRYPT_DIR}/src/modes/xts/xts_decrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/xts/xts_done.c
      ${LIBTOMCRYPT_DIR}/src/modes/xts/xts_encrypt.c
      ${LIBTOMCRYPT_DIR}/src/modes/xts/xts_init.c
      ${LIBTOMCRYPT_DIR}/src/modes/xts/xts_mult_x.c
      ${LIBTOMCRYPT_DIR}/src/modes/xts/xts_test.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/bit/der_decode_bit_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/bit/der_decode_raw_bit_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/bit/der_encode_bit_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/bit/der_encode_raw_bit_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/bit/der_length_bit_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/boolean/der_decode_boolean.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/boolean/der_encode_boolean.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/boolean/der_length_boolean.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/choice/der_decode_choice.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/generalizedtime/der_decode_generalizedtime.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/generalizedtime/der_encode_generalizedtime.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/generalizedtime/der_length_generalizedtime.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/ia5/der_decode_ia5_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/ia5/der_encode_ia5_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/ia5/der_length_ia5_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/integer/der_decode_integer.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/integer/der_encode_integer.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/integer/der_length_integer.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/object_identifier/der_decode_object_identifier.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/object_identifier/der_encode_object_identifier.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/object_identifier/der_length_object_identifier.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/octet/der_decode_octet_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/octet/der_encode_octet_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/octet/der_length_octet_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/printable_string/der_decode_printable_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/printable_string/der_encode_printable_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/printable_string/der_length_printable_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_decode_sequence_ex.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_decode_sequence_flexi.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_decode_sequence_multi.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_decode_subject_public_key_info.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_encode_sequence_ex.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_encode_sequence_multi.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_encode_subject_public_key_info.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_length_sequence.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_sequence_free.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/sequence/der_sequence_shrink.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/set/der_encode_set.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/set/der_encode_setof.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/short_integer/der_decode_short_integer.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/short_integer/der_encode_short_integer.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/short_integer/der_length_short_integer.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/teletex_string/der_decode_teletex_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/teletex_string/der_length_teletex_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/utctime/der_decode_utctime.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/utctime/der_encode_utctime.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/utctime/der_length_utctime.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/utf8/der_decode_utf8_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/utf8/der_encode_utf8_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/asn1/der/utf8/der_length_utf8_string.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh_check_pubkey.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh_export.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh_export_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh_free.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh_generate_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh_import.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh_set.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh_set_pg_dhparam.c
      ${LIBTOMCRYPT_DIR}/src/pk/dh/dh_shared_secret.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_decrypt_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_encrypt_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_export.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_free.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_generate_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_generate_pqg.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_import.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_make_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_set.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_set_pqg_dsaparam.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_shared_secret.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_sign_hash.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_verify_hash.c
      ${LIBTOMCRYPT_DIR}/src/pk/dsa/dsa_verify_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_ansi_x963_export.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_ansi_x963_import.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_decrypt_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_encrypt_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_export.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_free.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_get_size.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_import.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_make_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_shared_secret.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_sign_hash.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_sizes.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_test.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ecc_verify_hash.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ltc_ecc_is_valid_idx.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ltc_ecc_map.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ltc_ecc_mul2add.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ltc_ecc_mulmod.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ltc_ecc_mulmod_timing.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ltc_ecc_points.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ltc_ecc_projective_add_point.c
      ${LIBTOMCRYPT_DIR}/src/pk/ecc/ltc_ecc_projective_dbl_point.c
      ${LIBTOMCRYPT_DIR}/src/pk/katja/katja_decrypt_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/katja/katja_encrypt_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/katja/katja_export.c
      ${LIBTOMCRYPT_DIR}/src/pk/katja/katja_exptmod.c
      ${LIBTOMCRYPT_DIR}/src/pk/katja/katja_free.c
      ${LIBTOMCRYPT_DIR}/src/pk/katja/katja_import.c
      ${LIBTOMCRYPT_DIR}/src/pk/katja/katja_make_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/pkcs1/pkcs_1_i2osp.c
      ${LIBTOMCRYPT_DIR}/src/pk/pkcs1/pkcs_1_mgf1.c
      ${LIBTOMCRYPT_DIR}/src/pk/pkcs1/pkcs_1_oaep_decode.c
      ${LIBTOMCRYPT_DIR}/src/pk/pkcs1/pkcs_1_oaep_encode.c
      ${LIBTOMCRYPT_DIR}/src/pk/pkcs1/pkcs_1_os2ip.c
      ${LIBTOMCRYPT_DIR}/src/pk/pkcs1/pkcs_1_pss_decode.c
      ${LIBTOMCRYPT_DIR}/src/pk/pkcs1/pkcs_1_pss_encode.c
      ${LIBTOMCRYPT_DIR}/src/pk/pkcs1/pkcs_1_v1_5_decode.c
      ${LIBTOMCRYPT_DIR}/src/pk/pkcs1/pkcs_1_v1_5_encode.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_decrypt_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_encrypt_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_export.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_exptmod.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_free.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_get_size.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_import.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_import_pkcs8.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_import_x509.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_make_key.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_set.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_sign_hash.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_sign_saltlen_get.c
      ${LIBTOMCRYPT_DIR}/src/pk/rsa/rsa_verify_hash.c
      ${LIBTOMCRYPT_DIR}/src/prngs/chacha20.c
      ${LIBTOMCRYPT_DIR}/src/prngs/fortuna.c
      ${LIBTOMCRYPT_DIR}/src/prngs/rc4.c
      ${LIBTOMCRYPT_DIR}/src/prngs/rng_get_bytes.c
      ${LIBTOMCRYPT_DIR}/src/prngs/rng_make_prng.c
      ${LIBTOMCRYPT_DIR}/src/prngs/sober128.c
      ${LIBTOMCRYPT_DIR}/src/prngs/sprng.c
      ${LIBTOMCRYPT_DIR}/src/prngs/yarrow.c
      ${LIBTOMCRYPT_DIR}/src/stream/chacha/chacha_crypt.c
      ${LIBTOMCRYPT_DIR}/src/stream/chacha/chacha_done.c
      ${LIBTOMCRYPT_DIR}/src/stream/chacha/chacha_ivctr32.c
      ${LIBTOMCRYPT_DIR}/src/stream/chacha/chacha_ivctr64.c
      ${LIBTOMCRYPT_DIR}/src/stream/chacha/chacha_keystream.c
      ${LIBTOMCRYPT_DIR}/src/stream/chacha/chacha_setup.c
      ${LIBTOMCRYPT_DIR}/src/stream/chacha/chacha_test.c
      ${LIBTOMCRYPT_DIR}/src/stream/rc4/rc4_stream.c
      ${LIBTOMCRYPT_DIR}/src/stream/rc4/rc4_test.c
      ${LIBTOMCRYPT_DIR}/src/stream/sober128/sober128_stream.c
      ${LIBTOMCRYPT_DIR}/src/stream/sober128/sober128_test.c)

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_LIBTOMCRYPT_DEMOS)

    if(CONFIG_LIBTOMCRYPT_LTCRYPT)

      nuttx_add_application(
        NAME
        ${CONFIG_LIBTOMCRYPT_LTCRYPT_PROGNAME}
        STACKSIZE
        ${CONFIG_LIBTOMCRYPT_LTCRYPT_STACKSIZE}
        PRIORITY
        ${CONFIG_LIBTOMCRYPT_LTCRYPT_PRIORITY}
        SRCS
        ${LIBTOMCRYPT_DIR}/demos/ltcrypt.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        DEPENDS
        libtomcrypt)

    endif()

    if(CONFIG_LIBTOMCRYPT_HASHSUM)

      nuttx_add_application(
        NAME
        ${CONFIG_LIBTOMCRYPT_HASHSUM_PROGNAME}
        STACKSIZE
        ${CONFIG_LIBTOMCRYPT_HASHSUM_STACKSIZE}
        PRIORITY
        ${CONFIG_LIBTOMCRYPT_HASHSUM_PRIORITY}
        SRCS
        ${LIBTOMCRYPT_DIR}/demos/hashsum.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        DEPENDS
        libtomcrypt)

    endif()

  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  target_sources(libtomcrypt PRIVATE ${CSRCS})
  target_include_directories(libtomcrypt PRIVATE ${INCDIR})
  target_include_directories(
    libtomcrypt
    PRIVATE $<GENEX_EVAL:$<TARGET_PROPERTY:libtommath,INCLUDE_DIRECTORIES>>)
  target_compile_definitions(libtomcrypt PRIVATE LTC_SOURCE LTM_DESC)
  target_compile_options(libtomcrypt PRIVATE -Wno-deprecated-declarations)

endif()
