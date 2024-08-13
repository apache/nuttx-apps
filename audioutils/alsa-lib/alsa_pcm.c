/****************************************************************************
 * apps/audioutils/alsa-lib/alsa_pcm.c
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

#include <alsa_pcm.h>

#include <nuttx/audio/audio.h>
#include <nuttx/config.h>

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Prototypes
 ****************************************************************************/

#define DEFAULT_AUDIO_PERIODS 4
#define DEFAULT_AUDIO_PERIOD_TIME 10 * 1000

#define FFMIN(a, b) ((a) > (b) ? (b) : (a))

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct _snd_pcm
{
  int fd;                            /* File descriptor of active device */
  mqd_t mq;                          /* Message queue for the playthread */
  char mqname[32];                   /* Name of our message queue */
  char devname[32];                  /* Preferred audio device */
  snd_pcm_stream_t stream;

  int volume;                        /* Volume as a whole percentage (0-100) */
  snd_pcm_format_t format;
  unsigned int channels;
  int sample_bytes;                  /* bytes per sample * channels */
  unsigned int sample_rate;

  int periods;
  int period_time;                   /* us */
  snd_pcm_uframes_t period_size;

  snd_pcm_uframes_t start_threshold; /* auto start with frames */

  dq_queue_t bufferq;
  snd_pcm_state_t state;
  bool stopping;
  bool running;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int snd_pcm_ioctl(int fd, int cmd, unsigned long arg)
{
  int ret;

  ret = ioctl(fd, cmd, arg);
  if (ret < 0)
    {
      ret = -errno;
    }

  return ret;
}

#define snd_pcm_ioctl(fd, cmd, arg)                                       \
  snd_pcm_ioctl(fd, cmd, (unsigned long)(arg))

static void snd_pcm_deinit(snd_pcm_t *pcm)
{
  if (pcm->fd < 0)
    {
      return;
    }

  if (pcm->mq >= 0)
    {
      snd_pcm_ioctl(pcm->fd, AUDIOIOC_UNREGISTERMQ, 0);

      mq_close(pcm->mq);
      pcm->mq = -1;

      mq_unlink(pcm->mqname);
    }

  snd_pcm_ioctl(pcm->fd, AUDIOIOC_RELEASE, 0);

  close(pcm->fd);
  free(pcm);
}

static int snd_pcm_init(snd_pcm_t *pcm)
{
  char path[32];
  int ret;

  struct mq_attr attr = {
      .mq_maxmsg = 16,
      .mq_msgsize = sizeof(struct audio_msg_s),
  };

  /* open device */

  snprintf(path, sizeof(path), CONFIG_AUDIOUTILS_ALSA_LIB_DEV_PATH "/%s",
           pcm->devname);
  pcm->fd = open(path, O_RDWR | O_CLOEXEC);
  if (pcm->fd < 0)
    {
      return -errno;
    }

  /* reserve */

  ret = snd_pcm_ioctl(pcm->fd, AUDIOIOC_RESERVE, 0);
  if (ret < 0)
    {
      goto out;
    }

  /* create message queue */

  snprintf(pcm->mqname, sizeof(pcm->mqname), "/tmp/%p", pcm);
  pcm->mq =
      mq_open(pcm->mqname, O_RDWR | O_CREAT | O_CLOEXEC, 0644, &attr);
  if (pcm->mq < 0)
    {
      ret = -errno;
      goto out;
    }

  ret = snd_pcm_ioctl(pcm->fd, AUDIOIOC_REGISTERMQ, pcm->mq);
  if (ret < 0)
    {
      goto out;
    }

  pcm->state = SND_PCM_STATE_OPEN;
  pcm->volume = 50;
  return 0;

out:
  snd_pcm_deinit(pcm);
  return ret;
}

static int snd_pcm_get_bits_per_sample(snd_pcm_format_t format)
{
  switch (format)
    {
    case SND_PCM_FORMAT_U32:
    case SND_PCM_FORMAT_S32:
      return 32;
    case SND_PCM_FORMAT_U24:
    case SND_PCM_FORMAT_S24:
      return 24;
    case SND_PCM_FORMAT_U16:
    case SND_PCM_FORMAT_S16:
      return 16;
    default:
      return 8;
    }
}

static int snd_pcm_poll_available(snd_pcm_t *pcm)
{
  struct audio_buf_desc_s buf_desc;
  int now;
  int old = dq_count(&pcm->bufferq);
  bool nonblock = false;
  audinfo("[%s %d]enter.", __func__, __LINE__);

  while (1)
    {
      struct ap_buffer_s *buffer;
      struct audio_msg_s msg;
      struct mq_attr stat;
      int ret;

      ret = mq_getattr(pcm->mq, &stat);
      if (ret < 0)
        {
          return -errno;
        }

      if (nonblock && !stat.mq_curmsgs)
        {
          break;
        }

      ret = mq_receive(pcm->mq, (char *)&msg, sizeof(msg), NULL);
      if (ret < 0)
        {
          return -errno;
        }

      if (msg.msg_id == AUDIO_MSG_DEQUEUE)
        {
          if (pcm->stopping)
            {
              buf_desc.u.buffer = msg.u.ptr;
              snd_pcm_ioctl(pcm->fd, AUDIOIOC_FREEBUFFER, &buf_desc);
            }
          else
            {
              buffer = msg.u.ptr;
              buffer->curbyte = 0;
              dq_addlast(&buffer->dq_entry, &pcm->bufferq);
            }
        }
      else if (msg.msg_id == AUDIO_MSG_COMPLETE)
        {
          audinfo("[%s][%s] complete\n", __func__, pcm->devname);
          pcm->stopping = false;
          return 0;
        }

      nonblock = true;
    }

  now = dq_count(&pcm->bufferq);
  if (pcm->periods > 1 && now == pcm->periods && now > old)
    {
      auderr("[%s %d]audio %s, xrun occurs!\n", __func__, __LINE__,
             pcm->devname);

      if (pcm->stream == SND_PCM_STREAM_PLAYBACK)
        {
          snd_pcm_pause(pcm, 1);
        }
      else
        {
          while (!dq_empty(&pcm->bufferq))
            {
              buf_desc.u.buffer =
                  (struct ap_buffer_s *)dq_remfirst(&pcm->bufferq);
              snd_pcm_ioctl(pcm->fd, AUDIOIOC_ENQUEUEBUFFER, &buf_desc);
            }
        }

      pcm->state = SND_PCM_STATE_XRUN;

      return -EPIPE;
    }

  return now;
}

static int snd_pcm_peek_buffer(snd_pcm_t *pcm, struct ap_buffer_s **buffer)
{
  int ret;

  *buffer = (struct ap_buffer_s *)dq_peek(&pcm->bufferq);
  if (*buffer)
    {
      return 0;
    }

  ret = snd_pcm_poll_available(pcm);
  if (ret < 0)
    {
      return ret;
    }

  *buffer = (struct ap_buffer_s *)dq_peek(&pcm->bufferq);
  if (!*buffer)
    {
      return -EAGAIN;
    }

  return 0;
}

#define SCALAR(name, type)                                                \
  static void scalar_##name(uint8_t *dst, const uint8_t *src,             \
                            int nb_samples, int volume)                   \
  {                                                                       \
    type *d = (type *)dst;                                                \
    const type *s = (type *)src;                                          \
    int i = 0;                                                            \
                                                                          \
    for (i = 0; i < nb_samples; i++)                                      \
      {                                                                   \
        d[i] = s[i] * (type)volume / 100;                                 \
      }                                                                   \
  }

SCALAR(s16, int16_t)
SCALAR(u16, uint16_t)
#ifdef __INT24_DEFINED
SCALAR(s24, int24_t)
SCALAR(u24, uint24_t)
#endif
SCALAR(s32, int32_t)
SCALAR(u32, uint32_t)

static void snd_pcm_voluem_scalar(snd_pcm_t *pcm, uint8_t *dest,
                                  const uint8_t *src, int len)
{
  switch (pcm->format)
    {
    case SND_PCM_FORMAT_S16:
      scalar_s16(dest, src, len / 2, pcm->volume);
      break;
    case SND_PCM_FORMAT_U16:
      scalar_u16(dest, src, len / 2, pcm->volume);
      break;
#ifdef __INT24_DEFINED
    case SND_PCM_FORMAT_S24:
      scalar_s24(dest, src, len / 3, pcm->volume);
      break;
    case SND_PCM_FORMAT_U24:
      scalar_u24(dest, src, len / 3, pcm->volume);
      break;
#endif
    case SND_PCM_FORMAT_S32:
      scalar_s32(dest, src, len / 4, pcm->volume);
      break;
    case SND_PCM_FORMAT_U32:
      scalar_u32(dest, src, len / 4, pcm->volume);
      break;
    default:
      audinfo("[%s %d]Not support yet! format:%d", __func__, __LINE__,
              pcm->format);
      assert(0);
    }
}

static int snd_pcm_enqueue_buffer(snd_pcm_t *pcm,
                                  struct ap_buffer_s *buffer)
{
  struct audio_buf_desc_s desc;
  audinfo("[%s %d]enter.", __func__, __LINE__);

  if (buffer->curbyte != buffer->nmaxbytes)
    {
      memset(buffer->samp + buffer->curbyte, 0,
             buffer->nmaxbytes - buffer->curbyte);
    }

  buffer->nbytes = buffer->curbyte;
  buffer->curbyte = 0;
  desc.u.buffer = buffer;
  return snd_pcm_ioctl(pcm->fd, AUDIOIOC_ENQUEUEBUFFER, &desc);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int snd_pcm_open(snd_pcm_t **pcm, const char *name,
                 snd_pcm_stream_t stream, int mode)
{
  int ret;

  if (!pcm || !name)
    {
      return -EINVAL;
    }

  if (stream < SND_PCM_STREAM_PLAYBACK || stream > SND_PCM_STREAM_LAST)
    {
      return -EINVAL;
    }

  *pcm = (snd_pcm_t *)malloc(sizeof(snd_pcm_t));
  if (!*pcm)
    {
      return -ENOMEM;
    }

  memset(*pcm, 0, sizeof(snd_pcm_t));
  strlcpy(((*pcm)->devname), name, sizeof((*pcm)->devname));
  (*pcm)->stream = stream;

  ret = snd_pcm_init(*pcm);

  audinfo("[%s %d]doen. ret:%d", __func__, __LINE__, ret);
  return ret;
}

int snd_pcm_prepare(snd_pcm_t *pcm)
{
  struct audio_caps_desc_s caps_desc;
  struct audio_buf_desc_s buf_desc;
  struct ap_buffer_info_s buf_info;
  int bps;
  int ret;
  int i;

  audinfo("[%s %d]enter.", __func__, __LINE__);

  if (pcm->state == SND_PCM_STATE_XRUN)
    {
      pcm->state = SND_PCM_STATE_PREPARED;
      return 0;
    }

  if (pcm->state != SND_PCM_STATE_SETUP)
    {
      return -EPERM;
    }

  bps = snd_pcm_get_bits_per_sample(pcm->format);
  pcm->sample_bytes = bps * pcm->channels / 8;

  memset(&caps_desc, 0, sizeof(caps_desc));
  caps_desc.caps.ac_len = sizeof(struct audio_caps_s);
  caps_desc.caps.ac_type = pcm->stream == SND_PCM_STREAM_PLAYBACK
                               ? AUDIO_TYPE_OUTPUT
                               : AUDIO_TYPE_INPUT;
  caps_desc.caps.ac_channels = pcm->channels;
  caps_desc.caps.ac_chmap = 0;
  caps_desc.caps.ac_controls.hw[0] = pcm->sample_rate;
  caps_desc.caps.ac_controls.b[3] = pcm->sample_rate >> 16;
  caps_desc.caps.ac_controls.b[2] = bps;
  caps_desc.caps.ac_subtype = AUDIO_FMT_PCM;

  ret = snd_pcm_ioctl(pcm->fd, AUDIOIOC_CONFIGURE, &caps_desc);
  if (ret < 0)
    {
      return ret;
    }

  audinfo("[%s %d]configure. ch:%d, rate:%d", __func__, __LINE__,
          pcm->channels, pcm->sample_rate);

  /* try set recommand params */

  if (!pcm->periods)
    {
      pcm->periods = DEFAULT_AUDIO_PERIODS;
    }

  if (pcm->period_time)
    {
      pcm->period_size = pcm->sample_rate / 1000 * pcm->period_time / 1000;
    }

  if (!pcm->period_size)
    {
      pcm->period_time = DEFAULT_AUDIO_PERIOD_TIME;
      pcm->period_size = pcm->sample_rate / 1000 * pcm->period_time / 1000;
    }

  buf_info.nbuffers = pcm->periods;
  buf_info.buffer_size = pcm->period_size * pcm->sample_bytes;
  snd_pcm_ioctl(pcm->fd, AUDIOIOC_SETBUFFERINFO, &buf_info);

  ret = snd_pcm_ioctl(pcm->fd, AUDIOIOC_GETBUFFERINFO, &buf_info);
  if (ret >= 0)
    {
      pcm->periods = buf_info.nbuffers;
      pcm->period_size = buf_info.buffer_size / pcm->sample_bytes;
    }

  pcm->start_threshold = pcm->period_size * pcm->sample_bytes;

  audinfo("[%s %d]set buffer info. n:%d, size:%ld", __func__, __LINE__,
          pcm->periods, pcm->period_size);

  dq_init(&pcm->bufferq);

  for (i = 0; i < pcm->periods; i++)
    {
      struct ap_buffer_s *buffer;

      buf_desc.numbytes = pcm->period_size * pcm->sample_bytes;
      buf_desc.u.pbuffer = &buffer;
      ret = snd_pcm_ioctl(pcm->fd, AUDIOIOC_ALLOCBUFFER, &buf_desc);
      if (ret < 0)
        {
          return ret;
        }

      if (pcm->stream == SND_PCM_STREAM_PLAYBACK)
        {
          dq_addlast(&buffer->dq_entry, &pcm->bufferq);
        }
      else
        {
          buffer->nbytes = buffer->nmaxbytes;
          buf_desc.u.buffer = buffer;
          ret = snd_pcm_ioctl(pcm->fd, AUDIOIOC_ENQUEUEBUFFER, &buf_desc);
          if (ret < 0)
            {
              return ret;
            }
        }
    }

  pcm->state = SND_PCM_STATE_PREPARED;

  return 0;
}

int snd_pcm_start(snd_pcm_t *pcm)
{
  int ret;
  if (pcm->running)
    {
      return 0;
    }

  if (pcm->state != SND_PCM_STATE_PREPARED)
    {
      return 0;
    }

  ret = snd_pcm_ioctl(pcm->fd, AUDIOIOC_START, 0);
  audinfo("[%s %d]start ret:%d\n", __func__, __LINE__, ret);
  if (ret < 0)
    {
      return ret;
    }

  pcm->state = SND_PCM_STATE_RUNNING;
  pcm->running = true;
  return ret;
}

int snd_pcm_pause(snd_pcm_t *pcm, int enable)
{
  int ret;
  if (!pcm->running)
    {
      return 0;
    }

  ret =
      snd_pcm_ioctl(pcm->fd, enable ? AUDIOIOC_PAUSE : AUDIOIOC_RESUME, 0);

  audinfo("[%s %d]enable:%d, pause ret:%d\n", __func__, __LINE__, enable,
          ret);
  if (ret >= 0)
    {
      pcm->state = enable ? SND_PCM_STATE_PAUSED : SND_PCM_STATE_RUNNING;
    }

  return ret;
}

int snd_pcm_close(snd_pcm_t *pcm)
{
  struct audio_buf_desc_s buf_desc;
  int dc = dq_count(&pcm->bufferq);
  audinfo("[%s %d]enter.\n", __func__, __LINE__);

  if (!pcm->running && pcm->state == SND_PCM_STATE_PREPARED && dc > 0 &&
      dc < pcm->periods)
    {
      snd_pcm_ioctl(pcm->fd, AUDIOIOC_START, 0);
      pcm->running = true;
    }

  if (pcm->running)
    {
      snd_pcm_ioctl(pcm->fd, AUDIOIOC_STOP, 0);
      pcm->running = false;
      pcm->stopping = true;
    }

  while (!dq_empty(&pcm->bufferq))
    {
      buf_desc.u.buffer = (struct ap_buffer_s *)dq_remfirst(&pcm->bufferq);
      snd_pcm_ioctl(pcm->fd, AUDIOIOC_FREEBUFFER, &buf_desc);
    }

  while (pcm->stopping)
    {
      snd_pcm_poll_available(pcm);
    }

  pcm->state = SND_PCM_STATE_SUSPENDED;

  snd_pcm_deinit(pcm);

  return 0;
}

int snd_pcm_delay(snd_pcm_t *pcm, snd_pcm_sframes_t *delayp)
{
  int ret;
  *delayp = 0;

  if (pcm->stream != SND_PCM_STREAM_PLAYBACK)
    {
      return 0;
    }

  if (pcm->state != SND_PCM_STATE_RUNNING)
    {
      return 0;
    }

  ret = snd_pcm_ioctl(pcm->fd, AUDIOIOC_GETLATENCY, delayp);
  audinfo("[%s %d]get delay:%ld", __func__, __LINE__, *delayp);

  return ret;
}

int snd_pcm_drop(snd_pcm_t *pcm)
{
  return 0;
}

int snd_pcm_drain(snd_pcm_t *pcm)
{
  return 0;
}

int snd_pcm_resume(snd_pcm_t *pcm)
{
  if (pcm->state == SND_PCM_STATE_XRUN)
    {
      pcm->state = SND_PCM_STATE_PREPARED;
    }

  return 0;
}

int snd_pcm_recover(snd_pcm_t *pcm, int err, int silent)
{
  if (pcm->state == SND_PCM_STATE_XRUN)
    {
      pcm->state = SND_PCM_STATE_PREPARED;
    }

  return 0;
}

int snd_pcm_wait(snd_pcm_t *pcm, int timeout)
{
  audinfo("[%s %d]enter. wait:%dms", __func__, __LINE__, timeout);
  usleep(timeout * 1000);
  return 0;
}

snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer,
                                 snd_pcm_uframes_t size)
{
  int left = snd_pcm_frames_to_bytes(pcm, size);
  struct ap_buffer_s *apb;
  const uint8_t *data = (uint8_t *)buffer;
  int ret = 0;
  audinfo("[%s %d]enter. size:%ld", __func__, __LINE__, size);

  while (left > 0 && !pcm->stopping)
    {
      int len;

      ret = snd_pcm_peek_buffer(pcm, &apb);
      if (ret == -EPIPE)
        {
          return ret;
        }
      else if (ret == -EAGAIN)
        {
          continue;
        }
      else if (ret < 0)
        {
          break;
        }

      len = FFMIN(apb->nmaxbytes - apb->curbyte, left);
      snd_pcm_voluem_scalar(pcm, (apb->samp + apb->curbyte), data, len);
      apb->curbyte += len;

      if (apb->curbyte == apb->nmaxbytes)
        {
          dq_remfirst(&pcm->bufferq);

          ret = snd_pcm_enqueue_buffer(pcm, apb);
          if (ret < 0)
            {
              break;
            }

          if (pcm->state == SND_PCM_STATE_PREPARED &&
              ((pcm->periods - dq_count(&pcm->bufferq)) *
                   pcm->period_size >=
               pcm->start_threshold))
            {
              if (pcm->running)
                {
                  ret = snd_pcm_pause(pcm, 0);
                }
              else
                {
                  ret = snd_pcm_start(pcm);
                }

              if (ret < 0)
                {
                  break;
                }
            }
        }

      data += len;
      left -= len;
    }

  return size - snd_pcm_bytes_to_frames(pcm, left);
}

int snd_pcm_set_volume(snd_pcm_t *pcm, int volume)
{
  if (volume < 0 || volume > 100)
    {
      return -EINVAL;
    }

  audinfo("[%s %d]set volume:%d", __func__, __LINE__, volume);
  pcm->volume = volume;

  return 0;
}

int snd_pcm_set_params(snd_pcm_t *pcm, snd_pcm_format_t format,
                       snd_pcm_access_t access, unsigned int channels,
                       unsigned int rate, int soft_resample,
                       unsigned int latency)
{
  audinfo("[%s %d]format:%d, ch:%d, rate:%d", __func__, __LINE__, format,
          channels, rate);
  assert(pcm);

  pcm->format = format;
  pcm->channels = channels;
  pcm->sample_rate = rate;

  pcm->state = SND_PCM_STATE_SETUP;

  return 0;
}

int snd_pcm_get_params(snd_pcm_t *pcm, snd_pcm_uframes_t *buffer_size,
                       snd_pcm_uframes_t *period_size)
{
  assert(pcm);

  *period_size = pcm->period_size;
  *buffer_size = pcm->period_size * pcm->periods;

  return 0;
}

snd_pcm_sframes_t snd_pcm_bytes_to_frames(snd_pcm_t *pcm, ssize_t bytes)
{
  assert(pcm);
  return bytes / pcm->sample_bytes;
}

ssize_t snd_pcm_frames_to_bytes(snd_pcm_t *pcm, snd_pcm_sframes_t frames)
{
  assert(pcm);
  return frames * pcm->sample_bytes;
}

#define FORMAT(v) [SND_PCM_FORMAT_##v] = #v

static const char *const snd_pcm_format_names[] =
{
  FORMAT(S8),
  FORMAT(U8),
  FORMAT(S16_LE),
  FORMAT(S16_BE),
  FORMAT(U16_LE),
  FORMAT(U16_BE),
  FORMAT(S24_LE),
  FORMAT(S24_BE),
  FORMAT(U24_LE),
  FORMAT(U24_BE),
  FORMAT(S32_LE),
  FORMAT(S32_BE),
  FORMAT(U32_LE),
  FORMAT(U32_BE),
};

const char *snd_pcm_format_name(snd_pcm_format_t format)
{
  if (format > SND_PCM_FORMAT_LAST)
    {
      return NULL;
    }

  return snd_pcm_format_names[format];
}

/****************************************************************************
 * Hw Functions
 ****************************************************************************/

static inline int hw_is_mask(int var)
{
  return var >= SND_PCM_HW_PARAM_FIRST_MASK &&
         var <= SND_PCM_HW_PARAM_LAST_MASK;
}

static inline int hw_is_range(int var)
{
  return var >= SND_PCM_HW_PARAM_FIRST_RANGE &&
         var <= SND_PCM_HW_PARAM_LAST_RANGE;
}

static int _snd_pcm_hw_param_set(snd_pcm_hw_params_t *params,
                                 snd_pcm_hw_param_t var, unsigned int val)
{
  union snd_interval *interval = NULL;

  assert(params);
  interval = &params->intervals[var - SND_PCM_HW_PARAM_FIRST_INTERVAL];
  if (hw_is_mask(var))
    {
      interval->mask = (1 << val);
    }
  else
    {
      interval->range.min = val;
    }

  return 0;
}

static int _snd_pcm_hw_param_get(const snd_pcm_hw_params_t *params,
                                 snd_pcm_hw_param_t var, unsigned int *val)
{
  const union snd_interval *interval;

  assert(params);
  interval = &params->intervals[var - SND_PCM_HW_PARAM_FIRST_INTERVAL];
  if (hw_is_mask(var))
    {
      *val = (interval->mask >> 1);
    }
  else
    {
      *val = interval->range.min;
    }

  return 0;
}

#define _snd_pcm_hw_param_set(params, var, val)                           \
  _snd_pcm_hw_param_set(params, var, (unsigned int)(val))
#define _snd_pcm_hw_param_get(params, var, val)                           \
  _snd_pcm_hw_param_get(params, var, (unsigned int *)(val))

static int _snd_pcm_hw_params_internal(snd_pcm_t *pcm,
                                       snd_pcm_hw_params_t *params)
{
  int period_size;
  int buffer_size;

  assert(pcm && params);

  _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_FORMAT, &pcm->format);
  if (!pcm->format)
    {
      pcm->format = SND_PCM_FORMAT_S16;
    }

  _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_CHANNELS, &pcm->channels);
  if (!pcm->channels)
    {
      pcm->channels = 2;
    }

  _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_RATE, &pcm->sample_rate);
  if (!pcm->sample_rate)
    {
      pcm->sample_rate = 44100;
    }

  _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIODS, &pcm->periods);
  _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIOD_TIME,
                        &pcm->period_time);
  _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIOD_SIZE,
                        &period_size);
  _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_BUFFER_SIZE,
                        &buffer_size);
  if (period_size && buffer_size)
    {
      pcm->periods = buffer_size / period_size;
      pcm->period_time = period_size * 1000 * 1000 / pcm->sample_rate;
    }

  audinfo("[%s %d]format:%d, ch:%d, rate:%d, periods:%d, period_time:%d",
          __func__, __LINE__, pcm->format, pcm->channels, pcm->sample_rate,
          pcm->periods, pcm->period_time);

  pcm->state = SND_PCM_STATE_SETUP;

  return 0;
}

int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
  return 0;
}

int snd_pcm_hw_params_set_format(snd_pcm_t *pcm,
                                 snd_pcm_hw_params_t *params,
                                 snd_pcm_format_t format)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_FORMAT, format);
}

int snd_pcm_hw_params_set_channels(snd_pcm_t *pcm,
                                   snd_pcm_hw_params_t *params,
                                   unsigned int val)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_CHANNELS, val);
}

int snd_pcm_hw_params_get_channels(const snd_pcm_hw_params_t *params,
                                   unsigned int *val)
{
  return _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_CHANNELS, val);
}

int snd_pcm_hw_params_set_rate(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
                               unsigned int val, int dir)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_RATE, val);
}

int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm,
                                    snd_pcm_hw_params_t *params,
                                    unsigned int *val, int *dir)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_RATE, *val);
}

int snd_pcm_hw_params_get_rate(const snd_pcm_hw_params_t *params,
                               unsigned int *val, int *dir)
{
  return _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_RATE, val);
}

int snd_pcm_hw_params_set_period_time(snd_pcm_t *pcm,
                                      snd_pcm_hw_params_t *params,
                                      unsigned int us, int dir)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIOD_TIME, us);
}

int snd_pcm_hw_params_set_period_time_near(snd_pcm_t *pcm,
                                           snd_pcm_hw_params_t *params,
                                           unsigned int *us, int *dir)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIOD_TIME, *us);
}

int snd_pcm_hw_params_get_period_time(const snd_pcm_hw_params_t *params,
                                      unsigned int *us, int *dir)
{
  return _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIOD_TIME, us);
}

int snd_pcm_hw_params_set_period_size(snd_pcm_t *pcm,
                                      snd_pcm_hw_params_t *params,
                                      snd_pcm_uframes_t val, int dir)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIOD_SIZE, val);
}

int snd_pcm_hw_params_set_period_size_near(snd_pcm_t *pcm,
                                           snd_pcm_hw_params_t *params,
                                           snd_pcm_uframes_t *val,
                                           int *dir)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIOD_SIZE, *val);
}

int snd_pcm_hw_params_get_period_size(const snd_pcm_hw_params_t *params,
                                      snd_pcm_uframes_t *val, int *dir)
{
  return _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIOD_SIZE, val);
}

int snd_pcm_hw_params_set_periods(snd_pcm_t *pcm,
                                  snd_pcm_hw_params_t *params,
                                  unsigned int val, int dir)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIODS, val);
}

int snd_pcm_hw_params_set_periods_near(snd_pcm_t *pcm,
                                       snd_pcm_hw_params_t *params,
                                       unsigned int *val, int *dir)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIODS, *val);
}

int snd_pcm_hw_params_get_periods(const snd_pcm_hw_params_t *params,
                                  unsigned int *val, int *dir)
{
  return _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIODS, val);
}

int snd_pcm_hw_params_set_buffer_size(snd_pcm_t *pcm,
                                      snd_pcm_hw_params_t *params,
                                      snd_pcm_uframes_t val)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_BUFFER_SIZE, val);
}

int snd_pcm_hw_params_set_buffer_size_near(snd_pcm_t *pcm,
                                           snd_pcm_hw_params_t *params,
                                           snd_pcm_uframes_t *val)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_BUFFER_SIZE, *val);
}

int snd_pcm_hw_params_get_buffer_size(const snd_pcm_hw_params_t *params,
                                      snd_pcm_uframes_t *val)
{
  return _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_BUFFER_SIZE, val);
}

int snd_pcm_hw_params_set_buffer_time(snd_pcm_t *pcm,
                                      snd_pcm_hw_params_t *params,
                                      unsigned int us)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_BUFFER_TIME, us);
}

int snd_pcm_hw_params_set_buffer_time_near(snd_pcm_t *pcm,
                                           snd_pcm_hw_params_t *params,
                                           unsigned int *us, int *dir)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_BUFFER_TIME, *us);
}

int snd_pcm_hw_params_get_buffer_time(const snd_pcm_hw_params_t *params,
                                      unsigned int *us, int *dir)
{
  return _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_BUFFER_TIME, us);
}

int snd_pcm_hw_params_set_access(snd_pcm_t *pcm,
                                 snd_pcm_hw_params_t *params,
                                 snd_pcm_access_t access)
{
  return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_ACCESS, access);
}

int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
  int err;
  assert(pcm && params);
  err = _snd_pcm_hw_params_internal(pcm, params);
  if (err < 0)
    {
      return err;
    }

  err = snd_pcm_prepare(pcm);
  return err;
}

/****************************************************************************
 * Sw Functions
 ****************************************************************************/

int snd_pcm_sw_params_current(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
  assert(pcm && params);

  memset(params, 0, sizeof(snd_pcm_sw_params_t));
  params->start_threshold = pcm->start_threshold;

  return 0;
}

int snd_pcm_sw_params_set_start_threshold(snd_pcm_t *pcm,
                                          snd_pcm_sw_params_t *params,
                                          snd_pcm_uframes_t val)
{
  assert(pcm && params);

  params->start_threshold = val;
  return 0;
}

int snd_pcm_sw_params_get_start_threshold(snd_pcm_sw_params_t *params,
                                          snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->start_threshold;
  return 0;
}

int snd_pcm_sw_params_set_stop_threshold(snd_pcm_t *pcm,
                                         snd_pcm_sw_params_t *params,
                                         snd_pcm_uframes_t val)
{
  assert(pcm && params);

  params->stop_threshold = val;
  return 0;
}

int snd_pcm_sw_params_get_stop_threshold(snd_pcm_sw_params_t *params,
                                         snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->stop_threshold;
  return 0;
}

int snd_pcm_sw_params_set_silence_size(snd_pcm_t *pcm,
                                       snd_pcm_sw_params_t *params,
                                       snd_pcm_uframes_t val)
{
  assert(pcm && params);

  params->silence_size = val;
  return 0;
}

int snd_pcm_sw_params_get_silence_size(snd_pcm_sw_params_t *params,
                                       snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->silence_size;
  return 0;
}

int snd_pcm_sw_params_set_avail_min(snd_pcm_t *pcm,
                                    snd_pcm_sw_params_t *params,
                                    snd_pcm_uframes_t val)
{
  assert(pcm && params);

  params->avail_min = val;
  return 0;
}

int snd_pcm_sw_params_get_avail_min(const snd_pcm_sw_params_t *params,
                                    snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->avail_min;
  return 0;
}

int snd_pcm_sw_params_get_boundary(const snd_pcm_sw_params_t *params,
                                   snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->boundary;
  return 0;
}

int snd_pcm_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
  assert(pcm && params);

  pcm->start_threshold = params->start_threshold;
  return 0;
}
