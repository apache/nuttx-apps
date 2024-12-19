//***************************************************************************
// apps/include/crypto/controlse/chex_util.hxx
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

#include <cstddef>
#include <cstdint>

namespace Controlse
{

//***************************************************************************
// Class definitions
//***************************************************************************

class CHexUtil
{
public:
  static size_t GetByteArraySizeFromHexString(const char *hex_buffer);
  static size_t GetByteArraySizeFromHexStringSize(size_t hex_buffer_size);
  static size_t GetHexStringSizeFromByteArraySize(size_t byte_array_size);

  // result contains allocated pointer to byte array (delete[] afterwards) if
  // successfull otherwise nullptr
  static uint8_t *ConvertHexStringToByteArray(const char *hex_buffer);

  // result contains allocated pointer to byte array (delete[] afterwards) if
  // successfull otherwise nullptr
  static uint8_t *ConvertHexStringToByteArray(const char *hex_buffer,
                                              size_t hex_buffer_size);

  // result contains allocated pointer to hex string (delete[] afterwards) if
  // successfull otherwise nullptr
  static char *ByteArrayToHexString(const uint8_t bytearray[], size_t size);

private:
  static constexpr size_t AMOUNT_OF_HEXDIGITS_PER_BYTE = 2;
};
} // namespace Controlse
