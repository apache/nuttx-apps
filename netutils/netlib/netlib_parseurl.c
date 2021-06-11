/****************************************************************************
 * apps/netutils/netlib/netlib_parseurl.c
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
      return -EINVAL;
    }

  src++;

  if (*src != '/')
    {
      return -EINVAL;
    }

  src++;

  if (*src != '/')
    {
      return -EINVAL;
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
