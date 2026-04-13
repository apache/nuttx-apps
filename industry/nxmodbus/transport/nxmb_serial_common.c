/****************************************************************************
 * apps/industry/nxmodbus/transport/nxmb_serial_common.c
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

#include <nuttx/compiler.h>

#include <errno.h>
#include <stdint.h>

#ifdef CONFIG_SERIAL_TERMIOS
#  include <termios.h>
#endif

#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_SERIAL_TERMIOS

/****************************************************************************
 * Name: nxmb_serial_configure
 *
 * Description:
 *   Configure serial port with termios settings for Modbus communication.
 *
 * Input Parameters:
 *   fd       - File descriptor
 *   baudrate - Baud rate
 *   parity   - Parity setting
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int nxmb_serial_configure(int fd, uint32_t baudrate,
                          enum nxmb_parity_e parity)
{
  struct termios tio;
  speed_t        speed;

  if (tcgetattr(fd, &tio) < 0)
    {
      return -errno;
    }

  switch (baudrate)
    {
      case 9600:
        speed = B9600;
        break;
      case 19200:
        speed = B19200;
        break;
      case 38400:
        speed = B38400;
        break;
      case 57600:
        speed = B57600;
        break;
      case 115200:
        speed = B115200;
        break;
      default:
        return -EINVAL;
    }

  cfsetispeed(&tio, speed);
  cfsetospeed(&tio, speed);

  tio.c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB);
  tio.c_cflag |= CS8 | CLOCAL | CREAD;

  switch (parity)
    {
      case NXMB_PAR_NONE:

        /* Modbus spec: no parity requires 2 stop bits so the
         * character frame remains 11 bits.
         */

        tio.c_cflag |= CSTOPB;
        break;
      case NXMB_PAR_EVEN:
        tio.c_cflag |= PARENB;
        break;
      case NXMB_PAR_ODD:
        tio.c_cflag |= PARENB | PARODD;
        break;
      default:
        return -EINVAL;
    }

  tio.c_iflag &=
    ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tio.c_oflag &= ~OPOST;
  tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

  tio.c_cc[VMIN]  = 0;
  tio.c_cc[VTIME] = 0;

  if (tcsetattr(fd, TCSANOW, &tio) < 0)
    {
      return -errno;
    }

  return 0;
}

#endif /* CONFIG_SERIAL_TERMIOS */
