/****************************************************************************
 * apps/system/nxcodec/nxcodec_context.c
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

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/nuttx.h>

#include "nxcodec_context.h"
#include "nxcodec.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXCODEC_CONTEXT_BUFNUMBER 3

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline FAR nxcodec_t *
nxcodec_context_to_nxcodec(FAR nxcodec_context_t *ctx)
{
  return V4L2_TYPE_IS_OUTPUT(ctx->type) ?
         container_of(ctx, nxcodec_t, output) :
         container_of(ctx, nxcodec_t, capture);
}

static FAR nxcodec_context_buf_t *
nxcodec_context_dequeue_buf(FAR nxcodec_context_t *ctx)
{
  FAR nxcodec_t *codec = nxcodec_context_to_nxcodec(ctx);
  struct v4l2_buffer buf;
  int ret;

  memset(&buf, 0, sizeof(buf));
  buf.memory = V4L2_MEMORY_MMAP;
  buf.type = ctx->type;

  ret = ioctl(codec->fd, VIDIOC_DQBUF, &buf);
  if (ret < 0)
    {
      printf("nxcodec %s VIDIOC_DQBUF - %s\n",
             V4L2_TYPE_IS_OUTPUT(ctx->type) ? "output" : "capture",
             strerror(errno));
      return NULL;
    }

  ctx->buf[buf.index].free = true;
  ctx->buf[buf.index].buf = buf;

  return &ctx->buf[buf.index];
}

static FAR nxcodec_context_buf_t *
nxcodec_context_get_freebuf(FAR nxcodec_context_t *ctx)
{
  int i;

  if (V4L2_TYPE_IS_OUTPUT(ctx->type))
    {
      while (nxcodec_context_dequeue_buf(ctx));
    }

  for (i = 0; i < ctx->nbuffers; i++)
    {
      if (ctx->buf[i].free)
        {
          return &ctx->buf[i];
        }
    }

  return NULL;
}

static int nxcodec_context_write_data(FAR nxcodec_context_t *ctx,
                                      FAR const char *buf, int size)
{
  return write(ctx->fd, buf, size) < 0 ? -errno : 0;
}

static int nxcodec_context_read_yuv_data(FAR nxcodec_context_t *ctx,
                                         FAR char *buf,
                                         FAR uint32_t *bytesused)
{
  size_t buflen = ctx->format.fmt.pix.width *
                  ctx->format.fmt.pix.height * 3 / 2;
  ssize_t ret;

  ret = read(ctx->fd, buf, buflen);
  if (ret <= 0)
    {
      return -errno;
    }

  *bytesused = ret;
  return 0;
}

static int nxcodec_context_read_h264_data(FAR nxcodec_context_t *ctx,
                                          FAR char *buf,
                                          FAR uint32_t *bytesused)
{
  char start_code[4];
  ssize_t ret;
  int size;

  memset(start_code, 0, 4);

  ret = read(ctx->fd, buf, 4);
  if (ret <= 0)
    {
      return -errno;
    }

  if (buf[0] == 0x00 && buf[1] == 0x00 &&
      buf[2] == 0x00 && buf[3] == 0x01)
    {
      size = 4;
      while (1)
        {
          ret = read(ctx->fd, buf + size, 1);
          if (ret < 0)
            {
              return -errno;
            }
          else if (ret == 0)
            {
              break;
            }

          start_code[0] = start_code[1];
          start_code[1] = start_code[2];
          start_code[2] = start_code[3];
          start_code[3] = *(buf + size);
          size++;

          if (start_code[0] == 0x00 && start_code[1] == 0x00 &&
              start_code[2] == 0x00 && start_code[3] == 0x01)
            {
              size -= 4;
              lseek(ctx->fd, -4, SEEK_CUR);
              break;
            }
        }
    }
  else
    {
      return -EINVAL;
    }

  *bytesused = size;

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int nxcodec_context_set_status(FAR nxcodec_context_t *ctx, uint32_t cmd)
{
  FAR nxcodec_t *codec = nxcodec_context_to_nxcodec(ctx);

  return ioctl(codec->fd, cmd, &ctx->type) < 0 ? -errno : 0;
}

int nxcodec_context_enqueue_frame(FAR nxcodec_context_t *ctx)
{
  FAR nxcodec_t *codec = nxcodec_context_to_nxcodec(ctx);
  FAR nxcodec_context_buf_t *buf;
  int ret;

  buf = nxcodec_context_get_freebuf(ctx);
  if (!buf)
    {
      return -EAGAIN;
    }

  if (ctx->format.fmt.pix.pixelformat == V4L2_PIX_FMT_H264)
    {
      ret = nxcodec_context_read_h264_data(ctx,
                                           buf->addr,
                                           &buf->buf.bytesused);
      if (ret < 0)
        {
          return ret;
        }
    }
  else if (ctx->format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420)
    {
      ret = nxcodec_context_read_yuv_data(ctx,
                                          buf->addr,
                                          &buf->buf.bytesused);
      if (ret < 0)
        {
          return ret;
        }
    }

  ret = ioctl(codec->fd, VIDIOC_QBUF, &buf->buf);
  if (ret < 0)
    {
      return -errno;
    }

  buf->free = false;
  return 0;
}

int nxcodec_context_dequeue_frame(FAR nxcodec_context_t *ctx)
{
  FAR nxcodec_t *codec = nxcodec_context_to_nxcodec(ctx);
  FAR nxcodec_context_buf_t *buf;
  int ret;

  buf = nxcodec_context_dequeue_buf(ctx);
  if (!buf)
    {
      return -EAGAIN;
    }

  if (buf->buf.length > 0)
    {
      nxcodec_context_write_data(ctx, buf->addr, buf->buf.bytesused);
    }

  ret = ioctl(codec->fd, VIDIOC_QBUF, &buf->buf);
  if (ret < 0)
    {
      return -errno;
    }

  buf->free = false;
  return 0;
}

int nxcodec_context_get_format(FAR nxcodec_context_t *ctx)
{
  FAR nxcodec_t *codec = nxcodec_context_to_nxcodec(ctx);
  struct v4l2_fmtdesc fdesc;
  int ret;

  fdesc.type = ctx->type;
  while (true)
    {
      ret = ioctl(codec->fd, VIDIOC_ENUM_FMT, &fdesc);
      if (ret < 0)
        {
          printf("nxcodec %s enum_fmt error: %d\n",
                 V4L2_TYPE_IS_OUTPUT(ctx->type) ? "output" : "capture",
                 errno);
          return -errno;
        }

      if (fdesc.pixelformat == ctx->format.fmt.pix.pixelformat)
        {
          break;
        }

      fdesc.index++;
    }

  ctx->format.type = ctx->type;
  return ioctl(codec->fd, VIDIOC_TRY_FMT, &ctx->format) < 0 ? -errno : 0;
}

int nxcodec_context_set_format(FAR nxcodec_context_t *ctx)
{
  FAR nxcodec_t *codec = nxcodec_context_to_nxcodec(ctx);

  printf("nxcodec %s VIDIOC_S_FMT\n",
         V4L2_TYPE_IS_OUTPUT(ctx->type) ? "output" : "capture");

  return ioctl(codec->fd, VIDIOC_S_FMT, &ctx->format) < 0 ? -errno : 0;
}

int nxcodec_context_init(FAR nxcodec_context_t *ctx)
{
  FAR nxcodec_t *codec = nxcodec_context_to_nxcodec(ctx);
  struct v4l2_requestbuffers req;
  int ret;
  int i;

  memset(&req, 0, sizeof(req));
  req.count = NXCODEC_CONTEXT_BUFNUMBER;
  req.memory = V4L2_MEMORY_MMAP;
  req.type = ctx->type;

  ret = ioctl(codec->fd, VIDIOC_REQBUFS, &req);
  if (ret < 0)
    {
      printf("nxcodec type: %s, VIDIOC_REQBUFS failed: %s\n",
             V4L2_TYPE_IS_OUTPUT(ctx->type) ? "output" : "capture",
             strerror(errno));
      return -errno;
    }

  ctx->nbuffers = req.count;
  ctx->buf = calloc(ctx->nbuffers, sizeof(nxcodec_context_buf_t));
  if (!ctx->buf)
    {
      printf("nxcodec type: %s, alloc memory error\n",
             V4L2_TYPE_IS_OUTPUT(ctx->type) ? "output" : "capture");
      return -ENOMEM;
    }

  for (i = 0; i < ctx->nbuffers; i++)
    {
      FAR nxcodec_context_buf_t *buf = &ctx->buf[i];

      buf->buf.memory = V4L2_MEMORY_MMAP;
      buf->buf.type = ctx->type;
      buf->buf.index = i;

      ret = ioctl(codec->fd, VIDIOC_QUERYBUF, &buf->buf);
      if (ret < 0)
        {
          goto error;
        }

      buf->length = buf->buf.length;
      buf->addr = mmap(NULL,
                       buf->buf.length,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED,
                       codec->fd,
                       buf->buf.m.offset);

      if (buf->addr == MAP_FAILED)
        {
          goto error;
        }

      buf->free = true;

      if (V4L2_TYPE_IS_OUTPUT(ctx->type))
        {
          continue;
        }

      ret = ioctl(codec->fd, VIDIOC_QBUF, &buf->buf);
      if (ret < 0)
        {
          munmap(buf->addr, buf->length);
          goto error;
        }

      buf->free = false;
    }

  return 0;

error:
  free(ctx->buf);
  return -errno;
}

void nxcodec_context_uninit(FAR nxcodec_context_t *ctx)
{
  int i;

  if (!ctx->buf)
    {
      return;
    }

  for (i = 0; i < ctx->nbuffers; i++)
    {
      FAR nxcodec_context_buf_t *buf = &ctx->buf[i];

      if (buf->addr && buf->length)
        {
          if (munmap(buf->addr, buf->length) < 0)
            {
              printf("nxcodec type: %s, unmap plane (%s))\n",
                     V4L2_TYPE_IS_OUTPUT(ctx->type) ? "output" : "capture",
                     strerror(errno));
            }
        }
    }

  free(ctx->buf);
}
