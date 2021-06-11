/****************************************************************************
 * apps/netutils/libcurl4nx/curl4nx_private.h
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
 * Public Types
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
