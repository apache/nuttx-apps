/****************************************************************************
 * apps/fsutils/passwd/passwd_find.c
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "passwd.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_find
 *
 * Description:
 *   Find a password in the
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_find(FAR const char *username, FAR struct passwd_s *passwd)
{
  FAR char *iobuffer;
  FAR char *name;
  FAR char *src;
  FAR char *dest;
  FILE *stream;
  off_t offset;
  int enclen;
  int ret;

  /* Allocate an I/O buffer for the transfer */

  iobuffer = (FAR char *)malloc(CONFIG_FSUTILS_PASSWD_IOBUFFER_SIZE);
  if (iobuffer == NULL)
    {
      return -ENOMEM;
    }

  /* Open the password file for reading */

  stream = fopen(CONFIG_FSUTILS_PASSWD_PATH, "r");
  if (stream == NULL)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);
      return -errcode;
    }

  /* Read the password file line by line until the record with the matching
   * username is found, or until the end of the file is reached.
   */

  offset = 0;
  ret    = -ENOENT;

  while (fgets(iobuffer, CONFIG_FSUTILS_PASSWD_IOBUFFER_SIZE, stream) != NULL)
    {
      /* Skip over any leading whitespace */

      for (src = iobuffer; *src && isspace((int)*src); src++);
      if (*src == '\0')
        {
          /* Bad file format? */

          continue;
        }

      name = src;

      /* Skip to the end of the name and properly terminate it */

      for (; *src && !isspace((int)*src); src++);
      if (*src == '\0')
        {
          /* Bad file format? */

          continue;
        }

      *src++ = '\0';

      /* Check for a username match */

      if (strcmp(username, name) == 0)
        {
          /* We have a match, skip over any whitespace after the user name */

          for (; *src && isspace((int)*src); src++);
          if (*src == '\0')
            {
              /* Bad file format? */

              ret = -EINVAL;
              break;
            }

          /* Copy the offset and password into the returned structure */

          passwd->offset = offset;
          dest = passwd->encrypted;
          enclen = 0;

          while (*src && !isspace((int)*src) && enclen < MAX_ENCRYPTED)
            {
              *dest++ = *src++;
              enclen++;
            }

          if (enclen >= MAX_ENCRYPTED)
            {
              ret = -E2BIG;
              break;
            }

          *dest = '\0';
          ret = OK;
          break;
        }

      /* Get the next file offset */

      offset = ftell(stream);
    }

  fclose(stream);
  free(iobuffer);
  return ret;
}
