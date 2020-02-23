/****************************************************************************
 * apps/examples/xmlrpc/xmlrpc_main.c
 *
 *   Copyright (C) 2012 Max Holtzberg. All rights reserved.
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Author: Max Holtzberg <mh@uvc.de>
 *
 * Based on the embeddable lightweight XML-RPC server code discussed
 * in the article at: http://www.drdobbs.com/web-development/\
 *    an-embeddable-lightweight-xml-rpc-server/184405364
 *
 *  Copyright (c) 2002 Cogito LLC.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or
 *  without modification, is hereby granted without fee provided
 *  that the following conditions are met:
 *
 *    1.  Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the
 *        following disclaimer.
 *    2.  Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the
 *        following disclaimer in the documentation and/or other
 *        materials provided with the distribution.
 *    3.  Neither the name of Cogito LLC nor the names of its
 *        contributors may be used to endorse or promote products
 *        derived from this software without specific prior
 *        written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY COGITO LLC AND CONTRIBUTORS 'AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL COGITO LLC
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARAY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/

/*
 *  Lightweight Embedded XML-RPC Server main
 *
 *  mtj@cogitollc.com
 *
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <debug.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <netinet/in.h>

#include <nuttx/net/arp.h>
#include "netutils/netlib.h"
#include "netutils/xmlrpc.h"

#ifdef CONFIG_EXAMPLES_XMLRPC_DHCPC
#  include <arpa/inet.h>
#endif

/* Here we include the header file for the application(s) we use in
 * our project as defined in the config/<board-name>/defconfig file
 */

/* DHCPC may be used in conjunction with any other feature (or not) */

#ifdef CONFIG_EXAMPLES_XMLRPC_DHCPC
#  include "netutils/dhcpc.h"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *notimplemented = { "HTTP/1.1 501 Not Implemented\n\n" };
static const char *separator = { "\015\012\015\012" };

/****************************************************************************
 * External Function Prototypes
 ****************************************************************************/

extern void calls_register(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: xmlrpc_findbody
 *
 * Description:
 *   Find the message body of an HTTP Request Message
 *
 ****************************************************************************/

static char *xmlrpc_findbody(char *buf)
{
  char *temp;

  temp = strstr(buf, separator);

  if (temp == NULL)
    {
      return NULL;
    }
  else
    {
      return temp + 4;
    }
}

/****************************************************************************
 * Name: xmlrpc_getheader
 *
 * Description:
 *   Find the HTTP header and return it's value.
 *
 ****************************************************************************/

static int xmlrpc_getheader(char *buffer, char *header, char *value, int size)
{
  char *temp;
  int i = 0;

  temp = strstr(buffer, header);
  if (temp)
    {
      /* Skip the header element */

      temp += strlen(header);

      /* Skip any white-space */

      while (*temp == ' ')
        {
          temp++;
        }

      /* Copy the rest to the value parameter */

      while ((*temp != ' ') && (*temp != '\n') && (i < size))
        {
          value[i++] = *temp++;
        }

      value[i] = 0;
      return i;
    }

  return -1;
}

/****************************************************************************
 * Name: xmlrpc_handler
 *
 * Description:
 *    Parse and handle the current HTTP request message.
 *
 ****************************************************************************/

static void xmlrpc_handler(int fd)
{
  fd_set rfds;
  struct timeval tv;
  int ret, len, max = 0, loadlen = -1;
  char buffer[CONFIG_EXAMPLES_XMLRPC_BUFFERSIZE] = { 0 };
  char value[CONFIG_XMLRPC_STRINGSIZE + 1];
  char *temp;

  /* Read in the Request Header */

  do
    {
      FD_ZERO(&rfds);
      FD_SET(fd, &rfds);

      tv.tv_sec = 1;
      tv.tv_usec = 0;

      ninfo("[%d] select...\n", fd);
      ret = select(fd + 1, &rfds, NULL, NULL, &tv);
      ninfo("[%d] data ready\n", fd);

      if (ret > 0)
        {
          if (FD_ISSET(fd, &rfds))
            {
              len = recv(fd, &buffer[max], 1024, 0);
              ninfo("[%d] %d bytes received\n", fd, len);

              if (len > 0)
                {
                  max += len;
                  buffer[max] = 0;

                  ret = xmlrpc_getheader(buffer, "Content-Length:", value,
                                         CONFIG_EXAMPLES_XMLRPC_BUFFERSIZE);
                  if (ret > 0)
                    loadlen = atoi(value);
                }
              else
                {
                  ret = -1;
                  break;
                }
            }
        }
      else
        {
          /* Timeout... */

          nerr("ERROR: [%d] timeout\n", fd);
          ret = -1;
          break;
        }

      temp = strstr(buffer, separator);

      if (temp)
        {
          if (strlen(temp) - 4 == loadlen)
            break;
        }

    }
  while (1);

  /* Determine request */

  if (!strncmp(buffer, "POST", 4))
    {
      temp = xmlrpc_findbody(buffer);
      xmlrpc_parse(fd, temp);
    }
  else
    {
      write(fd, notimplemented, strlen(notimplemented));
    }
}

/****************************************************************************
 * Name: xmlrpc_netinit
 *
 * Description:
 *    Setup network configuration.
 *
 ****************************************************************************/

static int xmlrpc_netinit(void)
{
  /* If this task is excecutated as an NSH built-in function, then the network
   * has already been configured by NSH's start-up logic.
   */

#ifndef CONFIG_NSH_NETINIT
  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_XMLRPC_DHCPC) || defined(CONFIG_EXAMPLES_XMLRPC_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif
#ifdef CONFIG_EXAMPLES_XMLRPC_DHCPC
  void *handle;
#endif

/* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_XMLRPC_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

#ifdef CONFIG_EXAMPLES_XMLRPC_DHCPC
  addr.s_addr = 0;
#else
  addr.s_addr = HTONL(CONFIG_EXAMPLES_XMLRPC_IPADDR);
#endif
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_XMLRPC_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_XMLRPC_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");

#ifdef CONFIG_EXAMPLES_XMLRPC_DHCPC
  /* Get the MAC address of the NIC */

  netlib_getmacaddr("eth0", mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open("eth0", &mac, IFHWADDRLEN);

  /* Get an IP address.  Note: there is no logic here for renewing the address
   * in this example.  The address should be renewed in ds.lease_time/2
   * seconds.
   */

  printf("Getting IP address\n");
  if (handle)
    {
      struct dhcpc_state ds;
      dhcpc_request(handle, &ds);
      netlib_set_ipv4addr("eth0", &ds.ipaddr);

      if (ds.netmask.s_addr != 0)
        {
          netlib_set_ipv4netmask("eth0", &ds.netmask);
        }

      if (ds.default_router.s_addr != 0)
        {
          netlib_set_dripv4addr("eth0", &ds.default_router);
        }

      if (ds.dnsaddr.s_addr != 0)
        {
          netlib_set_ipv4dnsaddr(&ds.dnsaddr);
        }

      dhcpc_close(handle);
      printf("IP: %s\n", inet_ntoa(ds.ipaddr));
    }

#endif /* CONFIG_EXAMPLES_XMLRPC_DHCPC */
#endif /* CONFIG_NSH_NETINIT */

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: xmlrpc_main
 *
 * Description:
 *   The embedded HTTP server main
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int listenfd, connfd, on = 1;
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;

  if (xmlrpc_netinit() < 0)
    {
      nerr("ERROR: Could not initialize the network interface\n");
      return ERROR;
    }

  /* Register RPC functions. */

  calls_register();

  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  memset((void *)&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(80);

  bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  listen(listenfd, 5);

  for (;;)
    {
      clilen = sizeof(cliaddr);
      connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
      if (connfd <= 0)
        {
          break;
        }
      ninfo("Connection accepted: %d\n", connfd);

      xmlrpc_handler(connfd);
      close(connfd);
      ninfo("[%d] connection closed\n", connfd);
    }

  close(listenfd);
  return (0);
}
