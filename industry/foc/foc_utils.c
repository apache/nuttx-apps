/****************************************************************************
 * apps/industry/foc/foc_utils.c
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

#include <sys/ioctl.h>
#include <errno.h>

#include "industry/foc/foc_log.h"
#include "industry/foc/foc_utils.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_dev_stop
 ****************************************************************************/

int foc_dev_stop(int fd)
{
  int ret = OK;

  ret = ioctl(fd, MTRIOC_STOP, 0);
  if (ret != OK)
    {
      FOCLIBERR("ERROR: MTRIOC_STOP failed %d!\n", errno);
      ret = -errno;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_dev_start
 ****************************************************************************/

int foc_dev_start(int fd)
{
  int ret = OK;

  ret = ioctl(fd, MTRIOC_START, 0);
  if (ret != OK)
    {
      FOCLIBERR("ERROR: MTRIOC_START failed %d!\n", errno);
      ret = -errno;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_dev_clearfault
 ****************************************************************************/

int foc_dev_clearfault(int fd)
{
  int ret = OK;

  /* Clear fault state */

  ret = ioctl(fd, MTRIOC_CLEAR_FAULT, (unsigned long)0);
  if (ret != OK)
    {
      FOCLIBERR("ERROR: MTRIOC_CLEAR_FAULT failed %d!\n", errno);
      ret = -errno;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_dev_getstate
 ****************************************************************************/

int foc_dev_getstate(int fd, FAR struct foc_state_s *state)
{
  int ret = OK;

  /* Blocking operation */

  ret = ioctl(fd, MTRIOC_GET_STATE, (unsigned long)state);
  if (ret != OK)
    {
      FOCLIBERR("ERROR: MTRIOC_GET_STATE failed %d!\n", errno);
      ret = -errno;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_dev_setparams
 ****************************************************************************/

int foc_dev_setparams(int fd, FAR struct foc_params_s *params)
{
  int ret = OK;

  ret = ioctl(fd, MTRIOC_SET_PARAMS, (unsigned long)params);
  if (ret != OK)
    {
      FOCLIBERR("ERROR: MTRIOC_SET_PARAMS failed %d!\n", errno);
      ret = -errno;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_dev_setcfg
 ****************************************************************************/

int foc_dev_setcfg(int fd, FAR struct foc_cfg_s *cfg)
{
  int ret = OK;

  ret = ioctl(fd, MTRIOC_SET_CONFIG, (unsigned long)cfg);
  if (ret != OK)
    {
      FOCLIBERR("ERROR: MTRIOC_SET_CONFIG failed %d!\n", errno);
      ret = -errno;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_dev_getinfo
 ****************************************************************************/

int foc_dev_getinfo(int fd, FAR struct foc_info_s *info)
{
  int ret = OK;

  ret = ioctl(fd, MTRIOC_GET_INFO, (unsigned long)info);
  if (ret != OK)
    {
      FOCLIBERR("ERROR: MTRIOC_GET_INFO failed %d!\n", errno);
      ret = -errno;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_cfg_print
 ****************************************************************************/

void foc_cfg_print(FAR struct foc_cfg_s *cfg)
{
  FOCLIBLOG("PWM freq=%" PRIu32 "\n", cfg->pwm_freq);
  FOCLIBLOG("Notifier freq=%" PRIu32 "\n", cfg->notifier_freq);
}
