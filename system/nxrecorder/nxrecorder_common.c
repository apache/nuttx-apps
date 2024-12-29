/****************************************************************************
 * apps/system/nxrecorder/nxrecorder_common.c
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

#include "system/nxrecorder.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxrecorder_write_common
 *
 *   Performs common function to write data to apb buffer
 *
 * Input Parameters:
 *   fd     - recording file descriptor
 *   apb    - ap buffer
 *
 * Returned Value:
 *   OK if apb buffer write successfully.
 *
 ****************************************************************************/

int nxrecorder_write_common(int fd, FAR struct ap_buffer_s *apb)
{
  int ret = 0;

  ret = write(fd, apb->samp, apb->nbytes);
  if (ret < 0)
    {
      auderr("ERROR: precorder write failed: %d\n", ret);
    }

  return ret;
}
