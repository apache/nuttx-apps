//***************************************************************************
// apps/crypto/controlse/csecure_element.cxx
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

#include "crypto/controlse/csecure_element.hxx"

#include "crypto/controlse/ccertificate.hxx"
#include "crypto/controlse/cpublic_key.hxx"
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
extern "C"
{
#include <nuttx/crypto/se05x.h>
}

namespace Controlse
{

//***************************************************************************
// Class Method Implementations
//***************************************************************************

CSecureElement::CSecureElement(const char *se05x_device)
    : se05x_fd(open(se05x_device, O_RDONLY)), close_device_at_destructor(true)
{
}

CSecureElement::CSecureElement(int fd)
    : se05x_fd(fd), close_device_at_destructor(false)
{
}

CSecureElement::~CSecureElement()
{
  if ((se05x_fd >= 0) && close_device_at_destructor)
    {
      close(se05x_fd);
    }
}

bool CSecureElement::IsReady() const { return se05x_fd >= 0; }

bool CSecureElement::GenerateKey(struct se05x_generate_keypair_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_GENERATE_KEYPAIR, &args);
    }
  return result;
}

bool CSecureElement::SetKey(struct se05x_key_transmission_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_SET_KEY, &args);
    }
  return result;
}

bool CSecureElement::GetKey(struct se05x_key_transmission_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_GET_KEY, &args);
    }
  return result;
}

bool CSecureElement::DeleteKey(uint32_t id) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_DELETE_KEY, id);
    }
  return result;
}

bool CSecureElement::SetData(struct se05x_key_transmission_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_SET_DATA, &args);
    }
  return result;
}

bool CSecureElement::GetData(struct se05x_key_transmission_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_GET_DATA, &args);
    }
  return result;
}

bool CSecureElement::CreateSignature(struct se05x_signature_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_CREATE_SIGNATURE, &args);
    }
  return result;
}

bool CSecureElement::Verify(struct se05x_signature_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_VERIFY_SIGNATURE, &args);
    }
  return result;
}

bool CSecureElement::DeriveSymmetricalKey(
    struct se05x_derive_key_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_DERIVE_SYMM_KEY, &args);
    }
  return result;
}

bool CSecureElement::GetUid(struct se05x_uid_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_GET_UID, &args);
    }
  return result;
}

bool CSecureElement::GetInfo(struct se05x_info_s &args) const
{
  bool result = false;
  if (se05x_fd >= 0)
    {
      result = 0 == ioctl(se05x_fd, SEIOC_GET_INFO, &args);
    }
  return result;
}

CCertificate *CSecureElement::GetCertificate(uint32_t keystore_id)
{
  return new CCertificate(*this, keystore_id);
}

CPublicKey *CSecureElement::GetPublicKey(uint32_t keystore_id)
{
  return new CPublicKey(*this, keystore_id);
}

} // namespace Controlse
