//***************************************************************************
// apps/crypto/controlse/cpublic_key.cxx
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

//***************************************************************************
// Included Files
//***************************************************************************

#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include "crypto/controlse/cpublic_key.hxx"
#include "crypto/controlse/isecure_element.hxx"
#include "crypto/controlse/isecure_element_object.hxx"
#include <errno.h>
#include <mbedtls/pk.h>
#include <string.h>

namespace Controlse
{

//***************************************************************************
// Class Method Implementations
//***************************************************************************

CPublicKey::CPublicKey(const ISecureElement &se, uint32_t keystore_id)
{
  (void)LoadFromSecureElement(se, keystore_id);
}

CPublicKey::CPublicKey(const char *pem)
{
  uint8_t key_buf[300];
  m_key = nullptr;
  if (0
      == convert_public_key_pem_to_raw(key_buf, sizeof(key_buf), &m_size, pem))
    {
      m_key = new uint8_t[m_size];
      memcpy(m_key, key_buf, m_size);
    }
}

CPublicKey::CPublicKey(const uint8_t *buffer, size_t buffer_size)
    : m_key(new uint8_t[buffer_size]), m_size(buffer_size)
{
  memcpy(m_key, buffer, m_size);
}

CPublicKey::CPublicKey(const CPublicKey &p1) : CPublicKey(p1.m_key, p1.m_size)
{
}

CPublicKey::~CPublicKey() { Unload(); }

CPublicKey &CPublicKey::operator=(const CPublicKey &other)
{
  if (this != &other)
    {
      auto new_key = new uint8_t[other.m_size];
      memcpy(new_key, other.m_key, other.m_size);

      delete[] m_key;

      m_key = new_key;
      m_size = other.m_size;
    }
  return *this;
}

void CPublicKey::Unload()
{
  if (IsLoaded())
    {
      delete[] m_key;
      m_key = NULL;
      m_size = 0;
    }
}

bool CPublicKey::IsLoaded() const { return m_key != NULL; }

bool CPublicKey::StoreOnSecureElement(const ISecureElement &se,
                                      uint32_t keystore_id) const
{
  if (!IsLoaded())
    {
      return false;
    }

  struct se05x_key_transmission_s args = {
    .entry = { .id = keystore_id, .cipher = SE05X_ASYM_CIPHER_EC_NIST_P_256 },
    .content
    = { .buffer = m_key, .buffer_size = m_size, .buffer_content_size = m_size }
  };
  return se.SetKey(args);
}

bool CPublicKey::LoadFromSecureElement(const ISecureElement &se,
                                       uint32_t keystore_id)
{
  Unload();

  m_size = 100;
  m_key = new uint8_t[m_size];
  struct se05x_key_transmission_s args = {
    .entry = { .id = keystore_id, .cipher = SE05X_ASYM_CIPHER_EC_NIST_P_256 },
    .content
    = { .buffer = m_key, .buffer_size = m_size, .buffer_content_size = m_size }
  };

  bool result = se.GetKey(args);

  m_size = args.content.buffer_content_size;
  if (!result)
    {
      Unload();
    }

  return result;
}

bool CPublicKey::operator==(const CPublicKey &a) const
{
  if (this->m_size != a.m_size)
    {
      return false;
    }
  return 0 == memcmp(this->m_key, a.m_key, m_size);
}

bool CPublicKey::operator!=(const CPublicKey &a) const
{
  return !operator==(a);
}

size_t CPublicKey::GetRawSize() const { return m_size; }

void CPublicKey::GetRaw(uint8_t *raw_buffer) const
{
  memcpy(raw_buffer, m_key, m_size);
}

char *CPublicKey::GetPem() const
{
  char pem_buf[500];
  auto res
      = convert_public_key_raw_to_pem(pem_buf, sizeof(pem_buf), m_key, m_size);
  if (res < 0)
    {
      return nullptr;
    }
  auto pem_size = strlen(pem_buf);
  auto pem = new char[pem_size + 1];
  memcpy(pem, pem_buf, pem_size);
  pem[pem_size] = 0;
  return pem;
}

int CPublicKey::convert_public_key_raw_to_pem(char *pem_buf,
                                              size_t pem_buf_size,
                                              const uint8_t *key_buf,
                                              size_t key_buf_size)
{
  mbedtls_pk_context key = { 0 };

  mbedtls_pk_init(&key);
  const mbedtls_pk_info_t *info = mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY);

  int result = -1;
  if (info != NULL)
    {
      result = mbedtls_pk_setup(&key, info);
    }

  mbedtls_ecp_keypair *keypair = (mbedtls_ecp_keypair *)key.pk_ctx;
  if (result == 0)
    {
      result = mbedtls_ecp_group_load(&keypair->grp, MBEDTLS_ECP_DP_SECP256R1);
    }

  if (result == 0)
    {
      result = mbedtls_ecp_point_read_binary(&keypair->grp, &keypair->Q,
                                             key_buf, key_buf_size);
    }

  if (result == 0)
    {
      result = mbedtls_pk_write_pubkey_pem(&key, (uint8_t *)pem_buf,
                                           pem_buf_size);
    }

  mbedtls_pk_free(&key);
  return result < 0 ? -EINVAL : 0;
}

int CPublicKey::convert_public_key_pem_to_raw(uint8_t *key_buf,
                                              size_t key_buf_size,
                                              size_t *key_size,
                                              const char *pem_buf)
{
  int result = -1;
  mbedtls_pk_context key = { 0 };

  mbedtls_pk_init(&key);

  result = mbedtls_pk_parse_public_key(&key, (uint8_t *)pem_buf,
                                       strlen(pem_buf) + 1);

  if (result == 0)
    {
      result = mbedtls_pk_can_do(&key, MBEDTLS_PK_ECKEY) == 1 ? 0 : -1;
    }

  if (result == 0)
    {
      mbedtls_ecp_keypair *keypair = (mbedtls_ecp_keypair *)key.pk_ctx;
      result = mbedtls_ecp_point_write_binary(&keypair->grp, &keypair->Q,
                                              MBEDTLS_ECP_PF_UNCOMPRESSED,
                                              key_size, key_buf, key_buf_size);
    }

  mbedtls_pk_free(&key);
  return result < 0 ? -EINVAL : 0;
}

} // namespace Controlse
