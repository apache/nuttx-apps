//***************************************************************************
// apps/include/crypto/controlse/cpublic_key.hxx
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
#include <stddef.h>

struct mbedtls_x509_crt;

namespace Controlse
{

//***************************************************************************
// Class definitions
//***************************************************************************

class ISecureElement;

class CPublicKey : public ISecureElementObject
{
public:
  CPublicKey(const ISecureElement &se, uint32_t keystore_id);
  CPublicKey(const uint8_t *buffer, size_t buffer_size);
  CPublicKey(const char *pem);
  CPublicKey(const CPublicKey &p1);
  ~CPublicKey();

  CPublicKey &operator=(const CPublicKey &other);

  bool IsLoaded() const;
  bool StoreOnSecureElement(const ISecureElement &se,
                            uint32_t keystore_id) const;
  bool LoadFromSecureElement(const ISecureElement &se, uint32_t keystore_id);
  bool operator==(const CPublicKey &a) const;
  bool operator!=(const CPublicKey &a) const;

  size_t GetRawSize() const;
  void GetRaw(uint8_t *raw_buffer) const;

  // Get public key in PEM format
  // returns pointer to pem string when successful otherwise NULL
  // note: must be deleted by caller when not NULL
  char *GetPem() const;

private:
  void Unload();
  static int convert_public_key_raw_to_pem(char *pem_buf, size_t pem_buf_size,
                                           const uint8_t *key_buf,
                                           size_t key_buf_size);
  static int convert_public_key_pem_to_raw(uint8_t *key_buf,
                                           size_t key_buf_size,
                                           size_t *key_size,
                                           const char *pem_buf);

  uint8_t *m_key = nullptr;
  size_t m_size = 0;
};
} // namespace Controlse
