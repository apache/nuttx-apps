/****************************************************************************
 * apps/system/microros/transport/microros_transport_serial.c
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

#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>

#include "microros_transport.h"

/****************************************************************************
 * Private Helpers
 ****************************************************************************/

static inline int serial_fd(struct uxrCustomTransport *transport)
{
  return (int)(intptr_t)transport->args - 1;
}

static inline void serial_set_fd(struct uxrCustomTransport *transport,
                                 int fd)
{
  transport->args = (void *)(intptr_t)(fd + 1);
}

static int set_baud(struct termios *tio, int baud)
{
  speed_t s;

  switch (baud)
    {
      case 9600:
        s = B9600;
        break;

      case 19200:
        s = B19200;
        break;

      case 38400:
        s = B38400;
        break;

      case 57600:
        s = B57600;
        break;

      case 115200:
        s = B115200;
        break;

      case 230400:
        s = B230400;
        break;

      case 460800:
        s = B460800;
        break;

      case 921600:
        s = B921600;
        break;

      default:
        return -1;
    }

  cfsetispeed(tio, s);
  cfsetospeed(tio, s);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool microros_serial_open(struct uxrCustomTransport *transport)
{
  struct termios tio;
  int fd;

  fd = open(CONFIG_MICROROS_SERIAL_DEVICE, O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd < 0)
    {
      return false;
    }

  if (tcgetattr(fd, &tio) < 0)
    {
      close(fd);
      return false;
    }

  cfmakeraw(&tio);
  tio.c_cflag |= (CLOCAL | CREAD);
  tio.c_cflag &= ~CRTSCTS;
  tio.c_cc[VMIN]  = 0;
  tio.c_cc[VTIME] = 0;

  if (set_baud(&tio, CONFIG_MICROROS_SERIAL_BAUD) < 0)
    {
      close(fd);
      return false;
    }

  if (tcsetattr(fd, TCSANOW, &tio) < 0)
    {
      close(fd);
      return false;
    }

  tcflush(fd, TCIOFLUSH);
  serial_set_fd(transport, fd);
  return true;
}

bool microros_serial_close(struct uxrCustomTransport *transport)
{
  int fd = serial_fd(transport);

  if (fd >= 0)
    {
      close(fd);
      serial_set_fd(transport, -1);
    }

  return true;
}

size_t microros_serial_write(struct uxrCustomTransport *transport,
                             const uint8_t *buf, size_t len, uint8_t *err)
{
  int fd = serial_fd(transport);
  ssize_t n;

  if (fd < 0)
    {
      *err = 1;
      return 0;
    }

  n = write(fd, buf, len);
  if (n < 0)
    {
      *err = 1;
      return 0;
    }

  return n;
}

size_t microros_serial_read(struct uxrCustomTransport *transport,
                            uint8_t *buf, size_t len,
                            int timeout_ms, uint8_t *err)
{
  int fd = serial_fd(transport);
  struct pollfd pfd;
  ssize_t n;
  int r;

  if (fd < 0)
    {
      *err = 1;
      return 0;
    }

  pfd.fd     = fd;
  pfd.events = POLLIN;

  r = poll(&pfd, 1, timeout_ms);
  if (r < 0)
    {
      *err = 1;
      return 0;
    }

  if (r == 0)
    {
      return 0;
    }

  n = read(fd, buf, len);
  if (n < 0)
    {
      *err = 1;
      return 0;
    }

  return n;
}
