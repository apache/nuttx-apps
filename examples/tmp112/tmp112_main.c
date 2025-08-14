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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tmp112_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  float sample;

  fd = open(CONFIG_EXAMPLES_TMP112_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      printf("Failed to open TMP112 sensor at %s: %s (%d)\n",
             CONFIG_EXAMPLES_TMP112_DEVPATH, strerror(errno), errno);
      return EXIT_FAILURE;
    }

  ret = read(fd, &sample, sizeof(float));
  if (ret != sizeof(sample))
    {
      perror("Could not read");
      ret = EXIT_FAILURE;
    }
  else
    {
      printf("TMP112 = %.03f degrees Celsius\n", sample);
      ret = EXIT_SUCCESS;
    }

  close(fd);

  return ret;
}
