/****************************************************************************
 * apps/system/tcpdump/tcpdump.c
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

#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netpacket/packet.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <nuttx/net/netconfig.h>

#include "argtable3.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TCPDUMP_MAGIC 0xa1b23c4d /* nanosecond-resolution */

#define TCPDUMP_VERSION_MAJOR 2
#define TCPDUMP_VERSION_MINOR 4

#define DEFAULT_SNAPLEN 262144

/* https://www.tcpdump.org/linktypes.html */

#define LINKTYPE_ETHERNET 1   /* IEEE 802.3 Ethernet */
#define LINKTYPE_RAW      101 /* Raw IP */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pcap_filehdr_s
{
  uint32_t magic;          /* magic number */
  uint16_t version_major;  /* major version number */
  uint16_t version_minor;  /* minor version number */
  int32_t  thiszone;       /* GMT to local correction; this is always 0 */
  uint32_t sigfigs;        /* accuracy of timestamps; this is always 0 */
  uint32_t snaplen;        /* max length saved portion of each pkt */
  uint32_t linktype;       /* data link type (LINKTYPE_*) */
};

struct pcap_pkthdr_s
{
  uint32_t ts_sec;  /* timestamp seconds */
  uint32_t ts_nsec; /* timestamp nanoseconds */
  uint32_t caplen;  /* length of portion present */
  uint32_t len;     /* length of this packet (off wire) */
};

struct tcpdump_args_s
{
  FAR struct arg_str *interface;
  FAR struct arg_str *file;
  FAR struct arg_int *snaplen;
  FAR struct arg_end *end;
};

struct tcpdump_cfgs_s
{
  int fd;
  int sd;
  uint32_t snaplen;
  uint32_t linktype;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile bool g_exiting;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sigexit
 ****************************************************************************/

static void sigexit(int signo)
{
  g_exiting = true;
}

/****************************************************************************
 * Name: write_filehdr
 ****************************************************************************/

static int write_filehdr(int fd, uint32_t snaplen, uint32_t linktype)
{
  /* No need to change byte order of any field, reader will swap all fields
   * if magic number is in swapped order.
   */

  struct pcap_filehdr_s hdr =
    {
      TCPDUMP_MAGIC,         /* magic */
      TCPDUMP_VERSION_MAJOR, /* version_major */
      TCPDUMP_VERSION_MINOR, /* version_minor */
      0,                     /* thiszone */
      0,                     /* sigfigs */
      snaplen,               /* snaplen */
      linktype               /* linktype */
    };

  /* Write hdr into file. */

  if (write(fd, &hdr, sizeof(hdr)) < 0)
    {
      perror("ERROR: write() failed");
      return -errno;
    }

  return OK;
}

/****************************************************************************
 * Name: write_packet
 ****************************************************************************/

static int write_packet(int fd, uint32_t snaplen, uint32_t pkt_len,
                        FAR const void *buf, FAR const struct timespec *ts)
{
  struct pcap_pkthdr_s hdr =
    {
      ts->tv_sec,            /* ts_sec */
      ts->tv_nsec,           /* ts_nsec */
      MIN(snaplen, pkt_len), /* caplen */
      pkt_len                /* len */
    };

  /* Write hdr into file. */

  if (write(fd, &hdr, sizeof(hdr)) < 0)
    {
      perror("ERROR: write() failed");
      return -errno;
    }

  /* Write pkt into file. */

  if (write(fd, buf, hdr.caplen) < 0)
    {
      perror("ERROR: write() failed");
      return -errno;
    }

  return OK;
}

/****************************************************************************
 * Name: socket_open
 ****************************************************************************/

static int socket_open(int ifindex)
{
  int sd;
  struct sockaddr_ll addr;

  sd = socket(PF_PACKET, SOCK_RAW, 0);
  if (sd < 0)
    {
      perror("ERROR: failed to create packet socket");
      return -errno;
    }

  /* Prepare sockaddr struct */

  addr.sll_family = AF_PACKET;
  addr.sll_ifindex = ifindex;
  if (bind(sd, (FAR const struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      perror("ERROR: binding socket failed");
      close(sd);
      return -errno;
    }

  return sd;
}

/****************************************************************************
 * Name: get_linktype
 ****************************************************************************/

static uint32_t get_linktype(FAR const char *ifname)
{
  struct ifreq req;
  uint32_t ret = LINKTYPE_RAW;
  int sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
  if (sockfd >= 0)
    {
      strlcpy(req.ifr_name, ifname, IFNAMSIZ);
      if (ioctl(sockfd, SIOCGIFHWADDR, (unsigned long)&req) >= 0 &&
          req.ifr_hwaddr.sa_family == ARPHRD_ETHER)
        {
          ret = LINKTYPE_ETHERNET;
        }

      close(sockfd);
    }

  return ret;
}

/****************************************************************************
 * Name: do_capture
 ****************************************************************************/

static void do_capture(FAR const struct tcpdump_cfgs_s *cfgs)
{
  ssize_t len;
  uint8_t buf[MAX_NETDEV_PKTSIZE];
  struct timespec ts;

  /* Write file header */

  if (write_filehdr(cfgs->fd, cfgs->snaplen, cfgs->linktype) < 0)
    {
      return;
    }

  /* Dump packets */

  while ((len = read(cfgs->sd, buf, sizeof(buf))) >= 0 && !g_exiting)
    {
      if (len == 0)
        {
          continue;
        }

      if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
        {
          perror("ERROR: clock_gettime() failed");
          return;
        }

      if (write_packet(cfgs->fd, cfgs->snaplen, len, buf, &ts) < 0)
        {
          return;
        }
    }

  if (!g_exiting)
    {
      perror("ERROR: read() failed");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ifindex;
  int nerrors;
  struct tcpdump_cfgs_s cfgs;
  struct tcpdump_args_s args;

  g_exiting = false;
  signal(SIGINT, sigexit);

  args.interface = arg_str1("i", "interface", "interface", "Capture device");
  args.file      = arg_str1("w", NULL, "file", "Path to dump file");
  args.snaplen   = arg_int0("s", "snapshot-length", "snaplen",
                            "Max dump length of each packet");
  args.end       = arg_end(3);

  nerrors = arg_parse(argc, argv, (FAR void**)&args);
  if (nerrors != 0)
    {
      arg_print_errors(stdout, args.end, argv[0]);
      printf("Usage:\n");
      arg_print_glossary(stdout, (FAR void**)&args, "  %-30s %s\n");
      goto out;
    }

  ifindex = if_nametoindex(args.interface->sval[0]);
  if (ifindex == 0)
    {
      printf("Failed to get index of device %s\n", args.interface->sval[0]);
      goto out;
    }

  cfgs.fd = open(args.file->sval[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (cfgs.fd < 0)
    {
      perror("ERROR: open() failed");
      goto out;
    }

  cfgs.sd = socket_open(ifindex);
  if (cfgs.sd < 0)
    {
      close(cfgs.fd);
      goto out;
    }

  if (args.snaplen->count > 0)
    {
      cfgs.snaplen = *args.snaplen->ival;
    }
  else
    {
      cfgs.snaplen = DEFAULT_SNAPLEN;
    }

  cfgs.linktype = get_linktype(args.interface->sval[0]);

  do_capture(&cfgs);

  close(cfgs.sd);
  close(cfgs.fd);

out:
  arg_freetable((FAR void **)&args, 1);
  return 0;
}
