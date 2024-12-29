/****************************************************************************
 * apps/system/ymodem/rb_main.c
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
#include <pthread.h>

#include <nuttx/circbuf.h>

#include "ymodem.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ymodem_priv_s
{
  int fd;
  FAR char *foldname;
  size_t file_saved_size;
  FAR char *skip_perfix;
  FAR char *skip_suffix;

  /* Async */

  struct circbuf_s circ;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  size_t buffersize;
  size_t threshold;
  pthread_t pid;
  bool exited;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int flush_data(FAR struct ymodem_priv_s *priv)
{
  while (priv->fd > 0 && circbuf_used(&priv->circ))
    {
      FAR uint8_t *buffer;
      size_t i = 0;
      size_t size;

      buffer = circbuf_get_readptr(&priv->circ, &size);
      while (i < size)
        {
          ssize_t ret = write(priv->fd, buffer + i, size - i);
          if (ret < 0)
            {
              return -errno;
            }

          i += ret;
        }

      circbuf_readcommit(&priv->circ, size);
    }

  return 0;
}

static FAR void *async_write(FAR void *arg)
{
  FAR struct ymodem_priv_s *priv = arg;

  pthread_mutex_lock(&priv->mutex);
  while (priv->exited == false)
    {
      if (circbuf_used(&priv->circ) <= priv->threshold)
        {
          pthread_cond_wait(&priv->cond, &priv->mutex);
          continue;
        }

      if (flush_data(priv) < 0)
        {
          pthread_mutex_unlock(&priv->mutex);
          return NULL;
        }
    }

  flush_data(priv);
  pthread_mutex_unlock(&priv->mutex);
  return NULL;
}

static int write_data(FAR struct ymodem_priv_s *priv,
                      FAR const uint8_t *data, size_t size)
{
  size_t i = 0;

  if (priv->buffersize)
    {
      pthread_mutex_lock(&priv->mutex);
      while (i < size)
        {
          ssize_t ret = circbuf_write(&priv->circ, data + i, size - i);
          if (ret < 0)
            {
              pthread_mutex_unlock(&priv->mutex);
              return ret;
            }
          else if (ret == 0)
            {
              ret = flush_data(priv);
              if (ret < 0)
                {
                  pthread_mutex_unlock(&priv->mutex);
                  return ret;
                }
            }
          else
            {
              i += ret;
            }
        }

      if (circbuf_used(&priv->circ) > priv->threshold)
        {
          pthread_cond_signal(&priv->cond);
        }

      pthread_mutex_unlock(&priv->mutex);
    }
  else
    {
      while (i < size)
        {
          ssize_t ret = write(priv->fd, data + i, size - i);
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
      char temp[PATH_MAX + 1];
      FAR char *filename;

      if (priv->fd > 0)
        {
          if (priv->buffersize)
            {
              pthread_mutex_lock(&priv->mutex);
              ret = flush_data(priv);
              pthread_mutex_unlock(&priv->mutex);
              if (ret < 0)
                {
                  return ret;
                }
            }

          close(priv->fd);
        }

      filename = ctx->file_name;
      if (priv->skip_perfix != NULL)
        {
          size_t len = strlen(priv->skip_perfix);

          if (strncmp(ctx->file_name, priv->skip_perfix,
                      len) == 0)
            {
              filename += len;
            }
        }

      if (priv->skip_suffix != NULL)
        {
          size_t len = strlen(filename);
          size_t len2 = strlen(priv->skip_suffix);

          if (len > len2 && strcmp(filename + len - len2,
                                   priv->skip_suffix) == 0)
            {
              filename[len - len2] = '\0';
            }
        }

      if (priv->foldname != NULL)
        {
          snprintf(temp, sizeof(temp), "%s/%s", priv->foldname,
                   filename);
          filename = temp;
        }

      priv->fd = open(filename, O_CREAT | O_WRONLY, 0777);
      if (priv->fd < 0)
        {
          return -errno;
        }

      priv->file_saved_size = 0;
    }
  else if (ctx->packet_type == YMODEM_DATA_PACKET)
    {
      size_t size;

      if (priv->file_saved_size + ctx->packet_size > ctx->file_length)
        {
          size = ctx->file_length - priv->file_saved_size;
        }
      else
        {
          size = ctx->packet_size;
        }

      ret = write_data(priv, ctx->data, size);
      if (ret < 0)
        {
          return ret;
        }

      priv->file_saved_size += size;
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

  ret = pthread_create(&priv->pid, NULL, async_write, priv);
  if (ret > 0)
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
  fprintf(stderr, "USAGE: %s [OPTIONS]\n", progname);
  fprintf(stderr, "\nWhere:\n");
  fprintf(stderr, "\nand OPTIONS include the following:\n");
  fprintf(stderr,
          "\t-d <device>: Communication device to use."
           "Default: stdin & stdout\n");
  fprintf(stderr,
          "\t-f <foldname>: Save remote file fold. Default: Current fold\n");
  fprintf(stderr,
          "\t-p|--skip_prefix <prefix>: Remove file name prefix\n");
  fprintf(stderr,
          "\t-s|--skip_suffix <suffix>: Remove file name suffix\n");
  fprintf(stderr,
          "\t-b|--buffersize <size>: Asynchronously receive buffer size."
          "If greater than 0, accept data asynchronously, Default: 0kB\n");
  fprintf(stderr,
          "\t-t|--threshold <size>: Threshold for writing asynchronously."
          "Threshold must be less than or equal buffersize, Default: 0kB\n");
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
  int ret;

  struct option options[] =
    {
      {"buffersize", 1, NULL, 'b'},
      {"skip_prefix", 1, NULL, 'p'},
      {"skip_suffix", 1, NULL, 's'},
      {"threshold", 1, NULL, 't'},
      {"interval", 1, NULL, 'i'},
      {"retry", 1, NULL, 'r'},
    };

  memset(&priv, 0, sizeof(priv));
  memset(&ctx, 0, sizeof(ctx));
  ctx.interval = 15;
  ctx.retry = 100;
  while ((ret = getopt_long(argc, argv, "b:d:f:hk:p:s:t:i:r:",
                            options, NULL)) != ERROR)
    {
      switch (ret)
        {
          case 'b':
            priv.buffersize = atoi(optarg) * 1024;
            break;
          case 'd':
            devname = optarg;
            break;
          case 'f':
            priv.foldname = optarg;
            if (priv.foldname[strlen(priv.foldname)] == '/')
              {
                priv.foldname[strlen(priv.foldname)] = '\0';
              }

            break;
          case 'h':
            show_usage(argv[0]);
            break;
          case 'k':
            ctx.custom_size = atoi(optarg) * 1024;
            break;
          case 'p':
            priv.skip_perfix = optarg;
            break;
          case 's':
            priv.skip_suffix = optarg;
            break;
          case 't':
            priv.threshold = atoi(optarg) * 1024;
            break;
          case 'i':
            ctx.interval = atoi(optarg);
            break;
          case 'r':
            ctx.retry = atoi(optarg);
            break;

          case '?':
          default:
            show_usage(argv[0]);
            break;
        }
    }

  if (priv.buffersize && (priv.threshold > priv.buffersize ||
                          ctx.custom_size > priv.buffersize))
    {
      show_usage(argv[0]);
    }

  if (priv.buffersize)
    {
      ret = async_init(&priv);
      if (ret < 0)
        {
          return ret;
        }
    }

  ctx.packet_handler = handler;
  ctx.priv = &priv;
  if (devname)
    {
      ctx.recvfd = open(devname, O_RDWR);
      if (ctx.recvfd < 0)
        {
          fprintf(stderr, "ERROR: can't open %s\n", devname);
          goto out;
        }

        ctx.sendfd = ctx.recvfd;
    }
  else
    {
      ctx.recvfd = STDIN_FILENO;
      ctx.sendfd = STDOUT_FILENO;
    }

  ret = ymodem_recv(&ctx);
  if (ctx.recvfd > 0)
    {
      close(ctx.recvfd);
    }

out:
  if (priv.buffersize)
    {
      async_uninit(&priv);
    }

  if (priv.fd > 0)
    {
      close(priv.fd);
    }

  return ret;
}
