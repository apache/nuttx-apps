/****************************************************************************
 * apps/audioutils/alsa-lib/pcm_mixer_default.c
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

#include <byteswap.h>

#include "pcm_mixer_default.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void default_mixer_8(unsigned int size, FAR volatile int8_t *dst,
                             FAR int8_t *src)
{
  register int16_t sample;

  while (size-- > 0)
    {
      if (*dst == 0)
        {
          *dst = *src;
        }
      else
        {
          sample = *src + *dst;
          if (sample > INT8_MAX)
            {
              sample = INT8_MAX;
            }
          else if (sample < INT8_MIN)
            {
              sample = INT8_MIN;
            }

          *dst = (signed short)sample;
        }

      src++;
      dst++;
    }
}

static void default_mixer_16(unsigned int size, FAR volatile int16_t *dst,
                             FAR int16_t *src)
{
  register int32_t sample;

  while (size-- > 0)
    {
      if (*dst == 0)
        {
          *dst = *src;
        }
      else
        {
          sample = *src + *dst;
          if (sample > INT16_MAX)
            {
              sample = INT16_MAX;
            }
          else if (sample < INT16_MIN)
            {
              sample = INT16_MIN;
            }

          *dst = (signed short)sample;
        }

      src++;
      dst++;
    }
}

static void default_mixer_24(unsigned int size,
                             FAR volatile unsigned char *dst,
                             FAR unsigned char *src)
{
  register signed int sample;

  while (size-- > 0)
    {
      sample =
          (src[0] | (src[1] << 8) | (((FAR signed char *)src)[2] << 16));

      if (dst[0] | dst[1] | dst[2])
        {
          sample += (dst[0] | (dst[1] << 8) | (dst[2] << 16));

          if (sample > 0x7fffff)
            {
              sample = 0x7fffff;
            }
          else if (sample < -0x800000)
            {
              sample = -0x800000;
            }
        }

      dst[0] = (unsigned char)(sample & 0xff);
      dst[1] = (unsigned char)((sample >> 8) & 0xff);
      dst[2] = (unsigned char)((sample >> 16) & 0xff);

      dst += 3;
      src += 3;
    }
}

static void default_mixer_32(unsigned int size, FAR volatile int32_t *dst,
                             FAR int32_t *src)
{
  register int32_t sample;

  while (size-- > 0)
    {
      if (*dst == 0)
        {
          *dst = *src;
        }
      else
        {
          sample = (*src >> 8) + (*dst >> 8);

          if (sample > 0x7fffff)
            {
              sample = INT32_MAX;
            }
          else if (sample < -0x800000)
            {
              sample = INT32_MIN;
            }
          else
            {
              sample *= 256;
            }

          *dst = sample;
        }

      src++;
      dst++;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void default_mixer_init(FAR snd_pcm_dmix_t *dmix)
{
  switch (dmix->format)
    {
    case SND_PCM_FORMAT_S16_LE:
    case SND_PCM_FORMAT_S16_BE:
      dmix->mixer.mix_engine = (FAR mix_engine_t *)default_mixer_16;
      break;
    case SND_PCM_FORMAT_S32_LE:
    case SND_PCM_FORMAT_S32_BE:
      dmix->mixer.mix_engine = (FAR mix_engine_t *)default_mixer_32;
      break;
    case SND_PCM_FORMAT_S24_LE:
    case SND_PCM_FORMAT_S24_BE:
      dmix->mixer.mix_engine = (FAR mix_engine_t *)default_mixer_24;
      break;
    case SND_PCM_FORMAT_S8:
    case SND_PCM_FORMAT_U8:
      dmix->mixer.mix_engine = (FAR mix_engine_t *)default_mixer_8;
      break;
    default:
      return;
    }
}
