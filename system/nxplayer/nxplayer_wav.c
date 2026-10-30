/****************************************************************************
 * apps/system/nxplayer/nxplayer_wav.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <nuttx/config.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include <nuttx/audio/audio.h>
#include <nuttx/debug.h>

#include "system/nxplayer.h"

#if defined(CONFIG_AUDIO_FORMAT_PCM) && \
    defined(CONFIG_NXPLAYER_FMT_FROM_EXT)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint16_t nxplayer_wav_le16(FAR const uint8_t *data)
{
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t nxplayer_wav_le32(FAR const uint8_t *data)
{
  return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static int nxplayer_wav_read_exact(int fd, FAR uint8_t *buffer,
                                   size_t length)
{
  size_t offset = 0;

  while (offset < length)
    {
      ssize_t ret = read(fd, buffer + offset, length - offset);

      if (ret < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          return -errno;
        }

      if (ret == 0)
        {
          return -EINVAL;
        }

      offset += ret;
    }

  return OK;
}

static int nxplayer_parse_wav_data(int fd, FAR uint32_t *samprate,
                                   FAR uint8_t *nchannels,
                                   FAR uint8_t *bpsamp,
                                   FAR uint32_t *data_length)
{
  uint8_t header[12];
  uint8_t chunk[8];
  uint8_t format[16];
  off_t file_end;
  uint64_t riff_end;
  uint64_t position = sizeof(header);
  uint64_t data_offset = 0;
  uint32_t data_size = 0;
  uint32_t rate = 0;
  uint32_t byte_rate = 0;
  uint16_t channels = 0;
  uint16_t block_align = 0;
  uint16_t bits = 0;
  bool have_format = false;
  bool have_data = false;
  int ret;

  file_end = lseek(fd, 0, SEEK_END);
  if (file_end < 0)
    {
      return -errno;
    }

  if (file_end < (off_t)sizeof(header) ||
      lseek(fd, 0, SEEK_SET) != 0)
    {
      return -EINVAL;
    }

  ret = nxplayer_wav_read_exact(fd, header, sizeof(header));
  if (ret < 0)
    {
      return ret;
    }

  if (memcmp(header, "RIFF", 4) != 0 ||
      memcmp(header + 8, "WAVE", 4) != 0)
    {
      return -EINVAL;
    }

  riff_end = (uint64_t)nxplayer_wav_le32(header + 4) + 8;
  if (riff_end < sizeof(header) || riff_end > (uint64_t)file_end)
    {
      return -EINVAL;
    }

  while (position < riff_end)
    {
      uint32_t chunk_size;
      uint64_t next;

      if (riff_end - position < sizeof(chunk) ||
          lseek(fd, (off_t)position, SEEK_SET) != (off_t)position)
        {
          return -EINVAL;
        }

      ret = nxplayer_wav_read_exact(fd, chunk, sizeof(chunk));
      if (ret < 0)
        {
          return ret;
        }

      chunk_size = nxplayer_wav_le32(chunk + 4);
      next = position + sizeof(chunk) + chunk_size + (chunk_size & 1);
      if (next > riff_end || next > (uint64_t)file_end)
        {
          return -EINVAL;
        }

      if (memcmp(chunk, "fmt ", 4) == 0)
        {
          uint64_t expected_rate;

          if (have_format || chunk_size < sizeof(format))
            {
              return -EINVAL;
            }

          ret = nxplayer_wav_read_exact(fd, format, sizeof(format));
          if (ret < 0)
            {
              return ret;
            }

          if (nxplayer_wav_le16(format) != 1)
            {
              return -ENOTSUP;
            }

          channels = nxplayer_wav_le16(format + 2);
          rate = nxplayer_wav_le32(format + 4);
          byte_rate = nxplayer_wav_le32(format + 8);
          block_align = nxplayer_wav_le16(format + 12);
          bits = nxplayer_wav_le16(format + 14);
          expected_rate = (uint64_t)rate * channels * bits / 8;

          if ((channels != 1 && channels != 2) || rate == 0 ||
              bits != 16 || block_align != channels * 2 ||
              expected_rate > UINT32_MAX || byte_rate != expected_rate)
            {
              return -EINVAL;
            }

          have_format = true;
        }
      else if (memcmp(chunk, "data", 4) == 0)
        {
          if (have_data)
            {
              return -EINVAL;
            }

          data_offset = position + sizeof(chunk);
          data_size = chunk_size;
          have_data = true;
        }

      position = next;
    }

  if (!have_format || !have_data || data_size == 0 ||
      data_size % block_align != 0)
    {
      return -EINVAL;
    }

  if (lseek(fd, (off_t)data_offset, SEEK_SET) != (off_t)data_offset)
    {
      return errno > 0 ? -errno : -EIO;
    }

  *samprate = rate;
  *nchannels = (uint8_t)channels;
  *bpsamp = (uint8_t)bits;
  *data_length = data_size;
  return OK;
}

static int nxplayer_parse_wav(FAR struct nxplayer_s *pplayer, int fd,
                              FAR uint32_t *samplerate,
                              FAR uint8_t *nchannels, FAR uint8_t *bpsamp,
                              FAR int *subfmt)
{
  int ret;

  ret = nxplayer_parse_wav_data(fd, samplerate, nchannels, bpsamp,
                                &pplayer->wav_data_remaining);
  if (ret < 0)
    {
      auderr("ERROR: Invalid or unsupported WAV file: %d\n", ret);
      return ret;
    }

  if (nxplayer_wav_rate_cap(*samplerate) == 0)
    {
      auderr("ERROR: Unsupported WAV sample rate: %" PRIu32 "\n",
             *samplerate);
      return -ENOTSUP;
    }

  pplayer->wav_block_align = *nchannels * 2;
  pplayer->wav_playback = true;
  *bpsamp = 16;
  *subfmt = AUDIO_SUBFMT_PCM_S16_LE;
  return OK;
}

static int nxplayer_fill_wav(FAR struct nxplayer_s *pplayer, int fd,
                             FAR struct ap_buffer_s *apb)
{
  size_t wanted = apb->nmaxbytes < pplayer->wav_data_remaining ?
                  apb->nmaxbytes : pplayer->wav_data_remaining;
  size_t offset = 0;
  int error;

  apb->nbytes = 0;
  apb->curbyte = 0;
  apb->flags = 0;

  wanted -= wanted % pplayer->wav_block_align;
  if (wanted == 0)
    {
      error = -EINVAL;
      goto err_out;
    }

  while (offset < wanted)
    {
      ssize_t ret = read(fd, apb->samp + offset, wanted - offset);

      if (ret < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          apb->nbytes = offset - offset % pplayer->wav_block_align;
          error = -errno;
          goto err_out;
        }

      if (ret == 0)
        {
          apb->nbytes = offset - offset % pplayer->wav_block_align;
          error = -EIO;
          goto err_out;
        }

      offset += ret;
    }

  apb->nbytes = offset;
  pplayer->wav_data_remaining -= offset;
  if (pplayer->wav_data_remaining == 0)
    {
      apb->flags |= AUDIO_APB_FINAL;
      close(fd);
      pplayer->fd = -1;
    }

  return OK;

err_out:
  pplayer->wav_error = error;
  apb->flags |= AUDIO_APB_FINAL;
  close(fd);
  pplayer->fd = -1;
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool nxplayer_is_wav_file(FAR const char *pfilename)
{
  FAR const char *extension = strrchr(pfilename, '.');
  FAR const char *slash = strrchr(pfilename, '/');

  return extension != NULL && (slash == NULL || extension > slash) &&
         strcasecmp(extension + 1, "wav") == 0;
}

uint32_t nxplayer_wav_rate_cap(uint32_t rate)
{
  switch (rate)
    {
      case 8000:
        return AUDIO_SAMP_RATE_8K;
      case 11025:
        return AUDIO_SAMP_RATE_11K;
      case 12000:
        return AUDIO_SAMP_RATE_12K;
      case 16000:
        return AUDIO_SAMP_RATE_16K;
      case 22050:
        return AUDIO_SAMP_RATE_22K;
      case 24000:
        return AUDIO_SAMP_RATE_24K;
      case 32000:
        return AUDIO_SAMP_RATE_32K;
      case 44100:
        return AUDIO_SAMP_RATE_44K;
      case 48000:
        return AUDIO_SAMP_RATE_48K;
      case 88200:
        return AUDIO_SAMP_RATE_88K;
      case 96000:
        return AUDIO_SAMP_RATE_96K;
      case 128000:
        return AUDIO_SAMP_RATE_128K;
      case 160000:
        return AUDIO_SAMP_RATE_160K;
      case 172000:
        return AUDIO_SAMP_RATE_172K;
      case 192000:
        return AUDIO_SAMP_RATE_192K;
      default:
        return 0;
    }
}

const struct nxplayer_dec_ops_s g_wav_dec_ops =
{
  AUDIO_FMT_PCM,
  nxplayer_parse_wav,
  nxplayer_fill_wav
};

#endif /* CONFIG_AUDIO_FORMAT_PCM && CONFIG_NXPLAYER_FMT_FROM_EXT */
