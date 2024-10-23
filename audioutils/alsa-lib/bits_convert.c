/****************************************************************************
 * apps/audioutils/alsa-lib/bits_convert.c
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

#include "bits_convert.h"
#include "pcm_util.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int bitsconv_init(FAR struct bitsconv_data **bc, int channels,
                  snd_pcm_format_t src_format, snd_pcm_format_t dst_format,
                  int buf_size)
{
  if (*bc != NULL)
    {
      return 0;
    }

  /* support s16->s32, s32->s16 only */

  if ((src_format != SND_PCM_FORMAT_S16_LE &&
       src_format != SND_PCM_FORMAT_S32_LE) ||
      (dst_format != SND_PCM_FORMAT_S16_LE &&
       dst_format != SND_PCM_FORMAT_S32_LE))
    {
      SND_ERR("format not supported! src: %d, dst: %d", src_format,
              dst_format);
      return -ENOTSUP;
    }

  *bc = malloc(sizeof(struct bitsconv_data));
  if (*bc == NULL)
    {
      SND_ERR("bitsconv_init failed");
      return -ENOMEM;
    }

  (*bc)->bitsconv_buf = malloc(buf_size);
  if ((*bc)->bitsconv_buf == NULL)
    {
      free(*bc);
      *bc = NULL;
      SND_ERR("malloc bitsconv_buf failed");
      return -ENOMEM;
    }

  (*bc)->channels = channels;
  (*bc)->src_format = src_format;
  (*bc)->dst_format = dst_format;
  (*bc)->buf_size = buf_size;

  return 0;
}

int bitsconv_process(FAR struct bitsconv_data *bc, FAR const void *in_data,
                     int in_size, FAR void **out_data, FAR int *out_size)
{
  int out_len;
  int frames;
  int ch;

  if (!bc)
    {
      SND_ERR("bitsconv_init not run");
      return -EPERM;
    }

  if (bc->src_format == SND_PCM_FORMAT_S16_LE &&
      bc->dst_format == SND_PCM_FORMAT_S32_LE)
    {
      FAR const int16_t *src;
      FAR int32_t *dst;

      out_len = in_size * bc->channels * sizeof(int32_t);
      if (out_len > bc->buf_size)
        {
          SND_ERR("bitsconv out_buf too small");
          return -EPERM;
        }

      for (ch = 0; ch < bc->channels; ch++)
        {
          frames = in_size;
          src = (FAR const int16_t *)in_data;
          dst = (FAR int32_t *)bc->bitsconv_buf;
          while (frames-- > 0)
            {
              *(dst + ch) = (int32_t)((*(src + ch)) << 16);
              src += bc->channels;
              dst += bc->channels;
            }
        }
    }
  else if (bc->src_format == SND_PCM_FORMAT_S32_LE &&
           bc->dst_format == SND_PCM_FORMAT_S16_LE)
    {
      FAR int32_t *src;
      FAR int16_t *dst;

      out_len = in_size * bc->channels * sizeof(int16_t);
      if (out_len > bc->buf_size)
        {
          SND_ERR("bitsconv out_buf too small");
          return -EPERM;
        }

      for (ch = 0; ch < bc->channels; ch++)
        {
          frames = in_size;
          src = (FAR int32_t *)in_data;
          dst = (FAR int16_t *)bc->bitsconv_buf;
          while (frames-- > 0)
            {
              *(dst + ch) = (int16_t)((*(src + ch)) >> 16);
              src += bc->channels;
              dst += bc->channels;
            }
        }
    }
  else
    {
      SND_ERR("unknown format");
      return -ENOTSUP;
    }

  *out_data = bc->bitsconv_buf;
  *out_size = in_size;

  return 0;
}

int bitsconv_release(FAR struct bitsconv_data *bc)
{
  if (!bc)
    {
      return 0;
    }

  if (bc->bitsconv_buf)
    {
      free(bc->bitsconv_buf);
      bc->bitsconv_buf = NULL;
    }

  free(bc);

  return 0;
}
