/****************************************************************************
 * apps/system/ymodem/ymodem.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __APPS_SYSTEM_YMODEM_YMODEM_H
#define __APPS_SYSTEM_YMODEM_YMODEM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stddef.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define YMODEM_PACKET_SIZE               128
#define YMODEM_PACKET_1K_SIZE            1024

#define YMODEM_FILENAME_PACKET           0
#define YMODEM_DATA_PACKET               1

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ymodem_ctx_s
{
  /* User need initialization */

  int recvfd;
  int sendfd;
  CODE int (*packet_handler)(FAR struct ymodem_ctx_s *ctx);
  size_t custom_size;
  FAR void *priv;
  uint8_t interval;
  int retry;

  /* Public data */

  FAR uint8_t *data;
  size_t packet_size;
  int packet_type;
  char file_name[PATH_MAX];
  size_t file_length;

  /* Private data */

  FAR uint8_t *header;
#ifdef CONFIG_SYSTEM_YMODEM_DEBUG_FILEPATH
  int debug_fd;
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int ymodem_recv(FAR struct ymodem_ctx_s *ctx);
int ymodem_send(FAR struct ymodem_ctx_s *ctx);

#endif /* __APPS_SYSTEM_YMODEM_YMODEM_H */
