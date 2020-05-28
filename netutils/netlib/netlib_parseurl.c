/****************************************************************************
 * netutils/netlib/netlib_parseurl.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_parseurl
 *
 * Description:
 *   Parse an URL, not only HTTP ones. The parsing is according to this rule:
 *   SCHEME :// HOST [: PORT] / PATH
 *   - scheme is everything before the first colon
 *   - scheme must be followed by ://
 *   - host is everything until colon or slash
 *   - port is optional, parsed only if host ends with colon
 *   - path is everything after the host.
 *   This is noticeably simpler that the official URL parsing method, since
 *   - it does not take into account the user:pass@ part that can be present
 *     before the host. Support of these fields is planned in the url_s
 *     structure, but it is not parsed yet/
 *   - it does not separate the URL parameters nor the bookmark
 *   Note: see here for the documentation of a complete URL parsing routine:
 *   https://www.php.net/manual/fr/function.parse-url.php
 *
 ****************************************************************************/

int netlib_parseurl(FAR const char *str, FAR struct url_s *url)
{
  FAR const char *src = str;
  FAR char *dest;
  int bytesleft;
  int ret = OK;
  size_t pathlen;

  /* extract the protocol field, a set of a-z letters */

  dest      = url->scheme;
  bytesleft = url->schemelen;

  while (*src != '\0' && *src != ':')
    {
      /* Make sure that there is space for another character in the
       * scheme (reserving space for the null terminator).
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

  /* Parse and skip the scheme separator */

  if (*src != ':')
    {
      ret = -EINVAL;
    }

  src++;

  if (*src != '/')
    {
      ret = -EINVAL;
    }

  src++;

  if (*src != '/')
    {
      ret = -EINVAL;
    }

  src++;

  /* Concatenate the hostname following http:// and up to the termnator */

  dest      = url->host;
  bytesleft = url->hostlen;

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

      url->port = accum;
    }

  /* Make sure the file name starts with exactly one '/' */

  dest      = url->path;
  bytesleft = url->pathlen;

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
