//***************************************************************************
// apps/include/crypto/controlse/cserial_number.hxx
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

namespace Controlse
{

//***************************************************************************
// Class definitions
//***************************************************************************

class CSerialNumber : ISecureElementObject
{
public:
  static constexpr size_t SERIAL_NUMBER_SIZE = 20;
  CSerialNumber(const ISecureElement &se, uint32_t keystore_id);
  CSerialNumber(uint8_t const *serial_number_byte_array);

  bool operator==(CSerialNumber &a) const;
  bool operator!=(CSerialNumber &a) const;
  bool operator<(const CSerialNumber &a) const;

  bool IsLoaded() const;
  bool StoreOnSecureElement(const ISecureElement &se,
                            uint32_t keystore_id) const;
  bool LoadFromSecureElement(const ISecureElement &se, uint32_t keystore_id);

  bool
  GetSerialNumber(uint8_t serial_number_destination[SERIAL_NUMBER_SIZE]) const;

private:
  bool is_loaded = false;
  uint8_t serial_number[SERIAL_NUMBER_SIZE];
};
} // namespace Controlse
