/****************************************************************************
 * apps/examples/spi_test/spi_test_main.c
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
#include <assert.h>
#include <fcntl.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * spi_test_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  printf("Spi_test, World!!\n");

  /* Open SPI Test Driver */

  int fd = open("/dev/spitest0", O_RDWR);
  if (fd < 0)
    {
      int errcode = errno;
      printf("ERROR: Failed to open device %s: %d\n", "/dev/spitest0", errcode);
      goto errout;
    }

  /* Write to SPI Test Driver */

  char data[] = "Hello World";
  int bytes_written = write(fd, data, sizeof(data));
  assert(bytes_written == sizeof(data));

  /* Close SPI Test Driver */

  close(fd);

errout:
  return 0;
}
