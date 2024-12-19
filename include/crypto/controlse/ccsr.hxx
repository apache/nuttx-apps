//***************************************************************************
// apps/include/crypto/controlse/ccsr.hxx
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
//**************************************************************************

#pragma once

//***************************************************************************
// Included Files
//***************************************************************************

#include "crypto/controlse/isecure_element_object.hxx"
#include "mbedtls/x509_csr.h"

namespace Controlse
{

//***************************************************************************
// Class definitions
//***************************************************************************

class CCsr : public ISecureElementObject
{
public:
  class CsrBuilder;
  class CsrBuilder
  {
  public:
    CsrBuilder(ISecureElement &se, const char *subject, uint32_t key_slot_id);
    CsrBuilder(const CsrBuilder &) = delete;
    CsrBuilder(CsrBuilder &&) = default;

    CsrBuilder &operator=(const CsrBuilder &other) = delete;

    CsrBuilder *AddExtension(const char *oid, size_t oid_size,
                             const uint8_t *value, size_t value_size);

    // result: pointer to dynamically allocated Csr (to be deleted) or nullptr
    // if error
    CCsr *Build();

  private:
    mbedtls_x509write_csr csr_w;
    mbedtls_pk_context key;
    bool ready;
  };

  CCsr(const ISecureElement &se, uint32_t keystore_id);
  CCsr(const uint8_t *der_or_pem, size_t size);
  CCsr(const CCsr &) = delete;
  CCsr(CCsr &&) = default;
  ~CCsr();

  CCsr &operator=(const CCsr &other) = delete;

  bool IsLoaded() const;
  bool StoreOnSecureElement(const ISecureElement &se,
                            uint32_t keystore_id) const;
  bool LoadFromSecureElement(const ISecureElement &se, uint32_t keystore_id);

  // Get CSR in DER format
  // returns size of the der array otherwise 0
  // note: der must be deleted by caller when not NULL
  size_t GetDer(uint8_t **der) const;

  // Get certificate in PEM format
  // returns pointer to pem string when successful otherwise NULL
  // note: must be deleted by caller when not NULL
  char *GetPem() const;

private:
  mbedtls_x509_csr csr;
  bool is_loaded = false;
};
} // namespace Controlse
