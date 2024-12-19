//***************************************************************************
// apps/crypto/controlse/ccertificate.cxx
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
//***************************************************************************

// Copyright 2024 NXP

//***************************************************************************
// Included Files
//***************************************************************************

#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include "crypto/controlse/ccertificate.hxx"

#include "cmbedtls_se05x_extension.hxx"
#include "crypto/controlse/chex_util.hxx"
#include "crypto/controlse/cpublic_key.hxx"
#include "crypto/controlse/cserial_number.hxx"
#include "crypto/controlse/isecure_element.hxx"
#include <mbedtls/oid.h>
#include <mbedtls/pem.h>
#include <mbedtls/sha256.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <string.h>

namespace Controlse
{

//***************************************************************************
// Private Data
//***************************************************************************

static constexpr int SECONDS_IN_DAY = (60 * 60 * 24);
static constexpr size_t TBS_HASH_BUFFER_SIZE = 32;

static constexpr char certificate_header[] = "-----BEGIN CERTIFICATE-----\n";
static constexpr char certificate_footer[] = "-----END CERTIFICATE-----\n";
static constexpr size_t datetime_size = 15;

//***************************************************************************
// Class Method Implementations
//***************************************************************************

CCertificate::CCertificate(const ISecureElement &se, uint32_t keystore_id)
{
  mbedtls_x509_crt_init(&crt);
  is_loaded = LoadFromSecureElement(se, keystore_id);
}

CCertificate::CCertificate(const uint8_t *crt_der_or_pem, size_t crt_size)
{
  mbedtls_x509_crt_init(&crt);
  is_loaded = LoadFromDerOrPem(crt_der_or_pem, crt_size);
}

static void GetCurrentDateTime(char datetime[datetime_size], int seconds)
{
  time_t rawtime;
  struct tm tm_info;
  time(&rawtime);
  rawtime += seconds;
  strftime(datetime, datetime_size, "%Y%m%d%H%M%S",
           gmtime_r(&rawtime, &tm_info));
}

CCertificate::CCertificate(const ISecureElement &se,
                           const uint8_t *csr_der_or_pem, size_t csr_size,
                           uint32_t keystore_id)
{
  mbedtls_x509_crt_init(&crt);
  char from_datetime[datetime_size];
  char to_datetime[datetime_size];
  GetCurrentDateTime(from_datetime, 0);
  GetCurrentDateTime(to_datetime, SECONDS_IN_DAY);
  is_loaded = LoadFromCsrDerOrPem(se, csr_der_or_pem, csr_size, keystore_id,
                                  from_datetime, to_datetime);
}

CCertificate::CCertificate(const ISecureElement &se,
                           const uint8_t *csr_der_or_pem, size_t csr_size,
                           uint32_t keystore_id, const char *from_datetime,
                           const char *to_datetime)
{
  mbedtls_x509_crt_init(&crt);
  is_loaded = LoadFromCsrDerOrPem(se, csr_der_or_pem, csr_size, keystore_id,
                                  from_datetime, to_datetime);
}

CCertificate::~CCertificate() { mbedtls_x509_crt_free(&crt); }

bool CCertificate::IsLoaded() const { return is_loaded; }

bool CCertificate::StoreOnSecureElement(const ISecureElement &se,
                                        uint32_t keystore_id) const
{
  struct se05x_key_transmission_s args;
  args.entry.id = keystore_id;
  args.content.buffer = crt.raw.p;
  args.content.buffer_size = crt.raw.len;
  args.content.buffer_content_size = crt.raw.len;
  return se.SetData(args);
}

bool CCertificate::LoadFromSecureElement(const ISecureElement &se,
                                         uint32_t keystore_id)
{
  size_t certificate_der_size = 1000;
  uint8_t *certificate_der = new uint8_t[certificate_der_size];
  struct se05x_key_transmission_s args
      = { .entry = { .id = keystore_id },
          .content = { .buffer = certificate_der,
                       .buffer_size = certificate_der_size } };
  bool result = se.GetData(args);

  if (result)
    {
      result = 0
               == mbedtls_x509_crt_parse(&crt, certificate_der,
                                         args.content.buffer_content_size);
    }

  delete[] certificate_der;
  return result;
}

bool CCertificate::LoadFromDerOrPem(const uint8_t *crt_der_or_pem,
                                    size_t crt_size)
{
  return 0 == mbedtls_x509_crt_parse(&crt, crt_der_or_pem, crt_size);
}

bool CCertificate::LoadFromCsrDerOrPem(const ISecureElement &se,
                                       const uint8_t *csr_der_or_pem,
                                       size_t csr_size, uint32_t keystore_id,
                                       const char *from_datetime,
                                       const char *to_datetime)
{
  mbedtls_x509_csr csr;
  mbedtls_x509_csr_init(&csr);
  auto result = 0 == mbedtls_x509_csr_parse(&csr, csr_der_or_pem, csr_size);

  mbedtls_x509write_cert crt_builder;
  mbedtls_x509write_crt_init(&crt_builder);
  char subject_name[200];
  if (result)
    {
      mbedtls_x509write_crt_set_version(&crt_builder,
                                        MBEDTLS_X509_CRT_VERSION_3);
      result = 0 < mbedtls_x509_dn_gets(subject_name, sizeof(subject_name),
                                        &csr.subject);
    }

  mbedtls_pk_context key;
  mbedtls_pk_init(&key);
  if (result)
    {
      result = 0
               == MbedtlsSe05xExtension::mbedtls_pk_setup_key_se05x(
                   key, se, keystore_id);
    }

  mbedtls_pk_context public_key;
  public_key.pk_ctx = csr.pk.pk_ctx;
  public_key.pk_info = csr.pk.pk_info;

  // Invalidate the public key in CSR
  // The public key is transferred to CRT so the CSR may not free the memory
  csr.pk.pk_ctx = nullptr;
  csr.pk.pk_info = nullptr;
  mbedtls_x509_csr_free(&csr);

  if (result)
    {
      mbedtls_x509write_crt_set_subject_key(&crt_builder, &public_key);
      mbedtls_x509write_crt_set_issuer_key(&crt_builder, &key);
      result = 0
               == mbedtls_x509write_crt_set_subject_name(&crt_builder,
                                                         subject_name);
    }

  if (result)
    {
      result = 0
               == mbedtls_x509write_crt_set_issuer_name(&crt_builder,
                                                  "CN=CA,O=controlse,C=NL");
    }

  mbedtls_mpi serial;
  mbedtls_mpi_init(&serial);
  if (result)
    {
      mbedtls_x509write_crt_set_md_alg(&crt_builder, MBEDTLS_MD_SHA256);
      result = 0 == mbedtls_mpi_read_string(&serial, 10, "1");
    }

  if (result)
    {
      result = 0 == mbedtls_x509write_crt_set_serial(&crt_builder, &serial);
    }

  if (result)
    {
      result = 0
               == mbedtls_x509write_crt_set_validity(
                   &crt_builder, from_datetime, to_datetime);
    }

  size_t buf_size = 1000;
  uint8_t *buf = nullptr;
  if (result)
    {
      buf = new uint8_t[buf_size];
      result = buf != nullptr;
    }

  size_t buf_content_size;
  if (result)
    {
      auto der_result
          = mbedtls_x509write_crt_der(&crt_builder, buf, buf_size, nullptr, 0);
      buf_content_size = der_result;
      result = 0 < der_result;
    }

  mbedtls_x509write_crt_free(&crt_builder);
  MbedtlsSe05xExtension::mbedtls_pk_free_se05x(key);
  mbedtls_pk_free(&key);
  mbedtls_pk_free(&public_key);
  mbedtls_mpi_free(&serial);

  if (result)
    {
      result = LoadFromDerOrPem(buf + buf_size - buf_content_size,
                                buf_content_size);
    }

  if (buf)
    {
      delete[] buf;
    }
  return result;
}

bool CCertificate::VerifyAgainst(const ISecureElement &se,
                                 uint32_t verify_against_id) const
{
  if (!is_loaded)
    {
      return false;
    }

  uint8_t tbs_buffer[TBS_HASH_BUFFER_SIZE];
  bool result = 0 == mbedtls_sha256(crt.tbs.p, crt.tbs.len, tbs_buffer, 0);

  if (result)
    {
      struct se05x_signature_s verify_args = {
        .key_id = verify_against_id,
        .algorithm = SE05X_ALGORITHM_SHA256,
        .tbs = { .buffer = tbs_buffer,
                 .buffer_size = sizeof(tbs_buffer),
                 .buffer_content_size = sizeof(tbs_buffer) },
        .signature = { .buffer = crt.sig.p,
                       .buffer_size = crt.sig.len,
                       .buffer_content_size = crt.sig.len },
      };
      result = se.Verify(verify_args);
    }

  return result;
}

static bool check_time(const mbedtls_x509_time *before,
                       const mbedtls_x509_time *after)
{
  if (before->year > after->year)
    return false;

  if (before->year == after->year && before->mon > after->mon)
    return false;

  if (before->year == after->year && before->mon == after->mon
      && before->day > after->day)
    return false;

  if (before->year == after->year && before->mon == after->mon
      && before->day == after->day && before->hour > after->hour)
    return false;

  if (before->year == after->year && before->mon == after->mon
      && before->day == after->day && before->hour == after->hour
      && before->min > after->min)
    return false;

  if (before->year == after->year && before->mon == after->mon
      && before->day == after->day && before->hour == after->hour
      && before->min == after->min && before->sec > after->sec)
    return false;

  return true;
}

int CCertificate::TestValidTimerange(time_t now) const
{

  auto lt = gmtime(&now);

  mbedtls_x509_time mbedtls_now;
  mbedtls_now.year = lt->tm_year + 1900;
  mbedtls_now.mon = lt->tm_mon + 1;
  mbedtls_now.day = lt->tm_mday;
  mbedtls_now.hour = lt->tm_hour;
  mbedtls_now.min = lt->tm_min;
  mbedtls_now.sec = lt->tm_sec;

  if (!check_time(&mbedtls_now, &crt.valid_to))
    {
      return -1;
    }
  if (!check_time(&crt.valid_from, &mbedtls_now))
    {
      return 1;
    }

  return 0;
}

CPublicKey *CCertificate::GetPublicKey() const
{
  if (!is_loaded)
    {
      return nullptr;
    }

  size_t root_key_buf_size = 100;
  uint8_t *root_key_buf = new uint8_t[root_key_buf_size];

  bool result = false;
  if (root_key_buf)
    {
      mbedtls_ecp_keypair *keypair
          = reinterpret_cast<mbedtls_ecp_keypair *>(crt.pk.pk_ctx);
      result = 0
               == mbedtls_ecp_point_write_binary(
                   &keypair->grp, &keypair->Q, MBEDTLS_ECP_PF_UNCOMPRESSED,
                   &root_key_buf_size, root_key_buf, root_key_buf_size);
    }

  CPublicKey *public_key
      = result ? new CPublicKey(root_key_buf, root_key_buf_size) : nullptr;

  if (root_key_buf)
    {
      delete[] root_key_buf;
    }

  return public_key;
}

char *CCertificate::GetOid(const char *oid) const
{
  if (!is_loaded)
    {
      return nullptr;
    }
  auto item = mbedtls_asn1_find_named_data(&crt.subject, oid, 3);

  if (item)
    {
      auto data = new char[item->val.len + 1];
      memcpy(data, item->val.p, item->val.len);
      data[item->val.len] = 0;
      return data;
    }
  return nullptr;
}

CSerialNumber *CCertificate::GetSerialNumber() const
{
  if (!is_loaded)
    {
      return nullptr;
    }

  if (crt.serial.len == CSerialNumber::SERIAL_NUMBER_SIZE)
    {
      auto serial = new CSerialNumber(crt.serial.p);
      return serial;
    }
  return nullptr;
}

size_t CCertificate::GetNumberOfSubjectAlternativeNames() const
{
  if (!is_loaded)
    {
      return 0;
    }
  size_t size = 0;
  auto current = &crt.subject_alt_names;

  do
    {
      if (current->buf.tag
          == (MBEDTLS_ASN1_CONTEXT_SPECIFIC
              | MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER))
        {
          size++;
        }
      current = current->next;
    }
  while (current);
  return size;
}

char *CCertificate::GetSubjectAlternativeName(int item) const
{
  if (!is_loaded)
    {
      return nullptr;
    }

  auto current = &crt.subject_alt_names;

  // go to first uri
  while (current->buf.tag
         != (MBEDTLS_ASN1_CONTEXT_SPECIFIC
             | MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER))
    {
      current = current->next;
      if (current == nullptr)
        {
          return nullptr;
        }
    }

  // go the next uri until we get to the correct item
  for (int i = 0; i < item; i++)
    {
      do
        {
          current = current->next;
          if (current == nullptr)
            {
              return nullptr;
            }
        }
      while (current->buf.tag
             != (MBEDTLS_ASN1_CONTEXT_SPECIFIC
                 | MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER));
    }

  auto san = new char[current->buf.len + 1];
  if (san)
    {
      memcpy(san, current->buf.p, current->buf.len);
      san[current->buf.len] = 0;
    }
  return san;
}

size_t CCertificate::GetDer(uint8_t **der) const
{
  if (!is_loaded)
    {
      return 0;
    }
  *der = new uint8_t[crt.raw.len];
  memcpy(*der, crt.raw.p, crt.raw.len);
  return crt.raw.len;
}

char *CCertificate::GetPem() const
{
  if (!is_loaded)
    {
      return 0;
    }

  char pem_buf[1000];
  size_t pem_content_size;
  auto result = mbedtls_pem_write_buffer(
      certificate_header, certificate_footer, crt.raw.p, crt.raw.len,
      (uint8_t *)pem_buf, sizeof(pem_buf), &pem_content_size);
  if (result != 0)
    {
      return nullptr;
    }

  auto pem = new char[pem_content_size + 1];
  memcpy(pem, pem_buf, pem_content_size);
  pem[pem_content_size] = 0;
  return pem;
}

bool CCertificate::ContainsSan(const char *name, size_t size) const
{
  auto san_amount = GetNumberOfSubjectAlternativeNames();
  for (size_t i = 0; i < san_amount; i++)
    {
      auto san = GetSubjectAlternativeName(i);
      if (!san)
        {
          return false;
        }
      auto found = (memcmp(name, san, size) == 0);
      delete[] san;
      if (found)
        {
          return true;
        }
    }
  return false;
}

} // namespace Controlse
