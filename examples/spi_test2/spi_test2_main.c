/****************************************************************************
 * apps/examples/spi_test2/spi_test2_main.c
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

#include <sys/ioctl.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

#include <nuttx/ioexpander/gpio.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  printf("Spi_test2, World!!\n");

  /* Open GPIO Output for SPI Chip Select */

  int cs = open("/dev/gpout1", O_RDWR);
  assert(cs >= 0);

  /* Open SPI Test Driver */

  int fd = open("/dev/spitest0", O_RDWR);
  assert(fd >= 0);

  /* Set SPI Chip Select to Low */

  int ret = ioctl(cs, GPIOC_WRITE, 0);
  assert(ret >= 0);

  /* Write to SPI Test Driver */

  static char tx_data[] = "Hello World"; /* TODO */
  int bytes_written = write(fd, tx_data, sizeof(tx_data));
  assert(bytes_written == sizeof(tx_data));

  /* Read from SPI Test Driver */

  static char rx_data[256];  /* Buffer for SPI response */
  int bytes_read = read(fd, rx_data, sizeof(rx_data));
  assert(bytes_read == sizeof(tx_data));

  /* Set SPI Chip Select to High */

  ret = ioctl(cs, GPIOC_WRITE, 1);
  assert(ret >= 0);

  /* Dump the received data */

  printf("spi_test2: received\n  ");
  for (int i = 0; i < bytes_read; i++) 
    {
      printf("%02x ", rx_data[i]);
    }
  printf("\n");

  /* Close SPI Test Driver */

  close(fd);

  /* Close GPIO Output for SPI Chip Select */

  close(cs);
  return 0;
}
