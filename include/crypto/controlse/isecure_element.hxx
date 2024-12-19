//***************************************************************************
// apps/include/crypto/controlse/isecure_element.hxx
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

//***************************************************************************
// Included Files
//***************************************************************************

#pragma once

#include "crypto/controlse/isecure_element_object.hxx"
#include <nuttx/crypto/se05x.h>
#include <stdint.h>

struct se05x_key_transmission_s;
struct se05x_signature_s;
struct se05x_uid_s;
struct se05x_info_s;
struct se05x_generate_keypair_s;
struct se05x_derive_key_s;

namespace Controlse
{

//***************************************************************************
// Class definitions
//***************************************************************************

class ISecureElement
{
public:
  virtual ~ISecureElement() = default;

  virtual bool IsReady() const = 0;
  virtual bool GenerateKey(struct se05x_generate_keypair_s &args) const = 0;
  virtual bool SetKey(struct se05x_key_transmission_s &args) const = 0;
  virtual bool GetKey(struct se05x_key_transmission_s &args) const = 0;
  virtual bool DeleteKey(uint32_t id) const = 0;
  virtual bool SetData(struct se05x_key_transmission_s &args) const = 0;
  virtual bool GetData(struct se05x_key_transmission_s &args) const = 0;
  virtual bool CreateSignature(struct se05x_signature_s &args) const = 0;
  virtual bool Verify(struct se05x_signature_s &args) const = 0;
  virtual bool DeriveSymmetricalKey(struct se05x_derive_key_s &args) const = 0;
  virtual bool GetUid(struct se05x_uid_s &args) const = 0;
  virtual bool GetInfo(struct se05x_info_s &args) const = 0;

  virtual bool Set(uint32_t keystore_id,
                   const ISecureElementObject &object) const
  {
    return object.StoreOnSecureElement(*this, keystore_id);
  }
};
} // namespace Controlse
