/****************************************************************************
 * apps/system/ymodem/sb_main.c
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
#include <libgen.h>
#include <stdio.h>
#include <termios.h>
#include <sys/stat.h>

#include "ymodem.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

struct ymodem_fd
{
  int file_fd;
  FAR char **filelist;
  size_t filenum;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static ssize_t handler(FAR struct ymodem_ctx *ctx)
{
  FAR struct ymodem_fd *ym_fd = ctx->priv;
  ssize_t ret = -EINVAL;
  FAR char *filename;
  struct stat info;

  if (ctx->packet_type == YMODEM_FILE_SEND_NAME_PACKET)
    {
      if (ym_fd->file_fd != 0)
        {
          close(ym_fd->file_fd);
        }

      filename = ym_fd->filelist[ym_fd->filenum++];
      ret = lstat(filename, &info);
      if (ret < 0)
        {
          return ret;
        }

      ym_fd->file_fd = open(filename, O_RDWR);
      if (ym_fd->file_fd < 0)
        {
          return ym_fd->file_fd;
        }

      filename = basename(filename);
      strncpy(ctx->file_name, filename, YMODEM_FILE_NAME_LENGTH);
      ctx->file_length = info.st_size;
    }
  else if (ctx->packet_type == YMODEM_SEND_DATA_PACKET)
    {
      ret = read(ym_fd->file_fd, ctx->data, ctx->packet_size);
      if (ret < 0)
        {
          return ret;
        }
    }

  return ret;
}

static void show_usage(FAR const char *progname, int errcode)
{
  fprintf(stderr, "USAGE: %s [OPTIONS] <lname> [<lname> [<lname> ...]]\n",
                  progname);
  fprintf(stderr, "\nWhere:\n");
  fprintf(stderr, "\t<lname> is the local file name\n");
  fprintf(stderr, "\nand OPTIONS include the following:\n");
  fprintf(stderr,
    "\t-d <device>: Communication device to use. Default: stdin & stdout\n");

  exit(errcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ymodem_fd ym_fd;
  struct ymodem_ctx ctx;
  int option;

  memset(&ctx, 0, sizeof(struct ymodem_ctx));
  ctx.packet_handler = handler;
  ctx.timeout = 3000;
  ctx.recvfd = 0;
  ctx.sendfd = 1;
  ctx.priv = &ym_fd;
  while ((option = getopt(argc, argv, "d:h")) != ERROR)
    {
      switch (option)
        {
          case 'd':
            ctx.recvfd = open(optarg, O_RDWR);
            if (ctx.recvfd < 0)
              {
                fprintf(stderr, "ERROR: can't open %s\n", optarg);
              }

            ctx.sendfd = ctx.recvfd;
            break;

          case 'h':
            show_usage(argv[0], EXIT_FAILURE);

          default:
          case '?':
            fprintf(stderr, "ERROR: Unrecognized option\n");
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  ctx.need_sendfile_num = argc - optind;
  ym_fd.file_fd = 0;
  ym_fd.filelist = &argv[optind];
  ym_fd.filenum = 0;

  ymodem_send(&ctx);
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
