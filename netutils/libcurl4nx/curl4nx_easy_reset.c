/****************************************************************************
 * apps/netutils/libcurl4nx/curl4nx_easy_reset.c
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/version.h>

#include "netutils/netlib.h"
#include "netutils/curl4nx.h"
#include "curl4nx_private.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_LIBCURL4NX_USERAGENT
#  if CONFIG_VERSION_MAJOR != 0 || CONFIG_VERSION_MINOR != 0
#    define CONFIG_LIBCURL4NX_USERAGENT \
       "NuttX/" CONFIG_VERSION_STRING " (; http://www.nuttx.org/)"
#  else
#    define CONFIG_LIBCURL4NX_USERAGENT \
       "NuttX/7.x (; http://www.nuttx.org/)"
#  endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static size_t curl4nx_default_writefunc(FAR char *ptr, size_t size,
                                        size_t nmemb, FAR void *data)
{
  fwrite(ptr, size, nmemb, stdout);
  fflush(stdout);
  return size * nmemb;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: curl4nx_easy_reset()
 ****************************************************************************/

void curl4nx_easy_reset(FAR struct curl4nx_s *handle)
{
  curl4nx_info("handle=%p\n", handle);

  /* Setup default options */

  handle->port = 80;
  strncpy(handle->useragent, CONFIG_LIBCURL4NX_USERAGENT,
          sizeof(handle->useragent));
  strncpy(handle->method, "GET", sizeof(handle->method));
  handle->version = CURL4NX_HTTP_VERSION_1_1;
  handle->writefunc = curl4nx_default_writefunc;

  if (handle->rxbufsize == 0)
    {
      /* RX buffer not initialized */

      handle->rxbufsize = CONFIG_LIBCURL4NX_RXBUFLEN;
    }

  if (handle->rxbuf != NULL)
    {
      free(handle->rxbuf);
      handle->rxbuf = NULL;
    }

  handle->rxbuf = malloc(handle->rxbufsize);
  curl4nx_info("Allocated a %d bytes RX buffer\n", handle->rxbufsize);

  /* Delete all custom headers */
}
