/****************************************************************************
 * apps/audioutils/alsa-lib/pcm_util.h
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

#ifndef __ALSA_PCM_UTIL_H
#define __ALSA_PCM_UTIL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <mqueue.h>
#include <sys/ioctl.h>
#include <syslog.h>

#include <alsa/error.h>
#include <alsa/pcm.h>

/****************************************************************************
 * Pre-processor Prototypes
 ****************************************************************************/

#define SND_PCM_DEFAULT_PERIODS 4
#define SND_PCM_DEFAULT_PERIOD_TIME (20 * 1000)

#define _log(level, fmt, args...)                                \
  syslog(level, "[SND][%s:%d] " fmt, __func__, __LINE__, ##args)

#define _none(fmt, args...)                                      \
  do { if (0) syslog(LOG_ERR, fmt, ##args); } while (0)

#if defined(CONFIG_AUDIOUTILS_ALSA_LIB_LOG_DEBUG)
#define SND_DEBUG(fmt, args...) _log(LOG_DEBUG, fmt, ##args)
#define SND_INFO(fmt, args...) _log(LOG_INFO, fmt, ##args)
#define SND_WARN(fmt, args...) _log(LOG_WARNING, fmt, ##args)
#define SND_ERR(fmt, args...) _log(LOG_ERR, fmt, ##args)
#elif defined(CONFIG_AUDIOUTILS_ALSA_LIB_LOG_INFO)
#define SND_DEBUG(fmt, args...) _none(fmt, ##args)
#define SND_INFO(fmt, args...) _log(LOG_INFO, fmt, ##args)
#define SND_WARN(fmt, args...) _log(LOG_WARNING, fmt, ##args)
#define SND_ERR(fmt, args...) _log(LOG_ERR, fmt, ##args)
#elif defined(CONFIG_AUDIOUTILS_ALSA_LIB_LOG_WARN)
#define SND_DEBUG(fmt, args...) _none(fmt, ##args)
#define SND_INFO(fmt, args...) _none(fmt, ##args)
#define SND_WARN(fmt, args...) _log(LOG_WARNING, fmt, ##args)
#define SND_ERR(fmt, args...) _log(LOG_ERR, fmt, ##args)
#elif defined(CONFIG_AUDIOUTILS_ALSA_LIB_LOG_ERR)
#define SND_DEBUG(fmt, args...) _none(fmt, ##args)
#define SND_INFO(fmt, args...) _none(fmt, ##args)
#define SND_WARN(fmt, args...) _none(fmt, ##args)
#define SND_ERR(fmt, args...) _log(LOG_ERR, fmt, ##args)
#else
#define SND_DEBUG(fmt, args...) _none(fmt, ##args)
#define SND_INFO(fmt, args...) _none(fmt, ##args)
#define SND_WARN(fmt, args...) _none(fmt, ##args)
#define SND_ERR(fmt, args...) _none(fmt, ##args)
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct
{
  snd_pcm_state_t (*state)(FAR snd_pcm_t *pcm);
  int (*delay)(FAR snd_pcm_t *pcm, FAR snd_pcm_sframes_t *delayp);
  int (*prepare)(FAR snd_pcm_t *pcm);
  int (*reset)(FAR snd_pcm_t *pcm);
  int (*start)(FAR snd_pcm_t *pcm);
  void (*dump)(FAR snd_pcm_t *pcm);
  int (*drop)(FAR snd_pcm_t *pcm);
  int (*drain)(FAR snd_pcm_t *pcm);
  int (*close)(FAR snd_pcm_t *pcm);
  int (*pause)(FAR snd_pcm_t *pcm, int enable);
  int (*resume)(FAR snd_pcm_t *pcm);
  snd_pcm_sframes_t (*avail_update)(FAR snd_pcm_t *pcm);
  snd_pcm_sframes_t (*writei)(FAR snd_pcm_t *pcm, FAR const void *buffer,
                              snd_pcm_uframes_t size);
  int (*poll_descriptors)(FAR snd_pcm_t *pcm, FAR struct pollfd *pfds,
                          unsigned int space);
} snd_pcm_ops_t;

typedef struct
{
  snd_pcm_uframes_t nmaxframes; /* The maximum number of bytes */
  snd_pcm_uframes_t nframes;    /* The number of bytes used */
  FAR uint8_t *data;            /* Offset of the first sample */
} pcm_buffer_t;

struct snd_pcm_s
{
  int fd;          /* File descriptor of active device */
  mqd_t mq;        /* Message queue for the playthread */
  char mqname[32]; /* Name of our message queue */
  int dump_fd;     /* File descriptor for dump raw pcm data */
  int vis_fd;      /* File descriptor for visualizer */

  int setup;
  FAR char *name;
  int mode;
  snd_pcm_type_t type;
  snd_pcm_access_t access;
  snd_pcm_stream_t stream;
  snd_pcm_uframes_t start_threshold; /* auto start with frames */

  float volume; /* Volume percentage in [0-1] */
  snd_pcm_format_t format;
  unsigned int channels;
  unsigned int sample_rate;

  snd_pcm_state_t state;
  bool running;

  int sample_bits;
  int frame_bytes;
  int periods;
  int period_time; /* us */
  int period_bytes;
  int buffer_bytes;

  snd_pcm_uframes_t period_frames;
  snd_pcm_uframes_t buffer_frames;

  dq_queue_t bufferq;
  pcm_buffer_t last_buffer;

  FAR snd_pcm_ops_t *ops;
  FAR snd_pcm_t *ops_arg;
  FAR void *private_data;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int snd_pcm_new(FAR snd_pcm_t **pcmp, snd_pcm_type_t type,
                FAR const char *name, snd_pcm_stream_t stream, int mode);
int snd_pcm_free(FAR snd_pcm_t *pcm);
int snd_pcm_init(FAR snd_pcm_t *pcm);
void snd_pcm_deinit(FAR snd_pcm_t *pcm);

int snd_pcm_hw_open(FAR snd_pcm_t **pcmp, FAR const char *name,
                    snd_pcm_stream_t stream, int mode);
int snd_pcm_dmix_open(FAR snd_pcm_t **pcmp, FAR const char *name,
                      snd_pcm_stream_t stream, int mode);

void snd_pcm_vol_soft_scalar(FAR snd_pcm_t *pcm, FAR uint8_t *dest,
                             FAR const uint8_t *src, int samples);

#endif /* __ALSA_PCM_UTIL_H */