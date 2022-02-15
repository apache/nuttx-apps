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
 * Name: foc_device_init
 ****************************************************************************/

int foc_device_init(FAR struct foc_device_s *dev, int id)
{
  char             devpath[32];
  int              ret = OK;
  struct foc_cfg_s cfg;

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

  /* Get device info */

  ret = foc_dev_getinfo(dev->fd, &dev->info);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_dev_getinfo failed %d!\n", ret);
      goto errout;
    }

  /* Get FOC device configuration */

  cfg.pwm_freq      = (CONFIG_EXAMPLES_FOC_PWM_FREQ);
  cfg.notifier_freq = (CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ);

  /* Print FOC device configuration */

  foc_cfg_print(&cfg);

  /* Configure FOC device */

  ret = foc_dev_setcfg(dev->fd, &cfg);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_dev_setcfg %d!\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_device_deinit
 ****************************************************************************/

int foc_device_deinit(FAR struct foc_device_s *dev)
{
  int ret = OK;

  DEBUGASSERT(dev);

  /* Stop FOC device */

  ret = foc_dev_stop(dev->fd);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_dev_stop %d!\n", ret);
    }

  if (dev->fd > 0)
    {
      close(dev->fd);
    }

  return ret;
}

/****************************************************************************
 * Name: foc_device_start
 ****************************************************************************/

int foc_device_start(FAR struct foc_device_s *dev, bool state)
{
  int ret = OK;

  DEBUGASSERT(dev);

  if (state == true)
    {
      ret = foc_dev_start(dev->fd);
      if (ret < 0)
        {
          PRINTFV("ERROR: foc_dev_start failed %d!\n", ret);
          goto errout;
        }
    }
  else
    {
      ret = foc_dev_stop(dev->fd);
      if (ret < 0)
        {
          PRINTFV("ERROR: foc_dev_stop failed %d!\n", ret);
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_dev_state_get
 ****************************************************************************/

int foc_dev_state_get(FAR struct foc_device_s *dev)
{
  int ret = OK;

  DEBUGASSERT(dev);

  /* Get FOC state - blocking */

  ret = foc_dev_getstate(dev->fd, &dev->state);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_dev_getstate failed %d!\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_dev_params_set
 ****************************************************************************/

int foc_dev_params_set(FAR struct foc_device_s *dev)
{
  int ret = OK;

  DEBUGASSERT(dev);

  /* Write FOC parameters */

  ret = foc_dev_setparams(dev->fd, &dev->params);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_dev_setparams failed %d!\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_dev_state_handle
 ****************************************************************************/

int foc_dev_state_handle(FAR struct foc_device_s *dev, FAR bool *flag)
{
  int ret = OK;

  DEBUGASSERT(dev);
  DEBUGASSERT(flag);

  if (dev->state.fault > 0)
    {
      PRINTF("FAULT = %d\n", dev->state.fault);
      *flag = true;

      /* Clear fault state */

      ret = foc_dev_clearfault(dev->fd);
      if (ret != OK)
        {
          goto errout;
        }
    }
  else
    {
      *flag = false;
    }

errout:
  return ret;
}
