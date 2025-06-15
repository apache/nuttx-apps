/****************************************************************************
 * apps/system/ymodem/sb_main.c
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

#include <fcntl.h>
#include <stdio.h>
#include <getopt.h>
#include <libgen.h>
#include <pthread.h>
#include <sys/stat.h>

#include <nuttx/circbuf.h>

#include "ymodem.h"

/****************************************************************************
 * Private Typess
 ****************************************************************************/

struct ymodem_priv_s
{
  int fd;
  FAR char **filelist;
  size_t filenum;

  /* Async */

  struct circbuf_s circ;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  size_t buffersize;
  pthread_t pid;
  bool exited;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *async_read(FAR void *arg)
{
  FAR struct ymodem_priv_s *priv = arg;

  pthread_mutex_lock(&priv->mutex);
  while (priv->exited == false)
    {
      FAR uint8_t *buffer;
      ssize_t ret = 0;
      size_t size;

      if (priv->fd > 0)
        {
          buffer = circbuf_get_writeptr(&priv->circ, &size);
          ret = read(priv->fd, buffer, size);
          if (ret < 0)
            {
              break;
            }

          circbuf_writecommit(&priv->circ, ret);
        }

      if (circbuf_is_full(&priv->circ) || priv->fd <= 0 || ret == 0)
        {
          pthread_cond_wait(&priv->cond, &priv->mutex);
        }
    }

  pthread_mutex_unlock(&priv->mutex);
  return NULL;
}

static int read_data(FAR struct ymodem_priv_s *priv,
                     FAR uint8_t *data, size_t size)
{
  ssize_t i = 0;

  if (priv->buffersize)
    {
      pthread_mutex_lock(&priv->mutex);
      while (i < size)
        {
          ssize_t ret = circbuf_read(&priv->circ, data + i, size - i);

          if (ret < 0)
            {
              pthread_mutex_unlock(&priv->mutex);
              return ret;
            }

          if (ret == 0)
            {
              ret = read(priv->fd, data + i, size - i);
              if (ret < 0)
                {
                  pthread_mutex_unlock(&priv->mutex);
                  return -errno;
                }
            }

          i += ret;
        }

      pthread_cond_signal(&priv->cond);
      pthread_mutex_unlock(&priv->mutex);
    }
  else
    {
      while (i < size)
        {
          ssize_t ret = read(priv->fd, data + i, size - i);

          if (ret < 0)
            {
              return -errno;
            }

          i += ret;
        }
    }

  return 0;
}

static int handler(FAR struct ymodem_ctx_s *ctx)
{
  FAR struct ymodem_priv_s *priv = ctx->priv;
  int ret;

  if (ctx->packet_type == YMODEM_FILENAME_PACKET)
    {
      FAR char *filename;
      struct stat st;

      if (priv->fd > 0)
        {
          close(priv->fd);
          priv->fd = 0;
        }

      filename = priv->filelist[priv->filenum++];
      if (filename == NULL)
        {
          return -ENOENT;
        }

      ret = stat(filename, &st);
      if (ret < 0)
        {
          return -errno;
        }

      priv->fd = open(filename, O_RDONLY);
      if (priv->fd < 0)
        {
          return -errno;
        }

      filename = basename(filename);
      strlcpy(ctx->file_name, filename, PATH_MAX);
      ctx->file_length = st.st_size;
    }
  else if (ctx->packet_type == YMODEM_DATA_PACKET)
    {
      size_t size;

      if (ctx->file_length > ctx->packet_size)
        {
          size = ctx->packet_size;
        }
      else
        {
          size = ctx->file_length;
          memset(ctx->data + size, 0x1a,
                 ctx->packet_size - ctx->file_length);
        }

      ret = read_data(priv, ctx->data, size);
      if (ret < 0)
        {
          return ret;
        }

      ctx->file_length -= size;
    }

  return 0;
}

static int async_init(FAR struct ymodem_priv_s *priv)
{
  int ret;

  ret = circbuf_init(&priv->circ, NULL, priv->buffersize);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: circbuf_init error\n");
      return ret;
    }

  ret = pthread_mutex_init(&priv->mutex, NULL);
  if (ret > 0)
    {
      fprintf(stderr, "ERROR: pthread_mutex_init error\n");
      ret = -ret;
      goto circ_err;
    }

  ret = pthread_cond_init(&priv->cond, NULL);
  if (ret > 0)
    {
      fprintf(stderr, "ERROR: pthread_cond_init error\n");
      ret = -ret;
      goto mutex_err;
    }

  ret = pthread_create(&priv->pid, NULL, async_read, priv);
  if (ret < 0)
    {
      ret = -ret;
      goto cond_err;
    }

  return ret;
cond_err:
  pthread_cond_destroy(&priv->cond);
mutex_err:
  pthread_mutex_destroy(&priv->mutex);
circ_err:
  circbuf_uninit(&priv->circ);
  return ret;
}

static void async_uninit(FAR struct ymodem_priv_s *priv)
{
  pthread_mutex_lock(&priv->mutex);
  priv->exited = true;
  pthread_cond_signal(&priv->cond);
  pthread_mutex_unlock(&priv->mutex);
  pthread_join(priv->pid, NULL);
  pthread_cond_destroy(&priv->cond);
  pthread_mutex_destroy(&priv->mutex);
  circbuf_uninit(&priv->circ);
}

static void show_usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE: %s [OPTIONS] <lname> [<lname> [<lname> ...]]\n",
                  progname);
  fprintf(stderr, "\nWhere:\n");
  fprintf(stderr, "\t<lname> is the local file name\n");
  fprintf(stderr, "\nand OPTIONS include the following:\n");
  fprintf(stderr,
    "\t-d <device>: Communication device to use. Default: stdin & stdout\n");
  fprintf(stderr,
          "\t-b|--buffersize <size>: Asynchronously send buffer size."
          "If greater than 0, accept data asynchronously, Default: 0kB\n");
  fprintf(stderr,
          "\t-i|--interval <time>: Waiting interval for transmitting data."
          "Max:255 Min:1 Default:15 unit: 100 milliseconds\n");
  fprintf(stderr,
          "\t-r|--retry <retry>: Number of retries."
          "Will try <retry> times to transmitting, Default:100\n");
  fprintf(stderr,
          "\t-k <size>: Use a custom size to tansfer, Default: 1kB\n");

  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ymodem_priv_s priv;
  struct ymodem_ctx_s ctx;
  FAR char *devname = NULL;
  int ret = 0;
  struct option options[] =
    {
      {"buffersize", 1, NULL, 'b'},
      {"interval", 1, NULL, 'i'},
      {"retry", 1, NULL, 'r'},
    };

  memset(&priv, 0, sizeof(priv));
  memset(&ctx, 0, sizeof(ctx));
  ctx.interval = 15;
  ctx.retry = 100;
  while ((ret = getopt_long(argc, argv, "b:d:k:h", options, NULL))
         != ERROR)
    {
      switch (ret)
        {
          case 'b':
            priv.buffersize = atoi(optarg) * 1024;
            break;
          case 'd':
            devname = optarg;
            break;
          case 'k':
            ctx.custom_size = atoi(optarg) * 1024;
            if (ctx.custom_size == 0)
              {
                show_usage(argv[0]);
              }

            break;
          case 'i':
            ctx.interval = atoi(optarg);
            break;
          case 'r':
            ctx.retry = atoi(optarg);
            break;

          case 'h':
          case '?':
          default:
            show_usage(argv[0]);
            break;
        }
    }

  if (priv.buffersize && ctx.custom_size > priv.buffersize)
    {
      show_usage(argv[0]);
    }

  ctx.packet_handler = handler;
  if (devname)
    {
      ctx.recvfd = open(devname, O_RDWR);
      if (ctx.recvfd < 0)
        {
          fprintf(stderr, "ERROR: can't open %s\n", devname);
          return -errno;
        }

        ctx.sendfd = ctx.recvfd;
    }
  else
    {
      ctx.recvfd = STDIN_FILENO;
      ctx.sendfd = STDOUT_FILENO;
    }

  ctx.priv = &priv;
  priv.filelist = &argv[optind];
  if (priv.buffersize)
    {
      ret = async_init(&priv);
      if (ret < 0)
        {
          goto out;
        }
    }

  ret = ymodem_send(&ctx);

  if (priv.buffersize)
    {
      async_uninit(&priv);
    }

out:
  if (priv.fd > 0)
    {
      close(priv.fd);
    }

  if (ctx.recvfd > 0)
    {
      close(ctx.recvfd);
    }

  return ret;
}
