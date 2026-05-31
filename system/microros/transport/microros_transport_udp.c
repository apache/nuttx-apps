/****************************************************************************
 * apps/system/microros/transport/microros_transport_udp.c
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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "microros_transport.h"

/****************************************************************************
 * Private Helpers
 ****************************************************************************/

static inline int udp_fd(struct uxrCustomTransport *transport)
{
  return (int)(intptr_t)transport->args - 1;
}

static inline void udp_set_fd(struct uxrCustomTransport *transport, int fd)
{
  transport->args = (void *)(intptr_t)(fd + 1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool microros_udp_open(struct uxrCustomTransport *transport)
{
  struct sockaddr_in addr;
  int fd;

  fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  if (fd < 0)
    {
      return false;
    }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(CONFIG_MICROROS_AGENT_PORT);

  if (inet_pton(AF_INET, CONFIG_MICROROS_AGENT_IP, &addr.sin_addr) != 1)
    {
      close(fd);
      return false;
    }

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      close(fd);
      return false;
    }

  udp_set_fd(transport, fd);
  return true;
}

bool microros_udp_close(struct uxrCustomTransport *transport)
{
  int fd = udp_fd(transport);

  if (fd >= 0)
    {
      close(fd);
      udp_set_fd(transport, -1);
    }

  return true;
}

size_t microros_udp_write(struct uxrCustomTransport *transport,
                          const uint8_t *buf, size_t len, uint8_t *err)
{
  int fd = udp_fd(transport);
  ssize_t n;

  if (fd < 0)
    {
      *err = 1;
      return 0;
    }

  n = send(fd, buf, len, 0);
  if (n < 0)
    {
      *err = 1;
      return 0;
    }

  return n;
}

size_t microros_udp_read(struct uxrCustomTransport *transport,
                         uint8_t *buf, size_t len,
                         int timeout_ms, uint8_t *err)
{
  int fd = udp_fd(transport);
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

  n = recv(fd, buf, len, 0);
  if (n < 0)
    {
      *err = 1;
      return 0;
    }

  return n;
}
