/****************************************************************************
 * apps/audioutils/alsa-lib/pcm_hw.c
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

#include <string.h>
#include <sys/param.h>

#include <nuttx/audio/audio.h>

#include "pcm_util.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int snd_pcm_hw_poll_available(FAR snd_pcm_t *pcm)
{
  bool nonblock = pcm->mode & SND_PCM_NONBLOCK;
  int old = dq_count(&pcm->bufferq);
  struct audio_buf_desc_s buf_desc;
  int now;

  SND_DEBUG("enter");

  while (1)
    {
      FAR struct ap_buffer_s *buffer;
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

      ret = mq_receive(pcm->mq, (FAR char *)&msg, sizeof(msg), NULL);
      if (ret < 0)
        {
          return -errno;
        }

      if (msg.msg_id == AUDIO_MSG_DEQUEUE)
        {
          if (pcm->state == SND_PCM_STATE_DRAINING)
            {
              buf_desc.u.buffer = msg.u.ptr;
              ioctl(pcm->fd, AUDIOIOC_FREEBUFFER, &buf_desc);
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
          SND_INFO("%s complete\n", pcm->name);
          pcm->state = SND_PCM_STATE_DISCONNECTED;
          return 0;
        }

      nonblock = true;
    }

  now = dq_count(&pcm->bufferq);
  if (pcm->periods > 1 && now == pcm->periods && now > old)
    {
      SND_WARN("audio %s, xrun occurs!\n", pcm->name);

      if (pcm->stream == SND_PCM_STREAM_PLAYBACK)
        {
          pcm->state = SND_PCM_STATE_XRUN;
          return -EPIPE;
        }

      while (!dq_empty(&pcm->bufferq))
        {
          buf_desc.u.buffer =
              (FAR struct ap_buffer_s *)dq_remfirst(&pcm->bufferq);
          ioctl(pcm->fd, AUDIOIOC_ENQUEUEBUFFER, &buf_desc);
        }
    }

  return now;
}

static int snd_pcm_hw_peek_buffer(FAR snd_pcm_t *pcm,
                                  FAR struct ap_buffer_s **buffer)
{
  int ret;

  *buffer = (FAR struct ap_buffer_s *)dq_peek(&pcm->bufferq);
  if (*buffer)
    {
      return 0;
    }

  ret = snd_pcm_hw_poll_available(pcm);
  if (ret < 0)
    {
      return ret;
    }

  *buffer = (FAR struct ap_buffer_s *)dq_peek(&pcm->bufferq);
  if (!*buffer)
    {
      return -EAGAIN;
    }

  return 0;
}

static int snd_pcm_hw_enqueue_buffer(FAR snd_pcm_t *pcm,
                                     FAR struct ap_buffer_s *buffer)
{
  struct audio_buf_desc_s desc;

  SND_DEBUG("enter");

  if (buffer->curbyte != buffer->nmaxbytes)
    {
      memset(buffer->samp + buffer->curbyte, 0,
             buffer->nmaxbytes - buffer->curbyte);
    }

  buffer->nbytes = buffer->curbyte;
  buffer->curbyte = 0;
  desc.u.buffer = buffer;
  return ioctl(pcm->fd, AUDIOIOC_ENQUEUEBUFFER, &desc);
}

static int snd_pcm_hw_delay(FAR snd_pcm_t *pcm,
                            FAR snd_pcm_sframes_t *delayp)
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

  ret = ioctl(pcm->fd, AUDIOIOC_GETLATENCY, delayp);

  SND_INFO("get delay:%ld", *delayp);

  return ret;
}

static int snd_pcm_hw_prepare(FAR snd_pcm_t *pcm)
{
  struct audio_caps_desc_s caps_desc;
  struct audio_buf_desc_s buf_desc;
  struct ap_buffer_info_s buf_info;
  int ret;
  int i;

  SND_DEBUG("enter, pcm->state:%d", pcm->state);

  if (pcm->state == SND_PCM_STATE_XRUN)
    {
      pcm->state = SND_PCM_STATE_PREPARED;
      return 0;
    }

  if (pcm->state != SND_PCM_STATE_SETUP)
    {
      return -EPERM;
    }

  pcm->sample_bits = snd_pcm_get_sample_bits(pcm->format);
  pcm->frame_bytes = pcm->sample_bits / 8 * pcm->channels;

  memset(&caps_desc, 0, sizeof(caps_desc));
  caps_desc.caps.ac_len = sizeof(struct audio_caps_s);
  caps_desc.caps.ac_type = pcm->stream == SND_PCM_STREAM_PLAYBACK
                               ? AUDIO_TYPE_OUTPUT
                               : AUDIO_TYPE_INPUT;
  caps_desc.caps.ac_channels = pcm->channels;
  caps_desc.caps.ac_chmap = 0;
  caps_desc.caps.ac_controls.hw[0] = pcm->sample_rate;
  caps_desc.caps.ac_controls.b[3] = pcm->sample_rate >> 16;
  caps_desc.caps.ac_controls.b[2] = pcm->sample_bits;
  caps_desc.caps.ac_subtype = AUDIO_FMT_PCM;

  ret = ioctl(pcm->fd, AUDIOIOC_CONFIGURE, &caps_desc);
  if (ret < 0)
    {
      return ret;
    }

  SND_INFO("configure. ch:%d, rate:%d", pcm->channels, pcm->sample_rate);

  pcm->periods = pcm->periods ? pcm->periods : SND_PCM_DEFAULT_PERIODS;

  if (pcm->period_frames)
    {
      pcm->period_bytes = pcm->period_frames * pcm->frame_bytes;
    }
  else
    {
      pcm->period_time =
          pcm->period_time ? pcm->period_time : SND_PCM_DEFAULT_PERIOD_TIME;
      pcm->period_bytes = pcm->sample_rate / 1000 * pcm->period_time / 1000 *
                          pcm->frame_bytes;
    }

  buf_info.nbuffers = pcm->periods;
  buf_info.buffer_size = pcm->period_bytes;
  ioctl(pcm->fd, AUDIOIOC_SETBUFFERINFO, &buf_info);

  ret = ioctl(pcm->fd, AUDIOIOC_GETBUFFERINFO, &buf_info);
  if (ret >= 0)
    {
      pcm->periods = buf_info.nbuffers;
      pcm->period_bytes = buf_info.buffer_size;
    }

  pcm->buffer_bytes = pcm->period_bytes * pcm->periods;
  pcm->period_frames = pcm->period_bytes / pcm->frame_bytes;
  pcm->buffer_frames = pcm->period_frames * pcm->periods;
  pcm->period_time = pcm->period_frames * 1000 * 1000 / pcm->sample_rate;
  pcm->start_threshold = pcm->buffer_frames;

  SND_INFO("set buffer info. n:%d, size:%d", pcm->periods,
           pcm->period_bytes);

  dq_init(&pcm->bufferq);

  for (i = 0; i < pcm->periods; i++)
    {
      FAR struct ap_buffer_s *buffer;

      buf_desc.numbytes = pcm->period_bytes;
      buf_desc.u.pbuffer = &buffer;
      ret = ioctl(pcm->fd, AUDIOIOC_ALLOCBUFFER, &buf_desc);
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
          ret = ioctl(pcm->fd, AUDIOIOC_ENQUEUEBUFFER, &buf_desc);
          if (ret < 0)
            {
              return ret;
            }
        }
    }

  pcm->state = SND_PCM_STATE_PREPARED;

  return 0;
}

static int snd_pcm_hw_reset(FAR snd_pcm_t *pcm)
{
  return 0;
}

static int snd_pcm_hw_drop(FAR snd_pcm_t *pcm)
{
  return 0;
}

static int snd_pcm_hw_drain(FAR snd_pcm_t *pcm)
{
  return 0;
}

static int snd_pcm_hw_close(FAR snd_pcm_t *pcm)
{
  struct audio_buf_desc_s buf_desc;

  SND_DEBUG("enter");

  if (pcm->running)
    {
      ioctl(pcm->fd, AUDIOIOC_STOP, 0);
      pcm->running = false;
      pcm->state = SND_PCM_STATE_DRAINING;
    }

  while (!dq_empty(&pcm->bufferq))
    {
      buf_desc.u.buffer =
          (FAR struct ap_buffer_s *)dq_remfirst(&pcm->bufferq);
      ioctl(pcm->fd, AUDIOIOC_FREEBUFFER, &buf_desc);
    }

  while (pcm->state == SND_PCM_STATE_DRAINING)
    {
      snd_pcm_hw_poll_available(pcm);
    }

  snd_pcm_deinit(pcm);

  return 0;
}

static int snd_pcm_hw_resume(FAR snd_pcm_t *pcm)
{
  if (pcm->state == SND_PCM_STATE_XRUN)
    {
      pcm->state = SND_PCM_STATE_PREPARED;
    }

  return 0;
}

static snd_pcm_sframes_t snd_pcm_hw_avail_update(FAR snd_pcm_t *pcm)
{
  snd_pcm_sframes_t ret;

  ret = snd_pcm_hw_poll_available(pcm);

  if (ret > 0)
    {
      ret *= pcm->period_frames;
    }

  return ret;
}

static snd_pcm_sframes_t snd_pcm_hw_writei(FAR snd_pcm_t *pcm,
                                           FAR const void *buffer,
                                           snd_pcm_uframes_t size)
{
  int left = snd_pcm_frames_to_bytes(pcm, size);
  FAR const uint8_t *data = (FAR uint8_t *)buffer;
  snd_pcm_state_t state = snd_pcm_state(pcm);
  FAR struct ap_buffer_s *apb;
  snd_pcm_uframes_t written;
  snd_pcm_uframes_t xfer;
  int ret = 0;
  int len;

  SND_DEBUG("enter. size:%ld", size);

  if (left == 0)
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

  while (left > 0)
    {
      ret = snd_pcm_hw_peek_buffer(pcm, &apb);
      if (ret == -EAGAIN)
        {
          if (pcm->mode & SND_PCM_NONBLOCK)
            {
              goto end;
            }

          ret = snd_pcm_wait(pcm, -1);
          if (ret < 0)
            {
              break;
            }

          continue;
        }
      else if (ret < 0)
        {
          goto end;
        }

      len = MIN(apb->nmaxbytes - apb->curbyte, left);
      snd_pcm_vol_soft_scalar(pcm, (apb->samp + apb->curbyte), data,
                              snd_pcm_bytes_to_frames(pcm, len) *
                                  pcm->channels);
      apb->curbyte += len;
      if (apb->curbyte == apb->nmaxbytes)
        {
          dq_remfirst(&pcm->bufferq);

          ret = snd_pcm_hw_enqueue_buffer(pcm, apb);
          if (ret < 0)
            {
              break;
            }

          if (pcm->state == SND_PCM_STATE_PREPARED)
            {
              written = (pcm->periods - dq_count(&pcm->bufferq)) *
                        pcm->period_frames;
              if (pcm->running)
                {
                  pcm->state = SND_PCM_STATE_RUNNING;
                }
              else if (written >= pcm->start_threshold)
                {
                  ret = snd_pcm_start(pcm);
                  if (ret < 0)
                    {
                      break;
                    }
                }
            }
        }

      data += len;
      left -= len;
    }

end:
  xfer = size - snd_pcm_bytes_to_frames(pcm, left);
  SND_DEBUG("leave. xfer:%ld, ret:%d", xfer, ret);
  return xfer > 0 ? (snd_pcm_sframes_t)xfer : ret;
}

static int snd_pcm_hw_poll_descriptors(FAR snd_pcm_t *pcm,
                                       FAR struct pollfd *pfds,
                                       unsigned int space)
{
  pfds->fd = pcm->mq;
  pfds->events = POLLIN;

  return 1;
}

static snd_pcm_ops_t snd_pcm_hw_ops =
{
    .state = NULL,
    .delay = snd_pcm_hw_delay,
    .prepare = snd_pcm_hw_prepare,
    .reset = snd_pcm_hw_reset,
    .start = NULL,
    .dump = NULL,
    .drop = snd_pcm_hw_drop,
    .drain = snd_pcm_hw_drain,
    .close = snd_pcm_hw_close,
    .pause = NULL,
    .resume = snd_pcm_hw_resume,
    .avail_update = snd_pcm_hw_avail_update,
    .writei = snd_pcm_hw_writei,
    .poll_descriptors = snd_pcm_hw_poll_descriptors,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int snd_pcm_hw_open(FAR snd_pcm_t **pcmp, FAR const char *name,
                    snd_pcm_stream_t stream, int mode)
{
  int ret;
  FAR snd_pcm_t *pcm = NULL;

  if (!pcmp || !name)
    {
      return -EINVAL;
    }

  if (stream < SND_PCM_STREAM_PLAYBACK || stream > SND_PCM_STREAM_LAST)
    {
      return -EINVAL;
    }

  ret = snd_pcm_new(&pcm, SND_PCM_TYPE_HW, name, stream, mode);
  if (ret < 0)
    {
      return ret;
    }

  ret = snd_pcm_init(pcm);
  if (ret < 0)
    {
      snd_pcm_free(pcm);
      return ret;
    }

  pcm->ops = &snd_pcm_hw_ops;
  pcm->private_data = NULL;
  pcm->state = SND_PCM_STATE_OPEN;

  *pcmp = pcm;
  return ret;
}
