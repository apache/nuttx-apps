/****************************************************************************
 * apps/audioutils/alsa-lib/pcm_dmix.h
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

#ifndef __ALSA_PCM_DMIX_H
#define __ALSA_PCM_DMIX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <speex_resampler.h>

#include "bits_convert.h"
#include "channels_map.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void(mix_engine_t)(unsigned int size, FAR volatile void *dst,
                           FAR void *src);

typedef struct snd_pcm_mmap_status
{
  volatile unsigned long read_head;
  volatile unsigned long read_tail;
} snd_pcm_mmap_status_t;

typedef struct
{
  unsigned int refs;
  snd_pcm_format_t format;
  unsigned int channels;
  unsigned int sample_rate;
  snd_pcm_uframes_t period_frames;
  snd_pcm_uframes_t buffer_frames;
} snd_pcm_dmix_share_t;

typedef struct snd_pcm_dmix
{
  char ipc_key[16];                  /* IPC key for semaphore and memory */
  FAR sem_t *semid;                  /* IPC global semaphore identification */
  FAR snd_pcm_dmix_share_t *shmptr;  /* Pointer to shared memory area */

  unsigned long appl_ptr;
  unsigned long appl_reset;
  FAR snd_pcm_mmap_status_t *hw_ptr;

  snd_pcm_format_t format;
  unsigned int channels;
  unsigned int sample_rate;

  int sample_bits;
  int frame_bytes;

  int periods;
  int period_time;
  int period_bytes;
  int buffer_bytes;

  snd_pcm_uframes_t period_frames;
  snd_pcm_uframes_t buffer_frames;

  FAR struct bitsconv_data *bc;
  FAR struct bitsconv_data *bc_s16;
  FAR struct chmap_data *cm;
  FAR SpeexResamplerState *resampler;
  FAR uint8_t *resmp_out;

  struct
  {
    FAR uint8_t *mix_buffer;
    FAR mix_engine_t *mix_engine;
  } mixer;
} snd_pcm_dmix_t;

#endif /* __ALSA_PCM_DMIX_H */