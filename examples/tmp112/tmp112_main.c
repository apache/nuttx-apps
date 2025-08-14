/****************************************************************************
 * apps/examples/tmp112/tmp112_main.c
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

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tmp112_read_temperature
 ****************************************************************************/

static int tmp112_read_temperature(FAR const char *devpath, int devno)
{
  int fd;
  int ret;
  float sample;

  fd = open(devpath, O_RDONLY);
  if (fd < 0)
    {
      printf("Failed to open TMP112 sensor #%d at %s: %s (%d)\n",
             devno, devpath, strerror(errno), errno);
      return ERROR;
    }

  ret = read(fd, &sample, sizeof(float));
  if (ret != sizeof(sample))
    {
      perror("Could not read");
      return ERROR;
    }

  printf("Sensor #%d = %.03f degrees Celsius\n", devno, sample);

  close(fd);

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tmp112_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (tmp112_read_temperature(CONFIG_EXAMPLES_TMP112_DEVPATH, 1) < 0)
    {
      return EXIT_FAILURE;
    }

#ifdef CONFIG_TMP112_ENABLE_2
  if (tmp112_read_temperature(CONFIG_EXAMPLES_TMP112_DEVPATH_2, 2) < 0)
    {
      return EXIT_FAILURE;
    }
#endif

#ifdef CONFIG_TMP112_ENABLE_3
  if (tmp112_read_temperature(CONFIG_EXAMPLES_TMP112_DEVPATH_3, 3) < 0)
    {
      return EXIT_FAILURE;
    }
#endif

#ifdef CONFIG_TMP112_ENABLE_4
  if (tmp112_read_temperature(CONFIG_EXAMPLES_TMP112_DEVPATH_4, 4) < 0)
    {
      return EXIT_FAILURE;
    }
#endif

  return EXIT_SUCCESS;
}
