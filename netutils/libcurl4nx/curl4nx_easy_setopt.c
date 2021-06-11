/****************************************************************************
 * apps/netutils/libcurl4nx/curl4nx_easy_setopt.c
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
#include <nuttx/compiler.h>
#include <debug.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <nuttx/version.h>

#include "netutils/netlib.h"
#include "netutils/curl4nx.h"
#include "curl4nx_private.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: curl4nx_easy_setopt()
 ****************************************************************************/

int curl4nx_easy_setopt(FAR struct curl4nx_s *handle, int option,
                        FAR void *param)
{
  int ret;
  int cret = CURL4NXE_OK;
  char scheme[16];

  switch (option)
    {
      case CURL4NXOPT_URL:
        {
          struct url_s url;
          memset(&url, 0, sizeof(struct url_s));
          url.scheme = scheme;
          url.schemelen = sizeof(scheme);
          url.host = handle->host;
          url.hostlen = CONFIG_LIBCURL4NX_MAXHOST;
          url.path = handle->path;
          url.pathlen = CONFIG_LIBCURL4NX_MAXPATH;

          ret = netlib_parseurl(param, &url);

          if (ret == -E2BIG)
            {
              cret = CURL4NXE_OUT_OF_MEMORY;
              break;
            }
          else if (ret != 0) /* includes -EINVAL */
            {
              curl4nx_warn("Malformed URL: %s\n", param);
              cret = CURL4NXE_URL_MALFORMAT;
              break;
            }

          if (url.port == 0)
            {
              curl4nx_err("Invalid port 0\n");
              cret = CURL4NXE_URL_MALFORMAT; /* User passed invalid port */
              break;
            }

          if (url.host[0] == 0)
            {
              curl4nx_err("Empty host\n");
              cret = CURL4NXE_URL_MALFORMAT; /* User passed empty host */
              break;
            }

          if (strncmp(scheme, "http", strlen(scheme)))
            {
              curl4nx_err("Unsupported protocol '%s'\n", scheme);
              cret = CURL4NXE_UNSUPPORTED_PROTOCOL;
              break;
            }

          handle->port = url.port;
          curl4nx_info("found scheme: %s\n", scheme);
          curl4nx_info("found host  : %s\n", handle->host);
          curl4nx_info("found port  : %d\n", handle->port);
          curl4nx_info("found path  : %s\n", handle->path);
          break;
        }

      case CURL4NXOPT_PORT:
        {
          long port = (long)param;
          if (port == 0)
            {
              cret = CURL4NXE_BAD_FUNCTION_ARGUMENT;
              break;
            }

          handle->port = (uint16_t)(port & 0xffff);
          break;
        }

      case CURL4NXOPT_BUFFERSIZE:
        {
          long len = (long)param;
          if (len < CONFIG_LIBCURL4NX_MINRXBUFLEN ||
             len > CONFIG_LIBCURL4NX_MAXRXBUFLEN)
            {
              cret = CURL4NXE_BAD_FUNCTION_ARGUMENT;
              break;
            }

          if (handle->rxbuf)
            {
              curl4nx_info("Freeing previous buffer\n");
              free(handle->rxbuf);
            }

          handle->rxbuf = malloc(len);
          if (!handle->rxbuf)
            {
              cret = CURL4NXE_OUT_OF_MEMORY;
              break;
            }

          handle->rxbufsize = len;
          break;
        }

      case CURL4NXOPT_HEADERFUNCTION:
        if (param == NULL)
          {
            cret = CURL4NXE_BAD_FUNCTION_ARGUMENT;
            break;
          }

        handle->headerfunc = param;
        break;

      case CURL4NXOPT_HEADERDATA:
        handle->headerdata = param;
        break;

      case CURL4NXOPT_FAILONERROR:
        if ((long)param)
          {
            handle->flags |= CURL4NX_FLAGS_FAILONERROR;
          }
        else
          {
            handle->flags &= ~CURL4NX_FLAGS_FAILONERROR;
          }
        break;

      case CURL4NXOPT_FOLLOWLOCATION:
        if ((long)param)
          {
            handle->flags |= CURL4NX_FLAGS_FOLLOWLOCATION;
          }
        else
          {
            handle->flags &= ~CURL4NX_FLAGS_FOLLOWLOCATION;
          }
        break;

      case CURL4NXOPT_MAXREDIRS:
        {
          long redirs = (long)param;
          if (redirs < -1 ||
             redirs > CONFIG_LIBCURL4NX_MAXREDIRS)
            {
              cret = CURL4NXE_BAD_FUNCTION_ARGUMENT;
              break;
            }

          handle->max_redirs = redirs;
          break;
        }

      case CURL4NXOPT_VERBOSE:
        if ((long)param)
          {
            handle->flags |= CURL4NX_FLAGS_VERBOSE;
          }
        else
          {
            handle->flags &= ~CURL4NX_FLAGS_VERBOSE;
          }
        break;

      default:
        cret = CURL4NXE_UNKNOWN_OPTION;
        break;
    }

  return cret;
}
