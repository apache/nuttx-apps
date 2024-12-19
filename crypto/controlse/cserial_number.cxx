//***************************************************************************
// apps/crypto/controlse/cserial_number.cxx
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

#include "crypto/controlse/cserial_number.hxx"
#include "crypto/controlse/isecure_element.hxx"
#include <string.h>

namespace Controlse
{
constexpr size_t CSerialNumber::SERIAL_NUMBER_SIZE;

//***************************************************************************
// Class Method Implementations
//***************************************************************************

CSerialNumber::CSerialNumber(const ISecureElement &se, uint32_t keystore_id)
{
  is_loaded = LoadFromSecureElement(se, keystore_id);
}

CSerialNumber::CSerialNumber(uint8_t const *serial_number_byte_array)
{
  memcpy(serial_number, serial_number_byte_array, SERIAL_NUMBER_SIZE);
  is_loaded = true;
}

bool CSerialNumber::operator==(CSerialNumber &a) const
{
  return 0 == (memcmp(a.serial_number, serial_number, SERIAL_NUMBER_SIZE));
}

bool CSerialNumber::operator!=(CSerialNumber &a) const
{
  return !operator==(a);
}

bool CSerialNumber::operator<(const CSerialNumber &a) const
{
  return 0 < (memcmp(a.serial_number, serial_number, SERIAL_NUMBER_SIZE));
}

bool CSerialNumber::IsLoaded() const { return is_loaded; }

bool CSerialNumber::GetSerialNumber(
    uint8_t serial_number_destination[SERIAL_NUMBER_SIZE]) const
{
  if (!is_loaded)
    {
      return false;
    }

  memcpy(serial_number_destination, serial_number, SERIAL_NUMBER_SIZE);
  return true;
}

bool CSerialNumber::StoreOnSecureElement(const ISecureElement &se,
                                         uint32_t keystore_id) const
{
  return false;
}

bool CSerialNumber::LoadFromSecureElement(const ISecureElement &se,
                                          uint32_t keystore_id)
{
  struct se05x_key_transmission_s args
      = { .entry = { .id = keystore_id },
          .content
          = { .buffer = serial_number, .buffer_size = SERIAL_NUMBER_SIZE } };
  return se.GetData(args);
}
} // namespace Controlse
