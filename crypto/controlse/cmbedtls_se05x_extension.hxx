//***************************************************************************
// apps/crypto/controlse/cmbedtls_se05x_extensions.hxx
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
//**************************************************************************

// Copyright 2024 NXP

#pragma once

//***************************************************************************
// Included Files
//***************************************************************************

#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include "crypto/controlse/cpublic_key.hxx"
#include "crypto/controlse/isecure_element.hxx"
#include <../crypto/mbedtls/mbedtls/library/pk_wrap.h>
#include <cerrno>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_csr.h>
#include <nuttx/crypto/se05x.h>

using Controlse::ISecureElement;

struct mbedtls_se05x_ctx
{
  const ISecureElement *se;
  uint32_t private_key_slot_id;
};

namespace Controlse
{

//***************************************************************************
// Class definitions
//***************************************************************************

class MbedtlsSe05xExtension
{
public:
  static int mbedtls_pk_parse_se05x_public_key(mbedtls_pk_context &key,
                                               CPublicKey &se05x_key)
  {
    mbedtls_ecp_keypair *keypair;
    uint8_t *key_buffer = nullptr;
    size_t key_buffer_size = 0;
    if (se05x_key.IsLoaded())
      {
        key_buffer_size = se05x_key.GetRawSize();
        key_buffer = new uint8_t[key_buffer_size];
        keypair = (mbedtls_ecp_keypair *)key.pk_ctx;
      }

    int result = -ENOMEM;
    if (key_buffer)
      {
        se05x_key.GetRaw(key_buffer);
        result
            = mbedtls_ecp_group_load(&keypair->grp, MBEDTLS_ECP_DP_SECP256R1);
      }

    if (result == 0)
      {
        result = mbedtls_ecp_point_read_binary(&keypair->grp, &keypair->Q,
                                               key_buffer, key_buffer_size);
      }

    if (key_buffer)
      {
        delete[] key_buffer;
      }

    return result;
  }

  static int ecdsa_sign_wrap(mbedtls_pk_context *pk, mbedtls_md_type_t md_alg,
                             const unsigned char *hash, size_t hash_len,
                             unsigned char *sig, size_t sig_size,
                             size_t *sig_len,
                             int (*f_rng)(void *, unsigned char *, size_t),
                             void *p_rng)
  {

    mbedtls_ecp_keypair *key =
        reinterpret_cast<mbedtls_ecp_keypair *>(pk->pk_ctx);
    auto se05x_ctx = reinterpret_cast<mbedtls_se05x_ctx *>(key->d.p);

    struct se05x_signature_s args
        = { se05x_ctx->private_key_slot_id,
            SE05X_ALGORITHM_SHA256,
            { const_cast<uint8_t *>(hash), hash_len, hash_len },
            { sig, sig_size, sig_size } };
    auto result = se05x_ctx->se->CreateSignature(args);

    if (result)
      {
        *sig_len = args.signature.buffer_content_size;
      }

    return result ? 0 : -EPERM;
  }

  // return value is allocated ptr to mbedtls_pk_info_t (need to be deleted)
  // otherwise nulltpr
  static mbedtls_pk_info_t *CreatePkInfoSe05x()
  {
    auto info = mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY);
    auto se05x_ec_info = new mbedtls_pk_info_t;
    if (se05x_ec_info)
      {
        *se05x_ec_info = *info;
        se05x_ec_info->sign_func = ecdsa_sign_wrap;
      }
    return se05x_ec_info;
  }

  static int mbedtls_pk_setup_key_se05x(mbedtls_pk_context &key,
                                        const ISecureElement &se,
                                        uint32_t key_slot_id)
  {
    auto se05x_ec_info = CreatePkInfoSe05x();
    if (se05x_ec_info)
      {
        key.pk_info = se05x_ec_info;
        key.pk_ctx = se05x_ec_info->ctx_alloc_func();
      }

    int result = -ENOMEM;
    if (key.pk_ctx)
      {
        auto se05x_key = CPublicKey(se, key_slot_id);
        result = mbedtls_pk_parse_se05x_public_key(key, se05x_key);
      }

    mbedtls_se05x_ctx *se05x_ctx = nullptr;
    if (result == 0)
      {
        se05x_ctx = new mbedtls_se05x_ctx;
        result = se05x_ctx ? 0 : -ENOMEM;
      }

    if (result == 0)
      {
        se05x_ctx->private_key_slot_id = key_slot_id;
        se05x_ctx->se = &se;
      }

    ((mbedtls_ecp_keypair *)key.pk_ctx)->d.p
        = reinterpret_cast<mbedtls_mpi_uint *>(se05x_ctx);
    return result;
  }

  static void mbedtls_pk_free_se05x(mbedtls_pk_context &key)
  {
    auto key_ctx = reinterpret_cast<mbedtls_ecp_keypair *>(key.pk_ctx);
    if (key_ctx)
      {
        auto se05x_ctx = reinterpret_cast<mbedtls_se05x_ctx *>(key_ctx->d.p);
        if (se05x_ctx)
          {
            delete se05x_ctx;
            key_ctx->d.p = nullptr;
          }
      }
    auto pk_info = key.pk_info;
    if (pk_info)
      {
        mbedtls_pk_free(&key);
        delete pk_info;
      }
  }
};
} // namespace Controlse
