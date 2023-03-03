/****************************************************************************
 * apps/system/ymodem/ymodem.c
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

#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <nuttx/crc16.h>
#include "ymodem.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SOH           0x01  /* start of 128-byte data packet */
#define STX           0x02  /* start of 1024-byte data packet */
#define EOT           0x04  /* end of transmission */
#define ACK           0x06  /* acknowledge */
#define NAK           0x15  /* negative acknowledge */
#define CA            0x18  /* two of these in succession aborts transfer */
#define CRC16         0x43  /* 'C' == 0x43, request 16-bit CRC */

#define MAX_ERRORS    100

#define EEOT 200    /* End of transfer */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static long get_current_time(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static ssize_t ymodem_uart_recv(FAR struct ymodem_ctx *ctx,
                                FAR uint8_t *buf, size_t size,
                                uint32_t timeout)
{
  ssize_t i = 0;
  ssize_t ret;
  long base;
  struct pollfd pfd =
    {
      ctx->recvfd, POLLIN, 0
    };

  base = get_current_time();
  while (i < size && poll(&pfd, 1, timeout) > 0)
    {
      if (get_current_time() - base >= timeout)
        {
          ymodem_debug("ymodem_uart_recv timeout\n");
          return -1;
        }

      ret = read(ctx->recvfd, buf + i, size - i);
      if (ret >= 0)
        {
          i += ret;
        }
      else
        {
          ymodem_debug("ymodem_uart_recv read data error\n");
          return ret;
        }
    }

  if (i == 0)
    {
      return -1;
    }

  return i;
}

static ssize_t ymodem_uart_send(FAR struct ymodem_ctx *ctx,
                                FAR uint8_t *buf, size_t size,
                                uint32_t timeout)
{
  ssize_t ret = write(ctx->sendfd, buf, size);
  if (ret != size)
    {
      ymodem_debug("ymodem_uart_send error\n");
      return ret;
    }

  return ret;
}

static int ymodem_rcev_packet(FAR struct ymodem_ctx *ctx)
{
  uint32_t timeout = ctx->timeout;
  uint16_t packet_size;
  uint16_t rcev_crc;
  uint16_t cal_crc;
  uint8_t crchl[2];
  uint8_t chunk[1];
  int ret;

  ret = ymodem_uart_recv(ctx, chunk, 1, timeout);
  if (ret != 1)
    {
      return ret;
    }

  switch (chunk[0])
    {
      case SOH:
        packet_size = YMODEM_PACKET_SIZE;
        break;
      case STX:
        packet_size = YMODEM_PACKET_1K_SIZE;
        break;
      case EOT:
        return -EEOT;
      case CA:
        ret = ymodem_uart_recv(ctx, chunk, 1, timeout);
        if (ret != 1 && chunk[0] == CA)
          {
            return -ECANCELED;
          }
        else
          {
            return -EBADMSG;
          }

      default:
          ymodem_debug("rcev_packet: EBADMSG: chunk[0]=%d\n", chunk[0]);
          return -EBADMSG;
    }

  ret = ymodem_uart_recv(ctx, ctx->seq, 2, timeout);
  if (ret != 2)
    {
      ymodem_debug("rcev_packet: err=%d\n", ret);
      return ret;
    }

  ret = ymodem_uart_recv(ctx, ctx->data, packet_size, timeout);
  if (ret != packet_size)
    {
      ymodem_debug("rcev_packet: err=%d\n", ret);
      return ret;
    }

  ret = ymodem_uart_recv(ctx, crchl, 2, timeout);
  if (ret != 2)
    {
      ymodem_debug("rcev_packet: err=%d\n", ret);
      return ret;
    }

  if ((ctx->seq[0] + ctx->seq[1]) != 0xff)
    {
      ymodem_debug("rcev_packet: EILSEQ seq[]=%d %d\n", ctx->seq[0],
                                                    ctx->seq[1]);
      return -EILSEQ;
    }

  rcev_crc = (uint16_t)((crchl[0] << 8) + crchl[1]);
  cal_crc = crc16(ctx->data, packet_size);
  if (rcev_crc != cal_crc)
    {
      ymodem_debug("rcev_packet: EBADMSG rcev:cal=%x %x\n",
                   rcev_crc, cal_crc);
      return -EBADMSG;
    }

  ctx->packet_size = packet_size;
  ymodem_debug("rcev_packet:OK: size=%d, seq=%d\n",
               packet_size, ctx->seq[0]);
  return 0;
}

static int ymodem_rcev_file(FAR struct ymodem_ctx *ctx)
{
  uint32_t timeout = ctx->timeout;
  bool file_start = false;
  bool file_done = false;
  uint32_t total_seq = 0;
  bool canceled = false;
  uint8_t chunk[1];
  FAR char *str;
  int ret = 0;
  int err = 0;

  chunk[0] = CRC16;
  ymodem_uart_send(ctx, chunk, 1, timeout);
  while (!file_done)
    {
      ret = ymodem_rcev_packet(ctx);
      if (!ret)
        {
          if ((total_seq & 0xff) != ctx->seq[0])
            {
              ymodem_debug("rcev_file: total seq erro:%lu %u\n",
                           total_seq, ctx->seq[0]);
              chunk[0] = CRC16;
              ymodem_uart_send(ctx, chunk, 1, timeout);
            }
          else
            {
              /* update with the total sequence number */

              ctx->seq[0] = total_seq;

              /* file name packet */

              if (total_seq == 0)
                {
                  /* Filename packet is empty, end session */

                  if (ctx->data[0] == '\0')
                    {
                      ymodem_debug("rcev_file: session finished\n");
                      chunk[0] = ACK;
                      ymodem_uart_send(ctx, chunk, 1, timeout);

                      /* last file done, so the session also finished */

                      file_done = true;
                    }
                  else
                    {
                      str = (FAR char *)ctx->data;
                      ctx->packet_type = YMODEM_FILE_RECV_NAME_PACKET;
                      strncpy(ctx->file_name, str, YMODEM_FILE_NAME_LENGTH);
                      ctx->file_name[YMODEM_FILE_NAME_LENGTH - 1] = '\0';
                      str += strlen(str) + 1;
                      ctx->file_length = atoi(str);
                      ymodem_debug("rcev_file: new file %s(%lu) start\n",
                                    ctx->file_name, ctx->file_length);
                      ret = ctx->packet_handler(ctx);
                      if (ret)
                        {
                          ymodem_debug("rcev_file: handler err for file \
                                    name packet: ret=%d\n", ret);
                          canceled = true;
                          ret = -ENOEXEC;
                          break;
                        }

                      file_start = true;
                      chunk[0] = ACK;
                      ymodem_uart_send(ctx, chunk, 1, timeout);
                      chunk[0] = CRC16;
                      ymodem_uart_send(ctx, chunk, 1, timeout);
                    }
                }
              else
                {
                  /* data packet */

                  ctx->packet_type = YMODEM_RECV_DATA_PACKET;
                  ret = ctx->packet_handler(ctx);
                  if (ret)
                    {
                      ymodem_debug("rcev_file: handler err for data \
                                packet: ret=%d\n", ret);
                      canceled = true;
                      ret = -ENOEXEC;
                      break;
                    }

                  chunk[0] = ACK;
                  ymodem_uart_send(ctx, chunk, 1, timeout);
                }

              ymodem_debug("rcev_file: packet %lu %s\n",
                           total_seq, ret ? "failed" : "success");

              total_seq++;
            }
        }
      else if (ret == -ECANCELED)
        {
          ymodem_debug("rcev_file: canceled by sender\n");
          canceled = true;
          break;
        }
      else if (ret == -EEOT)
        {
          chunk[0] = ACK;
          ymodem_uart_send(ctx, chunk, 1, timeout);
          file_done = true;
          ymodem_debug("rcev_file: finished one file transfer\n");
        }
      else
        {
          /* other errors, like ETIME, EILSEQ, EBADMSG... */

          if (file_start)
            {
             err++;
            }

          if (err > MAX_ERRORS)
            {
              ymodem_debug("rcev_file: too many errors, cancel!!\n");
              canceled = true;
              break;
            }

          chunk[0] = CRC16;
          ymodem_uart_send(ctx, chunk, 1, timeout);
        }
    }

  if (canceled)
    {
      chunk[0] = CA;
      ymodem_uart_send(ctx, chunk, 1, timeout);
      ymodem_uart_send(ctx, chunk, 1, timeout);
      ymodem_debug("rcev_file: cancel command sent to sender\n");
    }

  return ret;
}

static int ymodem_send_packet(FAR struct ymodem_ctx *ctx)
{
  size_t size;
  uint16_t crc;
  uint8_t send_crc[2];

  crc = crc16(ctx->data, ctx->packet_size);
  size = ymodem_uart_send(ctx, &ctx->header, ctx->packet_size + 3,
                   ctx->timeout);

  if (size != ctx->packet_size + 3)
    {
      ymodem_debug("send packet error\n");
      return -1;
    }

  send_crc[0] = crc >> 8;
  send_crc[1] = crc & 0x00ff;
  size = ymodem_uart_send(ctx, send_crc, 2, ctx->timeout);
  if (size != 2)
    {
      ymodem_debug("send crc16 error\n");
      return -1;
    }

  return 0;
}

static int ymodem_rcev_cmd(FAR struct ymodem_ctx *ctx, uint8_t cmd)
{
  size_t size;
  uint8_t chunk[1];

  size = ymodem_uart_recv(ctx, chunk, 1, ctx->timeout);
  if (size != 1)
    {
      ymodem_debug("recv cmd error\n");
      return -1;
    }

  if (chunk[0] == NAK)
    {
      return -EAGAIN;
    }

  if (chunk[0] != cmd)
    {
      ymodem_debug("recv cmd error, must 0x%x, but receive %d\n",
               cmd, chunk[0]);
      return -EINVAL;
    }

  return 0;
}

static int ymodem_send_file(FAR struct ymodem_ctx *ctx)
{
  uint8_t chunk[1];
  ssize_t readsize;
  ssize_t size;
  int err = 0;
  int ret;

  if (!ctx || !ctx->packet_handler)
    {
      ymodem_debug("%s: invalid context config\n", __func__);
      return -EINVAL;
    }

  if (ctx->need_sendfile_num <= 0)
    {
      ymodem_debug("need_sendfile_num is %d, no file to send!\n",
               ctx->need_sendfile_num);
      return -EINVAL;
    }

  chunk[0] = 0;
  ymodem_debug("waiting handshake\n");
  do
    {
      size = ymodem_uart_recv(ctx, chunk, 1, ctx->timeout);
    }
  while (err++ < MAX_ERRORS && chunk[0] != CRC16);

  if (err >= MAX_ERRORS)
    {
      ymodem_debug("waiting handshake error\n");
      return -ETIMEDOUT;
    }

  ymodem_debug("ymodem send file start\n");
send_start:
  ctx->packet_type = YMODEM_FILE_SEND_NAME_PACKET;
  ctx->packet_handler(ctx);
  if ((ctx->file_name[0] == 0 || ctx->file_length == 0))
    {
      ymodem_debug("get fileinfo error\n");
      return -EINVAL;
    }

  ymodem_debug("sendfile filename:%s filelength:%lu\n",
               ctx->file_name, ctx->file_length);
  memset(ctx->data, 0, YMODEM_PACKET_SIZE);
  strncpy((FAR char *)ctx->data, ctx->file_name, YMODEM_FILE_NAME_LENGTH);
  sprintf((FAR char *)ctx->data + strlen(ctx->file_name) + 1, "%ld",
          ctx->file_length);
  ctx->header = SOH;
  ctx->packet_size = YMODEM_PACKET_SIZE;
  ctx->seq[0] = 0x00;
  ctx->seq[1] = 0xff;

send_name:
  ret = ymodem_send_packet(ctx);
  if (ret < 0)
    {
      ymodem_debug("send name packet error\n");
      return -EINVAL;
    }

  ret = ymodem_rcev_cmd(ctx, ACK);
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

  ret = ymodem_rcev_cmd(ctx, CRC16);
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

  ctx->packet_type = YMODEM_SEND_DATA_PACKET;
send_packet:
  if (ctx->file_length <= YMODEM_PACKET_1K_SIZE)
    {
      ctx->header = SOH;
      ctx->packet_size = YMODEM_PACKET_SIZE;
    }
  else
    {
      ctx->header = STX;
      ctx->packet_size = YMODEM_PACKET_1K_SIZE;
    }

    ymodem_debug("packet_size is %d\n", ctx->packet_size);
    ctx->seq[0]++;
    ctx->seq[1]--;
    readsize = ctx->packet_handler(ctx);
    ymodem_debug("%s:%d: readsize is %d\n", __FILE__, __LINE__, readsize);

  if (readsize < 0)
    {
      return readsize;
    }

  if (readsize == 0)
    {
      goto send_eot;
    }

  if (readsize < ctx->packet_size)
    {
      memset(ctx->data + readsize, 0x1a, ctx->packet_size - readsize);
    }

send_packet_again:
  ret = ymodem_send_packet(ctx);
  if (ret < 0)
    {
      ymodem_debug("send data packet error\n");
      return ret;
    }

  ret = ymodem_rcev_cmd(ctx, ACK);
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

  ctx->file_length -= readsize;
  if (ctx->file_length != 0)
    {
      ymodem_debug("The remain bytes sent are %lu\n", ctx->file_length);
      goto send_packet;
    }

send_eot:
  chunk[0] = EOT;
  size = ymodem_uart_send(ctx, chunk, 1, ctx->timeout);
  if (size < 0)
    {
      ymodem_debug("%s:%d: send EOT error\n", __FILE__, __LINE__);
      return -1;
    }

  ret = ymodem_rcev_cmd(ctx, ACK);
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

  ret = ymodem_rcev_cmd(ctx, CRC16);
  if (ret == -EAGAIN)
    {
      ymodem_debug("send EOT recv NAK, need send again\n");
      goto send_eot;
    }

  if (ret < 0)
    {
      ymodem_debug("send EOT, recv CRC16 error\n");
      return ret;
    }

  if (--ctx->need_sendfile_num != 0)
    {
      ymodem_debug("need sendfile num is %d, so send file again\n",
               ctx->need_sendfile_num);
      goto send_start;
    }

send_last:
  ctx->header = SOH;
  ctx->packet_type = YMODEM_SEND_DATA_PACKET;
  ctx->packet_size = YMODEM_PACKET_SIZE;
  ctx->seq[0] = 0x00;
  ctx->seq[1] = 0xff;
  memset(ctx->data, 0, YMODEM_PACKET_SIZE);
  ret = ymodem_send_packet(ctx);
  if (ret < 0)
    {
      ymodem_debug("send last packet error\n");
      return -1;
    }

  ret = ymodem_rcev_cmd(ctx, ACK);
  if (ret == -EAGAIN)
    {
      ymodem_debug("send last packet, need send again\n");
      goto send_last;
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

int ymodem_recv(FAR struct ymodem_ctx *ctx)
{
  struct termios saveterm;
  struct termios term;
  int ret;

  tcgetattr(ctx->recvfd, &saveterm);
  tcgetattr(ctx->recvfd, &term);
  cfmakeraw(&term);
  tcsetattr(ctx->recvfd, TCSANOW, &term);

#ifdef CONFIG_SYSTEM_YMODEM_DEBUGFILE_PATH
  ctx->debug_fd = open(CONFIG_SYSTEM_YMODEM_DEBUGFILE_PATH,
                       O_CREAT | O_RDWR);
  if (ctx->debug_fd < 0)
    {
      return -EINVAL;
    }
#endif

  if (!ctx || !ctx->packet_handler)
    {
      ymodem_debug("ymodem: invalid context config\n");
      return -EINVAL;
    }

  while (1)
    {
      ret = ymodem_rcev_file(ctx);
      if (ret == -EEOT)
        {
          continue;
        }

      break;
    }

#ifdef CONFIG_SYSTEM_YMODEM_DEBUGFILE_PATH
  close(ctx->debug_fd);
#endif

  tcsetattr(ctx->recvfd, TCSANOW, &saveterm);
  return ret;
}

int ymodem_send(FAR struct ymodem_ctx *ctx)
{
  struct termios saveterm;
  struct termios term;
  int ret;

  tcgetattr(ctx->recvfd, &saveterm);
  tcgetattr(ctx->recvfd, &term);
  cfmakeraw(&term);
  tcsetattr(ctx->recvfd, TCSANOW, &term);

#ifdef CONFIG_SYSTEM_YMODEM_DEBUGFILE_PATH
  ctx->debug_fd = open(CONFIG_SYSTEM_YMODEM_DEBUGFILE_PATH,
                       O_CREAT | O_RDWR);
  if (ctx->debug_fd < 0)
    {
      return -EINVAL;
    }
#endif

  ret = ymodem_send_file(ctx);
  if (ret < 0)
    {
      ymodem_debug("ymodem send file error, ret:%d\n", ret);
    }

#ifdef CONFIG_SYSTEM_YMODEM_DEBUGFILE_PATH
  close(ctx->debug_fd);
#endif

  tcsetattr(ctx->recvfd, TCSANOW, &saveterm);
  return 0;
}
