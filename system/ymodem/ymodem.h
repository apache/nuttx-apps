/****************************************************************************
 * apps/system/ymodem/ymodem.h
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

#include <stdint.h>
#include <stdlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_YMODEM_DEBUGFILE_PATH
#  define ymodem_debug(...) dprintf(ctx->debug_fd, ##__VA_ARGS__)
#else
#  define ymodem_debug(...)
#endif

#define YMODEM_PACKET_SIZE               128
#define YMODEM_PACKET_1K_SIZE            1024
#define YMODEM_FILE_NAME_LENGTH          64

#define YMODEM_FILE_RECV_NAME_PACKET        0
#define YMODEM_RECV_DATA_PACKET             1
#define YMODEM_FILE_SEND_NAME_PACKET        2
#define YMODEM_SEND_DATA_PACKET             3

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ymodem_ctx
{
  uint8_t header;
  uint8_t seq[2];
  uint8_t data[YMODEM_PACKET_1K_SIZE];
  char file_name[YMODEM_FILE_NAME_LENGTH];
  uint16_t packet_size;
  uint32_t file_length;
  uint32_t timeout;
  uint32_t packet_type;
  int recvfd;
  int sendfd;
  CODE ssize_t (*packet_handler)(FAR struct ymodem_ctx *ctx);
  FAR void *priv;
  uint16_t need_sendfile_num;
#ifdef CONFIG_SYSTEM_YMODEM_DEBUGFILE_PATH
  int debug_fd;
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int ymodem_recv(FAR struct ymodem_ctx *ctx);
int ymodem_send(FAR struct ymodem_ctx *ctx);

#endif /* __APPS_SYSTEM_YMODEM_YMODEM_H */
