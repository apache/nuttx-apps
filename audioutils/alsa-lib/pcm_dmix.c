/****************************************************************************
 * apps/audioutils/alsa-lib/pcm_dmix.c
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
#include <stdio.h>
#include <sys/mman.h>
#include <sys/param.h>

#include <nuttx/audio/audio.h>

#include "pcm_dmix.h"
#include "pcm_mixer_default.h"
#include "pcm_util.h"

/****************************************************************************
 * Pre-processor Prototypes
 ****************************************************************************/

#define RESAMPLE_IO_FORMART SND_PCM_FORMAT_S16_LE

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int snd_pcm_dmix_bps_to_format(int bps)
{
  switch (bps)
    {
    case 16:
      return SND_PCM_FORMAT_S16_LE;
    case 32:
      return SND_PCM_FORMAT_S32_LE;
    default:
      return SND_PCM_FORMAT_UNKNOWN;
    }
}

static int snd_pcm_dmix_configure(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  struct audio_caps_desc_s caps_desc;
  struct ap_buffer_info_s buf_info;
  int sample_bits;
  int frame_bytes;
  int ret;

  if (dmix->shmptr->format == 0)
    {
      dmix->shmptr->format =
          CONFIG_AUDIOUTILS_ALSA_LIB_OUTPUT_FORMAT
              ? snd_pcm_dmix_bps_to_format(
                    CONFIG_AUDIOUTILS_ALSA_LIB_OUTPUT_FORMAT)
              : pcm->format;

      dmix->shmptr->channels =
          CONFIG_AUDIOUTILS_ALSA_LIB_OUTPUT_CHANNELS
              ? CONFIG_AUDIOUTILS_ALSA_LIB_OUTPUT_CHANNELS
              : pcm->channels;

      dmix->shmptr->sample_rate =
          CONFIG_AUDIOUTILS_ALSA_LIB_OUTPUT_RATE
              ? CONFIG_AUDIOUTILS_ALSA_LIB_OUTPUT_RATE
              : pcm->sample_rate;
    }

  sample_bits = snd_pcm_get_sample_bits(dmix->shmptr->format);
  frame_bytes = sample_bits / 8 * dmix->shmptr->channels;

  memset(&caps_desc, 0, sizeof(caps_desc));
  caps_desc.caps.ac_len = sizeof(struct audio_caps_s);
  caps_desc.caps.ac_type = pcm->stream == SND_PCM_STREAM_PLAYBACK
                               ? AUDIO_TYPE_OUTPUT
                               : AUDIO_TYPE_INPUT;
  caps_desc.caps.ac_channels = dmix->shmptr->channels;
  caps_desc.caps.ac_chmap = 0;
  caps_desc.caps.ac_controls.hw[0] = dmix->shmptr->sample_rate;
  caps_desc.caps.ac_controls.b[3] = dmix->shmptr->sample_rate >> 16;
  caps_desc.caps.ac_controls.b[2] = sample_bits;
  caps_desc.caps.ac_subtype = AUDIO_FMT_PCM;

  ret = ioctl(pcm->fd, AUDIOIOC_CONFIGURE, &caps_desc);
  if (ret < 0)
    {
      return ret;
    }

  SND_INFO("configure. ch:%d, rate:%d, format:%d", dmix->shmptr->channels,
           dmix->shmptr->sample_rate, dmix->shmptr->format);

  /* try set recommand params */

  if (dmix->shmptr->period_frames == 0)
    {
      dmix->periods = pcm->periods ? pcm->periods : SND_PCM_DEFAULT_PERIODS;
      if (pcm->period_frames)
        {
          dmix->period_bytes = pcm->period_frames * frame_bytes;
        }
      else
        {
          pcm->period_time = pcm->period_time ? pcm->period_time
                                              : SND_PCM_DEFAULT_PERIOD_TIME;
          dmix->period_bytes = pcm->period_time * dmix->shmptr->sample_rate /
                               1000 / 1000 * frame_bytes;
        }
    }
  else
    {
      dmix->periods =
          dmix->shmptr->buffer_frames / dmix->shmptr->period_frames;
      dmix->period_bytes = dmix->shmptr->period_frames * frame_bytes;
    }

  buf_info.nbuffers = dmix->periods;
  buf_info.buffer_size = dmix->period_bytes;
  ioctl(pcm->fd, AUDIOIOC_SETBUFFERINFO, &buf_info);

  ret = ioctl(pcm->fd, AUDIOIOC_GETBUFFERINFO, &buf_info);
  if (ret >= 0)
    {
      dmix->periods = buf_info.nbuffers;
      dmix->period_bytes = buf_info.buffer_size;
    }

  dmix->period_frames = dmix->period_bytes / frame_bytes;
  dmix->buffer_frames = dmix->period_frames * dmix->periods;
  dmix->shmptr->period_frames = dmix->period_frames;
  dmix->shmptr->buffer_frames = dmix->buffer_frames;

  SND_INFO("get buffer info. n:%d, size:%d", dmix->periods,
           dmix->period_bytes);

  return 0;
}

static int snd_pcm_dmix_share_info_open(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int len = sizeof(snd_pcm_dmix_share_t);

  dmix->shmptr =
      mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, pcm->fd, 0);
  if (dmix->shmptr == MAP_FAILED)
    {
      SND_ERR("mmap error:%d, len:%d", -errno, len);
      return -errno;
    }

  dmix->shmptr->refs++;
  return 0;
}

static int snd_pcm_dmix_share_info_close(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int len = dmix->buffer_bytes;

  dmix->shmptr->refs--;
  if (dmix->mixer.mix_buffer != MAP_FAILED &&
      munmap(dmix->mixer.mix_buffer, len) < 0)
    {
      SND_ERR("munmap error:%d, len:%d", -errno, len);
      return -errno;
    }

  dmix->mixer.mix_buffer = MAP_FAILED;
  return 0;
}

static int snd_pcm_dmix_mix_buffer_open(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int len = dmix->buffer_bytes;

  dmix->mixer.mix_buffer =
      mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, pcm->fd, 0);
  if (dmix->mixer.mix_buffer == MAP_FAILED)
    {
      SND_ERR("mmap error:%d, len:%d", -errno, len);
      return -errno;
    }

  return 0;
}

static int snd_pcm_dmix_mix_buffer_close(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int len = dmix->buffer_bytes;

  if (dmix->mixer.mix_buffer != MAP_FAILED &&
      munmap(dmix->mixer.mix_buffer, len) < 0)
    {
      SND_ERR("munmap error:%d, len:%d", -errno, len);
      return -errno;
    }

  dmix->mixer.mix_buffer = MAP_FAILED;
  return 0;
}

static int snd_pcm_dmix_hw_ptr_open(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int len = sizeof(snd_pcm_mmap_status_t);

  dmix->hw_ptr =
      mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, pcm->fd, 0);
  if (dmix->hw_ptr == MAP_FAILED)
    {
      SND_ERR("mmap error:%d, len:%d", -errno, len);
      return -errno;
    }

  return 0;
}

static int snd_pcm_dmix_hw_ptr_close(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int len = sizeof(snd_pcm_mmap_status_t);

  if (dmix->hw_ptr != MAP_FAILED && munmap(dmix->hw_ptr, len) < 0)
    {
      SND_ERR("munmap error:%d, len:%d", -errno, len);
      return -errno;
    }

  dmix->hw_ptr = MAP_FAILED;
  return 0;
}

int snd_pcm_dmix_sem_open(FAR snd_pcm_dmix_t *dmix)
{
  dmix->semid = sem_open(dmix->ipc_key, O_CREAT | O_EXCL, 0666, 1);
  if (dmix->semid == SEM_FAILED)
    {
      if (errno == EEXIST)
        {
          dmix->semid = sem_open(dmix->ipc_key, 0);
          if (dmix->semid == SEM_FAILED)
            {
              SND_ERR("sem_open error:%d", -errno);
              return -errno;
            }
        }
      else
        {
          SND_ERR("sem_open error:%d", -errno);
          return -errno;
        }
    }

  return 0;
}

static int snd_pcm_dmix_sem_close(FAR snd_pcm_dmix_t *dmix)
{
  if (dmix->semid != SEM_FAILED)
    {
      sem_close(dmix->semid);
      dmix->semid = SEM_FAILED;
    }

  return 0;
}

static inline int dmix_down_sem(FAR snd_pcm_dmix_t *dmix)
{
  int ret;
  while ((ret = sem_wait(dmix->semid) != 0))
    {
      if (errno != EINTR)
        {
          return ret;
        }
    }

  return ret;
}

static inline int dmix_up_sem(FAR snd_pcm_dmix_t *dmix)
{
  return sem_post(dmix->semid);
}

static int snd_pcm_dmix_appl_reset(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int ret;

  ret = ioctl(pcm->fd, AUDIOIOC_PTR_APPL, 0);
  if (ret < 0)
    {
      SND_ERR("ioctl failed: %d", ret);
      return ret;
    }

  dmix->appl_ptr = dmix->hw_ptr->read_head;
  dmix->appl_reset = dmix->appl_ptr;
  SND_DEBUG("[%ld %ld %ld]", dmix->hw_ptr->read_tail,
            dmix->hw_ptr->read_head, dmix->appl_ptr);
  return 0;
}

static int snd_pcm_dmix_appl_forward(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int ret;

  ret = ioctl(pcm->fd, AUDIOIOC_PTR_APPL, 1);
  if (ret < 0)
    {
      SND_ERR("ioctl failed:%d", ret);
      return ret;
    }

  dmix->appl_ptr++;
  return 0;
}

static snd_pcm_uframes_t snd_pcm_dmix_playback_avail(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  snd_pcm_sframes_t used;

  if (dmix->appl_ptr < dmix->hw_ptr->read_head ||
      (pcm->state == SND_PCM_STATE_RUNNING &&
       dmix->appl_ptr > dmix->appl_reset &&
       dmix->appl_ptr <= dmix->hw_ptr->read_tail))
    {
      pcm->state = SND_PCM_STATE_XRUN;
      return -EPIPE;
    }

  used = (dmix->appl_ptr - dmix->hw_ptr->read_tail) * dmix->period_frames;

  return dmix->buffer_frames - used;
}

static snd_pcm_sframes_t snd_pcm_dmix_playback_hw_avail(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  snd_pcm_sframes_t avail;

  avail = (dmix->appl_ptr - dmix->hw_ptr->read_head) * dmix->period_frames;
  if (avail < 0)
    {
      return 0;
    }

  return avail;
}

static snd_pcm_uframes_t snd_pcm_dmix_written(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  snd_pcm_uframes_t written;

  written = (dmix->appl_ptr - dmix->appl_reset) * pcm->period_frames;

  return written;
}

static int snd_pcm_dmix_offset(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int offset;

  offset = dmix->appl_ptr % dmix->periods * dmix->period_bytes;

  return offset;
}

static int snd_pcm_dmix_delay(FAR snd_pcm_t *pcm,
                              FAR snd_pcm_sframes_t *delayp)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int ret;

  switch (pcm->state)
    {
    case SND_PCM_STATE_DRAINING:
    case SND_PCM_STATE_RUNNING:
    case SND_PCM_STATE_PREPARED:
    case SND_PCM_STATE_SUSPENDED:
      ret = ioctl(pcm->fd, AUDIOIOC_GETLATENCY, delayp);
      *delayp += snd_pcm_dmix_playback_hw_avail(pcm);
      *delayp = *delayp * pcm->sample_rate / dmix->sample_rate;
      *delayp += pcm->last_buffer.nframes;
      SND_DEBUG("get delay:%ld", *delayp);
      return ret;
    case SND_PCM_STATE_XRUN:
      return -EPIPE;
    case SND_PCM_STATE_DISCONNECTED:
      return -ENODEV;
    default:
      return -EBADFD;
    }
}

static int snd_pcm_dmix_prepare(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int max_buf_len;
  int ret;

  SND_DEBUG("enter, pcm->state:%d", pcm->state);

  if (pcm->state == SND_PCM_STATE_XRUN)
    {
      snd_pcm_dmix_appl_reset(pcm);
      pcm->state = SND_PCM_STATE_PREPARED;
      return 0;
    }

  if (pcm->state != SND_PCM_STATE_SETUP)
    {
      return -EPERM;
    }

  ret = dmix_down_sem(dmix);
  if (ret < 0)
    {
      SND_ERR("down sem fail, ret:%d, errno:%d", ret, errno);
      return ret;
    }

  ret = snd_pcm_init(pcm);
  if (ret < 0)
    {
      SND_ERR("snd_pcm_dmix_init fail");
      goto err;
    }

  ret = snd_pcm_dmix_share_info_open(pcm);
  if (ret < 0)
    {
      SND_ERR("unable to create IPC shm instance");
      goto err;
    }

  if (dmix->shmptr->refs == 1)
    {
      ret = snd_pcm_dmix_configure(pcm);
      if (ret < 0)
        {
          SND_ERR("snd_pcm_dmix_configure fail");
          goto err;
        }
    }
  else
    {
      dmix->period_frames = dmix->shmptr->period_frames;
      dmix->buffer_frames = dmix->shmptr->buffer_frames;
    }

  dmix->format = dmix->shmptr->format;
  dmix->channels = dmix->shmptr->channels;
  dmix->sample_rate = dmix->shmptr->sample_rate;
  dmix->sample_bits = snd_pcm_get_sample_bits(dmix->format);
  dmix->frame_bytes = dmix->sample_bits / 8 * dmix->channels;
  dmix->period_bytes = dmix->period_frames * dmix->frame_bytes;
  dmix->buffer_bytes = dmix->buffer_frames * dmix->frame_bytes;
  dmix->periods = dmix->buffer_frames / dmix->period_frames;
  dmix->period_time = dmix->period_frames * 1000 * 1000 / dmix->sample_rate;

  pcm->sample_bits = snd_pcm_get_sample_bits(pcm->format);
  pcm->frame_bytes = pcm->sample_bits / 8 * pcm->channels;
  pcm->periods = dmix->periods;
  pcm->period_frames = ceilf((float)dmix->period_frames * pcm->sample_rate /
                             dmix->sample_rate);
  pcm->buffer_frames = pcm->period_frames * pcm->periods;
  pcm->period_bytes = pcm->period_frames * pcm->frame_bytes;
  pcm->buffer_bytes = pcm->buffer_frames * pcm->frame_bytes;
  pcm->period_time = pcm->period_frames * 1000 * 1000 / pcm->sample_rate;
  pcm->start_threshold =
      (dmix->shmptr->refs == 1) ? pcm->buffer_frames : pcm->period_frames;

  pcm->last_buffer.data = malloc(pcm->period_bytes);
  if (!pcm->last_buffer.data)
    {
      SND_ERR("malloc last_buffer.data fail");
      ret = -ENOMEM;
      goto err;
    }

  pcm->last_buffer.nmaxframes = pcm->period_frames;
  pcm->last_buffer.nframes = 0;

  SND_DEBUG("[pcm:%d %d %d][dmix:%d %d %d]", pcm->format, pcm->channels,
            pcm->sample_rate, dmix->format, dmix->channels,
            dmix->sample_rate);

  max_buf_len = MAX(dmix->period_bytes, pcm->period_bytes);

  /* pcm->format -> s16 */

  if (pcm->format != RESAMPLE_IO_FORMART)
    {
      ret = bitsconv_init(&dmix->bc_s16, pcm->channels, pcm->format,
                          RESAMPLE_IO_FORMART, max_buf_len);
      if (ret < 0)
        {
          SND_ERR("Failed to initialize bitconverter: %d\n", ret);
          goto err;
        }
    }

  /* pcm->channels -> dmix->channels */

  if (dmix->channels != pcm->channels)
    {
      ret = chmap_init(&dmix->cm, RESAMPLE_IO_FORMART, pcm->channels,
                       dmix->channels, max_buf_len);
      if (ret < 0)
        {
          SND_ERR("Failed to initialize channel map: %d\n", ret);
          goto err;
        }
    }

  /* pcm->sample_rate -> dmix->sample_rate */

  if (dmix->sample_rate != pcm->sample_rate)
    {
      dmix->resampler = speex_resampler_init(
          dmix->channels, pcm->sample_rate, dmix->sample_rate, 0, &ret);
      if (ret != RESAMPLER_ERR_SUCCESS)
        {
          SND_ERR("Failed to initialize resampler: %s\n",
                  speex_resampler_strerror(ret));
          ret = -ret;
          goto err;
        }

      dmix->resmp_out = malloc(dmix->period_bytes);
      if (!dmix->resmp_out)
        {
          SND_ERR("Failed to mallo resmp_out\n");
          ret = -ENOMEM;
          goto err;
        }
    }

  /* s16 -> dmix->format */

  if (dmix->format != RESAMPLE_IO_FORMART)
    {
      ret = bitsconv_init(&dmix->bc, dmix->channels, RESAMPLE_IO_FORMART,
                          dmix->format, dmix->period_bytes);
      if (ret < 0)
        {
          SND_ERR("Failed to initialize bitconverter: %d\n", ret);
          goto err;
        }
    }

  /* shm2 ringbuffer */

  ret = snd_pcm_dmix_mix_buffer_open(pcm);
  if (ret < 0)
    {
      SND_ERR("unable to initialize sum ring buffer");
      goto err;
    }

  ret = snd_pcm_dmix_hw_ptr_open(pcm);
  if (ret < 0)
    {
      SND_ERR("unable to initialize mmap status");
      goto err;
    }

  default_mixer_init(dmix);
  pcm->state = SND_PCM_STATE_PREPARED;
  dmix_up_sem(dmix);
  return 0;

err:
  if (dmix->hw_ptr != MAP_FAILED)
    {
      snd_pcm_dmix_hw_ptr_close(pcm);
    }

  if (dmix->mixer.mix_buffer != MAP_FAILED)
    {
      snd_pcm_dmix_mix_buffer_close(pcm);
    }

  if (dmix->shmptr != MAP_FAILED)
    {
      snd_pcm_dmix_share_info_close(pcm);
    }

  if (dmix->bc_s16)
    {
      bitsconv_release(dmix->bc_s16);
      dmix->bc_s16 = NULL;
    }

  if (dmix->bc)
    {
      bitsconv_release(dmix->bc);
      dmix->bc = NULL;
    }

  if (dmix->cm)
    {
      chmap_release(dmix->cm);
      dmix->cm = NULL;
    }

  if (dmix->resampler)
    {
      speex_resampler_destroy(dmix->resampler);
      dmix->resampler = NULL;
    }

  if (dmix->resmp_out)
    {
      free(dmix->resmp_out);
    }

  snd_pcm_deinit(pcm);
  dmix_up_sem(dmix);
  SND_ERR("fail ret:%d", ret);
  return ret;
}

static int snd_pcm_dmix_reset(FAR snd_pcm_t *pcm)
{
  return snd_pcm_dmix_appl_reset(pcm);
}

static void snd_pcm_dmix_dump(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  SND_INFO("format       : %s\n", snd_pcm_format_name(dmix->format));
  SND_INFO("channels     : %u\n", dmix->channels);
  SND_INFO("rate         : %u\n", dmix->sample_rate);
  SND_INFO("period       : %u\n", dmix->periods);
  SND_INFO("period_time  : %u\n", dmix->period_time);
  SND_INFO("period_bytes : %d\n", dmix->period_bytes);
  SND_INFO("buffer_bytes : %d\n", dmix->buffer_bytes);
  SND_INFO("period_frames: %ld\n", dmix->period_frames);
  SND_INFO("buffer_frames: %ld\n", dmix->buffer_frames);
}

static int snd_pcm_dmix_drop(FAR snd_pcm_t *pcm)
{
  return 0;
}

static int snd_pcm_dmix_drain(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int ret;

  if (pcm->mode & SND_PCM_NONBLOCK || pcm->state != SND_PCM_STATE_RUNNING)
    {
      return -EPERM;
    }

  while (snd_pcm_dmix_playback_avail(pcm) < dmix->buffer_frames)
    {
      ret = snd_pcm_wait(pcm, -1);
      if (ret == -EPIPE)
        {
          break;
        }
    }

  return 0;
}

static int snd_pcm_dmix_close(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  int ret;

  SND_INFO("enter");

  ret = dmix_down_sem(dmix);
  if (ret < 0)
    {
      SND_ERR("down sem fail, ret:%d, errno:%d", ret, errno);
      return ret;
    }

  if (pcm->running)
    {
      ioctl(pcm->fd, AUDIOIOC_STOP, 0);
      pcm->running = false;
    }

  snd_pcm_deinit(pcm);

  if (dmix->mixer.mix_buffer != MAP_FAILED)
    {
      snd_pcm_dmix_mix_buffer_close(pcm);
    }

  if (dmix->shmptr != MAP_FAILED)
    {
      snd_pcm_dmix_share_info_close(pcm);
    }

  if (pcm->last_buffer.data)
    {
      free(pcm->last_buffer.data);
    }

  if (dmix->bc_s16)
    {
      bitsconv_release(dmix->bc_s16);
    }

  if (dmix->bc)
    {
      bitsconv_release(dmix->bc);
    }

  if (dmix->cm)
    {
      chmap_release(dmix->cm);
    }

  if (dmix->resampler)
    {
      speex_resampler_destroy(dmix->resampler);
    }

  if (dmix->resmp_out)
    {
      free(dmix->resmp_out);
    }

  dmix_up_sem(dmix);
  snd_pcm_dmix_sem_close(dmix);
  pcm->private_data = NULL;
  free(dmix);
  return 0;
}

int snd_pcm_dmix_resume(FAR snd_pcm_t *pcm)
{
  if (pcm->state == SND_PCM_STATE_XRUN)
    {
      snd_pcm_dmix_appl_reset(pcm);
      pcm->state = SND_PCM_STATE_PREPARED;
    }

  return 0;
}

static snd_pcm_sframes_t snd_pcm_dmix_avail_update(FAR snd_pcm_t *pcm)
{
  snd_pcm_sframes_t avail;

  if (pcm->state == SND_PCM_STATE_XRUN)
    {
      return -EPIPE;
    }

  avail = snd_pcm_dmix_playback_avail(pcm);

  if (avail < 0 && !pcm->running)
    {
      snd_pcm_dmix_appl_reset(pcm);
      avail = snd_pcm_dmix_playback_avail(pcm);
    }

  return avail;
}

static void snd_pcm_dmix_raw_pcm_dump(FAR snd_pcm_t *pcm)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  time_t current_time;
  char dump_file[64];
  char time_str[16];
  struct tm tm;
  int result;

  result = access(CONFIG_AUDIOUTILS_ALSA_LIB_DUMP_FLAG, F_OK);

  if (result == 0 && pcm->dump_fd < 0)
    {
      time(&current_time);
      localtime_r(&current_time, &tm);
      strftime(time_str, 64, "%H-%M-%S", &tm);

      snprintf(dump_file, sizeof(dump_file), "%s/%d_%d-%d-%d_%s.pcm",
               CONFIG_AUDIOUTILS_ALSA_LIB_DUMP_PATH, getpid(), dmix->format,
               dmix->sample_rate, dmix->channels, time_str);

      pcm->dump_fd = open(dump_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (pcm->dump_fd < 0)
        {
          SND_ERR("open dump file fail, errno:%d, file:%s", -errno,
                  dump_file);
        }
    }

  if (result != 0 && pcm->dump_fd >= 0)
    {
      close(pcm->dump_fd);
      pcm->dump_fd = -1;
    }
}

static snd_pcm_sframes_t snd_pcm_dmix_write_period(FAR snd_pcm_t *pcm,
                                                   FAR const void *buffer)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  FAR const uint8_t *in_data = buffer;
  snd_pcm_uframes_t xfer = 0;
  snd_pcm_sframes_t ret = 0;
  snd_pcm_uframes_t in_frames;
  snd_pcm_uframes_t out_frames;
  snd_pcm_uframes_t written;
  FAR uint8_t *out_data;
  uint32_t in_len;
  uint32_t out_len;
  int out_size;
  int offset;

  SND_DEBUG("enter");

  in_frames = pcm->period_frames;
  out_frames = dmix->period_frames;

  if (dmix->bc_s16)
    {
      ret = bitsconv_process(dmix->bc_s16, in_data, in_frames,
                             (void **)&out_data, &out_size);
      if (ret < 0)
        {
          SND_ERR("bitsconv err, ret:%ld", ret);
          goto end;
        }

      in_data = out_data;
    }

  if (dmix->cm)
    {
      ret = chmap_process(dmix->cm, in_data, in_frames, (void **)&out_data,
                          &out_size);
      if (ret < 0)
        {
          SND_ERR("chmap err, ret:%ld", ret);
          goto end;
        }

      in_data = out_data;
    }

  if (dmix->resampler)
    {
      in_len = in_frames * pcm->channels;
      out_len = out_frames * dmix->channels;
      ret = speex_resampler_process_interleaved_int(
          dmix->resampler, (FAR int16_t *)in_data,
          (FAR spx_uint32_t *)&in_len, (FAR int16_t *)dmix->resmp_out,
          (FAR spx_uint32_t *)&out_len);
      if (ret != RESAMPLER_ERR_SUCCESS)
        {
          ret = -ret;
          SND_ERR("resample err, ret:%ld", ret);
          goto end;
        }

      in_data = (FAR const uint8_t *)dmix->resmp_out;
    }

  if (dmix->bc)
    {
      ret = bitsconv_process(dmix->bc, in_data, out_frames,
                             (void **)&out_data, &out_size);
      if (ret < 0)
        {
          SND_ERR("bitsconv err, ret:%ld", ret);
          goto end;
        }

      in_data = out_data;
    }

  offset = snd_pcm_dmix_offset(pcm);

  SND_DEBUG("offset:%d, [frame:%ld, forward:%.2f], [%ld  %d %ld]", offset,
            out_frames, (float)out_frames / dmix->period_frames,
            dmix->appl_ptr, pcm->periods, dmix->period_frames);

  if (pcm->vis_fd >= 0)
    {
      write(pcm->vis_fd, in_data, out_frames * dmix->frame_bytes);
    }

  if (pcm->volume != 1.0)
    {
      snd_pcm_vol_soft_scalar(pcm, pcm->last_buffer.data, in_data,
                              out_frames * dmix->channels);
      in_data = pcm->last_buffer.data;
    }

  ret = dmix_down_sem(dmix);
  if (ret < 0)
    {
      SND_ERR("down sem fail, ret:%ld, errno:%d", ret, errno);
      return ret;
    }

  dmix->mixer.mix_engine(out_frames * dmix->channels,
                         (FAR unsigned char *)dmix->mixer.mix_buffer +
                             offset,
                         (FAR unsigned char *)in_data);
  dmix_up_sem(dmix);
  ret = snd_pcm_dmix_appl_forward(pcm);
  if (ret < 0)
    {
      goto end;
    }

  /* check dump raw pcm data per 50 periods */

  if (dmix->appl_ptr % 50 == 0)
    {
      snd_pcm_dmix_raw_pcm_dump(pcm);
    }

  if (pcm->dump_fd >= 0)
    {
      write(pcm->dump_fd, in_data, out_frames * dmix->frame_bytes);
    }

  xfer = in_frames;
  if (pcm->state == SND_PCM_STATE_PREPARED)
    {
      written = snd_pcm_dmix_written(pcm);
      if (pcm->running)
        {
          pcm->state = SND_PCM_STATE_RUNNING;
        }
      else if (written >= pcm->start_threshold)
        {
          ret = snd_pcm_start(pcm);
          if (ret < 0)
            {
              goto end;
            }
        }
    }

end:
  SND_DEBUG("leave, xfer:%ld, ret:%ld", xfer, ret);
  return xfer > 0 ? (snd_pcm_sframes_t)xfer : ret;
}

static snd_pcm_sframes_t snd_pcm_dmix_writei(FAR snd_pcm_t *pcm,
                                             FAR const void *buffer,
                                             snd_pcm_uframes_t size)
{
  FAR snd_pcm_dmix_t *dmix = pcm->private_data;
  snd_pcm_uframes_t xfer = 0;
  snd_pcm_sframes_t ret = 0;
  snd_pcm_state_t state = snd_pcm_state(pcm);
  int offset_src;
  int offset_dst;
  FAR const uint8_t *in_data = buffer;
  snd_pcm_sframes_t avail;
  snd_pcm_sframes_t transfer;

  SND_DEBUG("enter, buffer:%p, size:%ld", buffer, size);

  if (size == 0)
    {
      return 0;
    }

  switch (state)
    {
    case SND_PCM_STATE_PREPARED:
    case SND_PCM_STATE_RUNNING:
    case SND_PCM_STATE_PAUSED:
      break;
    case SND_PCM_STATE_XRUN:
      return -EPIPE;
    case SND_PCM_STATE_SUSPENDED:
      return -ESTRPIPE;
    case SND_PCM_STATE_DISCONNECTED:
      return -ENODEV;
    default:
      return -EBADFD;
    }

  while (size > 0)
    {
      avail = snd_pcm_avail_update(pcm);
      SND_DEBUG("snd_pcm_avail_update ret:%ld", avail);
      if (avail < 0)
        {
          ret = avail;
          goto end;
        }

      if (avail == 0)
        {
          if (pcm->mode & SND_PCM_NONBLOCK)
            {
              ret = -EAGAIN;
              goto end;
            }

          ret = snd_pcm_wait(pcm, -1);
          if (ret < 0)
            {
              break;
            }

          continue;
        }

      if (avail < dmix->period_frames)
        {
          continue;
        }

      offset_src = snd_pcm_frames_to_bytes(pcm, xfer);
      if (size < pcm->period_frames ||
          (pcm->last_buffer.nframes > 0 &&
           pcm->last_buffer.nframes < pcm->last_buffer.nmaxframes))
        {
          transfer = MIN(size, pcm->last_buffer.nmaxframes -
                                   pcm->last_buffer.nframes);
          offset_dst =
              snd_pcm_frames_to_bytes(pcm, pcm->last_buffer.nframes);
          SND_DEBUG("[size:%ld, nframes:%ld][oft_src:%d oft_dst:%d, "
                    "len:%ld]",
                    size, pcm->last_buffer.nframes, offset_src, offset_dst,
                    transfer * pcm->frame_bytes);
          memcpy(pcm->last_buffer.data + offset_dst, in_data + offset_src,
                 transfer * pcm->frame_bytes);
          pcm->last_buffer.nframes += transfer;
          size -= transfer;
          xfer += transfer;

          if (pcm->last_buffer.nframes < pcm->last_buffer.nmaxframes)
            {
              goto end;
            }

          transfer = snd_pcm_dmix_write_period(pcm, pcm->last_buffer.data);
          if (transfer < 0)
            {
              ret = transfer;
              goto end;
            }

          pcm->last_buffer.nframes -= transfer;
          continue;
        }

      offset_src = snd_pcm_frames_to_bytes(pcm, xfer);
      transfer = snd_pcm_dmix_write_period(pcm, in_data + offset_src);
      if (transfer < 0)
        {
          ret = transfer;
          goto end;
        }

      size -= transfer;
      xfer += transfer;
    }

end:
  SND_DEBUG("leave, xfer:%ld, ret:%ld", xfer, ret);
  return xfer > 0 ? (snd_pcm_sframes_t)xfer : ret;
}

static int snd_pcm_dmix_poll_descriptors(FAR snd_pcm_t *pcm,
                                         FAR struct pollfd *pfds,
                                         unsigned int space)
{
  pfds->fd = pcm->fd;
  pfds->events = POLLOUT;

  return 1;
}

static snd_pcm_ops_t snd_pcm_dmix_ops =
{
    .state = NULL,
    .delay = snd_pcm_dmix_delay,
    .prepare = snd_pcm_dmix_prepare,
    .reset = snd_pcm_dmix_reset,
    .start = NULL,
    .dump = snd_pcm_dmix_dump,
    .drop = snd_pcm_dmix_drop,
    .drain = snd_pcm_dmix_drain,
    .close = snd_pcm_dmix_close,
    .pause = NULL,
    .resume = snd_pcm_dmix_resume,
    .avail_update = snd_pcm_dmix_avail_update,
    .writei = snd_pcm_dmix_writei,
    .poll_descriptors = snd_pcm_dmix_poll_descriptors,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int snd_pcm_dmix_open(FAR snd_pcm_t **pcmp, FAR const char *name,
                      snd_pcm_stream_t stream, int mode)
{
  FAR snd_pcm_t *pcm = NULL;
  FAR snd_pcm_dmix_t *dmix = NULL;
  int ret;

  if (!pcmp || !name)
    {
      return -EINVAL;
    }

  if (stream != SND_PCM_STREAM_PLAYBACK)
    {
      SND_ERR("The dmix plugin supports only playback stream");
      return -EINVAL;
    }

  dmix = calloc(1, sizeof(snd_pcm_dmix_t));
  if (!dmix)
    {
      ret = -ENOMEM;
      goto open_err;
    }

  dmix->shmptr = MAP_FAILED;
  dmix->hw_ptr = MAP_FAILED;
  dmix->mixer.mix_buffer = MAP_FAILED;
  snprintf(dmix->ipc_key, sizeof(dmix->ipc_key), "/%s", name);

  ret = snd_pcm_dmix_sem_open(dmix);
  if (ret < 0)
    {
      SND_ERR("unable to create IPC semaphore");
      goto open_err;
    }

  ret = snd_pcm_new(&pcm, SND_PCM_TYPE_DMIX, name, stream, mode);
  if (ret < 0)
    {
      goto open_err;
    }

  pcm->ops = &snd_pcm_dmix_ops;
  pcm->private_data = dmix;
  pcm->state = SND_PCM_STATE_OPEN;

  *pcmp = pcm;
  return 0;

open_err:
  if (dmix)
    {
      if (dmix->semid)
        {
          snd_pcm_dmix_sem_close(dmix);
        }

      free(dmix);
    }

  if (pcm)
    {
      snd_pcm_free(pcm);
    }

  SND_ERR("fail ret:%d", ret);
  return ret;
}
