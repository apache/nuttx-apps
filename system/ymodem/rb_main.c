/****************************************************************************
 * apps/system/ymodem/rb_main.c
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

#include <fcntl.h>
#include <stdio.h>
#include <getopt.h>
#include <nuttx/fs/ioctl.h>
#include "ymodem.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ymodem_fd
{
  int file_fd;
  char pathname[PATH_MAX];
  size_t file_saved_size;
  char *removeperfix;
  char *removesuffix;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int handler(FAR struct ymodem_ctx *ctx)
{
  char pathname[PATH_MAX + YMODEM_FILE_NAME_LENGTH];
  FAR struct ymodem_fd *ym_fd = ctx->priv;
  FAR char *filename;
  size_t size;
  size_t ret;

  if (ctx->packet_type == YMODEM_FILE_RECV_NAME_PACKET)
    {
      if (ym_fd->file_fd != 0)
        {
          close(ym_fd->file_fd);
        }

      filename = ctx->file_name;
      if (ym_fd->removeperfix)
        {
          if (strncmp(ctx->file_name, ym_fd->removeperfix,
                      strlen(ym_fd->removeperfix)) == 0)
            {
              filename = filename + strlen(ym_fd->removeperfix);
            }
        }

      if (ym_fd->removesuffix)
        {
          int len = strlen(filename);
          if (len > strlen(ym_fd->removesuffix) &&
              strcmp(filename + len - strlen(ym_fd->removesuffix),
                     ym_fd->removesuffix) == 0)
            {
              filename[len - strlen(ym_fd->removesuffix)] = 0;
            }
        }

      if (strlen(ym_fd->pathname) != 0)
        {
          sprintf(pathname, "%s/%s", ym_fd->pathname, filename);
          filename = pathname;
        }

      ym_fd->file_fd = open(filename, O_CREAT | O_RDWR);
      if (ym_fd->file_fd < 0)
        {
          return -errno;
        }

      ym_fd->file_saved_size = 0;
    }
  else if (ctx->packet_type == YMODEM_RECV_DATA_PACKET)
    {
      if (ym_fd->file_saved_size + ctx->packet_size > ctx->file_length)
        {
          size = ctx->file_length - ym_fd->file_saved_size;
        }
      else
        {
          size = ctx->packet_size;
        }

      ret = write(ym_fd->file_fd, ctx->data, size);
      if (ret < 0)
        {
          return -errno;
        }
      else if (ret < size)
        {
          return ERROR;
        }

      ym_fd->file_saved_size += ret;
    }

  return 0;
}

static void show_usage(FAR const char *progname, int errcode)
{
  fprintf(stderr, "USAGE: %s [OPTIONS]\n", progname);
  fprintf(stderr, "\nWhere:\n");
  fprintf(stderr, "\nand OPTIONS include the following:\n");
  fprintf(stderr,
    "\t-d <device>: Communication device to use. Default: stdin & stdout\n");
  fprintf(stderr,
          "\t-p <path>: Save remote file path. Default: Current path\n");
  fprintf(stderr,
          "\t--removeprefix <prefix>: Remove save file name prefix\n");
  fprintf(stderr,
          "\t--removesuffix <suffix>: Remove save file name suffix\n");
  exit(errcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ymodem_fd ym_fd;
  struct ymodem_ctx ctx;
  int index;
  int ret;
  struct option options[] =
    {
      {"removeprefix", 1, NULL, 1},
      {"removesuffix", 1, NULL, 2},
    };

  memset(&ym_fd, 0, sizeof(struct ymodem_fd));
  memset(&ctx, 0, sizeof(struct ymodem_ctx));
  ctx.packet_handler = handler;
  ctx.timeout = 200;
  ctx.priv = &ym_fd;
  ctx.recvfd = 0;
  ctx.sendfd = 1;
  while ((ret = getopt_long(argc, argv, "p:d:h", options, &index))
         != ERROR)
    {
      switch (ret)
        {
          case 1:
            ym_fd.removeperfix = optarg;
            break;
          case 2:
            ym_fd.removesuffix = optarg;
            break;
          case 'p':
            strlcpy(ym_fd.pathname, optarg, PATH_MAX);
            if (ym_fd.pathname[strlen(ym_fd.pathname)] == '/')
              {
                ym_fd.pathname[strlen(ym_fd.pathname)] = 0;
              }

            break;
          case 'd':
            ctx.recvfd = open(optarg, O_RDWR | O_NONBLOCK);
            if (ctx.recvfd < 0)
              {
                fprintf(stderr, "ERROR: can't open %s\n", optarg);
              }

            ctx.sendfd = ctx.recvfd;
            break;
          case 'h':
            show_usage(argv[0], EXIT_FAILURE);
            break;
          default:
          case '?':
            fprintf(stderr, "ERROR: Unrecognized option\n");
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  ymodem_recv(&ctx);
  if (ctx.recvfd)
    {
      close(ctx.recvfd);
    }

  if (ym_fd.file_fd)
    {
      close(ym_fd.file_fd);
    }

  return 0;
}
