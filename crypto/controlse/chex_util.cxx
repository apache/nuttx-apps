//***************************************************************************
// apps/crypto/controlse/chex_util.cxx
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

#include "crypto/controlse/chex_util.hxx"
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace Controlse
{

//***************************************************************************
// Class Method Implementations
//***************************************************************************

size_t CHexUtil::GetByteArraySizeFromHexString(const char *hex_buffer)
{
  auto hex_buffer_size = strcspn(hex_buffer, " \r\n");
  return GetByteArraySizeFromHexStringSize(hex_buffer_size);
}

size_t CHexUtil::GetByteArraySizeFromHexStringSize(size_t hex_buffer_size)
{
  return hex_buffer_size / AMOUNT_OF_HEXDIGITS_PER_BYTE;
}

size_t CHexUtil::GetHexStringSizeFromByteArraySize(size_t byte_array_size)
{
  return byte_array_size * AMOUNT_OF_HEXDIGITS_PER_BYTE;
}

uint8_t *CHexUtil::ConvertHexStringToByteArray(const char *hex_buffer)
{
  auto hex_buffer_size = strcspn(hex_buffer, " \r\n");
  if (hex_buffer_size & 1)
    {
      return nullptr;
    }

  return ConvertHexStringToByteArray(hex_buffer, hex_buffer_size);
}

uint8_t *CHexUtil::ConvertHexStringToByteArray(const char *hex_buffer,
                                               size_t hex_buffer_size)
{
  auto bin_buffer
      = new uint8_t[GetByteArraySizeFromHexStringSize(hex_buffer_size)];
  if (bin_buffer)
    {
      size_t hex_buffer_pos;
      size_t bin_buffer_pos = 0;
      for (hex_buffer_pos = 0; (hex_buffer_pos < hex_buffer_size);
           hex_buffer_pos += AMOUNT_OF_HEXDIGITS_PER_BYTE)
        {
          sscanf(&hex_buffer[hex_buffer_pos], "%2hhx",
                 &bin_buffer[bin_buffer_pos]);
          bin_buffer_pos++;
        }
    }
  return bin_buffer;
}

char *CHexUtil::ByteArrayToHexString(const uint8_t bytearray[], size_t size)
{
  auto string = new char[GetHexStringSizeFromByteArraySize(size) + 1];
  if (string)
    {
      char *ptr = string;
      for (size_t i = 0; i < size; i++)
        {
          ptr += sprintf(ptr, "%02x", bytearray[i]);
        }
    }
  return string;
}

} // namespace Controlse
