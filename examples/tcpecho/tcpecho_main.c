/****************************************************************************
 * examples/tcpecho/tcpecho_main.c
 *
 *   Copyright (C) 2013 Max Holtzberg. All rights reserved.
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *
 *   Authors: Max Holtzberg <mh@uvc.de>
 *            Gregory Nutt <gnutt@nuttx.org>
 *
 * This code is based upon the poll example from W. Richard Stevens'
 * UNIX Network Programming Book.
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

#include <debug.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <net/if.h>
#include <nuttx/net/uip/uip.h>
#include <nuttx/net/uip/uip-arp.h>

#include <apps/netutils/uiplib.h>

#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
#  include <arpa/inet.h>
#endif

/* Here we include the header file for the application(s) we use in
 * our project as defined in the config/<board-name>/defconfig file
 */

/* DHCPC may be used in conjunction with any other feature (or not) */

#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
#  include <apps/netutils/dnsclient.h>
#  include <apps/netutils/dhcpc.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TCPECHO_MAXLINE 64
#define TCPECHO_POLLTIMEOUT 10000

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int tcpecho_netsetup(void);
static int tcpecho_server(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int tcpecho_netsetup()
{
  /* If this task is excecutated as an NSH built-in function, then the
   * network has already been configured by NSH's start-up logic.
   */

#ifndef CONFIG_NSH_BUILTIN_APPS
  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_TCPECHO_DHCPC) || defined(CONFIG_EXAMPLES_TCPECHO_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif
#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
  struct dhcpc_state ds;
  void *handle;
#endif

/* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_TCPECHO_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  uip_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
  addr.s_addr = 0;
#else
  addr.s_addr = HTONL(CONFIG_EXAMPLES_TCPECHO_IPADDR);
#endif
  uip_sethostaddr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_TCPECHO_DRIPADDR);
  uip_setdraddr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_TCPECHO_NETMASK);
  uip_setnetmask("eth0", &addr);

#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
  /* Set up the resolver */

  dns_bind();

  /* Get the MAC address of the NIC */

  uip_getmacaddr("eth0", mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open(&mac, IFHWADDRLEN);

  /* Get an IP address.  Note:  there is no logic here for renewing the address in this
   * example.  The address should be renewed in ds.lease_time/2 seconds.
   */

  if (!handle)
    {
      return ERROR;
    }

  if (dhcpc_request(handle, &ds) != OK)
    {
      return ERROR;
    }

  uip_sethostaddr("eth1", &ds.ipaddr);

  if (ds.netmask.s_addr != 0)
    {
      uip_setnetmask("eth0", &ds.netmask);
    }

  if (ds.default_router.s_addr != 0)
    {
      uip_setdraddr("eth0", &ds.default_router);
    }

  if (ds.dnsaddr.s_addr != 0)
    {
      dns_setserver(&ds.dnsaddr);
    }

  dhcpc_close(handle);
  printf("IP: %s\n", inet_ntoa(ds.ipaddr));

#endif /* CONFIG_EXAMPLES_TCPECHO_DHCPC */
#endif /* CONFIG_NSH_BUILTIN_APPS */

  return OK;
}

static int tcpecho_server(void)
{
  int i, maxi, listenfd, connfd, sockfd;
  int nready;
  int ret;
  ssize_t n;
  char buf[TCPECHO_MAXLINE];
  socklen_t clilen;
  bool stop = false;
  struct pollfd client[CONFIG_EXAMPLES_TCPECHO_NCONN];
  struct sockaddr_in cliaddr, servaddr;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  if (listenfd < 0)
    {
      perror("ERROR: failed to create socket.\n");
      return ERROR;
    }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port        = htons(CONFIG_EXAMPLES_TCPECHO_PORT);

  ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if (ret < 0)
    {
      perror("ERROR: failed to bind socket.\n");
      return ERROR;
    }

  ndbg("start listening on port: %d\n", CONFIG_EXAMPLES_TCPECHO_PORT);

  ret = listen(listenfd, CONFIG_EXAMPLES_TCPECHO_BACKLOG);
  if (ret < 0)
    {
      perror("ERROR: failed to start listening\n");
      return ERROR;
    }

  client[0].fd = listenfd;
  client[0].events = POLLRDNORM;
  for (i = 1; i < CONFIG_EXAMPLES_TCPECHO_NCONN; i++)
    {
      client[i].fd = -1;        /* -1 indicates available entry */
    }

  maxi = 0;                     /* max index into client[] array */

  while (!stop)
    {
      nready = poll(client, maxi+1, TCPECHO_POLLTIMEOUT);

      if (client[0].revents & POLLRDNORM)
        {
          /* new client connection */

          clilen = sizeof(cliaddr);
          connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);

          ndbg("new client: %s\n", inet_ntoa(cliaddr.sin_addr));

          for (i = 1; i < CONFIG_EXAMPLES_TCPECHO_NCONN; i++)
            {
              if (client[i].fd < 0)
                {
                  client[i].fd = connfd;  /* save descriptor */
                  break;
                }
            }

          if (i == CONFIG_EXAMPLES_TCPECHO_NCONN)
            {
              perror("ERROR: too many clients");
              return ERROR;
            }

          client[i].events = POLLRDNORM;
          if (i > maxi)
            {
              maxi = i;    /* max index in client[] array */
            }

          if (--nready <= 0)
            {
              continue;    /* no more readable descriptors */
            }
        }

      for (i = 1; i <= maxi; i++)
        {
          /* check all clients for data */

          if ((sockfd = client[i].fd) < 0)
            {
              continue;
            }

          if (client[i].revents & (POLLRDNORM | POLLERR))
            {
              if ( (n = read(sockfd, buf, TCPECHO_MAXLINE)) < 0)
                {
                  if (errno == ECONNRESET)
                    {
                      /* connection reset by client */

                      ndbg("client[%d] aborted connection\n", i);

                      close(sockfd);
                      client[i].fd = -1;
                    }
                  else
                    {
                      perror("ERROR: readline error\n");
                      close(sockfd);
                      client[i].fd = -1;
                    }
                }
              else if (n == 0)
                {
                  /* connection closed by client */

                  ndbg("client[%d] closed connection\n", i);

                  close(sockfd);
                  client[i].fd = -1;
                }
              else
                {
                  if (strcmp(buf, "exit\r\n") == 0)
                    {
                      ndbg("client[%d] closed connection\n", i);
                      close(sockfd);
                      client[i].fd = -1;
                    }
                  else
                    {
                      write(sockfd, buf, n);
                    }
                }

              if (--nready <= 0)
                {
                  break;  /* no more readable descriptors */
                }
            }
        }
    }

  for (i = 0; i <= maxi; i++)
    {
      if (client[i].fd < 0)
        {
          continue;
        }

      close(client[i].fd);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * discover_main
 ****************************************************************************/

int tcpecho_main(int argc, char *argv[])
{
  int ret;

  ret = tcpecho_netsetup();
  if (ret != OK)
    {
      perror("ERROR: failed to setup network\n");
      exit(1);
    }

  printf("Start echo server\n");

  ret = tcpecho_server();

  return ret;
}

