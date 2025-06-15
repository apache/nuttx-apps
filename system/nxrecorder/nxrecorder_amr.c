/****************************************************************************
 * apps/system/nxrecorder/nxrecorder_amr.c
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

#include <stdio.h>
#include <unistd.h>

#include <nuttx/audio/audio.h>

#include "system/nxrecorder.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

static const uint8_t AMR_NB[6] = "#!AMR\n";
static const uint8_t AMR_WB[9] = "#!AMR-WB\n";

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxrecorder_record_amr
 *
 *   nxrecorder_record_amr() add amr file header
 *
 ****************************************************************************/

int nxrecorder_write_amr(int fd, uint32_t samplerate,
                         uint8_t chans, uint8_t bps)
{
  const uint8_t *data;
  int size;

  if (samplerate != 8000 && samplerate != 16000)
    {
      return -EINVAL;
    }

  data = samplerate == 16000 ? &AMR_WB[0] : &AMR_NB[0];
  size = samplerate == 16000 ? sizeof(AMR_WB) : sizeof(AMR_NB);

  return write(fd, data, size);
}
