/****************************************************************************
 * apps/examples/ina226/ina226_main.c
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <nuttx/config.h>
#include <nuttx/sensors/ina226.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_INA226_DEVICE_PATH
#  define CONFIG_INA226_DEVICE_PATH "/dev/ina226"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ina226_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ina226_s sample;
  int fd;
  int ret;

  fd = open(CONFIG_INA226_DEVICE_PATH, O_RDWR);
  if (fd < 0)
    {
      int errcode = errno;
      printf("ERROR: Failed to open %s: %d\n",
             CONFIG_INA226_DEVICE_PATH, errcode);
      return EXIT_FAILURE;
    }

  while (1)
    {
      ret = read(fd, &sample, sizeof(sample));
      if (ret != sizeof(sample))
        {
          break;
        }

      printf("U=%12u uV I=%12d uA\n", sample.voltage, sample.current);

      usleep(500000);
    }

  close(fd);
  return 0;
}
