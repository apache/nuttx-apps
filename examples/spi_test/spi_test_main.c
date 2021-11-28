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
#include <nuttx/spi/spi.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * spi_test_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  printf("Spi_test, World!!\n");

#ifdef TODO
  /* Open device */

  fd = open(DEV_NAME, O_RDWR);
  if (fd < 0)
    {
      int errcode = errno;
      printf("ERROR: Failed to open device %s: %d\n", DEV_NAME, errcode);
      goto errout;
    }

  /* Configure SPI */

  SPI_LOCK(spi, 1);
  SPI_SETBITS(spi, 8);
  SPI_SETMODE(spi, SPIDEV_MODE0);
  SPI_SETFREQUENCY(spi, CONFIG_LPWAN_SX127X_SPIFREQ);

  /* From sx127x_lock() in https://github.com/lupyuen/incubator-nuttx/blob/master/drivers/wireless/lpwan/sx127x/sx127x.c#L463-L477 */

  /* Close SPI */

  close(fd);

#endif  //  TODO

  return 0;
}
