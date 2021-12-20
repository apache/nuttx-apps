/****************************************************************************
 * apps/examples/isl29023/isl29023_main.c
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
#include <nuttx/sensors/isl29023.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEV_PATH  "/dev/als0"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * isl29023_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct isl29023_data_s data;
  int fd;
  int ret;

  fd = open(DEV_PATH, O_RDONLY);
  if (fd < 0)
    {
      printf("Failed to open device driver at '%s'\n", DEV_PATH);
    }

  /* Set continuous ALS mode */

  ret = ioctl(fd, SNIOC_SET_OPERATIONAL_MODE,
              ISL29023_OP_MODE_ALS_CONTINUES);
  if (ret < 0)
    {
      printf("Failed to set isl29023 mode: %d\n", errno);
      goto out;
    }

  /* Set max resolution (16 bits) */

  ret = ioctl(fd, SNIOC_SET_RESOLUTION, ISL29023_RESOLUTION_16BITS);
  if (ret < 0)
    {
      printf("Failed to set isl29023 resolution: %d\n", errno);
      goto out;
    }

  /* Set max range (64000) */

  ret = ioctl(fd, SNIOC_SET_RANGE, ISL29023_ALS_RANGE_64000);
  if (ret < 0)
    {
      printf("Failed to set isl29023 range: %d\n", errno);
      goto out;
    }

  /* Wait for the initial conversion to complete.
   * 90 ms (typ) for 16-bit conversion on isl29023,
   * 105 ms for 16-bit conversion on isl29035 (compatible),
   * so going for 110 ms.
   */

  usleep(110000);

  /* Read both lux and raw values */

  ret = read(fd, &data, sizeof(data));
  if (ret < 0)
    {
      printf("FAILED to read lux and raw values from isl29023: %d\n", errno);
      goto out;
    }

  printf("lux: %d\n"
         "raw: %d\n", data.lux, data.raw);

  ret = 0;

out:
  close(fd);
  return ret;
}
