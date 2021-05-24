/****************************************************************************
 * apps/examples/foc/foc_device.c
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

#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "foc_debug.h"
#include "foc_device.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_device_open
 ****************************************************************************/

int foc_device_open(FAR struct foc_device_s *dev, int id)
{
  char devpath[32];
  int  ret   = OK;

  DEBUGASSERT(dev);

  /* Get FOC devpath */

  sprintf(devpath, "%s%d", CONFIG_EXAMPLES_FOC_DEVPATH, id);

  /* Open FOC device */

  dev->fd = open(devpath, 0);
  if (dev->fd <= 0)
    {
      PRINTF("ERROR: open %s failed %d\n", devpath, errno);
      ret = -errno;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_device_close
 ****************************************************************************/

int foc_device_close(FAR struct foc_device_s *dev)
{
  int ret = OK;

  DEBUGASSERT(dev);

  if (dev->fd > 0)
    {
      close(dev->fd);
    }

  return ret;
}
