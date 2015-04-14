/****************************************************************************
 * netutils/pppd/chat.c
 *
 *   Copyright (C) 2015 Max Nekludov. All rights reserved.
 *   Author: Max Nekludov <macscomp@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "ppp_conf.h"
#include "ppp_arch.h"
#include "chat.h"

#include <poll.h>

#include <apps/netutils/pppd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CHAT_MAX_SKIP     8
#define CHAT_ECHO_TIMEOUT 500

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: chat_read_byte
 ****************************************************************************/

static int chat_read_byte(int fd, char* c, int timeout)
{
  int ret;
  struct pollfd fds;

  fds.fd = fd;
  fds.events = POLLIN;
  fds.revents = 0;

  ret = poll(&fds, 1, timeout);
  if (ret <= 0)
    {
      return -1;
    }

  ret = read(fd, c, 1);
  if (ret != 1)
    {
      return -1;
    }

  printf("chat: char = %c (0x%02X)\n", *c, *c);

  return 0;
}

/****************************************************************************
 * Name: chat_flush
 ****************************************************************************/

static void chat_flush(int fd)
{
  char tmp;
  while (chat_read_byte(fd, &tmp, 0) == 0);
}

/****************************************************************************
 * Name: chat_check_response
 ****************************************************************************/

static int chat_check_response(int fd, const char* response, int timeout)
{
  char c;
  int ret;
  int skip = CHAT_MAX_SKIP;

  while (*response)
    {
      ret = chat_read_byte(fd, &c, timeout);
      if (ret < 0)
        {
          return ret;
        }

      if (skip > 0 && (c == '\r' || c == '\n')) 
        {
          --skip;
          continue;
        }

      if (c == *response)
        {
          ++response;
        }
      else
        {
          return -1;
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ppp_chat
 ****************************************************************************/

int ppp_chat(int fd, struct chat_script_s *script, int echo)
{
  int ret;
  size_t len;
  struct chat_line_s *line = script->lines;
  const char* request = line->request;
  const char* response = line->response;

  while (request)
    {
      chat_flush(fd);

      printf("chat: send '%s`\n", request);
      len = strlen(request);
      ret = write(fd, request, len);
      if (ret < 0)
        {
          return ret;
        }
      else if ((size_t)ret != len)
        {
          return -1;
        }

      ret = write(fd, "\r\n", 2);
      if (ret != 2)
        {
          return -1;
        }

      /* Check echo if enabled */

      if (echo)
        {
          ret = chat_check_response(fd, request, CHAT_ECHO_TIMEOUT);
          if (ret < 0)
            {
              printf("chat: invalid echo\n");
              return ret;
            }
        }

      if (response)
        {
          printf("chat: wait for '%s`\n", response);
          ret = chat_check_response(fd, response, script->timeout * 1000);
          if (ret < 0)
            {
              printf("chat: bad response\n");
              return ret;
            }

          printf("chat: got it!\n");
        }

      ++line;
      request = line->request;
      response = line->response;
    }

  return 0;
}
