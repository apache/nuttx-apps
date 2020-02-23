/****************************************************************************
 * apps/netutils/curl4nx/curl4nx_private.h
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

#ifndef __APPS_NETUTILS_CURL4NX_CURL4NX_PRIVATE_H
#define __APPS_NETUTILS_CURL4NX_CURL4NX_PRIVATE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_LIBCURL4NX_MAXHOST
#define CONFIG_LIBCURL4NX_MAXHOST 128
#endif

#ifndef CONFIG_LIBCURL4NX_MAXPATH
#define CONFIG_LIBCURL4NX_MAXPATH 128
#endif

#ifndef CONFIG_LIBCURL4NX_MAXMETHOD
#define CONFIG_LIBCURL4NX_MAXMETHOD 10
#endif

#ifndef CONFIG_LIBCURL4NX_MAXUSERAGENT
#define CONFIG_LIBCURL4NX_MAXUSERAGENT 64
#endif

#ifndef CONFIG_LIBCURL4NX_RXBUFLEN
#define CONFIG_LIBCURL4NX_RXBUFLEN 512
#endif

#ifndef CONFIG_LIBCURL4NX_MAXHEADERLINE
#define CONFIG_LIBCURL4NX_MAXHEADERLINE 128
#endif

#ifndef CONFIG_LIBCURL4NX_MAXREDIRS
#define CONFIG_LIBCURL4NX_MAXREDIRS -1 /* Means unlimited */
#endif

#define CURL4NX_FLAGS_FAILONERROR     0x00000001
#define CURL4NX_FLAGS_FOLLOWLOCATION  0x00000002
#define CURL4NX_FLAGS_VERBOSE         0x00000004

#define curl4nx_err(fmt, ...)  \
    do \
      { \
        if(handle->flags & CURL4NX_FLAGS_VERBOSE) \
          _err(fmt, ##__VA_ARGS__); \
      } \
    while(0)

#define curl4nx_warn(fmt, ...) \
    do \
      { \
        if(handle->flags & CURL4NX_FLAGS_VERBOSE) \
          _warn(fmt, ##__VA_ARGS__); \
      } \
    while(0)

#define curl4nx_info(fmt, ...) \
    do \
      { \
        if(handle->flags & CURL4NX_FLAGS_VERBOSE) \
          _info(fmt, ##__VA_ARGS__); \
      } \
    while(0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Name: struct curl4nx_header_s
 *
 * Description: Holds data for an individual http header added in a request.
 *
 ****************************************************************************/

struct curl4nx_header_s
{
  struct curl4nx_header_s *next;
  FAR const char *value;
};

/****************************************************************************
 * Name: struct curl4nx_s
 *
 * Description: Holds the integrity of all information required for a curl4nx
 *   transfer.
 *
 ****************************************************************************/

struct curl4nx_s
{
  /* Network stuff */

  int                         sockfd;

  /* General stuff */

  uint32_t                    flags;
  bool                        verbose;
  curl4nx_xferinfofunc_f      progressfunc; /* Called when data chunks are received */
  FAR void *                  progressdata;

  /* Request side */

  char                        host[CONFIG_LIBCURL4NX_MAXHOST];
  char                        path[CONFIG_LIBCURL4NX_MAXPATH];
  uint16_t                    port;
  int                         version;
  char                        method[CONFIG_LIBCURL4NX_MAXMETHOD];
  char                        useragent[CONFIG_LIBCURL4NX_MAXUSERAGENT];
  FAR struct curl4nx_header_s *headers;
  int                         max_redirs;

  curl4nx_iofunc_f            writefunc; /* Called when data has to be uploaded */
  FAR void *                  writedata;

  /* Response side */

  int                         rxbufsize;
  FAR char *                  rxbuf;

  int                         status;
  unsigned long long          content_length;

  curl4nx_iofunc_f            readfunc; /* Called when data has to be downloaded */
  FAR void *                  readdata;

  curl4nx_iofunc_f            headerfunc; /* Called when a complete HTTP header has been received */
  FAR void *                  headerdata;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_NETLIB_CURL4NX_CURL4NX_PRIVATE_H */
