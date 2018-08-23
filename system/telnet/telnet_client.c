/****************************************************************************
 * apps/system/telnet/telnet_client.c
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

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <netdb.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_ZLIB
#  include "zlib.h"
#endif

#include "system/readline.h"
#include "netutils/telnetc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_NSH_TELNETD_PORT
#  define DEFAULT_PORT CONFIG_NSH_TELNETD_PORT
#else
#  define DEFAULT_PORT 23
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct telnet_s *g_telnet;
static int g_echo;

static const struct telnet_telopt_s g_telopts[] =
{
  {
    TELNET_TELOPT_ECHO, TELNET_WONT, TELNET_DO
  },
  {
    TELNET_TELOPT_TTYPE, TELNET_WILL, TELNET_DONT
  },
  {
    TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DO
  },
  {
    TELNET_TELOPT_MSSP, TELNET_WONT, TELNET_DO
  },
  {
    -1, 0, 0
  }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void send_local_input(char *buffer, int size)
{
  static char crlf[] = { '\r', '\n' };
  int i;

  for (i = 0; i != size; ++i)
    {
      /* If we got a CR or LF, replace with CRLF NOTE that usually you'd get a
       * CR in UNIX, but in raw mode we get LF instead (not sure why).
       */

      if (buffer[i] == '\r' || buffer[i] == '\n')
        {
          if (g_echo)
            {
              printf("\r\n");
            }

          telnet_send(g_telnet, crlf, 2);
        }
      else
        {
          if (g_echo)
            {
              putchar(buffer[i]);
            }

          telnet_send(g_telnet, buffer + i, 1);
        }
    }

  fflush(stdout);
}

static void telnet_ev_send(int sock, const char *buffer, size_t size)
{
  int ret;

  /* Send data */

  while (size > 0)
    {
      if ((ret = send(sock, buffer, size, 0)) == -1)
        {
          fprintf(stderr, "send() failed: %d\n", errno);
          exit(1);
        }
      else if (ret == 0)
        {
          fprintf(stderr, "send() unexpectedly returned 0\n");
          exit(1);
        }

      /* Update pointer and size to see if we've got more to send */

      buffer += ret;
      size -= ret;
    }
}

static void _event_handler(struct telnet_s *telnet,
                           union telnet_event_u *ev, void *user_data)
{
  int sock = *(int *)user_data;

  switch (ev->type)
    {
    /* Data received */

    case TELNET_EV_DATA:
      printf("%.*s", (int)ev->data.size, ev->data.buffer);
      fflush(stdout);
      break;

    /* Data must be sent */

    case TELNET_EV_SEND:
      telnet_ev_send(sock, ev->data.buffer, ev->data.size);
      break;

    /* Request to enable remote feature (or receipt) */

    case TELNET_EV_WILL:
      /* We'll agree to turn off our echo if server wants us to stop */

      if (ev->neg.telopt == TELNET_TELOPT_ECHO)
        {
          g_echo = 0;
        }

      break;

    /* Notification of disabling remote feature (or receipt) */

    case TELNET_EV_WONT:
      if (ev->neg.telopt == TELNET_TELOPT_ECHO)
        {
          g_echo = 1;
        }
      break;

    /* Request to enable local feature (or receipt) */

    case TELNET_EV_DO:
      break;

    /* Demand to disable local feature (or receipt) */

    case TELNET_EV_DONT:
      break;

    /* Respond to TTYPE commands */

    case TELNET_EV_TTYPE:
      /* Respond with our terminal type, if requested */

      if (ev->ttype.cmd == TELNET_TTYPE_SEND)
        {
          telnet_ttype_is(telnet, getenv("TERM"));
        }
      break;

    /* Respond to particular subnegotiations */

    case TELNET_EV_SUBNEGOTIATION:
      break;

    /* Error */

    case TELNET_EV_ERROR:
      fprintf(stderr, "ERROR: %s\n", ev->error.msg);
      exit(1);

    default:
      /* Ignore */

      break;
    }
}

static void show_usage(const char *progname, int exitcode)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "\t%s <server-IP-addr> [<port>]\n", progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t<server-IP-addr> is the address of the Telnet server.  Either\n");
  fprintf(stderr, "\t\tIPv4 form: ddd.ddd.ddd.ddd\n");
  fprintf(stderr, "\t\tIPv6 form: xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx\n");
  fprintf(stderr, "\t<port> is the (optional) listening port of the Telnet server.\n");
  fprintf(stderr, "\t\tDefault: %u\n", DEFAULT_PORT);
  exit(exitcode)  ;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_LOADABLE
int main(int argc, FAR char *argv[])
#else
int telnet_main(int argc, char *argv[])
#endif
{
  char buffer[512];
  union
  {
    struct sockaddr generic;
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
  } server;
  union
  {
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
  } local;
  struct pollfd pfd[2];
  sa_family_t family;
  uint16_t addrlen;
  int portno;
  int sock;
  int ret;

  /* Check usage */

  if (argc < 2 || argc > 3)
    {
      fprintf(stderr, "Invalid number of arguments\n");
      show_usage(argv[0], 1);
    }

  /* Convert the port number to binary if provided */

  if (argc == 3)
    {
      portno = atoi(argv[2]);
      if (portno < 0 || portno > UINT16_MAX)
        {
          fprintf(stderr, "Invalid port number\n");
          show_usage(argv[0], 1);
        }
    }
  else
    {
      portno = DEFAULT_PORT;
    }

  /* Convert the <server-IP-addr> argument into a binary address */

  memset(&local, 0, sizeof(local));

#ifdef CONFIG_NET_IPv6
  addrlen                 = sizeof(struct sockaddr_in6);
  family                  = AF_INET6;

  local.ipv6.sin6_family  = AF_INET6;
  server.ipv6.sin6_family = AF_INET6;
  server.ipv6.sin6_port   = htons(portno);

  ret = inet_pton(AF_INET6, argv[1], server.ipv6.sin6_addr.s6_addr);
  if (ret < 0)
#endif
#ifdef CONFIG_NET_IPv4
    {
      addrlen                = sizeof(struct sockaddr_in);
      family                 = AF_INET;

      local.ipv4.sin_family  = AF_INET;
      server.ipv4.sin_family = AF_INET;
      server.ipv4.sin_port   = htons(portno);

      ret = inet_pton(AF_INET, argv[1], &server.ipv4.sin_addr);
    }

  if (ret < 0)
#endif
    {
      fprintf(stderr, "ERROR: <server-IP-addr> is invalid\n");
      show_usage(argv[0], 1);
    }

  /* Create server socket */

  sock = socket(family, SOCK_STREAM, 0);
   if (sock < 0)
    {
      fprintf(stderr, "socket() failed: %d\n", errno);
      return 1;
    }

  /* Bind server socket */

  ret = bind(sock, (struct sockaddr *)&local, addrlen);
  if (ret < 0)
    {
      fprintf(stderr, "bind() failed: %d\n", errno);
      return 1;
    }

  /* Connect */

  ret = connect(sock, &server.generic, addrlen);
  if (ret < 0)
    {
      fprintf(stderr, "connect() failed: %d\n", errno);
      return 1;
    }

  /* Set input echoing on by default */

  g_echo = 1;

  /* Initialize telnet box */

  g_telnet = telnet_init(g_telopts, _event_handler, 0, &sock);

  /* Initialize poll descriptors */

  memset(pfd, 0, sizeof(pfd));
  pfd[0].fd = 1;
  pfd[0].events = POLLIN;
  pfd[1].fd = sock;
  pfd[1].events = POLLIN;

  /* Loop while both connections are open */

  while (poll(pfd, 2, -1) != -1)
    {
      /* Read from stdin */

      if (pfd[0].revents & POLLIN)
        {
          ret = std_readline(buffer, sizeof(buffer));
          if (ret > 0)
            {
              send_local_input(buffer, ret);
            }
          else if (ret == 0)
            {
              break;
            }
          else
            {
              fprintf(stderr, "recv(server) failed: %d\n", errno);
              exit(1);
            }
        }

      /* Read from client */

      if (pfd[1].revents & POLLIN)
        {
          if ((ret = recv(sock, buffer, sizeof(buffer), 0)) > 0)
            {
              telnet_recv(g_telnet, buffer, ret);
            }
          else if (ret == 0)
            {
              break;
            }
          else
            {
              fprintf(stderr, "recv(client) failed: %d\n", errno);
              exit(1);
            }
        }
    }

  /* Clean up */

  telnet_free(g_telnet);
  close(sock);
  return 0;
}
