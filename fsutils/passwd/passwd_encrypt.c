/****************************************************************************
 * apps/fsutils/passwd/passwd_encrypt.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
 *   The ASCII base64 character
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

int passwd_encrypt(FAR const char *password, char encrypted[MAX_ENCRYPTED + 1])
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
  int converted;
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

  for (converted = 0; converted < remaining; converted += 8)
    {
      /* Copy bytes */

      gulpsize = 8;
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

      for (; i < 8; i++)
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
