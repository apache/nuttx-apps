/****************************************************************************
 * apps/graphics/input/generator/input_gen_dev.c
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

#include <debug.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include <nuttx/input/mouse.h>

#include "input_gen_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define INPUT_GEN_DEV_PATH_UTOUCH    "/dev/utouch"
#define INPUT_GEN_DEV_PATH_UBUTTON   "/dev/ubutton"
#define INPUT_GEN_DEV_PATH_UMOUSE    "/dev/umouse"
#define INPUT_GEN_DEV_PATH_UKEYBOARD "/dev/ukeyboard"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: input_gen_dev2path
 *
 * Description:
 *   Convert device type to device path.
 *
 * Input Parameters:
 *   device - Device type
 *
 * Returned Value:
 *   Device path string.
 *
 ****************************************************************************/

FAR const char *input_gen_dev2path(input_gen_dev_t device)
{
  switch (device)
    {
      case INPUT_GEN_DEV_UTOUCH:
        return INPUT_GEN_DEV_PATH_UTOUCH;
      case INPUT_GEN_DEV_UBUTTON:
        return INPUT_GEN_DEV_PATH_UBUTTON;
      case INPUT_GEN_DEV_UMOUSE:
        return INPUT_GEN_DEV_PATH_UMOUSE;
      case INPUT_GEN_DEV_UKEYBOARD:
        return INPUT_GEN_DEV_PATH_UKEYBOARD;
      default:
        gerr("ERROR: Invalid device type: %d\n", device);
        return "";
    }
}

/****************************************************************************
 * Name: input_gen_utouch_write
 *
 * Description:
 *   Write touch sample to the device.
 *
 * Input Parameters:
 *   fd     - File descriptor of the device
 *   sample - Pointer to the touch sample structure
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_utouch_write(int fd, FAR const struct touch_sample_s *sample)
{
  size_t  nbytes = SIZEOF_TOUCH_SAMPLE_S(sample->npoints);
  ssize_t ret    = write(fd, sample, nbytes);

  if (ret != nbytes)
    {
      gerr("ERROR: utouch_write failed: nbytes = %zu, ret = %zd, error = %d,"
           " data = %d points, first at x = %d, y = %d, pressure = %u\n",
           nbytes, ret, errno, sample->npoints, sample->point[0].x,
           sample->point[0].y, sample->point[0].pressure);
      return ret < 0 ? -errno : -EIO;
    }

  ginfo("utouch_write: %d points, first at x = %d, y = %d, pressure = %u\n",
        sample->npoints, sample->point[0].x, sample->point[0].y,
        sample->point[0].pressure);
  return OK;
}

/****************************************************************************
 * Name: input_gen_ubutton_write
 *
 * Description:
 *   Write button state to the device.
 *
 * Input Parameters:
 *   fd   - File descriptor of the device
 *   mask - Button mask
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_ubutton_write(int fd, btn_buttonset_t mask)
{
  ssize_t ret = write(fd, &mask, sizeof(mask));

  if (ret != sizeof(mask))
    {
      gerr("ERROR: ubutton_write failed: %zd, error = %d\n", ret, errno);
      return ret < 0 ? -errno : -EIO;
    }

  ginfo("ubutton_write: mask = 0x%08" PRIX32 "\n", mask);
  return OK;
}

/****************************************************************************
 * Name: input_gen_umouse_write
 *
 * Description:
 *   Write mouse wheel state to the device.
 *
 * Input Parameters:
 *   fd    - File descriptor of the device
 *   wheel - Mouse wheel value
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

#ifdef CONFIG_INPUT_MOUSE_WHEEL
int input_gen_umouse_write(int fd, int16_t wheel)
{
  struct mouse_report_s sample;
  ssize_t ret;

  memset(&sample, 0, sizeof(sample));
  sample.wheel = wheel;

  ret = write(fd, &sample, sizeof(sample));
  if (ret != sizeof(sample))
    {
      gerr("ERROR: umouse_write failed: %zd, error = %d\n", ret, errno);
      return ret < 0 ? -errno : -EIO;
    }

  ginfo("umouse_write: wheel = %d\n", wheel);
  return OK;
}
#endif

/****************************************************************************
 * Name: input_gen_grab / input_gen_ungrab
 *
 * Description:
 *   Grab or ungrab the input device.
 *
 * Input Parameters:
 *   dev - The input generator device.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_grab(FAR struct input_gen_dev_s *dev)
{
/* Note: We use flock instead of GRAB ioctl because GRAB will block user
 *       programs like LVGL from receiving events from the device.
 */

#if CONFIG_FS_LOCK_BUCKET_SIZE > 0
  if (flock(dev->fd, LOCK_EX | LOCK_NB) < 0)
    {
      gerr("ERROR: input_gen_grab failed: %d\n", errno);
      return -errno;
    }
#endif

  return OK;
}

int input_gen_ungrab(FAR struct input_gen_dev_s *dev)
{
#if CONFIG_FS_LOCK_BUCKET_SIZE > 0
  if (flock(dev->fd, LOCK_UN) < 0)
    {
      gerr("ERROR: input_gen_ungrab failed: %d\n", errno);
      return -errno;
    }
#endif

  return OK;
}
