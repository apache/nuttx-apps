/****************************************************************************
 * apps/netutils/libcurl4nx/curl4nx_easy_reset.c
 * Implementation of the HTTP client, cURL like interface.
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
