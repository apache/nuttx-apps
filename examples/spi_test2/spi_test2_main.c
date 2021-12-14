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
  /* Open GPIO Output for SPI Chip Select */

  int cs = open("/dev/gpio1", O_RDWR);
  assert(cs >= 0);

  /* Open SPI Test Driver */

  int fd = open("/dev/spitest0", O_RDWR);
  assert(fd >= 0);

  /* Set SPI Chip Select to Low */

  int ret = ioctl(cs, GPIOC_WRITE, 0);
  assert(ret >= 0);

  /* Transmit command to SX1262: Get Status */

  static char get_status[] = { 0xc0, 0x00 };
  int bytes_written = write(fd, get_status, sizeof(get_status));
  assert(bytes_written == sizeof(get_status));

  /* Read response from SX1262 */

  static char rx_data[256];  /* Buffer for SPI response */
  int bytes_read = read(fd, rx_data, sizeof(rx_data));
  assert(bytes_read == sizeof(get_status));

  /* Set SPI Chip Select to High */

  ret = ioctl(cs, GPIOC_WRITE, 1);
  assert(ret >= 0);

  /* Show the received status */

  printf("Get Status: received\n  ");
  for (int i = 0; i < bytes_read; i++) 
    {
      printf("%02x ", rx_data[i]);
    }
  printf("\nSX1262 Status is %d\n", (rx_data[1] >> 4) & 0b111);  /* Bits 6:4 */

  /* Wait for SX1262 to be ready */

  sleep(1);

  /* Set SPI Chip Select to Low */

  ret = ioctl(cs, GPIOC_WRITE, 0);
  assert(ret >= 0);

  /* Transmit command to SX1262: Read Register 8 */

  static char read_reg[] = { 0x1d, 0x00, 0x08, 0x00, 0x00 };
  bytes_written = write(fd, read_reg, sizeof(read_reg));
  assert(bytes_written == sizeof(read_reg));

  /* Read response from SX1262 */

  bytes_read = read(fd, rx_data, sizeof(rx_data));
  assert(bytes_read == sizeof(read_reg));

  /* Set SPI Chip Select to High */

  ret = ioctl(cs, GPIOC_WRITE, 1);
  assert(ret >= 0);

  /* Show the received register value */

  printf("Read Register 8: received\n  ");
  for (int i = 0; i < bytes_read; i++) 
    {
      printf("%02x ", rx_data[i]);
    }
  printf("\nSX1262 Register 8 is 0x%02x\n", rx_data[4]);

  /* Close SPI Test Driver */

  close(fd);

  /* Close GPIO Output for SPI Chip Select */

  close(cs);
  return 0;
}

/* Output Log:
gpio_pin_register: Registering /dev/gpin0
gpio_pin_register: Registering /dev/gpout1
gpint_enable: Disable the interrupt
gpiogistering /dev/gpint2
bl602_spi=400000, actual=0
bl602_spi_setits=8
bl602_spi_setmode: mode=0driver_register: devpath=/dev/spitest0, spidev=0
bl602_spi_select: devid: 0, CS: free

NuttShell (NSH) NuttX-10.2.0-RC0
nsh> spi_test2
spi_test_driver_open: 
gpout_write: Writing 0
spi_test_driver_write: buflen=2
spi_test_driver_configspi: 
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_setfrequency: frequency=1000000, actual=0
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=c0 and recv=51
bl602_spi_poll_send: send=0 and recv=51
bl602_spi_select: devid: 0, CS: free
spi_test_driver_read: buflen=256
gpout_write: Writing 1
Get Status: received
  51 51 
SX1262 Status is 5
gpout_write: Writing 0
spi_test_driver_write: buflen=5
spi_test_driver_configspi: 
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: dS: select
bl602_spi_poll_send: d=1d and recv=a8
bl602_spi_poll_send: send=0 and recv=a8
bl602_spi_poll_send: send=8 and recv=a8
bl602_spi_poll_send: send=0 and recv=a8
bl602_spi_poll_send: send=0 and recv=80
bl602_spi_select: devid: 0, CS: free
spi_test_driver_read: buflen=256
gpout_write: Writing 1
Read Register 8: received
  a8 a8 a8 a8 80 
SX1262 Register 8 is 0x80
spi_test_driver_close: 
*/