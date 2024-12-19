//***************************************************************************
// apps/crypto/controlse/csan_builder.cxx
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

#include "crypto/controlse/csan_builder.hxx"
#include "mbedtls/asn1write.h"
#include <cstring>

namespace Controlse
{

//***************************************************************************
// Class Method Implementations
//***************************************************************************

CSanBuilder::~CSanBuilder()
{
  while (entry != nullptr)
    {
      delete[] entry->p;
      auto next_entry = entry->next;
      delete entry;
      entry = next_entry;
    }
}

CSanBuilder *CSanBuilder::AddSan(uint8_t type, const char *value,
                                 size_t value_size)
{
  auto old_entry = entry;
  entry = new List();
  if (entry)
    {
      entry->type = type;
      entry->p = new char[value_size];
      if (entry->p)
        {
          memcpy(entry->p, value, value_size);
          entry->size = value_size;
          entry->next = old_entry;
          total_size += value_size;
          total_size += 2;
        }
      else
        {
          delete entry;
        }
    }
  return this;
}

//*
// result: pointer to dynamically allocated san (to be deleted with delete[])
// or error when size == 0
///
size_t CSanBuilder::Build(uint8_t **san)
{
  if (!entry)
    {
      return 0;
    }

  const size_t reservation_for_additional_length_bytes = 8;
  const size_t buffer_size
      = total_size + reservation_for_additional_length_bytes;
  auto *buffer = new uint8_t[buffer_size];
  auto current = entry;
  const unsigned char *start = buffer;
  unsigned char *p = buffer + buffer_size;
  int ret = 0;
  size_t len = 0;
  while (current)
    {
      ret = mbedtls_asn1_write_tagged_string(&p, start, current->type,
                                             current->p, current->size);
      current = current->next;
    }

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&p, start, total_size));
  MBEDTLS_ASN1_CHK_ADD(
      len, mbedtls_asn1_write_tag(
               &p, start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  *san = new uint8_t[total_size + len];
  memcpy(*san, p, total_size + len);
  delete[] buffer;
  return total_size + len;
}

uint32_t CSanBuilder::GetNumberOfSan()
{
  auto e = entry;
  uint32_t amount = 0;
  while (e != nullptr)
    {
      e = e->next;
      amount++;
    }
  return amount;
}

}
