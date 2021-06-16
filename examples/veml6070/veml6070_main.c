/****************************************************************************
 * apps/examples/veml6070/veml6070_main.c
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
#include <nuttx/sensors/veml6070.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * veml6070_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  uint16_t sample;

  fd = open("/dev/uvlight0", O_RDWR);
  while (1)
    {
      ret = read(fd, &sample, sizeof(uint16_t));
      if (ret != sizeof(sample))
        {
          break;
        }

      printf("UV value = %04d\n", sample);

      usleep(500000);
    }

  close(fd);
  return 0;
}
