/****************************************************************************
 * apps/fsutils/passwd/passwd_encrypt.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <nuttx/crypto/tea.h>

#include "passwd.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This should be better protected */

static uint32_t g_tea_key[4] =
{
  CONFIG_FSUTILS_PASSWD_KEY1,
  CONFIG_FSUTILS_PASSWD_KEY2,
  CONFIG_FSUTILS_PASSWD_KEY3,
  CONFIG_FSUTILS_PASSWD_KEY4
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_base64
 *
 * Description:
 *   Encode a 5 bit value as a base64 character.
 *
 * Input Parameters:
 *   binary - 5 bit value
 *
 * Returned Value:
 *   The ASCII base64 character.  Must not return the field delimiter ':'
 *
 ****************************************************************************/

static char passwd_base64(uint8_t binary)
{
  /* 0-26 -> 'A'-'Z' */

  binary &= 63;
  if (binary < 26)
    {
      return 'A' + binary;
    }

  /* 26-51 -> 'a'-'z' */

  binary -= 26;
  if (binary < 26)
    {
      return 'a' + binary;
    }

  /* 52->61 -> '0'-'9' */

  binary -= 26;
  if (binary < 10)
    {
      return '0' + binary;
    }

  /* 62 -> '+' */

  binary -= 10;
  if (binary == 0)
    {
      return '+';
    }

  /* 63 -> '/' */

  return '/';
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_encrypt
 *
 * Description:
 *   Encrypt a password.  Currently uses the Tiny Encryption Algorithm.
 *
 * Input Parameters:
 *   password -- The password string to be encrypted
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_encrypt(FAR const char *password,
                   char encrypted[MAX_ENCRYPTED + 1])
{
  union
  {
    char     b[8];
    uint16_t h[4];
    uint32_t l[2];
  } value;

  FAR const char *src;
  FAR char *bptr;
  FAR char *dest;
  uint32_t tmp;
  uint8_t remainder;
  int remaining;
  int gulpsize;
  int nbits;
  int i;

  /* How long is the password? */

  remaining = strlen(password);
  if (remaining > MAX_PASSWORD)
    {
      return -E2BIG;
    }

  /* Convert the password in 8-byte TEA cycles */

  src        = password;
  dest       = encrypted;
  *dest      = '\0';

  remainder  = 0;
  nbits      = 0;

  for (; remaining > 0; remaining -= gulpsize)
    {
      /* Copy bytes */

      gulpsize = sizeof(value.b);
      if (gulpsize > remaining)
        {
          gulpsize = remaining;
        }

      bptr = value.b;
      for (i = 0; i < gulpsize; i++)
        {
          *bptr++ = *src++;
        }

      /* Pad with spaces if necessary */

      for (; i < sizeof(value.b); i++)
        {
          *bptr++ = ' ';
        }

      /* Perform the conversion for this cycle */

      tea_encrypt(value.l, g_tea_key);

      /* Generate the base64 output string from this cycle */

      tmp = remainder;

      for (i = 0; i < 4; i++)
        {
          tmp    = (uint32_t)value.h[i] << nbits | tmp;
          nbits += 16;

          while (nbits >= 6)
            {
              *dest++ = passwd_base64((uint8_t)(tmp & 0x3f));
              tmp   >>= 6;
              nbits  -= 6;
            }
        }

      remainder = (uint8_t)tmp;
      *dest     = '\0';
    }

  /* Handle any remainder */

  if (nbits > 0)
    {
      *dest++ = passwd_base64(remainder);
      *dest   = '\0';
    }

  return OK;
}
