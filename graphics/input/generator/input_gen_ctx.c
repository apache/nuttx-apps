/****************************************************************************
 * apps/graphics/input/generator/input_gen_ctx.c
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
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/types.h>

#include "input_gen_internal.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct input_gen_ctx_s
{
  struct input_gen_dev_s devs[INPUT_GEN_DEV_NUM];
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: input_gen_create
 *
 * Description:
 *   Create an input generator context.
 *
 * Input Parameters:
 *   ctx     - A pointer to the input generator context, or NULL if the
 *             context failed to be created.
 *   devices - Device types to be opened.
 *
 * Returned Value:
 *   Success device count.  On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int input_gen_create(FAR input_gen_ctx_t *ctx, uint32_t devices)
{
  int success = 0;
  int i;

  if (ctx == NULL || devices & ~INPUT_GEN_DEV_ALL)
    {
      gerr("ERROR: Invalid parameter: ctx=%p, devices=0x%08" PRIX32 "\n",
           ctx, devices);
      return -EINVAL;
    }

  *ctx = (input_gen_ctx_t)zalloc(sizeof(struct input_gen_ctx_s));
  if (*ctx == NULL)
    {
      return -ENOMEM;
    }

  /* Initialize the device context */

  for (i = 0; i < INPUT_GEN_DEV_NUM; i++)
    {
      FAR struct input_gen_dev_s *dev = &(*ctx)->devs[i];
      input_gen_dev_t current = (1 << i);

      if (devices & current)
        {
          dev->fd = open(input_gen_dev2path(current),
                         O_WRONLY | O_NONBLOCK);
          if (dev->fd < 0)
            {
              dev->device = INPUT_GEN_DEV_NONE;
              gerr("ERROR: Open %s failed: %d\n",
                   input_gen_dev2path(current), errno);
              continue;
            }

          ginfo("Opened %s, fd = %d\n",
                input_gen_dev2path(current), dev->fd);
          dev->device = current;
          success++;
        }
      else
        {
          dev->fd = -1;
          dev->device = INPUT_GEN_DEV_NONE;
        }
    }

  ginfo("%d devices opened\n", success);

  if (success == 0)
    {
      free(*ctx);
      *ctx = NULL;
      return -ENODEV;
    }

  return success;
}

/****************************************************************************
 * Name: input_gen_destroy
 *
 * Description:
 *   Destroy an input generator context.
 *
 * Input Parameters:
 *   ctx - The input generator context to destroy.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_destroy(input_gen_ctx_t ctx)
{
  int i;

  if (ctx == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < INPUT_GEN_DEV_NUM; i++)
    {
      if (ctx->devs[i].fd >= 0)
        {
          ginfo("Closing %s, fd = %d\n",
                input_gen_dev2path(ctx->devs[i].device), ctx->devs[i].fd);
          close(ctx->devs[i].fd);
        }
    }

  free(ctx);
  return OK;
}

/****************************************************************************
 * Name: input_gen_search_dev
 *
 * Description:
 *   Get the input generator device by type.
 *
 * Input Parameters:
 *   ctx    - The input generator context.
 *   device - The device type.
 *
 * Returned Value:
 *   A pointer to the input generator device structure, or NULL if not found.
 *
 ****************************************************************************/

FAR struct input_gen_dev_s *input_gen_search_dev(input_gen_ctx_t ctx,
                                                 input_gen_dev_t device)
{
  int i;

  if (ctx == NULL || device > INPUT_GEN_DEV_ALL)
    {
      return NULL;
    }

  for (i = 0; i < INPUT_GEN_DEV_NUM; i++)
    {
      if (ctx->devs[i].device & device)
        {
          return &ctx->devs[i];
        }
    }

  nwarn("WARNING: Device with type %d not found\n", device);
  return NULL;
}

/****************************************************************************
 * Name: input_gen_query_device
 *
 * Description:
 *   Query whether the device with the specified type is opened.
 *
 * Input Parameters:
 *   ctx    - The input generator context.
 *   device - The device type.
 *
 * Returned Value:
 *   True if the device is opened, false otherwise.
 *
 ****************************************************************************/

bool input_gen_query_device(input_gen_ctx_t ctx, input_gen_dev_t device)
{
  return input_gen_search_dev(ctx, device) != NULL;
}

/****************************************************************************
 * Name: input_gen_reset_devices
 *
 * Description:
 *   Reset the device state with the specified type.
 *
 * Input Parameters:
 *   ctx     - The input generator context.
 *   devices - The device types to reset state.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_reset_devices(input_gen_ctx_t ctx, uint32_t devices)
{
  int ret;

  if (ctx == NULL)
    {
      return -EINVAL;
    }

  /* Use actions with duration 0 to reset the device state */

  if (devices & INPUT_GEN_DEV_UTOUCH)
    {
      ret = input_gen_swipe(ctx, 0, 0, 0, 0, 0);
      if (ret < 0 && ret != -ENODEV)
        {
          return ret;
        }
    }

  if (devices & INPUT_GEN_DEV_UBUTTON)
    {
      ret = input_gen_button_longpress(ctx, 0, 0);
      if (ret < 0 && ret != -ENODEV)
        {
          return ret;
        }
    }

  /* TODO: Reset keyboard when we support it. */

  return OK;
}

/****************************************************************************
 * Name: input_gen_write_raw
 *
 * Description:
 *   Write raw data to the device.
 *
 * Input Parameters:
 *   ctx    - The input generator context.
 *   device - The device type.
 *   buf    - The buffer to write.
 *   nbytes - The number of bytes to write.
 *
 * Returned Value:
 *   The number of bytes written, or a negated errno value on failure.
 *
 ****************************************************************************/

ssize_t input_gen_write_raw(input_gen_ctx_t ctx, input_gen_dev_t device,
                            FAR const void *buf, size_t nbytes)
{
  FAR struct input_gen_dev_s *dev = input_gen_search_dev(ctx, device);
  ssize_t ret;

  if (dev == NULL)
    {
      gerr("ERROR: Device with type %d not found\n", device);
      return -ENODEV;
    }

  ret = write(dev->fd, buf, nbytes);
  return ret < 0 ? -errno : ret;
}
