/****************************************************************************
 * apps/examples/ads7046/ads7046_main.c
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
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <nuttx/analog/ioctl.h>
#include <nuttx/analog/ads7046.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* As per the datasheet, the ADS7046 returns the following readings:
 * INPUT VOLTAGE (AINP - AINM)               DESCRIPTION                 HEX
 * --------------------------------------    ------------------------    ---
 * <= 1 LSB                                  Negative full-scale code    000
 * 1 LSB to 2 LSB                            -                           001
 * V_REF / 2 to V_REF / 2 + 1 LSB            Mid code                    7FF
 * V_REF / 2 + 1 LSB to V_REF / 2 + 2 LSB    -                           800
 * >= V_REF - 1 LSB                          Positive full-scale code    FFF
 */

#define SAMPLE_TO_PCT(sample)  (((sample) * 100) / 0xfff)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ads7046_main
 ****************************************************************************/

int main(const int argc, FAR char *argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  int ret;
  int fd;
  uint16_t sample;

  fd = open(CONFIG_EXAMPLES_ADS7046_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      printf("Failed to open %s: %s (%d)\n", CONFIG_EXAMPLES_ADS7046_DEVPATH,
             strerror(errno), errno);
      return EXIT_FAILURE;
    }

  ret = ioctl(fd, ANIOC_ADS7046_READ, &sample);
  if (ret != OK)
    {
      perror("Could not ioctl fd");
      close(fd);
      return EXIT_FAILURE;
    }

  printf("ADS7046: hex=%x, dec=%"PRIu16", adc_percentage=%u%%\n",
         sample, sample, SAMPLE_TO_PCT(sample));

  close(fd);

  return EXIT_SUCCESS;
}
