/****************************************************************************
 * apps/system/nxplayer/nxplayer_sbc.c
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

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <nuttx/audio/audio.h>

#include "system/nxplayer.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SBC_SYNC_WORD         0x9C
#define SBC_AM_LOUDNESS       0x00
#define SBC_AM_SNR            0x01
#define SBC_FRAME_HEADER_SIZE 32

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * SBC frame header
 * syncword                  8                  BsMsbf
 * sampling_frequency        2                  UiMsbf
 * blocks                    2                  UiMsbf
 * channel_mode              2                  UiMsbf
 * allocation_method         1                  UiMsbf
 * subbands                  1                  UiMsbf
 * bitpool                   8                  UiMsbf
 * crc_check                 8                  UiMsbf
 ****************************************************************************/

struct sbc_frame_header
{
  uint8_t syncword;
  uint32_t sampling_frequency;
  uint8_t blocks;
  uint8_t nchannels;
  uint8_t allocation_method;
  uint8_t subbands;
  uint8_t bitpool;
  uint8_t crc_check;
  uint8_t reserve;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * SBC header bits define
 ****************************************************************************/

/****************************************************************************
 * sampling_frequency
 * bits         rate(HZ)
 * 00           16000
 * 01           32000
 * 10           44100
 * 11           48000
 ****************************************************************************/

static const uint32_t g_sampling_freq[4] =
{
  16000, 32000, 44100, 48000
};

/****************************************************************************
 * blocks
 * bits         nrof_blocks
 * 00               4
 * 01               8
 * 10               12
 * 11               16
 ****************************************************************************/

static const uint8_t g_nblocks[4] =
{
  4, 8, 12, 16
};

/****************************************************************************
 * channel mode
 * bits     channel mode     channel numbers
 * 00          MONO                 1
 * 01          DUAL_CHANNEL         2
 * 10          STEREO               2
 * 11          JOINT_STEREO         2
 ****************************************************************************/

static const uint8_t g_nchannels[4] =
{
  1, 2, 2, 2
};

/****************************************************************************
 * allocation_method
 * bits         allocation_method
 * 0                LOUDNESS
 * 1                SNR
 ****************************************************************************/

static const uint8_t g_allocation_method[2] =
{
  SBC_AM_LOUDNESS, SBC_AM_SNR
};

/****************************************************************************
 * subbands
 * bits         nrof_subbands
 * 0                4
 * 1                8
 ****************************************************************************/

static const uint8_t g_nsubbands[2] =
{
  4, 8
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void init_sbc_frame_header(FAR struct sbc_frame_header *sbc,
                                  FAR char *buffer)
{
  sbc->syncword = buffer[0];
  sbc->sampling_frequency = g_sampling_freq[buffer[1] >> 6];
  sbc->blocks = g_nblocks[buffer[1] >> 4 & 0x03];
  sbc->nchannels = g_nchannels[buffer[1] >> 2 & 0x03];
  sbc->allocation_method = g_allocation_method[buffer[1] >> 1 & 0x01];
  sbc->subbands = g_nsubbands[buffer[1] & 0x01];
  sbc->bitpool = buffer[2];
  sbc->crc_check = buffer[3];
  sbc->reserve = 0;
}

static int check_sbc_frame_header_info(FAR struct sbc_frame_header *sbc)
{
  if (sbc->syncword != SBC_SYNC_WORD)
    {
      return -EINVAL;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxplayer_parse_sbc
 *
 *   nxplayer_parse_sbc() parse sbc header, get samplerate, channels, bps.
 *
 ****************************************************************************/

int nxplayer_parse_sbc(int fd, FAR uint32_t *samplerate,
                       FAR uint8_t *chans, FAR uint8_t *bps)
{
  char buffer[SBC_FRAME_HEADER_SIZE];
  struct sbc_frame_header sbc;
  int ret = OK;

  ret = read(fd, buffer, SBC_FRAME_HEADER_SIZE);
  if (ret < SBC_FRAME_HEADER_SIZE)
    {
      return -EINVAL;
    }

  init_sbc_frame_header(&sbc, buffer);
  ret = check_sbc_frame_header_info(&sbc);
  if (ret < 0)
    {
      return ret;
    }

  *samplerate = sbc.sampling_frequency;
  *chans = sbc.nchannels;
  *bps = 16;
  lseek(fd, 0, SEEK_SET);

  return ret;
}
