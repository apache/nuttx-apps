/****************************************************************************
 * apps/system/ymodem/ymodem.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <nuttx/crc16.h>

#include "ymodem.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_YMODEM_DEBUG_FILEPATH
#  define ymodem_debug(...) \
  do \
    { \
      dprintf(ctx->debug_fd, ##__VA_ARGS__); \
      fsync(ctx->debug_fd); \
    } \
  while(0)

#else
#  define ymodem_debug(...)
#endif

#define SOH           0x01  /* Start of 128-byte data packet */
#define STX           0x02  /* Start of 1024-byte data packet */
#define STC           0x03  /* Start of custom byte data packet */
#define EOT           0x04  /* End of transmission */
#define ACK           0x06  /* Acknowledge */
#define NAK           0x15  /* Negative acknowledge */
#define CAN           0x18  /* Two of these in succession aborts transfer */
#define CRC           0x43  /* 'C' == 0x43, request 16-bit CRC */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int ymodem_recv_buffer(FAR struct ymodem_ctx_s *ctx, FAR uint8_t *buf,
                              size_t size)
{
  size_t i = 0;

  ymodem_debug("recv buffer data, read size is %zu\n", size);
  while (i < size)
    {
      ssize_t ret = read(ctx->recvfd, buf + i, size - i);
      if (ret >= 0)
        {
          ymodem_debug("recv buffer data, size %zd\n", ret);
          i += ret;
        }
      else
        {
          ymodem_debug("recv buffer error, ret %d\n", -errno);
          return -errno;
        }
    }

  return 0;
}

static int ymodem_send_buffer(FAR struct ymodem_ctx_s *ctx,
                              FAR const uint8_t *buf, size_t size)
{
  size_t i = 0;

  ymodem_debug("send buffer data, write size is %zu\n", size);
  while (i < size)
    {
      ssize_t ret = write(ctx->sendfd, buf, size);
      if (ret >= 0)
        {
          ymodem_debug("send buffer data, size %zd\n", ret);
          i += ret;
        }
      else
        {
          ymodem_debug("send buffer error, ret %d\n", -errno);
          return -errno;
        }
    }

  return 0;
}

static int ymodem_recv_packet(FAR struct ymodem_ctx_s *ctx)
{
  uint16_t recv_crc;
  uint16_t cal_crc;
  int ret;

  ret = ymodem_recv_buffer(ctx, ctx->header, 1);
  if (ret < 0)
    {
      return ret;
    }

  switch (ctx->header[0])
    {
      case SOH:
        ctx->packet_size = YMODEM_PACKET_SIZE;
        break;
      case STX:
        ctx->packet_size = YMODEM_PACKET_1K_SIZE;
        break;
      case STC:
        ctx->packet_size = ctx->custom_size;
        break;
      case EOT:
        return -EAGAIN;
      case CAN:
        ret = ymodem_recv_buffer(ctx, ctx->header, 1);
        if (ret < 0)
          {
            return ret;
          }
        else if (ctx->header[0] == CAN)
          {
            return -ECANCELED;
          }

      default:
          ymodem_debug("recv_packet: EBADMSG: header[0]=0x%x\n",
                       ctx->header[0]);
          return -EBADMSG;
    }

  ret = ymodem_recv_buffer(ctx, &ctx->header[1],
                           2 + ctx->packet_size + 2);
  if (ret < 0)
    {
      ymodem_debug("recv_packet: err=%d\n", ret);
      return ret;
    }

  if ((ctx->header[1] + ctx->header[2]) != 0xff)
    {
      ymodem_debug("recv_packet: EILSEQ seq[]=%d %d\n",
                   ctx->header[1], ctx->header[2]);
      return -EILSEQ;
    }

  recv_crc = (ctx->data[ctx->packet_size] << 8) +
              ctx->data[ctx->packet_size + 1];
  cal_crc = crc16xmodem(ctx->data, ctx->packet_size);
  if (cal_crc != recv_crc)
    {
      ymodem_debug("recv_packet: EBADMSG rcev:cal=0x%x 0x%x\n",
                   recv_crc, cal_crc);
      return -EBADMSG;
    }

  ymodem_debug("recv_packet:OK: size=%d, seq=%d\n",
               ctx->packet_size, ctx->header[1]);
  return 0;
}

static int ymodem_recv_file(FAR struct ymodem_ctx_s *ctx)
{
  FAR char *str = NULL;
  uint32_t total_seq = 0;
  int retries = 0;
  int ret;

  ctx->header[0] = CRC;
recv_packet:
  ymodem_send_buffer(ctx, ctx->header, 1);
  ret = ymodem_recv_packet(ctx);
  if (ret == -ECANCELED)
    {
      ymodem_debug("recv_file: canceled by sender\n");
      goto cancel;
    }
  else if (ret == -EAGAIN)
    {
      ctx->header[0] = ACK;
      ymodem_send_buffer(ctx, ctx->header, 1);
      ymodem_debug("recv_file: finished one file transfer\n");
      ctx->header[0] = CRC;
      total_seq = 0;
      goto recv_packet;
    }
  else if (ret < 0)
    {
      /* other errors, like ETIMEDOUT, EILSEQ, EBADMSG... */

      tcflush(ctx->recvfd, TCIOFLUSH);
      if (++retries > ctx->retry)
        {
          ymodem_debug("recv_file: too many errors, cancel!!\n");
          goto cancel;
        }

      /* Use str to mask transfer start */

      ctx->header[0] = str ? NAK : CRC;
      goto recv_packet;
    }

  if ((total_seq & 0xff) - 1 == ctx->header[1])
    {
      ymodem_debug("recv_file: Received the previous packet that has"
                   "been received, continue %" PRIu32 " %u\n", total_seq,
                   ctx->header[1]);

      ctx->header[0] = ACK;
      goto recv_packet;
    }
  else if ((total_seq & 0xff) != ctx->header[1])
    {
      ymodem_debug("recv_file: total seq error:%" PRIu32 " %u\n", total_seq,
                   ctx->header[1]);
      ctx->header[0] = CRC;
      goto recv_packet;
    }

  /* File name packet */

  if (total_seq == 0)
    {
      /* Filename packet is empty, end session */

      if (ctx->data[0] == '\0')
        {
          /* Last file done, so the session also finished */

          ymodem_debug("recv_file: session finished\n");
          ctx->header[0] = ACK;
          ymodem_send_buffer(ctx, ctx->header, 1);
          return 0;
        }

      str = (FAR char *)ctx->data;
      ctx->packet_type = YMODEM_FILENAME_PACKET;
      strlcpy(ctx->file_name, str, PATH_MAX);
      str += strlen(str) + 1;
      ctx->file_length = atoi(str);
      ymodem_debug("recv_file: new file %s(%zu) start\n", ctx->file_name,
                   ctx->file_length);
      ret = ctx->packet_handler(ctx);
      if (ret < 0)
        {
          ymodem_debug("recv_file: handler err for file name packet:"
                       " ret=%d\n", ret);
          goto cancel;
        }

      ctx->header[0] = ACK;
      ymodem_send_buffer(ctx, ctx->header, 1);
      ctx->header[0] = CRC;
      total_seq++;
      goto recv_packet;
    }

  /* data packet */

  ctx->packet_type = YMODEM_DATA_PACKET;
  ret = ctx->packet_handler(ctx);
  if (ret < 0)
    {
      ymodem_debug("recv_file: handler err for data packet: ret=%d\n", ret);
      goto cancel;
    }

  ctx->header[0] = ACK;
  total_seq++;
  ymodem_debug("recv_file: recv data success\n");
  retries = 0;
  goto recv_packet;

cancel:
  ctx->header[0] = CAN;
  ymodem_send_buffer(ctx, ctx->header, 1);
  ymodem_send_buffer(ctx, ctx->header, 1);
  ymodem_debug("recv_file: cancel command sent to sender\n");
  return ret;
}

static int ymodem_recv_cmd(FAR struct ymodem_ctx_s *ctx, uint8_t cmd)
{
  uint8_t recv;
  int ret;

  ret = ymodem_recv_buffer(ctx, &recv, 1);
  if (ret < 0)
    {
      ymodem_debug("recv cmd error\n");
      return ret;
    }

  if (recv == NAK)
    {
      return -EAGAIN;
    }

  if (recv != cmd)
    {
      ymodem_debug("recv cmd error, must 0x%x, but receive 0x%x\n",
                   cmd, recv);
      return -EINVAL;
    }

  return 0;
}

static int ymodem_send_file(FAR struct ymodem_ctx_s *ctx)
{
  uint16_t crc;
  int retries;
  int ret;

  ymodem_debug("waiting handshake\n");
  for (retries = 0; retries < ctx->retry; retries++)
    {
      ret = ymodem_recv_cmd(ctx, CRC);
      if (ret >= 0)
        {
          break;
        }
    }

  if (retries >= ctx->retry)
    {
      ymodem_debug("waiting handshake error\n");
      return -ETIMEDOUT;
    }

  ymodem_debug("ymodem send file start\n");
send_start:
  ctx->packet_type = YMODEM_FILENAME_PACKET;
  ret = ctx->packet_handler(ctx);
  if (ret < 0)
    {
      goto send_last;
    }

  ymodem_debug("sendfile filename:%s filelength:%zu\n",
               ctx->file_name, ctx->file_length);
  sprintf((FAR char *)ctx->data, "%s%c%zu", ctx->file_name,
          '\0', ctx->file_length);
  ctx->header[0] = SOH;
  ctx->header[1] = 0x00;
  ctx->header[2] = 0xff;
  ctx->packet_size = YMODEM_PACKET_SIZE;
  crc = crc16xmodem(ctx->data, ctx->packet_size);
  ctx->data[ctx->packet_size] = crc >> 8;
  ctx->data[ctx->packet_size + 1] = crc;

send_name:
  ret = ymodem_send_buffer(ctx, ctx->header, 3 + ctx->packet_size + 2);
  if (ret < 0)
    {
      ymodem_debug("send name packet error\n");
      return ret;
    }

  ret = ymodem_recv_cmd(ctx, ACK);
  if (ret == -EAGAIN)
    {
      ymodem_debug("send name packet recv NAK, need send again\n");
      goto send_name;
    }

  if (ret < 0)
    {
      ymodem_debug("send name packet, recv error cmd\n");
      return ret;
    }

  ret = ymodem_recv_cmd(ctx, CRC);
  if (ret == -EAGAIN)
    {
      ymodem_debug("send name packet recv NAK, need send again\n");
      goto send_name;
    }

  if (ret < 0)
    {
      ymodem_debug("send name packet, recv error cmd\n");
      return ret;
    }

  ctx->packet_type = YMODEM_DATA_PACKET;
send_packet:
  if (ctx->file_length <= YMODEM_PACKET_SIZE)
    {
      ctx->header[0] = SOH;
      ctx->packet_size = YMODEM_PACKET_SIZE;
    }
  else if (ctx->custom_size != 0)
    {
      ctx->header[0] = STC;
      ctx->packet_size = ctx->custom_size;
    }
  else
    {
      ctx->header[0] = STX;
      ctx->packet_size = YMODEM_PACKET_1K_SIZE;
    }

  ymodem_debug("packet_size is %zu\n", ctx->packet_size);
  ctx->header[1]++;
  ctx->header[2]--;
  ret = ctx->packet_handler(ctx);
  if (ret < 0)
    {
      return ret;
    }

  crc = crc16xmodem(ctx->data, ctx->packet_size);
  ctx->data[ctx->packet_size] = crc >> 8;
  ctx->data[ctx->packet_size + 1] = crc;
send_packet_again:
  ret = ymodem_send_buffer(ctx, ctx->header, 3 + ctx->packet_size + 2);
  if (ret < 0)
    {
      ymodem_debug("send data packet error\n");
      return ret;
    }

  ret = ymodem_recv_cmd(ctx, ACK);
  if (ret == -EAGAIN)
    {
      ymodem_debug("send data packet recv NAK, need send again\n");
      goto send_packet_again;
    }

  if (ret < 0)
    {
      ymodem_debug("send data packet, recv error\n");
      return ret;
    }

  if (ctx->file_length != 0)
    {
      ymodem_debug("The remain bytes sent are %zu\n", ctx->file_length);
      goto send_packet;
    }

send_eot:
  ctx->header[0] = EOT;
  ret = ymodem_send_buffer(ctx, ctx->header, 1);
  if (ret < 0)
    {
      ymodem_debug("send EOT error\n");
      return ret;
    }

  ret = ymodem_recv_cmd(ctx, ACK);
  if (ret == -EAGAIN)
    {
      ymodem_debug("send EOT recv NAK, need send again\n");
      goto send_eot;
    }

  if (ret < 0)
    {
      ymodem_debug("send EOT, recv ACK error\n");
      return ret;
    }

  ret = ymodem_recv_cmd(ctx, CRC);
  if (ret == -EAGAIN)
    {
      ymodem_debug("send EOT recv NAK, need send again\n");
      goto send_eot;
    }

  if (ret < 0)
    {
      ymodem_debug("send EOT, recv CRC error\n");
      return ret;
    }

  goto send_start;

send_last:
  ctx->header[0] = SOH;
  ctx->header[1] = 0x00;
  ctx->header[2] = 0xff;
  ctx->packet_type = YMODEM_DATA_PACKET;
  ctx->packet_size = YMODEM_PACKET_SIZE;
  memset(ctx->data, 0, YMODEM_PACKET_SIZE);
  crc = crc16xmodem(ctx->data, ctx->packet_size);
  ctx->data[ctx->packet_size] = crc >> 8;
  ctx->data[ctx->packet_size + 1] = crc;
send_last_again:
  ret = ymodem_send_buffer(ctx, ctx->header, 3 + ctx->packet_size + 2);
  if (ret < 0)
    {
      ymodem_debug("send last packet error\n");
      return ret;
    }

  ret = ymodem_recv_cmd(ctx, ACK);
  if (ret == -EAGAIN)
    {
      ymodem_debug("send last packet, need send again\n");
      goto send_last_again;
    }

  if (ret < 0)
    {
      ymodem_debug("send last packet, recv error\n");
      return ret;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ymodem_recv(FAR struct ymodem_ctx_s *ctx)
{
  struct termios saveterm;
  struct termios term;
  int ret;

  if (ctx == NULL || ctx->packet_handler == NULL)
    {
      return -EINVAL;
    }

  if (ctx->custom_size != 0)
    {
      ctx->header = calloc(1, 3 + ctx->custom_size + 2);
    }
  else
    {
      ctx->header = calloc(1, 3 + YMODEM_PACKET_1K_SIZE + 2);
    }

  if (ctx->header == NULL)
    {
      return -ENOMEM;
    }

  ctx->data = ctx->header + 3;
#ifdef CONFIG_SYSTEM_YMODEM_DEBUG_FILEPATH
  ctx->debug_fd = open(CONFIG_SYSTEM_YMODEM_DEBUG_FILEPATH,
                       O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (ctx->debug_fd < 0)
    {
      free(ctx->header);
      return -errno;
    }
#endif

  tcgetattr(ctx->recvfd, &term);
  memcpy(&saveterm, &term, sizeof(struct termios));
  cfmakeraw(&term);
  term.c_cc[VTIME] = ctx->interval;
  term.c_cc[VMIN] = 255;
  tcsetattr(ctx->recvfd, TCSANOW, &term);

  ret = ymodem_recv_file(ctx);

  tcsetattr(ctx->recvfd, TCSANOW, &saveterm);
#ifdef CONFIG_SYSTEM_YMODEM_DEBUG_FILEPATH
  close(ctx->debug_fd);
#endif

  free(ctx->header);
  return ret;
}

int ymodem_send(FAR struct ymodem_ctx_s *ctx)
{
  struct termios saveterm;
  struct termios term;
  int ret;

  if (ctx == NULL || ctx->packet_handler == NULL)
    {
      return -EINVAL;
    }

  if (ctx->custom_size != 0)
    {
      ctx->header = calloc(1, 3 + ctx->custom_size + 2);
    }
  else
    {
      ctx->header = calloc(1, 3 + YMODEM_PACKET_1K_SIZE + 2);
    }

  if (ctx->header == NULL)
    {
      return -ENOMEM;
    }

  ctx->data = ctx->header + 3;
#ifdef CONFIG_SYSTEM_YMODEM_DEBUG_FILEPATH
  ctx->debug_fd = open(CONFIG_SYSTEM_YMODEM_DEBUG_FILEPATH,
                       O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (ctx->debug_fd < 0)
    {
      free(ctx->header);
      return -errno;
    }
#endif

  tcgetattr(ctx->recvfd, &term);
  memcpy(&saveterm, &term, sizeof(struct termios));
  cfmakeraw(&term);
  tcsetattr(ctx->recvfd, TCSANOW, &term);

  ret = ymodem_send_file(ctx);
  if (ret < 0)
    {
      ymodem_debug("ymodem send file error, ret:%d\n", ret);
    }

  tcsetattr(ctx->recvfd, TCSANOW, &saveterm);
#ifdef CONFIG_SYSTEM_YMODEM_DEBUG_FILEPATH
  close(ctx->debug_fd);
#endif

  free(ctx->header);
  return ret;
}
