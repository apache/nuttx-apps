/****************************************************************************
 * apps/examples/ina219/ina219_main.c
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
#include <inttypes.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <nuttx/sensors/ina219.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ina219_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ina219_s sample;
  int fd;
  int ret;

  fd = open("/dev/ina219", O_RDWR);
  while (1)
    {
      ret = read(fd, &sample, sizeof(sample));
      if (ret != sizeof(sample))
        {
          break;
        }

      printf("U=%12" PRIu32 " uV I=%12" PRId32 " uA\n",
             sample.voltage, sample.current);
    }

  close(fd);
  return 0;
}
