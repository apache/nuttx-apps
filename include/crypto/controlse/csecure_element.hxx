//***************************************************************************
// apps/include/crypto/controlse/csecure_element.hxx
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

#include "crypto/controlse/isecure_element.hxx"

namespace Controlse
{

//***************************************************************************
// Class definitions
//***************************************************************************

class CCertificate;
class CertificateCatalog;
class CPublicKey;

class CSecureElement : public ISecureElement
{
public:
  explicit CSecureElement(const char *se05x_device);
  explicit CSecureElement(int fd);
  CSecureElement(const CSecureElement &) = delete;
  CSecureElement(CSecureElement &&) = default;
  ~CSecureElement();

  CSecureElement &operator=(const CSecureElement &other) = delete;

  bool IsReady() const;
  bool GenerateKey(struct se05x_generate_keypair_s &args) const;
  bool SetKey(struct se05x_key_transmission_s &args) const;
  bool GetKey(struct se05x_key_transmission_s &args) const;
  bool DeleteKey(uint32_t id) const;
  bool SetData(struct se05x_key_transmission_s &args) const;
  bool GetData(struct se05x_key_transmission_s &args) const;
  bool CreateSignature(struct se05x_signature_s &args) const;
  bool Verify(struct se05x_signature_s &args) const;
  bool DeriveSymmetricalKey(struct se05x_derive_key_s &args) const;
  bool GetUid(struct se05x_uid_s &args) const;
  bool GetInfo(struct se05x_info_s &args) const;

  CCertificate *GetCertificate(uint32_t keystore_id);
  CPublicKey *GetPublicKey(uint32_t keystore_id);

private:
  const int se05x_fd;
  const bool close_device_at_destructor;
};
} // namespace Controlse
