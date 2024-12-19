//***************************************************************************
// apps/crypto/controlse/ccsr.cxx
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
#include "crypto/controlse/ccsr.hxx"

#include "cmbedtls_se05x_extension.hxx"
#include "crypto/controlse/cpublic_key.hxx"
#include "crypto/controlse/csecure_element.hxx"
#include <cstring>
#include <mbedtls/pem.h>

using Controlse::MbedtlsSe05xExtension;

namespace Controlse
{

//***************************************************************************
// Private Data
//***************************************************************************

static const char certificate_request_header[]
    = "-----BEGIN CERTIFICATE REQUEST-----\n";
static const char certificate_request_footer[]
    = "-----END CERTIFICATE REQUEST-----\n";

//***************************************************************************
// Class Method Implementations
//***************************************************************************

CCsr::CCsr(const ISecureElement &se, uint32_t keystore_id)
{
  mbedtls_x509_csr_init(&csr);
  is_loaded = LoadFromSecureElement(se, keystore_id);
}

CCsr::CCsr(const uint8_t *der_or_pem, const size_t size)
{
  mbedtls_x509_csr_init(&csr);
  is_loaded = 0 == mbedtls_x509_csr_parse(&csr, der_or_pem, size);
}

CCsr::~CCsr() { mbedtls_x509_csr_free(&csr); }

bool CCsr::IsLoaded() const { return is_loaded; }

bool CCsr::StoreOnSecureElement(const ISecureElement &se,
                                uint32_t keystore_id) const
{
  struct se05x_key_transmission_s args;
  args.entry.id = keystore_id;
  args.content.buffer = csr.raw.p;
  args.content.buffer_size = csr.raw.len;
  args.content.buffer_content_size = csr.raw.len;
  return se.SetData(args);
}

bool CCsr::LoadFromSecureElement(const ISecureElement &se,
                                 uint32_t keystore_id)
{
  size_t csr_der_size = 1000;
  uint8_t *csr_der = new uint8_t[csr_der_size];
  struct se05x_key_transmission_s args
      = { .entry = { .id = keystore_id },
          .content = { .buffer = csr_der, .buffer_size = csr_der_size } };
  bool result = se.GetData(args);

  if (result)
    {
      result = 0
               == mbedtls_x509_csr_parse(&csr, csr_der,
                                         args.content.buffer_content_size);
    }

  delete[] csr_der;
  return result;
}

size_t CCsr::GetDer(uint8_t **der) const
{
  if (!IsLoaded())
    {
      return 0;
    }
  *der = new uint8_t[csr.raw.len];
  memcpy(*der, csr.raw.p, csr.raw.len);
  return csr.raw.len;
}

char *CCsr::GetPem() const
{
  if (!IsLoaded())
    {
      return nullptr;
    }

  char pem_buf[1000];
  size_t pem_content_size;
  auto result = mbedtls_pem_write_buffer(
      certificate_request_header, certificate_request_footer, csr.raw.p,
      csr.raw.len, (uint8_t *)pem_buf, sizeof(pem_buf), &pem_content_size);
  if (result != 0)
    {
      return nullptr;
    }

  auto pem = new char[pem_content_size + 1];
  memcpy(pem, pem_buf, pem_content_size);
  pem[pem_content_size] = 0;
  return pem;
}

CCsr::CsrBuilder::CsrBuilder(ISecureElement &se, const char *subject,
                             uint32_t key_slot_id)
{
  mbedtls_x509write_csr_init(&csr_w);
  mbedtls_pk_init(&key);

  ready = 0
          == MbedtlsSe05xExtension::mbedtls_pk_setup_key_se05x(key, se,
                                                               key_slot_id);

  if (ready)
    {
      mbedtls_x509write_csr_set_key(&csr_w, &key);
      ready = 0 == mbedtls_x509write_csr_set_subject_name(&csr_w, subject);
    }

  if (ready)
    {
      mbedtls_x509write_csr_set_md_alg(&csr_w, MBEDTLS_MD_SHA256);
      ready = 0
              == mbedtls_x509write_csr_set_key_usage(
                  &csr_w, MBEDTLS_X509_KU_KEY_CERT_SIGN);
    }
}

CCsr::CsrBuilder *CCsr::CsrBuilder::AddExtension(const char *oid,
                                                 size_t oid_size,
                                                 const uint8_t *value,
                                                 size_t value_size)
{
  if (ready)
    {
      ready = 0
              == mbedtls_x509write_csr_set_extension(&csr_w, oid, oid_size, 0,
                                                     value, value_size);
    }

  return this;
}

CCsr *CCsr::CsrBuilder::Build()
{
  size_t buf_size = 1000;
  uint8_t *buf = nullptr;
  if (ready)
    {
      buf = new uint8_t[buf_size];
      ready = nullptr != buf;
    }

  int write_result_size = 0;
  if (ready)
    {
      write_result_size
          = mbedtls_x509write_csr_der(&csr_w, buf, buf_size, nullptr, 0);
      ready = write_result_size > 0;
    }

  MbedtlsSe05xExtension::mbedtls_pk_free_se05x(key);
  mbedtls_x509write_csr_free(&csr_w);

  CCsr *result_csr = nullptr;
  if (ready)
    {
      result_csr
          = new CCsr(buf + buf_size - write_result_size, write_result_size);
    }

  if (buf)
    {
      delete[] buf;
    }

  return result_csr;
}

} // namespace Controlse
