/****************************************************************************
 * examples/netlink_route/netlink_route_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <sys/types.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <sched.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <net/ethernet.h>
#include <netinet/in.h>

#include <nuttx/net/arp.h>
#include <nuttx/net/neighbor.h>

#include "netutils/netlib.h"

#ifdef CONFIG_NETLINK_ROUTE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_DEVICES 16
#define MAX_ROUTES  64

#ifdef CONFIG_NET_IPv6
#  define ROUTE_BUFSIZE INET6_ADDRSTRLEN
#else
#  define ROUTE_BUFSIZE INET_ADDRSTRLEN
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifndef CONFIG_NETLINK_DISABLE_GETLINK
static void dump_devices(sa_family_t family)
{
  struct netlib_device_s devlist[MAX_DEVICES];
  ssize_t nentries;
  int i;

  printf("\nDevice List");

  /* Read the device list */

  nentries = netlib_get_devices(devlist, MAX_DEVICES, family);
  if (nentries < 0)
    {
      fprintf(stderr, "\nERROR: netlib_get_devices failed: %d\n",
              (int)nentries);
      return;
    }

  printf(" (Entries: %ld)\n", (long)nentries);

  /* Dump the device list */

  for (i = 0; i < nentries; i++)
    {
      FAR struct netlib_device_s *dev = &devlist[i];

#ifdef CONFIG_NETDEV_IFINDEX
      printf("  Index: %2d  ", dev->ifindex);
#else
      printf("  ");
#endif
      printf("Name: \"%s\"\n", dev->ifname);
    }
}
#else
#  define dump_devices(f)
#endif

#if defined(CONFIG_NET_IPv6) && !defined(CONFIG_NETLINK_DISABLE_GETNEIGH)
static void dump_neighbor(void)
{
  FAR struct neighbor_entry_s *nbtab;
  FAR char buffer[INET6_ADDRSTRLEN];
  size_t allocsize;
  ssize_t nentries;
  int i;
  int j;

  printf("\nIPv6 Neighbor Table");

  /* Allocate a buffer to hold the neighbor table */

  allocsize = CONFIG_NET_IPv6_NCONF_ENTRIES *
              sizeof(struct neighbor_entry_s);
  nbtab = (FAR struct neighbor_entry_s *)malloc(allocsize);
  if (nbtab == NULL)
    {
      fprintf(stderr, "\nERROR: Failed to allocate neighbor table\n");
      return;
    }

  /* Get the neighbor table */

  nentries = netlib_get_nbtable(nbtab, CONFIG_NET_IPv6_NCONF_ENTRIES);
  if (nentries < 0)
    {
      fprintf(stderr, "\nERROR: netlib_get_nbtable failed: %d\n",
              (int)nentries);
      return;
    }

  printf(" (Entries: %ld)\n", (long)nentries);

  /* Show each neighbor */

  for (i = 0; i < nentries; i++)
    {
      FAR struct neighbor_entry_s *nb = &nbtab[i];

      inet_ntop(AF_INET6, &nb->ne_ipaddr, buffer, INET6_ADDRSTRLEN);
      printf("  Dest: %s MAC Type: %2u Size: %3u Addr: ",
             buffer, nb->ne_addr.na_lltype, nb->ne_addr.na_llsize);

      for (j = 0; j < nb->ne_addr.na_llsize; j++)
        {
          if ((j + 1) >= nb->ne_addr.na_llsize)
            {
              printf("%02x", nb->ne_addr.u.na_addr);
            }
          else
            {
              printf("%02x.", nb->ne_addr.u.na_addr);
            }
        }

      for (; j < 7; j++)
        {
          printf("   ");
        }

      if (j < 8)
        {
          printf("  ");
        }

#ifdef CONFIG_SYSTEM_TIME64
      printf("Time 0x%" PRIx64 "\n", nb->ne_time);
#else
      printf("Time 0x%" PRIx32 "\n", nb->ne_time);
#endif
    }

  free(nbtab);
}
#else
#  define dump_neighbor()
#endif

#if defined(CONFIG_NET_ARP) && !defined(CONFIG_NETLINK_DISABLE_GETNEIGH)
static void dump_arp(void)
{
  FAR struct arp_entry_s *arptab;
  FAR char buffer[INET_ADDRSTRLEN];
  size_t allocsize;
  ssize_t nentries;
  int i;
  int j;

  printf("\nIPv4 ARP Table");

  /* Allocate a buffer to hold the ARP table */

  allocsize = CONFIG_NET_ARPTAB_SIZE * sizeof(struct arp_entry_s);
  arptab = (FAR struct arp_entry_s *)malloc(allocsize);
  if (arptab == NULL)
    {
      fprintf(stderr, "\nERROR: Failed to allocate ARP table\n");
      return;
    }

  /* Get the ARP table */

  nentries = netlib_get_arptable(arptab, CONFIG_NET_ARPTAB_SIZE);
  if (nentries < 0)
    {
      fprintf(stderr, "\nERROR: netlib_get_arptable failed: %d\n",
              (int)nentries);
      return;
    }

  printf(" (Entries: %ld)\n", (long)nentries);

  /* Show each ARP table entry */

  for (i = 0; i < nentries; i++)
    {
      FAR struct arp_entry_s *arp = &arptab[i];

      inet_ntop(AF_INET, &arp->at_ipaddr, buffer, INET_ADDRSTRLEN);
      printf("  Dest: %s MAC Addr: ", buffer);

      for (j = 0; j < ETHER_ADDR_LEN; j++)
        {
          if (j == (ETHER_ADDR_LEN - 1))
            {
              printf("%02x", arp->at_ethaddr.ether_addr_octet[j]);
            }
          else
            {
              printf("%02x.", arp->at_ethaddr.ether_addr_octet[j]);
            }
        }

#ifdef CONFIG_SYSTEM_TIME64
      printf(" Time 0x%" PRIx64 "\n", arp->at_time);
#else
      printf(" Time 0x%" PRIx32 "\n", arp->at_time);
#endif
    }

  free(arptab);
}
#else
#  define dump_arp()
#endif

#if defined(CONFIG_NET_ROUTE) && !defined(CONFIG_NETLINK_DISABLE_GETROUTE)
static void dump_route(sa_family_t family)
{
  FAR struct rtentry *rttab;
  FAR char buffer[ROUTE_BUFSIZE];
  size_t allocsize;
  ssize_t nentries;
  int i;

  printf("\nIPv%c Routing Table", family == AF_INET ? '4' : '6');

  /* Allocate a buffer to hold the routing table */

  allocsize = MAX_ROUTES * sizeof(struct rtentry);
  rttab = (FAR struct rtentry *)malloc(allocsize);
  if (rttab == NULL)
    {
      fprintf(stderr, "\nERROR: Failed to allocate routing table\n");
      return;
    }

  /* Get the routing table */

  nentries = netlib_get_route(rttab, MAX_ROUTES, family);
  if (nentries < 0)
    {
      fprintf(stderr, "\nERROR: netlib_get_route failed: %d\n",
              (int)nentries);
      return;
    }

  printf(" (Entries: %ld)\n", (long)nentries);

  /* Show each routing table entry */

  for (i = 0; i < nentries; i++)
    {
      FAR struct rtentry *rte = &rttab[i];

      inet_ntop(rte->rt_dst.ss_family, &rte->rt_dst.ss_data, buffer,
                ROUTE_BUFSIZE);
      printf("  Dest: %s ", buffer);

      inet_ntop(rte->rt_gateway.ss_family, &rte->rt_gateway.ss_data,
                buffer, ROUTE_BUFSIZE);
      printf("Gateway: %s ", buffer);

      inet_ntop(rte->rt_genmask.ss_family, &rte->rt_genmask.ss_data,
                buffer, ROUTE_BUFSIZE);
      printf("GenMASK: %s ", buffer);
      printf("Flags: %04x\n", rte->rt_flags);
    }

  free(rttab);
}
#else
#  define dump_route(f)
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * netlink_route_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  dump_devices(AF_PACKET);
  dump_neighbor();
  dump_arp();
  dump_route(AF_INET);
  dump_route(AF_INET6);

  return EXIT_SUCCESS;
}

#endif /* CONFIG_NETLINK_ROUTE */
