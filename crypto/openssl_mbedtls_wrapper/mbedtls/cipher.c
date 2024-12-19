/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/cipher.c
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
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/aes.h>
#include <openssl/cipher.h>

#include <mbedtls/aes.h>
#include <mbedtls/cipher.h>
#include <mbedtls/cmac.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int EVP_DecryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER * cipher,
                       ENGINE *impl, const uint8_t *key, const uint8_t *iv)
{
  mbedtls_cipher_context_t *m_ctx = (mbedtls_cipher_context_t *)ctx;
  mbedtls_cipher_init(m_ctx);
  mbedtls_cipher_setup(m_ctx, (const mbedtls_cipher_info_t *)cipher);
  mbedtls_cipher_setkey(m_ctx, key, m_ctx->cipher_info->key_bitlen,
                        MBEDTLS_DECRYPT);
  mbedtls_cipher_set_iv(m_ctx, iv, m_ctx->cipher_info->iv_size);
  return 1;
}

int EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, uint8_t *out, int *out_len,
                      const uint8_t *in, int in_len)
{
  size_t len = 0;
  int ret = mbedtls_cipher_update((mbedtls_cipher_context_t *)ctx,
                                  in, in_len, out, &len);
  *out_len = len;
  return !ret;
}

int EVP_DecryptFinal_ex(EVP_CIPHER_CTX *ctx, uint8_t *out, int *out_len)
{
  size_t len = 0;
  int ret = mbedtls_cipher_finish((mbedtls_cipher_context_t *)ctx,
                                  out, &len);
  *out_len = len;
  return !ret;
}

const EVP_CIPHER *EVP_des_ede(void)
{
  return NULL;
}

const EVP_CIPHER *EVP_des_ede3(void)
{
  return NULL;
}

const EVP_CIPHER *EVP_des_ede_cbc(void)
{
  return NULL;
}

const EVP_CIPHER *EVP_des_ede3_cbc(void)
{
  return NULL;
}

int EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
                       ENGINE *impl, const unsigned char *key,
                       const unsigned char *iv)
{
  mbedtls_cipher_context_t *m_ctx = (mbedtls_cipher_context_t *)ctx;
  mbedtls_cipher_init(m_ctx);
  if (mbedtls_cipher_setup(m_ctx, (const mbedtls_cipher_info_t *)type) != 0)
    {
      mbedtls_cipher_free(m_ctx);
      return 0;
    }

  if (mbedtls_cipher_setkey(m_ctx, key,
                            m_ctx->cipher_info->key_bitlen,
                            MBEDTLS_ENCRYPT) != 0)
    {
      mbedtls_cipher_free(m_ctx);
      return 0;
    }

  if (mbedtls_cipher_set_iv(m_ctx, iv, m_ctx->cipher_info->iv_size) != 0)
    {
      mbedtls_cipher_free(m_ctx);
      return 0;
    }

  return 1;
}

int EVP_EncryptUpdate(EVP_CIPHER_CTX * ctx, uint8_t *out, int *out_len,
                      const uint8_t *in, int in_len)
{
  return !mbedtls_cipher_update((mbedtls_cipher_context_t *)ctx,
                                in, in_len, out, (size_t *)out_len);
}

int EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx,
                        unsigned char *out, int *out_len)
{
  return !mbedtls_cipher_finish((mbedtls_cipher_context_t *)ctx, out,
                                (size_t *)out_len);
}

void AES_cbc_encrypt(const uint8_t *in, uint8_t *out, size_t len,
                     const AES_KEY *key, uint8_t *ivec, const int enc)
{
  const mbedtls_aes_context *ctx = (const mbedtls_aes_context *)key;
  mbedtls_aes_crypt_cbc((mbedtls_aes_context *)ctx, enc, len, ivec, in, out);
}

int EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *ctx, int command,
                        int tag_len, void *tag)
{
  mbedtls_cipher_context_t *m_ctx = (mbedtls_cipher_context_t *)ctx;
  int ret = 0;
  if (command == EVP_CTRL_GCM_GET_TAG)
    {
      ret = mbedtls_cipher_write_tag(m_ctx, (unsigned char *)tag, tag_len);
    }
  else if (command == EVP_CTRL_GCM_SET_TAG)
    {
      ret = mbedtls_cipher_check_tag(m_ctx,
                                    (const unsigned char *)tag,
                                    tag_len);
    }

  return !ret;
}

int AES_set_encrypt_key(const uint8_t *key, unsigned bits, AES_KEY *aeskey)
{
  return -1;
}

int AES_set_decrypt_key(const uint8_t *key, unsigned bits, AES_KEY *aeskey)
{
  return -1;
}

void AES_encrypt(const uint8_t *in, uint8_t *out, const AES_KEY *key)
{
}

void AES_decrypt(const uint8_t *in, uint8_t *out, const AES_KEY *key)
{
}
