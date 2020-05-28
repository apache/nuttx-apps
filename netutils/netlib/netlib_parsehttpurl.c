/****************************************************************************
 * netutils/netlib/netlib_parsehttpurl.c
 *
 *   Copyright (C) 2009, 2011 Gregory Nutt. All rights reserved.
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
#include <string.h>
#include <errno.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_http[] = "http://";
#define HTTPLEN 7

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_parsehttpurl
 ****************************************************************************/

int netlib_parsehttpurl(FAR const char *url, FAR uint16_t *port,
                        FAR char *hostname, int hostlen,
                        FAR char *filename, int namelen)
{
  FAR const char *src = url;
  FAR char *dest;
  int bytesleft;
  int ret = OK;
  size_t pathlen;

  /* A valid HTTP URL must begin with http:// if it does not, we will assume
   * that it is a file name only, but still return an error.  wget() depends
   * on this strange behavior.
   */

  if (strncmp(src, g_http, HTTPLEN) != 0)
    {
      ret = -EINVAL;
    }
  else
    {
      /* Skip over the http:// */

      src += HTTPLEN;

      /* Concatenate the hostname following http:// and up to the termnator */

      dest      = hostname;
      bytesleft = hostlen;

      while (*src != '\0' && *src != '/' && *src != ' ' && *src != ':')
        {
          /* Make sure that there is space for another character in the
           * hostname (reserving space for the null terminator).
           */

          if (bytesleft > 1)
            {
              /* Copy the byte */

              *dest++ = *src++;
              bytesleft--;
            }
          else
            {
              /* Note the error, but continue parsing until the end of the
               * hostname
               */

              src++;
              ret = -E2BIG;
            }
        }

      *dest = '\0';

      /* Check if the hostname is following by a port number */

      if (*src == ':')
        {
          uint16_t accum = 0;
          src++; /* Skip over the colon */

          while (*src >= '0' && *src <= '9')
            {
              accum = 10*accum + *src - '0';
              src++;
            }

          *port = accum;
        }
    }

  /* The rest of the line is the file name */

  if (*src == '\0' || *src == ' ')
    {
      ret = -ENOENT;
    }

  /* Make sure the file name starts with exactly one '/' */

  dest      = filename;
  bytesleft = namelen;

  while (*src == '/')
    {
      src++;
    }

  *dest++ = '/';
  bytesleft--;

  /* The copy the rest of the file name to the user buffer */

  pathlen = strlen(src);
  if (bytesleft >= pathlen + 1)
    {
      memcpy(dest, src, pathlen);
      dest[pathlen] = '\0';
    }
  else
    {
      dest[0] = '\0';
      ret = -E2BIG;
    }

  return ret;
}
