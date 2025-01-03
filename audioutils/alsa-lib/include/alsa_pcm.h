/****************************************************************************
 * apps/audioutils/alsa-lib/include/alsa_pcm.h
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

#ifndef __ALSA_PCM_H
#define __ALSA_PCM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <alloca.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Pre-processor Prototypes
 ****************************************************************************/

#define SND_PCM_HW_PARAM_ACCESS 0
#define SND_PCM_HW_PARAM_FORMAT 1
#define SND_PCM_HW_PARAM_FIRST_MASK SND_PCM_HW_PARAM_ACCESS
#define SND_PCM_HW_PARAM_LAST_MASK SND_PCM_HW_PARAM_FORMAT
#define SND_PCM_HW_PARAM_SAMPLE_BITS 2
#define SND_PCM_HW_PARAM_FRAME_BITS 3
#define SND_PCM_HW_PARAM_CHANNELS 4
#define SND_PCM_HW_PARAM_RATE 5
#define SND_PCM_HW_PARAM_PERIOD_TIME 6
#define SND_PCM_HW_PARAM_PERIOD_SIZE 7
#define SND_PCM_HW_PARAM_PERIOD_BYTES 8
#define SND_PCM_HW_PARAM_PERIODS 9
#define SND_PCM_HW_PARAM_BUFFER_TIME 10
#define SND_PCM_HW_PARAM_BUFFER_SIZE 11
#define SND_PCM_HW_PARAM_BUFFER_BYTES 12
#define SND_PCM_HW_PARAM_FIRST_RANGE SND_PCM_HW_PARAM_SAMPLE_BITS
#define SND_PCM_HW_PARAM_LAST_RANGE SND_PCM_HW_PARAM_BUFFER_BYTES
#define SND_PCM_HW_PARAM_FIRST_INTERVAL SND_PCM_HW_PARAM_ACCESS
#define SND_PCM_HW_PARAM_LAST_INTERVAL SND_PCM_HW_PARAM_BUFFER_BYTES

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Unsigned frames quantity */

typedef unsigned long snd_pcm_uframes_t;

/* Signed frames quantity */

typedef long snd_pcm_sframes_t;

typedef int snd_pcm_hw_param_t;

/* PCM handle */

typedef struct _snd_pcm snd_pcm_t;

/* PCM stream (direction) */

typedef enum _snd_pcm_stream
{
  /* Playback stream */

  SND_PCM_STREAM_PLAYBACK = 0,

  /* Capture stream */

  SND_PCM_STREAM_CAPTURE,
  SND_PCM_STREAM_LAST = SND_PCM_STREAM_CAPTURE
} snd_pcm_stream_t;

/* PCM access type */

typedef enum _snd_pcm_access
{
  /* mmap access with simple interleaved channels */

  SND_PCM_ACCESS_MMAP_INTERLEAVED = 0,

  /* mmap access with simple non interleaved channels */

  SND_PCM_ACCESS_MMAP_NONINTERLEAVED,

  /* mmap access with complex placement */

  SND_PCM_ACCESS_MMAP_COMPLEX,

  /* snd_pcm_readi/snd_pcm_writei access */

  SND_PCM_ACCESS_RW_INTERLEAVED,

  /* snd_pcm_readn/snd_pcm_writen access */

  SND_PCM_ACCESS_RW_NONINTERLEAVED,
  SND_PCM_ACCESS_LAST = SND_PCM_ACCESS_RW_NONINTERLEAVED
} snd_pcm_access_t;

/* PCM sample format */

typedef enum _snd_pcm_format
{
  /* Unknown */

  SND_PCM_FORMAT_UNKNOWN = -1,

  /* Signed 8 bit */

  SND_PCM_FORMAT_S8 = 0,

  /* Unsigned 8 bit */

  SND_PCM_FORMAT_U8,

  /* Signed 16 bit Little Endian */

  SND_PCM_FORMAT_S16_LE,

  /* Signed 16 bit Big Endian */

  SND_PCM_FORMAT_S16_BE,

  /* Unsigned 16 bit Little Endian */

  SND_PCM_FORMAT_U16_LE,

  /* Unsigned 16 bit Big Endian */

  SND_PCM_FORMAT_U16_BE,

  /* Signed 24 bit Little Endian using low three bytes in 32-bit word */

  SND_PCM_FORMAT_S24_LE,

  /* Signed 24 bit Big Endian using low three bytes in 32-bit word */

  SND_PCM_FORMAT_S24_BE,

  /* Unsigned 24 bit Little Endian using low three bytes in 32-bit word */

  SND_PCM_FORMAT_U24_LE,

  /* Unsigned 24 bit Big Endian using low three bytes in 32-bit word */

  SND_PCM_FORMAT_U24_BE,

  /* Signed 32 bit Little Endian */

  SND_PCM_FORMAT_S32_LE,

  /* Signed 32 bit Big Endian */

  SND_PCM_FORMAT_S32_BE,

  /* Unsigned 32 bit Little Endian */

  SND_PCM_FORMAT_U32_LE,

  /* Unsigned 32 bit Big Endian */

  SND_PCM_FORMAT_U32_BE,

  /* only support little endian Signed 16 bit CPU endian */

  SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_LE,

  /* Unsigned 16 bit CPU endian */

  SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_LE,

  /* Signed 24 bit CPU endian */

  SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_LE,

  /* Unsigned 24 bit CPU endian */

  SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_LE,

  /* Signed 32 bit CPU endian */

  SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_LE,

  /* Unsigned 32 bit CPU endian */

  SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_LE,

  SND_PCM_FORMAT_LAST = SND_PCM_FORMAT_U32_BE,
} snd_pcm_format_t;

/* PCM state */

typedef enum _snd_pcm_state
{
  /* Open */

  SND_PCM_STATE_OPEN = 0,

  /* Setup installed */

  SND_PCM_STATE_SETUP,

  /* Ready to start */

  SND_PCM_STATE_PREPARED,

  /* Running */

  SND_PCM_STATE_RUNNING,

  /* Stopped: underrun (playback) or overrun (capture) detected */

  SND_PCM_STATE_XRUN,

  /* Draining: running (playback) or stopped (capture) */

  SND_PCM_STATE_DRAINING,

  /* Paused */

  SND_PCM_STATE_PAUSED,

  /* Hardware is suspended */

  SND_PCM_STATE_SUSPENDED,

  /* Hardware is disconnected */

  SND_PCM_STATE_DISCONNECTED,
  SND_PCM_STATE_LAST = SND_PCM_STATE_DISCONNECTED,

  /* Private - used internally in the library - do not use */

  SND_PCM_STATE_PRIVATE1 = 1024
} snd_pcm_state_t;

union snd_interval
{
  struct
  {
    uint32_t min;
    uint32_t max;
    int openmin; /* whether the interval is left-open */
    int openmax; /* whether the interval is right-open */
    int integer; /* whether the value is integer or not */
    int empty;
  } range;
  uint32_t mask;
};

typedef struct snd_pcm_hw_params
{
  union snd_interval intervals[SND_PCM_HW_PARAM_LAST_INTERVAL -
                               SND_PCM_HW_PARAM_FIRST_INTERVAL + 1];
} snd_pcm_hw_params_t;

typedef struct snd_pcm_sw_params
{
  snd_pcm_uframes_t avail_min;       /* min avail frames for wakeup */
  snd_pcm_uframes_t start_threshold; /* min hw_avail frames for automatic start */
  snd_pcm_uframes_t stop_threshold;  /* min avail frames for automatic stop */
  snd_pcm_uframes_t silence_size;    /* silence block size */
  snd_pcm_uframes_t boundary;        /* pointers wrap point */
} snd_pcm_sw_params_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int snd_pcm_open(FAR snd_pcm_t **pcm, FAR const char *name,
                 snd_pcm_stream_t stream, int mode);
int snd_pcm_close(FAR snd_pcm_t *pcm);
int snd_pcm_prepare(FAR snd_pcm_t *pcm);
int snd_pcm_start(FAR snd_pcm_t *pcm);
int snd_pcm_pause(FAR snd_pcm_t *pcm, int enable);
int snd_pcm_delay(FAR snd_pcm_t *pcm, FAR snd_pcm_sframes_t *delayp);
int snd_pcm_drop(FAR snd_pcm_t *pcm);
int snd_pcm_drain(FAR snd_pcm_t *pcm);
int snd_pcm_resume(FAR snd_pcm_t *pcm);
int snd_pcm_recover(FAR snd_pcm_t *pcm, int err, int silent);
int snd_pcm_wait(FAR snd_pcm_t *pcm, int timeout);

snd_pcm_sframes_t snd_pcm_writei(FAR snd_pcm_t *pcm, FAR const void *buffer,
                                 snd_pcm_uframes_t size);

int snd_pcm_set_volume(FAR snd_pcm_t *pcm, int volume);
int snd_pcm_set_params(FAR snd_pcm_t *pcm, snd_pcm_format_t format,
                       snd_pcm_access_t access, unsigned int channels,
                       unsigned int rate, int soft_resample,
                       unsigned int latency);
int snd_pcm_get_params(FAR snd_pcm_t *pcm,
                       FAR snd_pcm_uframes_t *buffer_size,
                       FAR snd_pcm_uframes_t *period_size);

snd_pcm_sframes_t snd_pcm_bytes_to_frames(FAR snd_pcm_t *pcm, ssize_t bytes);
ssize_t snd_pcm_frames_to_bytes(FAR snd_pcm_t *pcm,
                                snd_pcm_sframes_t frames);

FAR const char *snd_pcm_format_name(const snd_pcm_format_t format);

/* hw & sw define */

#define __snd_alloca(ptr, type)                 \
    do {                                        \
        *ptr = (FAR type*)alloca(sizeof(type)); \
        memset(*ptr, 0, sizeof(type));          \
    } while (0)

#define snd_pcm_hw_params_alloca(ptr) __snd_alloca(ptr, snd_pcm_hw_params_t)

int snd_pcm_hw_params_any(FAR snd_pcm_t *pcm,
                          FAR snd_pcm_hw_params_t *params);

int snd_pcm_hw_params_set_format(FAR snd_pcm_t *pcm,
                                 FAR snd_pcm_hw_params_t *params,
                                 snd_pcm_format_t format);

int snd_pcm_hw_params_set_channels(FAR snd_pcm_t *pcm,
                                   FAR snd_pcm_hw_params_t *params,
                                   unsigned int val);
int snd_pcm_hw_params_get_channels(FAR const snd_pcm_hw_params_t *params,
                                   FAR unsigned int *val);

int snd_pcm_hw_params_set_rate(FAR snd_pcm_t *pcm,
                               FAR snd_pcm_hw_params_t *params,
                               unsigned int val, int dir);
int snd_pcm_hw_params_set_rate_near(FAR snd_pcm_t *pcm,
                                    FAR snd_pcm_hw_params_t *params,
                                    FAR unsigned int *val, FAR int *dir);
int snd_pcm_hw_params_get_rate(FAR const snd_pcm_hw_params_t *params,
                               FAR unsigned int *val, FAR int *dir);

int snd_pcm_hw_params_set_period_time(FAR snd_pcm_t *pcm,
                                      FAR snd_pcm_hw_params_t *params,
                                      unsigned int us, int dir);
int snd_pcm_hw_params_set_period_time_near(FAR snd_pcm_t *pcm,
                                           FAR snd_pcm_hw_params_t *params,
                                           FAR unsigned int *us,
                                           FAR int *dir);
int snd_pcm_hw_params_get_period_time(FAR const snd_pcm_hw_params_t *params,
                                      FAR unsigned int *us, FAR int *dir);

int snd_pcm_hw_params_set_period_size(FAR snd_pcm_t *pcm,
                                      FAR snd_pcm_hw_params_t *params,
                                      snd_pcm_uframes_t val, int dir);
int snd_pcm_hw_params_set_period_size_near(FAR snd_pcm_t *pcm,
                                           FAR snd_pcm_hw_params_t *params,
                                           FAR snd_pcm_uframes_t *val,
                                           FAR int *dir);
int snd_pcm_hw_params_get_period_size(FAR const snd_pcm_hw_params_t *params,
                                      FAR snd_pcm_uframes_t *val,
                                      FAR int *dir);

int snd_pcm_hw_params_set_periods(FAR snd_pcm_t *pcm,
                                  FAR snd_pcm_hw_params_t *params,
                                  unsigned int val, int dir);
int snd_pcm_hw_params_set_periods_near(FAR snd_pcm_t *pcm,
                                       FAR snd_pcm_hw_params_t *params,
                                       FAR unsigned int *val, FAR int *dir);
int snd_pcm_hw_params_get_periods(FAR const snd_pcm_hw_params_t *params,
                                  FAR unsigned int *val, FAR int *dir);

int snd_pcm_hw_params_set_buffer_size(FAR snd_pcm_t *pcm,
                                      FAR snd_pcm_hw_params_t *params,
                                      snd_pcm_uframes_t val);
int snd_pcm_hw_params_set_buffer_size_near(FAR snd_pcm_t *pcm,
                                           FAR snd_pcm_hw_params_t *params,
                                           FAR snd_pcm_uframes_t *val);
int snd_pcm_hw_params_get_buffer_size(FAR const snd_pcm_hw_params_t *params,
                                      FAR snd_pcm_uframes_t *val);

int snd_pcm_hw_params_set_buffer_time(FAR snd_pcm_t *pcm,
                                      FAR snd_pcm_hw_params_t *params,
                                      unsigned int us);
int snd_pcm_hw_params_set_buffer_time_near(FAR snd_pcm_t *pcm,
                                           FAR snd_pcm_hw_params_t *params,
                                           FAR unsigned int *us,
                                           FAR int *dir);
int snd_pcm_hw_params_get_buffer_time(FAR const snd_pcm_hw_params_t *params,
                                      FAR unsigned int *us, FAR int *dir);

int snd_pcm_hw_params_set_access(FAR snd_pcm_t *pcm,
                                 FAR snd_pcm_hw_params_t *params,
                                 snd_pcm_access_t access);

int snd_pcm_hw_params(FAR snd_pcm_t *pcm, FAR snd_pcm_hw_params_t *params);

#define snd_pcm_sw_params_alloca(ptr) __snd_alloca(ptr, snd_pcm_sw_params_t)

int snd_pcm_sw_params_current(FAR snd_pcm_t *pcm,
                              FAR snd_pcm_sw_params_t *params);

int snd_pcm_sw_params_set_start_threshold(FAR snd_pcm_t *pcm,
                                          FAR snd_pcm_sw_params_t *params,
                                          snd_pcm_uframes_t val);
int snd_pcm_sw_params_get_start_threshold(FAR snd_pcm_sw_params_t *params,
                                          FAR snd_pcm_uframes_t *val);

int snd_pcm_sw_params_set_stop_threshold(FAR snd_pcm_t *pcm,
                                         FAR snd_pcm_sw_params_t *params,
                                         snd_pcm_uframes_t val);
int snd_pcm_sw_params_get_stop_threshold(FAR snd_pcm_sw_params_t *params,
                                         FAR snd_pcm_uframes_t *val);

int snd_pcm_sw_params_set_silence_size(FAR snd_pcm_t *pcm,
                                       FAR snd_pcm_sw_params_t *params,
                                       snd_pcm_uframes_t val);
int snd_pcm_sw_params_get_silence_size(FAR snd_pcm_sw_params_t *params,
                                       FAR snd_pcm_uframes_t *val);

int snd_pcm_sw_params_set_avail_min(FAR snd_pcm_t *pcm,
                                    FAR snd_pcm_sw_params_t *params,
                                    snd_pcm_uframes_t val);
int snd_pcm_sw_params_get_avail_min(FAR const snd_pcm_sw_params_t *params,
                                    FAR snd_pcm_uframes_t *val);

int snd_pcm_sw_params_get_boundary(FAR const snd_pcm_sw_params_t *params,
                                   FAR snd_pcm_uframes_t *val);

int snd_pcm_sw_params(FAR snd_pcm_t *pcm, FAR snd_pcm_sw_params_t *params);

#ifdef __cplusplus
}
#endif

#endif /* __ALSA_PCM_H */
