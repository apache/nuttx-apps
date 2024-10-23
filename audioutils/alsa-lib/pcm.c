/****************************************************************************
 * apps/audioutils/alsa-lib/pcm.c
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
#include <math.h>
#include <sys/stat.h>

#include <nuttx/audio/audio.h>

#include "pcm_util.h"

/****************************************************************************
 * Pre-processor Prototypes
 ****************************************************************************/

#define CHECK_PCM_SETUP(pcm)                                                \
  do                                                                        \
    {                                                                       \
      assert(pcm);                                                          \
      if (!(pcm)->setup)                                                    \
        {                                                                   \
          SND_ERR("PCM not set up");                                        \
          return -EIO;                                                      \
        }                                                                   \
    }                                                                       \
  while (0)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int snd_pcm_open(FAR snd_pcm_t **pcmp, FAR const char *name,
                 snd_pcm_stream_t stream, int mode)
{
  assert(pcmp && name);

  if (strcmp(name, "default") == 0)
    {
      name = "pcm0p";
    }

#if defined(CONFIG_AUDIOUTILS_ALSA_LIB_DEVICE_DMIX)
  return snd_pcm_dmix_open(pcmp, name, stream, mode);
#elif defined(CONFIG_AUDIOUTILS_ALSA_LIB_DEVICE_HW)
  return snd_pcm_hw_open(pcmp, name, stream, mode);
#else
  return -EPERM;
#endif
}

int snd_pcm_close(FAR snd_pcm_t *pcm)
{
  int ret;

  assert(pcm);
  ret = pcm->ops->close(pcm->ops_arg);

  snd_pcm_free(pcm);
  return ret;
}

FAR const char *snd_pcm_name(FAR snd_pcm_t *pcm)
{
  assert(pcm);
  return pcm->name;
}

snd_pcm_type_t snd_pcm_type(FAR snd_pcm_t *pcm)
{
  assert(pcm);
  return pcm->type;
}

snd_pcm_stream_t snd_pcm_stream(FAR snd_pcm_t *pcm)
{
  assert(pcm);
  return pcm->stream;
}

int snd_pcm_poll_descriptors_count(FAR snd_pcm_t *pcm)
{
  return 1;
}

int snd_pcm_poll_descriptors(FAR snd_pcm_t *pcm, FAR struct pollfd *pfds,
                             unsigned int space)
{
  assert(pcm && pfds);
  return pcm->ops->poll_descriptors(pcm->ops_arg, pfds, space);
}

int snd_pcm_nonblock(FAR snd_pcm_t *pcm, int nonblock)
{
  assert(pcm);
  if (nonblock)
    {
      pcm->mode |= SND_PCM_NONBLOCK;
    }
  else
    {
      pcm->mode &= ~SND_PCM_NONBLOCK;
    }

  return 0;
}

int snd_pcm_prepare(FAR snd_pcm_t *pcm)
{
  CHECK_PCM_SETUP(pcm);

  return pcm->ops->prepare(pcm->ops_arg);
}

int snd_pcm_reset(FAR snd_pcm_t *pcm)
{
  CHECK_PCM_SETUP(pcm);

  return pcm->ops->reset(pcm->ops_arg);
}

int snd_pcm_start(FAR snd_pcm_t *pcm)
{
  int ret;
  CHECK_PCM_SETUP(pcm);

  snd_pcm_dump(pcm, stdout);

  if (pcm->ops->start)
    {
      return pcm->ops->start(pcm->ops_arg);
    }

  if (pcm->state != SND_PCM_STATE_PREPARED)
    {
      return 0;
    }

  ret = ioctl(pcm->fd, AUDIOIOC_START, 0);
  if (ret < 0)
    {
      SND_ERR("start error:%d\n", ret);
      return ret;
    }

  pcm->state = SND_PCM_STATE_RUNNING;
  pcm->running = true;

  return ret;
}

int snd_pcm_pause(FAR snd_pcm_t *pcm, int enable)
{
  int ret;
  CHECK_PCM_SETUP(pcm);

  if (pcm->ops->pause)
    {
      return pcm->ops->pause(pcm->ops_arg, enable);
    }

  if (!pcm->running)
    {
      return 0;
    }

  ret = ioctl(pcm->fd, enable ? AUDIOIOC_PAUSE : AUDIOIOC_RESUME, 0);
  if (ret < 0)
    {
      SND_ERR("pause:%d, ret:%d", enable, ret);
      return ret;
    }

  if (enable)
    {
      pcm->state = SND_PCM_STATE_PAUSED;
    }
  else
    {
      pcm->state = SND_PCM_STATE_RUNNING;
      snd_pcm_reset(pcm);
    }

  return ret;
}

snd_pcm_state_t snd_pcm_state(FAR snd_pcm_t *pcm)
{
  assert(pcm);

  if (pcm->ops->state)
    {
      return pcm->ops->state(pcm->ops_arg);
    }

  return pcm->state;
}

int snd_pcm_delay(FAR snd_pcm_t *pcm, FAR snd_pcm_sframes_t *delayp)
{
  CHECK_PCM_SETUP(pcm);

  return pcm->ops->delay(pcm->ops_arg, delayp);
}

int snd_pcm_drop(FAR snd_pcm_t *pcm)
{
  CHECK_PCM_SETUP(pcm);

  return pcm->ops->drop(pcm->ops_arg);
}

int snd_pcm_drain(FAR snd_pcm_t *pcm)
{
  CHECK_PCM_SETUP(pcm);

  return pcm->ops->drain(pcm->ops_arg);
}

int snd_pcm_resume(FAR snd_pcm_t *pcm)
{
  CHECK_PCM_SETUP(pcm);

  return pcm->ops->resume(pcm->ops_arg);
}

snd_pcm_sframes_t snd_pcm_avail_update(FAR snd_pcm_t *pcm)
{
  CHECK_PCM_SETUP(pcm);

  return pcm->ops->avail_update(pcm->ops_arg);
}

int snd_pcm_wait(FAR snd_pcm_t *pcm, int timeout)
{
  struct pollfd pfd;
  int ret;

  ret = snd_pcm_poll_descriptors(pcm, &pfd, 1);
  if (ret <= 0)
    {
      SND_ERR("Invalid snd_pcm_poll_descriptors return: %d", ret);
      return -EIO;
    }

  while (1)
    {
      ret = poll(&pfd, 1, timeout);
      if (ret < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          return -errno;
        }
      else if (ret == 0)
        {
          return 0;
        }

      if (pfd.revents & (POLLERR | POLLNVAL))
        {
          return -EIO;
        }
      else if (pfd.revents & (POLLIN | POLLOUT))
        {
          return 1;
        }
    }
}

snd_pcm_sframes_t snd_pcm_writei(FAR snd_pcm_t *pcm, FAR const void *buffer,
                                 snd_pcm_uframes_t size)
{
  CHECK_PCM_SETUP(pcm);

  return pcm->ops->writei(pcm->ops_arg, buffer, size);
}

int snd_pcm_recover(FAR snd_pcm_t *pcm, int err, int silent)
{
  if (err > 0)
    {
      err = -err;
    }

  if (err == -EINTR)
    {
      return 0;
    }

  if (err == -EPIPE)
    {
      FAR const char *s;
      if (snd_pcm_stream(pcm) == SND_PCM_STREAM_PLAYBACK)
        {
          s = "underrun";
        }
      else
        {
          s = "overrun";
        }

      err = snd_pcm_prepare(pcm);
      if (err < 0)
        {
          SND_ERR("cannot recovery from %s, prepare failed: %s", s,
                  snd_strerror(err));
          return err;
        }

      return 0;
    }

  return err;
}

int snd_pcm_set_params(FAR snd_pcm_t *pcm, snd_pcm_format_t format,
                       snd_pcm_access_t access, unsigned int channels,
                       unsigned int rate, int soft_resample,
                       unsigned int latency)
{
  SND_INFO("format:%d, ch:%d, rate:%d", format, channels, rate);
  assert(pcm);

  pcm->format = format;
  pcm->channels = channels;
  pcm->sample_rate = rate;

  pcm->state = SND_PCM_STATE_SETUP;

  return 0;
}

int snd_pcm_get_params(FAR snd_pcm_t *pcm,
                       FAR snd_pcm_uframes_t *buffer_size,
                       FAR snd_pcm_uframes_t *period_size)
{
  assert(pcm);

  *period_size = pcm->period_frames;
  *buffer_size = pcm->buffer_frames;

  return 0;
}

int snd_pcm_get_sample_bits(snd_pcm_format_t format)
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

snd_pcm_sframes_t snd_pcm_bytes_to_frames(FAR snd_pcm_t *pcm, ssize_t bytes)
{
  CHECK_PCM_SETUP(pcm);

  return bytes / pcm->frame_bytes;
}

ssize_t snd_pcm_frames_to_bytes(FAR snd_pcm_t *pcm, snd_pcm_sframes_t frames)
{
  CHECK_PCM_SETUP(pcm);

  return frames * pcm->frame_bytes;
}

FAR const char *snd_pcm_stream_name(snd_pcm_stream_t stream)
{
  switch (stream)
    {
    case SND_PCM_STREAM_PLAYBACK:
      return "PLAYBACK";
    case SND_PCM_STREAM_CAPTURE:
      return "CAPTURE";
    default:
      return NULL;
    }
}

FAR const char *snd_pcm_access_name(snd_pcm_access_t acc)
{
  switch (acc)
    {
    case SND_PCM_ACCESS_MMAP_INTERLEAVED:
      return "MMAP_INTERLEAVED";
    case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
      return "MMAP_NONINTERLEAVED";
    case SND_PCM_ACCESS_MMAP_COMPLEX:
      return "MMAP_COMPLEX";
    case SND_PCM_ACCESS_RW_INTERLEAVED:
      return "RW_INTERLEAVED";
    case SND_PCM_ACCESS_RW_NONINTERLEAVED:
      return "RW_NONINTERLEAVED";
    default:
      return NULL;
    }
}

FAR const char *snd_pcm_format_name(snd_pcm_format_t format)
{
  switch (format)
    {
    case SND_PCM_FORMAT_S8:
      return "S8";
    case SND_PCM_FORMAT_U8:
      return "U8";
    case SND_PCM_FORMAT_S16_LE:
      return "S16_LE";
    case SND_PCM_FORMAT_S16_BE:
      return "S16_BE";
    case SND_PCM_FORMAT_U16_LE:
      return "U16_LE";
    case SND_PCM_FORMAT_U16_BE:
      return "U16_BE";
    case SND_PCM_FORMAT_S24_LE:
      return "S24_LE";
    case SND_PCM_FORMAT_S24_BE:
      return "S24_BE";
    case SND_PCM_FORMAT_U24_LE:
      return "U24_LE";
    case SND_PCM_FORMAT_U24_BE:
      return "U24_BE";
    case SND_PCM_FORMAT_S32_LE:
      return "S32_LE";
    case SND_PCM_FORMAT_S32_BE:
      return "S32_BE";
    case SND_PCM_FORMAT_U32_LE:
      return "U32_LE";
    case SND_PCM_FORMAT_U32_BE:
      return "U32_BE";
    default:
      return NULL;
    }
}

FAR const char *snd_pcm_format_description(snd_pcm_format_t format)
{
  switch (format)
    {
    case SND_PCM_FORMAT_S8:
      return "Signed 8 bit";
    case SND_PCM_FORMAT_U8:
      return "Unsigned 8 bit";
    case SND_PCM_FORMAT_S16_LE:
      return "Signed 16 bit Little Endian";
    case SND_PCM_FORMAT_S16_BE:
      return "Signed 16 bit Big Endian";
    case SND_PCM_FORMAT_U16_LE:
      return "Unsigned 16 bit Little Endian";
    case SND_PCM_FORMAT_U16_BE:
      return "Unsigned 16 bit Big Endian";
    case SND_PCM_FORMAT_S24_LE:
      return "Signed 24 bit Little Endian";
    case SND_PCM_FORMAT_S24_BE:
      return "Signed 24 bit Big Endian";
    case SND_PCM_FORMAT_U24_LE:
      return "Unsigned 24 bit Little Endian";
    case SND_PCM_FORMAT_U24_BE:
      return "Unsigned 24 bit Big Endian";
    case SND_PCM_FORMAT_S32_LE:
      return "Signed 32 bit Little Endian";
    case SND_PCM_FORMAT_S32_BE:
      return "Signed 32 bit Big Endian";
    case SND_PCM_FORMAT_U32_LE:
      return "Unsigned 32 bit Little Endian";
    case SND_PCM_FORMAT_U32_BE:
      return "Unsigned 32 bit Big Endian";
    default:
      return NULL;
    }
}

FAR const char *snd_pcm_state_name(snd_pcm_state_t state)
{
  switch (state)
    {
    case SND_PCM_STATE_OPEN:
      return "OPEN";
    case SND_PCM_STATE_SETUP:
      return "SETUP";
    case SND_PCM_STATE_PREPARED:
      return "PREPARED";
    case SND_PCM_STATE_RUNNING:
      return "RUNNING";
    case SND_PCM_STATE_XRUN:
      return "XRUN";
    case SND_PCM_STATE_DRAINING:
      return "DRAINING";
    case SND_PCM_STATE_PAUSED:
      return "PAUSED";
    case SND_PCM_STATE_SUSPENDED:
      return "SUSPENDED";
    case SND_PCM_STATE_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return NULL;
    }
}

int snd_pcm_set_volume(FAR snd_pcm_t *pcm, int volume)
{
  if (volume < 0 || volume > 100)
    {
      return -EINVAL;
    }

  pcm->volume = volume / 100.0;
  SND_INFO("set volume:%f", pcm->volume);

  return 0;
}

int snd_pcm_set_volume_db(FAR snd_pcm_t *pcm, int db)
{
  if (db > 0)
    {
      return -EINVAL;
    }

  pcm->volume = powf(10.0,  (double)db / 2000.0);
  SND_INFO("set volume:%f", pcm->volume);

  return 0;
}

int snd_pcm_set_vis_fifo(FAR snd_pcm_t *pcm, FAR const char *fifo_name)
{
  if (!fifo_name)
    {
      return -EINVAL;
    }

  if (mkfifo(fifo_name, 0666) < 0 && errno != EEXIST)
    {
      SND_INFO("mkfifo error: %s\n", strerror(errno));
      return -errno;
    }

  pcm->vis_fd = open(fifo_name, O_RDONLY | O_NONBLOCK, 0);
  if (pcm->vis_fd < 0)
    {
      SND_INFO("open fifo error: %s\n", strerror(errno));
      return -errno;
    }

  SND_INFO("open vis fifo:%s, %d", fifo_name, pcm->vis_fd);

  return 0;
}

int snd_pcm_close_vis_fifo(FAR snd_pcm_t *pcm)
{
  if (pcm->vis_fd >= 0)
    {
      close(pcm->vis_fd);
      pcm->vis_fd = -1;
    }

  return 0;
}

int snd_pcm_format_little_endian(snd_pcm_format_t format)
{
  switch (format)
    {
    case SND_PCM_FORMAT_S16_LE:
    case SND_PCM_FORMAT_U16_LE:
    case SND_PCM_FORMAT_S24_LE:
    case SND_PCM_FORMAT_U24_LE:
    case SND_PCM_FORMAT_S32_LE:
    case SND_PCM_FORMAT_U32_LE:
      return 1;
    case SND_PCM_FORMAT_S16_BE:
    case SND_PCM_FORMAT_U16_BE:
    case SND_PCM_FORMAT_S24_BE:
    case SND_PCM_FORMAT_U24_BE:
    case SND_PCM_FORMAT_S32_BE:
    case SND_PCM_FORMAT_U32_BE:
      return 0;
    default:
      return -EINVAL;
    }
}

int snd_pcm_format_big_endian(snd_pcm_format_t format)
{
  int val;

  val = snd_pcm_format_little_endian(format);
  if (val < 0)
    {
      return val;
    }

  return !val;
}

int snd_pcm_format_cpu_endian(snd_pcm_format_t format)
{
  return snd_pcm_format_little_endian(format);
}

int snd_pcm_dump(FAR snd_pcm_t *pcm, FAR snd_output_t *out)
{
  UNUSED(out);
  CHECK_PCM_SETUP(pcm);

  /* dump hardware info */

  SND_INFO("stream       : %s\n", snd_pcm_stream_name(pcm->stream));
  SND_INFO("access       : %s\n", snd_pcm_access_name(pcm->access));
  SND_INFO("format       : %s\n", snd_pcm_format_name(pcm->format));
  SND_INFO("channels     : %u\n", pcm->channels);
  SND_INFO("rate         : %u\n", pcm->sample_rate);
  SND_INFO("period       : %u\n", pcm->periods);
  SND_INFO("period_time  : %u\n", pcm->period_time);
  SND_INFO("period_bytes : %d\n", pcm->period_bytes);
  SND_INFO("buffer_bytes : %d\n", pcm->buffer_bytes);
  SND_INFO("period_frames: %ld\n", pcm->period_frames);
  SND_INFO("buffer_frames: %ld\n", pcm->buffer_frames);

  /* dump software info */

  SND_INFO("start_threshold  : %ld\n", pcm->start_threshold);

  /* dump device info */

  if (pcm->ops->dump)
    {
      pcm->ops->dump(pcm->ops_arg);
    }

  return 0;
}
