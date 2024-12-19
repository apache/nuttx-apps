//***************************************************************************
// apps/crypto/controlse/cstring.cxx
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

#include "crypto/controlse/cstring.hxx"
#include "crypto/controlse/csecure_element.hxx"
#include <cstring>

namespace Controlse
{

//***************************************************************************
// Class Method Implementations
//***************************************************************************

CString::CString(const ISecureElement &se, uint32_t keystore_id)
{
  (void)LoadFromSecureElement(se, keystore_id);
}

CString::CString(char *string, size_t size) : m_size(size)
{
  m_string = new char[m_size];
  memcpy(m_string, string, m_size);
}

CString::CString(const CString &p1) : CString(p1.m_string, p1.m_size) {}

CString::~CString()
{
  if (m_string != nullptr)
    {
      delete[] m_string;
      m_size = 0;
    }
}

CString &CString::operator=(const CString &other)
{
  if (this != &other)
    {
      auto new_string = new char[other.m_size];
      memcpy(new_string, other.m_string, other.m_size);

      delete[] m_string;

      m_string = new_string;
      m_size = other.m_size;
    }
  return *this;
}

bool CString::operator==(CString &a) const
{
  if (a.m_size != m_size)
    {
      return false;
    }
  return 0 == (memcmp(a.m_string, m_string, m_size));
}

bool CString::operator!=(CString &a) const { return !operator==(a); }

char *CString::c_str(void) const
{
  char *c_str = nullptr;
  if (IsLoaded() && (m_size > 0))
    {
      bool add_termination_character = false;
      if (m_string[m_size - 1] != 0)
        {
          add_termination_character = true;
        }
      c_str = new char[m_size + (add_termination_character ? 1 : 0)];
      memcpy(c_str, m_string, m_size);
      if (add_termination_character)
        {
          c_str[m_size] = 0;
        }
    }
  return c_str;
}

bool CString::IsLoaded() const { return m_string != nullptr; }

bool CString::StoreOnSecureElement(const ISecureElement &se,
                                   uint32_t keystore_id) const
{
  auto result = false;
  if (IsLoaded())
    {
      struct se05x_key_transmission_s args
          = { .entry = { .id = keystore_id },
              .content = { .buffer = reinterpret_cast<uint8_t *>(m_string),
                           .buffer_size = m_size } };
      result = se.SetData(args);
    }
  return result;
}

bool CString::LoadFromSecureElement(const ISecureElement &se,
                                    uint32_t keystore_id)
{
  uint8_t buffer[1000];
  struct se05x_key_transmission_s args
      = { .entry = { .id = keystore_id },
          .content = { .buffer = buffer, .buffer_size = sizeof(buffer) } };
  auto result = se.GetData(args);

  if (result)
    {
      m_size = args.content.buffer_content_size;
      m_string = new char[m_size + 1];
      memcpy(m_string, buffer, m_size);
      m_string[m_size] = 0;
    }
  return result;
}
} // namespace Controlse
