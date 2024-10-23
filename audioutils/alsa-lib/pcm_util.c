/****************************************************************************
 * apps/audioutils/alsa-lib/pcm_util.c
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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <nuttx/audio/audio.h>

#include "pcm_util.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int snd_pcm_new(FAR snd_pcm_t **pcmp, snd_pcm_type_t type,
                FAR const char *name, snd_pcm_stream_t stream, int mode)
{
  FAR snd_pcm_t *pcm;
  pcm = calloc(1, sizeof(*pcm));
  if (!pcm)
    {
      return -ENOMEM;
    }

  pcm->type = type;
  if (name)
    {
      pcm->name = strdup(name);
    }

  pcm->stream = stream;
  pcm->mode = mode;
  pcm->ops_arg = pcm;
  pcm->volume = 1.0;
  pcm->dump_fd = -1;
  pcm->vis_fd = -1;
  *pcmp = pcm;
  return 0;
}

int snd_pcm_free(FAR snd_pcm_t *pcm)
{
  assert(pcm);
  free(pcm->name);
  free(pcm);
  return 0;
}

int snd_pcm_init(FAR snd_pcm_t *pcm)
{
  char path[32];
  int ret;

  struct mq_attr attr = {
      .mq_maxmsg = 16,
      .mq_msgsize = sizeof(struct audio_msg_s),
  };

  /* open device */

  snprintf(path, sizeof(path), CONFIG_AUDIOUTILS_ALSA_LIB_DEV_PATH "/%s",
           pcm->name);
  pcm->fd = open(path, O_RDWR | O_CLOEXEC);
  if (pcm->fd < 0)
    {
      return -errno;
    }

  /* reserve */

  ret = ioctl(pcm->fd, AUDIOIOC_RESERVE, 0);
  if (ret < 0)
    {
      goto out;
    }

  /* create message queue */

  if (pcm->type != SND_PCM_TYPE_DMIX)
    {
      snprintf(pcm->mqname, sizeof(pcm->mqname), "/tmp/%p", pcm);
      pcm->mq =
          mq_open(pcm->mqname, O_RDWR | O_CREAT | O_CLOEXEC, 0644, &attr);
      if (pcm->mq < 0)
        {
          ret = -errno;
          goto out;
        }

      ret = ioctl(pcm->fd, AUDIOIOC_REGISTERMQ, pcm->mq);
      if (ret < 0)
        {
          goto out;
        }
    }
  else
    {
      pcm->mq = -1;
    }

  return 0;

out:
  snd_pcm_deinit(pcm);
  return ret;
}

void snd_pcm_deinit(FAR snd_pcm_t *pcm)
{
  if (pcm->fd < 0)
    {
      return;
    }

  if (pcm->mq >= 0)
    {
      ioctl(pcm->fd, AUDIOIOC_UNREGISTERMQ, 0);

      mq_close(pcm->mq);
      pcm->mq = -1;

      mq_unlink(pcm->mqname);
    }

  if (pcm->dump_fd >= 0)
    {
      close(pcm->dump_fd);
      pcm->dump_fd = -1;
    }

  if (pcm->vis_fd >= 0)
    {
      close(pcm->vis_fd);
      pcm->vis_fd = -1;
    }

  ioctl(pcm->fd, AUDIOIOC_RELEASE, 0);
  close(pcm->fd);
}
