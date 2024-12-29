/****************************************************************************
 * apps/system/nxplayer/nxplayer_common.c
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

#include <assert.h>
#include <debug.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <nuttx/audio/audio.h>

#include "system/nxplayer.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxplayer_fill_common
 *
 *   nxplayer_fill_common fill data into apb buffer.
 *
 ****************************************************************************/

int nxplayer_fill_common(int fd, FAR struct ap_buffer_s *apb)
{
  /* Read data into the buffer. */

  apb->nbytes  = read(fd, apb->samp, apb->nmaxbytes);
  apb->curbyte = 0;
  apb->flags   = 0;

  while (0 < apb->nbytes && apb->nbytes < apb->nmaxbytes)
    {
      int n   = apb->nmaxbytes - apb->nbytes;
      int ret = read(fd, &apb->samp[apb->nbytes], n);

      if (0 >= ret)
        {
          break;
        }

      apb->nbytes += ret;
    }

  if (apb->nbytes < apb->nmaxbytes)
    {
#if defined (CONFIG_DEBUG_AUDIO_INFO) || defined (CONFIG_DEBUG_AUDIO_ERROR)
      int errcode = errno;

      audinfo("Closing audio file, nbytes=%d errcode=%d\n",
              apb->nbytes, errcode);
#endif

      /* Set a flag to indicate that this is the final buffer in the stream */

      apb->flags |= AUDIO_APB_FINAL;

#ifdef CONFIG_DEBUG_AUDIO_ERROR
      /* Was this a file read error */

      if (apb->nbytes == 0 && errcode != 0)
        {
          DEBUGASSERT(errcode > 0);
          auderr("ERROR: fread failed: %d\n", errcode);
        }
#endif

      return -ENODATA;
    }

  /* Return OK to indicate that the buffer should be passed through to the
   * audio device.  This does not necessarily indicate that data was read
   * correctly.
   */

  return OK;
}
