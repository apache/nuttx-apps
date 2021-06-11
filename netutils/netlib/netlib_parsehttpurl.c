/****************************************************************************
 * apps/netutils/netlib/netlib_parsehttpurl.c
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
