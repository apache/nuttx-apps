/****************************************************************************
 * apps/examples/nxterm/nxterm_pty.c
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

#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <nuttx/input/keyboard.h>

#include "nxterm_internal.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nxterm_pty_s
{
  int masterfd;
  int nxtermfd;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct nxterm_pty_s g_pty;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxterm_writeall
 ****************************************************************************/

static int nxterm_writeall(int fd, FAR const void *buffer, size_t buflen)
{
  FAR const uint8_t *src = buffer;

  while (buflen > 0)
    {
      ssize_t nwritten = write(fd, src, buflen);
      if (nwritten < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          return -errno;
        }

      src    += nwritten;
      buflen -= nwritten;
    }

  return OK;
}

/****************************************************************************
 * Name: nxterm_pty_key
 ****************************************************************************/

static void nxterm_pty_key(FAR struct nxterm_pty_s *priv, uint32_t code,
                           bool special)
{
  FAR const char *sequence = NULL;
  size_t seqlen = 0;
  char normal;

  if (!special)
    {
      normal = (char)code;
      nxterm_writeall(priv->masterfd, &normal, 1);
      return;
    }

  switch (code)
    {
      case KEYCODE_FWDDEL:
        sequence = "\033[3~";
        seqlen   = 4;
        break;

      case KEYCODE_BACKDEL:
        sequence = "\177";
        seqlen   = 1;
        break;

      case KEYCODE_HOME:
        sequence = "\033[H";
        seqlen   = 3;
        break;

      case KEYCODE_END:
        sequence = "\033[F";
        seqlen   = 3;
        break;

      case KEYCODE_LEFT:
        sequence = "\033[D";
        seqlen   = 3;
        break;

      case KEYCODE_RIGHT:
        sequence = "\033[C";
        seqlen   = 3;
        break;

      case KEYCODE_UP:
        sequence = "\033[A";
        seqlen   = 3;
        break;

      case KEYCODE_DOWN:
        sequence = "\033[B";
        seqlen   = 3;
        break;

      case KEYCODE_PAGEUP:
        sequence = "\033[5~";
        seqlen   = 4;
        break;

      case KEYCODE_PAGEDOWN:
        sequence = "\033[6~";
        seqlen   = 4;
        break;

      case KEYCODE_INSERT:
        sequence = "\033[2~";
        seqlen   = 4;
        break;

      case KEYCODE_ENTER:
        sequence = "\n";
        seqlen   = 1;
        break;

      default:
        break;
    }

  if (sequence != NULL)
    {
      nxterm_writeall(priv->masterfd, sequence, seqlen);
    }
}

/****************************************************************************
 * Name: nxterm_pty_bridge
 ****************************************************************************/

static int nxterm_pty_bridge(int argc, FAR char *argv[])
{
  FAR struct nxterm_pty_s *priv = &g_pty;
  struct keyboard_event_s events[8];
  struct pollfd fds[2];
  uint8_t buffer[64];
  int keyboardfd = -1;
  bool waiting = false;

  UNUSED(argc);
  UNUSED(argv);

  for (; ; )
    {
      if (keyboardfd < 0)
        {
          keyboardfd = open(CONFIG_EXAMPLES_NXTERM_KBDDEV,
                            O_RDONLY | O_NONBLOCK);
          if (keyboardfd < 0 && !waiting)
            {
              static const char message[] = "Waiting for a keyboard...\r\n";

              nxterm_writeall(priv->nxtermfd, message, sizeof(message) - 1);
              waiting = true;
            }
        }

      fds[0].fd      = priv->masterfd;
      fds[0].events  = POLLIN;
      fds[0].revents = 0;
      fds[1].fd      = keyboardfd;
      fds[1].events  = POLLIN;
      fds[1].revents = 0;

      if (poll(fds, keyboardfd < 0 ? 1 : 2,
               keyboardfd < 0 ? 1000 : -1) < 0)
        {
          if (errno != EINTR)
            {
              break;
            }

          continue;
        }

      if ((fds[0].revents & POLLIN) != 0)
        {
          ssize_t nread = read(priv->masterfd, buffer, sizeof(buffer));
          if (nread <= 0 ||
              nxterm_writeall(priv->nxtermfd, buffer, nread) < 0)
            {
              break;
            }
        }

      if (keyboardfd >= 0 && (fds[1].revents & POLLIN) != 0)
        {
          ssize_t nread = read(keyboardfd, events, sizeof(events));
          size_t i;

          if (nread <= 0)
            {
              close(keyboardfd);
              keyboardfd = -1;
              waiting    = false;
              continue;
            }

          waiting = false;
          for (i = 0;
               i < (size_t)nread / sizeof(struct keyboard_event_s);
               i++)
            {
              if (events[i].type == KEYBOARD_PRESS)
                {
                  nxterm_pty_key(priv, events[i].code, false);
                }
              else if (events[i].type == KEYBOARD_SPECPRESS)
                {
                  nxterm_pty_key(priv, events[i].code, true);
                }
            }
        }

      if (keyboardfd >= 0 &&
          (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
        {
          close(keyboardfd);
          keyboardfd = -1;
          waiting    = false;
        }
    }

  if (keyboardfd >= 0)
    {
      close(keyboardfd);
    }

  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxterm_pty_redirect
 ****************************************************************************/

int nxterm_pty_redirect(int nxtermfd)
{
  int slavefd;
  int ret;

  ret = openpty(&g_pty.masterfd, &slavefd, NULL, NULL, NULL);
  if (ret < 0)
    {
      return -errno;
    }

  g_pty.nxtermfd = nxtermfd;

  ret = task_create("NxTermPTY", CONFIG_EXAMPLES_NXTERM_CLIENTPRIO,
                    CONFIG_EXAMPLES_NXTERM_STACKSIZE,
                    nxterm_pty_bridge, NULL);
  if (ret < 0)
    {
      close(slavefd);
      close(g_pty.masterfd);
      return -errno;
    }

  if (dup2(slavefd, STDIN_FILENO) < 0 ||
      dup2(slavefd, STDOUT_FILENO) < 0 ||
      dup2(slavefd, STDERR_FILENO) < 0)
    {
      ret = -errno;
      close(slavefd);
      return ret;
    }

  close(slavefd);
  return OK;
}
