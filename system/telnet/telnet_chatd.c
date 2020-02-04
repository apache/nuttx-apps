/****************************************************************************
 * apps/system/telnet/telnet_chatd.c
 *
 * Leveraged from libtelnet, https://github.com/seanmiddleditch/libtelnet.
 * Modified and re-released under the BSD license:
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * The original authors of libtelnet are listed below.  Per their licesne,
 * "The author or authors of this code dedicate any and all copyright
 * interest in this code to the public domain. We make this dedication for
 * the benefit of the public at large and to the detriment of our heirs and
 * successors.  We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * code under copyright law."
 *
 *   Author: Sean Middleditch <sean@sourcemud.org>
 *   (Also listed in the AUTHORS file are Jack Kelly <endgame.dos@gmail.com>
 *   and Katherine Flavel <kate@elide.org>)
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

#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <netdb.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "netutils/telnetc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_USERS       64
#define LINEBUFFER_SIZE 256

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct user_s
{
  char *name;
  int sock;
  struct telnet_s *telnet;
  char linebuf[256];
  int linepos;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct telnet_telopt_s g_telopts[] =
{
  {
    TELNET_TELOPT_COMPRESS2, TELNET_WILL, TELNET_DONT
  },
  {
    -1, 0, 0
  }
};

static struct user_s g_users[MAX_USERS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void cleanup_exit(void)
{
  int i;

  for (i = 0; i != MAX_USERS; ++i)
    {
      if (g_users[i].sock != -1)
        {
          close(g_users[i].sock);
          free(g_users[i].name);
          telnet_free(g_users[i].telnet);
      }
    }

  exit(1);
}

static void linebuffer_push(char *buffer, size_t size, int *linepos,
                            char ch, void (*cb) (const char *line, int overflow,
                                                 void *ud), void *ud)
{
  /* CRLF -- line terminator */

  if (ch == '\n' && *linepos > 0 && buffer[*linepos - 1] == '\r')
    {
      /* NUL terminate (replaces \r in buffer), notify app, clear */

      buffer[*linepos - 1] = 0;
      cb(buffer, 0, ud);
      *linepos = 0;

      /* CRNUL -- just a CR */
    }
  else if (ch == 0 && *linepos > 0 && buffer[*linepos - 1] == '\r')
    {
      /* So nothing, the CR is already in the buffer */

      /* Anything else (including technically invalid CR followed by anything
       * besides LF or NUL -- just buffer if we have room \r
       */
    }
  else if (*linepos != size)
    {
      buffer[(*linepos)++] = ch;

      /* Buffer overflow */
    }
  else
    {
      /* Terminate (NOTE: eats a byte), notify app, clear buffer */

      buffer[size - 1] = 0;
      cb(buffer, size - 1, ud);
      *linepos = 0;
    }
}

static void _message(const char *from, const char *msg)
{
  int i;

  for (i = 0; i != MAX_USERS; ++i)
    {
      if (g_users[i].sock != -1)
        {
          telnet_printf(g_users[i].telnet, "%s: %s\n", from, msg);
        }
    }
}

static void _send(int sock, const char *buffer, unsigned int size)
{
  int ret;

  /* Ignore on invalid socket */

  if (sock == -1)
    {
      return;
    }

  /* Send data */

  while (size > 0)
    {
      if ((ret = send(sock, buffer, size, 0)) == -1)
        {
          if (errno != EINTR && errno != ECONNRESET)
            {
              fprintf(stderr, "send() failed: %d\n", errno);
              cleanup_exit();
            }
          else
            {
              return;
            }
        }
      else if (ret == 0)
        {
          fprintf(stderr, "send() unexpectedly returned 0\n");
          cleanup_exit();
        }

      /* Update pointer and size to see if we've got more to send */

      buffer += ret;
      size -= ret;
    }
}

/* Process input line */

static void _online(const char *line, int overflow, void *ud)
{
  struct user_s *user = (struct user_s *)ud;
  int i;

  /* If the user has no name, this is his "login" */

  if (user->name == 0)
    {
      /* Must not be empty, must be at least 32 chars */

      if (strlen(line) == 0 || strlen(line) > 32)
        {
          telnet_printf(user->telnet, "Invalid name.\nEnter name: ");
          return;
        }

      /* Must not already be in use */

      for (i = 0; i != MAX_USERS; ++i)
        {
          if (g_users[i].name != 0 && strcmp(g_users[i].name, line) == 0)
            {
              telnet_printf(user->telnet, "Name in use.\nEnter name: ");
              return;
            }
        }

      /* Keep name */

      user->name = strdup(line);
      telnet_printf(user->telnet, "Welcome, %s!\n", line);
      return;
    }

  /* If line is "quit" then, well, quit */

  if (strcmp(line, "quit") == 0)
    {
      close(user->sock);
      user->sock = -1;
      _message(user->name, "** HAS QUIT **");
      free(user->name);
      user->name = 0;
      telnet_free(user->telnet);
      return;
    }

  /* Just a message -- send to all users */

  _message(user->name, line);
}

static void _input(struct user_s *user, const char *buffer, unsigned int size)
{
  unsigned int i;

  for (i = 0; user->sock != -1 && i != size; ++i)
    {
      linebuffer_push(user->linebuf, sizeof(user->linebuf), &user->linepos,
                      (char)buffer[i], _online, user);
    }
}

static void _event_handler(struct telnet_s *telnet,
                           union telnet_event_u *ev, void *user_data)
{
  struct user_s *user = (struct user_s *)user_data;

  switch (ev->type)
    {
    /* Data received */

    case TELNET_EV_DATA:
      _input(user, ev->data.buffer, ev->data.size);
      telnet_negotiate(telnet, TELNET_WONT, TELNET_TELOPT_ECHO);
      telnet_negotiate(telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
      break;

    /* Data must be sent */

    case TELNET_EV_SEND:
      _send(user->sock, ev->data.buffer, ev->data.size);
      break;

    /* Enable compress2 if accepted by client */

    case TELNET_EV_DO:
      if (ev->neg.telopt == TELNET_TELOPT_COMPRESS2)
        {
          telnet_begin_compress2(telnet);
        }
      break;

    /* Error */

    case TELNET_EV_ERROR:
      close(user->sock);
      user->sock = -1;
      if (user->name != 0)
        {
          _message(user->name, "** HAS HAD AN ERROR **");
          free(user->name);
          user->name = 0;
        }

      telnet_free(user->telnet);
      break;

    default:
      /* Ignore */

      break;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char buffer[512];
  short listen_port;
  int listen_sock;
  int ret;
  int i;
  struct sockaddr_in addr;
  socklen_t addrlen;
  struct pollfd pfd[MAX_USERS + 1];

  /* Check usage */

  if (argc != 2)
    {
      fprintf(stderr, "Usage:\n %s <port>\n", argv[0]);
      return 1;
    }

  /* Initialize data structures */

  memset(&pfd, 0, sizeof(pfd));
  memset(g_users, 0, sizeof(g_users));

  for (i = 0; i != MAX_USERS; ++i)
    {
      g_users[i].sock = -1;
    }

  /* Parse listening port */

  listen_port = (short)strtol(argv[1], 0, 10);

  /* Create listening socket */

  if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      fprintf(stderr, "socket() failed: %d\n", errno);
      return 1;
    }

  /* Reuse address option */

  ret = 1;
  setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&ret, sizeof(ret));

  /* Bind to listening addr/port */

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(listen_port);
  if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
      fprintf(stderr, "bind() failed: %d\n", errno);
      return 1;
    }

  /* Listen for clients */

  if (listen(listen_sock, 5) == -1)
    {
      fprintf(stderr, "listen() failed: %d\n", errno);
      return 1;
    }

  printf("LISTENING ON PORT %d\n", listen_port);

  /* Initialize listening descriptors */

  pfd[MAX_USERS].fd = listen_sock;
  pfd[MAX_USERS].events = POLLIN;

  /* Loop for ever */

  for (;;)
    {
      /* Prepare for poll */

      for (i = 0; i != MAX_USERS; ++i)
        {
          if (g_users[i].sock != -1)
            {
              pfd[i].fd = g_users[i].sock;
              pfd[i].events = POLLIN;
            }
          else
            {
              pfd[i].fd = -1;
              pfd[i].events = 0;
            }
        }

      /* Poll */

      ret = poll(pfd, MAX_USERS + 1, -1);
      if (ret == -1 && errno != EINTR)
        {
          fprintf(stderr, "poll() failed: %d\n", errno);
          cleanup_exit();
        }

      /* New connection */

      if (pfd[MAX_USERS].revents & (POLLIN | POLLERR | POLLHUP))
        {
          /* Accept the sock */

          addrlen = sizeof(addr);
          if ((ret = accept(listen_sock, (struct sockaddr *)&addr,
                           &addrlen)) == -1)
            {
              fprintf(stderr, "accept() failed: %d\n", errno);
              cleanup_exit();
            }

          printf("Connection received.\n");

          /* Find a free user */

          for (i = 0; i != MAX_USERS; ++i)
            {
              if (g_users[i].sock == -1)
                {
                  break;
                }
            }

          if (i == MAX_USERS)
            {
              printf("  rejected (too many users)\n");
              _send(ret, "Too many users.\r\n", 14);
              close(ret);
            }

          /* Init, welcome */

          g_users[i].sock = ret;
          g_users[i].telnet = telnet_init(g_telopts, _event_handler, 0, &g_users[i]);
          telnet_negotiate(g_users[i].telnet, TELNET_WILL,
                           TELNET_TELOPT_COMPRESS2);
          telnet_printf(g_users[i].telnet, "Enter name: ");

          telnet_negotiate(g_users[i].telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
        }

      /* Read from client */

      for (i = 0; i != MAX_USERS; ++i)
        {
          /* Skip users that aren't actually connected */

          if (g_users[i].sock == -1)
            {
              continue;
            }

          if (pfd[i].revents & (POLLIN | POLLERR | POLLHUP))
            {
              if ((ret = recv(g_users[i].sock, buffer, sizeof(buffer), 0)) > 0)
                {
                  telnet_recv(g_users[i].telnet, buffer, ret);
                }
              else if (ret == 0)
                {
                  printf("Connection closed.\n");
                  close(g_users[i].sock);
                  g_users[i].sock = -1;
                  if (g_users[i].name != 0)
                    {
                      _message(g_users[i].name, "** HAS DISCONNECTED **");
                      free(g_users[i].name);
                      g_users[i].name = 0;
                    }

                  telnet_free(g_users[i].telnet);
                }
              else if (errno != EINTR)
                {
                  fprintf(stderr, "recv(client) failed: %d\n", errno);
                  cleanup_exit();
                }
            }
        }
    }

  /* Not that we can reach this, but GCC will cry if it's not here */

  return 0;
}
