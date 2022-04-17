/****************************************************************************
 * apps/examples/termios/termios_main.c
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
#include <termios.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  int error = OK;
  struct termios tio;

  printf("Termios example\n");

  fd = open("/dev/ttyS0", O_RDONLY);
  if (fd < 0)
    {
      error = errno;
      printf("Error opening serial: %d\n", error);
    }

  /* Fill the termios struct with the current values. */

  ret = tcgetattr(fd, &tio);
  if (ret < 0)
    {
      error = errno;
      printf("Error getting attributes: %d\n", error);
    }

  /* Configure a baud rate.
   * NuttX doesn't support different baud rates for RX and TX.
   * So, both cfisetospeed() and cfisetispeed() are overwritten
   * by cfsetspeed.
   */

  ret = cfsetspeed(&tio, B57600);
  if (ret < 0)
    {
      error = errno;
      printf("Error setting baud rate: %d\n", error);
    }

  /* Configure 2 stop bits. */

  tio.c_cflag |= CSTOPB;

  /* Enable parity and configure odd parity. */

  tio.c_cflag |= PARENB | PARODD;

  /* Change the data size to 7 bits */

  tio.c_cflag &= ~CSIZE; /* Clean the bits */
  tio.c_cflag |= CS7;    /* 7 bits */

#ifdef CONFIG_EXAMPLES_TERMIOS_DIS_HW_FC

  /* Disable the HW flow control */

  tio.c_cflag &= ~CCTS_OFLOW;    /* Output flow control */
  tio.c_cflag &= ~CRTS_IFLOW;    /* Input flow control */

  printf("Disabled HW Flow Control\n");
#endif

  printf("Please, reopen the terminal with the new attributes,"
         " otherwise you will have garbage or a stuck terminal.\n"
         "If you disabled HW flow control, try disconnecting RTS and CTS.\n"
         "You may try: picocom /dev/ttyUSB0 --baud 57600"
         " --parity o --databits 7 --stopbits 2\n\n");

  fflush(stdout); /* Clean stdout buffer */

  /* Wait to empty the hardware buffer, otherwise the above message
   * will not be seen because the following command will take effect
   * before the hardware buffer gets empty. A small delay is enough.
   */

  sleep(1);

  /* Change the attributes now. */

  ret = tcsetattr(fd, TCSANOW, &tio);
  if (ret < 0)
    {
      error = errno;
      /* Print the error code in the loop because at this
       * moment the serial attributes already changed
       */
    }

  close(fd);

  /* Now, we should reopen the terminal with the new
   * attributes to see if they took effect;
   */

  while (1)
    {
      printf("If you can read this message, the changes took effect.\n"
            "Expected error code: 0. Current code: %d\n", error);
      sleep(1);
    }

  return 0;
}

