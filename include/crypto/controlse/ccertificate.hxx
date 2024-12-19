//***************************************************************************
// apps/include/crypto/controlse/ccertificate.hxx
//
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2024 NXP
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

#pragma once

//***************************************************************************
// Included Files
//***************************************************************************

#include "crypto/controlse/isecure_element_object.hxx"
#include <mbedtls/x509_crt.h>

//***************************************************************************
// Class definitions
//***************************************************************************

namespace Controlse
{
class ISecureElement;
class CPublicKey;
class CSerialNumber;

class CCertificate : public ISecureElementObject
{
public:
  CCertificate(const ISecureElement &se, uint32_t keystore_id);
  CCertificate(const uint8_t *crt_der_or_pem, size_t crt_size);
  CCertificate(const ISecureElement &se, const uint8_t *csr_der_or_pem,
               size_t csr_size, uint32_t keystore_id);

  // from_datetime and to_datetime need to have format: YYYYMMDDHHMMSSZ
  CCertificate(const ISecureElement &se, const uint8_t *csr_der_or_pem,
               size_t csr_size, uint32_t keystore_id,
               const char *from_datetime, const char *to_datetime);
  CCertificate(const CCertificate &) = delete;
  CCertificate(CCertificate &&) = default;
  ~CCertificate();

  CCertificate &operator=(const CCertificate &other) = delete;

  bool IsLoaded() const;
  bool StoreOnSecureElement(const ISecureElement &se,
                            uint32_t keystore_id) const;
  bool LoadFromSecureElement(const ISecureElement &se, uint32_t keystore_id);
  bool LoadFromDerOrPem(const uint8_t *crt_der_or_pem, size_t crt_size);
  bool LoadFromCsrDerOrPem(const ISecureElement &se,
                           const uint8_t *csr_der_or_pem, size_t csr_size,
                           uint32_t keystore_id, const char *from_datetime,
                           const char *to_datetime);

  bool VerifyAgainst(const ISecureElement &se,
                     uint32_t verify_against_id) const;

  // Test time range is valid
  // returns 0 if valid
  //  -1 when expired
  //  1 when not yet valid
  int TestValidTimerange(time_t now) const;

  // Get public key from certificate
  // returns pointer to public key when successful otherwise NULL
  // note: must be deleted by caller when not NULL
  CPublicKey *GetPublicKey() const;

  // Get oid from certificate if available
  // oid must be one of MBEDTLS_OID_AT* from mbedtls/oid.h
  //
  // returns zero terminated text string when successful otherwise NULL
  // note: must be deleted by caller when not NULL
  char *GetOid(const char *oid) const;

  // Get serial number from from certificate
  // returns pointer to CSerialNumber when successful otherwise NULL
  // note: must be deleted by caller when not NULL
  CSerialNumber *GetSerialNumber() const;

  size_t GetNumberOfSubjectAlternativeNames() const;

  // Get SAN from from certificate
  // returns pointer to array when successful otherwise NULL
  // note: must be deleted by caller when not NULL
  char *GetSubjectAlternativeName(int item) const;

  // Get certificate in DER format
  // returns size of the der array otherwise 0
  // note: der must be deleted by caller when not NULL
  size_t GetDer(uint8_t **der) const;

  // Get certificate in PEM format
  // returns pointer to pem string when successful otherwise NULL
  // note: must be deleted by caller when not NULL
  char *GetPem() const;

  bool ContainsSan(const char *name, size_t size) const;

  static constexpr char TAG_ID_SIZE = 18;

private:
  bool is_loaded = false;

  mbedtls_x509_crt crt;
};
} // namespace Controlse
