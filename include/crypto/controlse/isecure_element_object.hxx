//***************************************************************************
// apps/include/crypto/controlse/isecure_element_object.hxx
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

#include <stdint.h>

namespace Controlse
{

//***************************************************************************
// Class definitions
//***************************************************************************

class ISecureElement;

class ISecureElementObject
{
public:
  virtual ~ISecureElementObject() = default;
  virtual bool IsLoaded() const = 0;
  virtual bool StoreOnSecureElement(const ISecureElement &se,
                                    uint32_t keystore_id) const = 0;
  virtual bool LoadFromSecureElement(const ISecureElement &se,
                                     uint32_t keystore_id)
      = 0;
};
} // namespace Controlse
