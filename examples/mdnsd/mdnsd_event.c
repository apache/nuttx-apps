/****************************************************************************
 * apps/examples/mdnsd/mdnsd_event.c
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
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/if_addr.h>
#include <netpacket/netlink.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "netutils/mdnsd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MDNSD_EVENT_BUFSIZE   512
#define MDNSD_EVENT_SETTLE_MS 500

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mdnsd_event_has_ipv4addr
 *
 * Description:
 *   Check whether any network interface already has a non-zero IPv4
 *   address.  Uses SIOCGIFCONF to enumerate interfaces.
 *
 * Returned Value:
 *   true if at least one interface has a non-zero IPv4 address,
 *   false otherwise.
 *
 ****************************************************************************/

static bool mdnsd_event_has_ipv4addr(void)
{
  struct ifconf ifc;
  char buf[sizeof(struct ifreq) * 4];
  int sd;
  int i;

  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0)
    {
      return false;
    }

  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(sd, SIOCGIFCONF, &ifc) < 0)
    {
      close(sd);
      return false;
    }

  close(sd);

  for (i = 0; i < ifc.ifc_len; i += sizeof(struct ifreq))
    {
      FAR struct ifreq *ifr = (FAR struct ifreq *)&buf[i];
      FAR struct sockaddr_in *sin;

      if (ifr->ifr_addr.sa_family != AF_INET)
        {
          continue;
        }

      sin = (FAR struct sockaddr_in *)&ifr->ifr_addr;
      if (sin->sin_addr.s_addr != INADDR_ANY &&
          sin->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
        {
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: mdnsd_event_is_newaddr
 *
 * Description:
 *   Checks whether the given Netlink message header represents a new
 *   IPv4 or IPv6 address event (RTM_NEWADDR).
 *
 * Input Parameters:
 *   hdr - Pointer to the Netlink message header to inspect.
 *
 * Returned Value:
 *   true if the message is a new IPv4 or IPv6 address notification,
 *   false otherwise.
 *
 ****************************************************************************/

static bool mdnsd_event_is_newaddr(FAR struct nlmsghdr *hdr)
{
  FAR struct ifaddrmsg *addr;

  if (hdr->nlmsg_type != RTM_NEWADDR)
    {
      return false;
    }

  if (hdr->nlmsg_len < NLMSG_LENGTH(sizeof(struct ifaddrmsg)))
    {
      return false;
    }

  addr = (FAR struct ifaddrmsg *)NLMSG_DATA(hdr);
  return addr->ifa_family == AF_INET || addr->ifa_family == AF_INET6;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Entry point for the mdnsd_event application.  Listens for Netlink
 *   address events and starts the mDNS daemon when an IP address is
 *   assigned to a network interface.
 *
 * Input Parameters:
 *   argc - Number of command-line arguments.
 *   argv - Array of command-line argument strings.
 *
 * Returned Value:
 *   EXIT_SUCCESS on success, EXIT_FAILURE on error.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_NETLINK_ROUTE
  struct sockaddr_nl addr;
  uint8_t buffer[MDNSD_EVENT_BUFSIZE];
  int sd;
  int ret;

  /* If an interface already has an IP (DHCP finished before we started),
   * skip the netlink wait and start mDNS right away.
   */

  if (mdnsd_event_has_ipv4addr())
    {
      printf("mdnsd_event: address already set, starting mDNS\n");
      usleep(MDNSD_EVENT_SETTLE_MS * 1000);
      ret = mdnsd_start(CONFIG_EXAMPLES_MDNS_SERVICE,
                         CONFIG_EXAMPLES_MDNS_SERVICE_PORT);
      return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
    }

  sd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sd < 0)
    {
      fprintf(stderr, "mdnsd_event: socket failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;
  addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

  ret = bind(sd, (FAR struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
    {
      fprintf(stderr, "mdnsd_event: bind failed: %d\n", errno);
      close(sd);
      return EXIT_FAILURE;
    }

  printf("mdnsd_event: waiting for network address event\n");

  for (; ; )
    {
      FAR struct nlmsghdr *hdr;
      ssize_t nread;
      int remaining;

      nread = recv(sd, buffer, sizeof(buffer), 0);
      if (nread < 0)
        {
          fprintf(stderr, "mdnsd_event: recv failed: %d\n", errno);
          close(sd);
          return EXIT_FAILURE;
        }

      remaining = nread;
      for (hdr = (FAR struct nlmsghdr *)buffer;
           NLMSG_OK(hdr, remaining);
           hdr = NLMSG_NEXT(hdr, remaining))
        {
          if (mdnsd_event_is_newaddr(hdr))
            {
              printf("mdnsd_event: address set, starting mDNS\n");
              usleep(MDNSD_EVENT_SETTLE_MS * 1000);
              ret = mdnsd_start(CONFIG_EXAMPLES_MDNS_SERVICE,
                                CONFIG_EXAMPLES_MDNS_SERVICE_PORT);
              close(sd);
              return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
            }
        }
    }
#else
  fprintf(stderr, "mdnsd_event: CONFIG_NETLINK_ROUTE is required\n");
  return EXIT_FAILURE;
#endif
}
