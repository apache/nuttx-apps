/****************************************************************************
 * apps/crypto/mbedtls/source/dev_alt.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "dev_alt.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int cryptodev_init(FAR cryptodev_context_t *ctx)
{
  int ret;
  int fd;

  memset(ctx, 0, sizeof(cryptodev_context_t));
  fd = open("/dev/crypto", O_RDWR, 0);
  if (fd < 0)
    {
      return -errno;
    }

  ret = ioctl(fd, CRIOGET, &ctx->fd);
  close(fd);
  if (ret < 0)
    {
      ret = -errno;
    }

  return ret;
}

void cryptodev_free(FAR cryptodev_context_t *ctx)
{
  close(ctx->fd);
  memset(ctx, 0, sizeof(cryptodev_context_t));
}

int cryptodev_get_session(FAR cryptodev_context_t *ctx)
{
  int ret;

  ret = ioctl(ctx->fd, CIOCGSESSION, &ctx->session);
  if (ret < 0)
    {
      return -errno;
    }

  ctx->crypt.ses = ctx->session.ses;
  return ret;
}

void cryptodev_free_session(FAR cryptodev_context_t *ctx)
{
  ioctl(ctx->fd, CIOCFSESSION, &ctx->session.ses);
  ctx->crypt.ses = 0;
}

int cryptodev_crypt(FAR cryptodev_context_t *ctx)
{
  int ret;

  ret = ioctl(ctx->fd, CIOCCRYPT, &ctx->crypt);
  return ret < 0 ? -errno : ret;
}

int cryptodev_clone(FAR cryptodev_context_t *dst,
                    FAR const cryptodev_context_t *src)
{
  dst->session = src->session;
  dst->crypt = src->crypt;
  return dup2(src->fd, dst->fd);
}
