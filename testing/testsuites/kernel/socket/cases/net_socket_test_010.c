/****************************************************************************
 * apps/testing/testsuites/kernel/socket/cases/net_socket_test_010.c
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
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <inttypes.h>
#include <fcntl.h>

#include "SocketTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IFF_PROMISC 0x100

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static char gdefaultnetif[IFNAMSIZ] = "eth0";

static void initifreq(struct ifreq *ifr)
{
  *ifr = (struct ifreq)
    {
        {
          0
        }
    };

  /* (void)strncpy_s(ifr->ifr_name, sizeof(ifr->ifr_name) - 1, */

  /* gdefaultnetif, sizeof(ifr->ifr_name) - 1); */

  (void)strlcpy(ifr->ifr_name, gdefaultnetif, sizeof(ifr->ifr_name));
  ifr->ifr_name[sizeof(ifr->ifr_name) - 1] = '\0';
}

static char *ifindex2name(int fd, unsigned index, char *name)
{
#ifdef CONFIG_NETDEV_IFINDEX
  return if_indextoname(index, name);
#else
  struct ifreq ifr;
  int ret;

  ifr.ifr_ifindex = index;
  ret = ioctl(fd, siocgifname, &ifr);
  if (ret < 0)
    {
      return NULL;
    }

  /* ret = strncpy_s(name, IF_NAMESIZE - 1, ifr.ifr_name, IF_NAMESIZE -
   * 1);
   */

  strncpy(name, ifr.ifr_name, IF_NAMESIZE - 1);
  name[IF_NAMESIZE - 1] = '\0';
  return name;
#endif
}

static unsigned ifname2index(int fd, const char *name)
{
#ifdef CONFIG_NETDEV_IFINDEX
  return if_nametoindex(name);
#else
  struct ifreq ifr;
  int ret;

  /* (void)strncpy_s(ifr.ifr_name, sizeof(ifr.ifr_name) - 1, name,
   * sizeof(ifr.ifr_name) - 1);
   */

  (void)strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
  ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
  ret = ioctl(fd, SIOCGIFINDEX, &ifr);
  return ret < 0 ? 0 : ifr.ifr_ifindex;
#endif
}

/****************************************************************************
 * Name: test_nuttx_net_socket_test10
 ****************************************************************************/

static int ioctltestinternal(int sfd)
{
  struct ifreq ifr =
  {
    {
      0
    }
  };

  char ifname[IFNAMSIZ] =
  {
    0
  };

  char *p = NULL;
  unsigned int loindex = 0;
  unsigned int lanindex = 0;
  char lanname[IFNAMSIZ];
  int maxindex = 256;
  int ret;
  char *ret_s = NULL;

  for (int i = 0; i < maxindex; ++i)
    {
      p = ifindex2name(sfd, i, ifname);
      if (p)
        {
          if (strcmp(p, "lo") == 0)
            {
              loindex = i;
            }

          else
            {
              lanindex = i;
              strcpy(lanname, p);
              syslog(LOG_INFO, "name of lan:  %s", lanname);
            }
        }
    }

  syslog(LOG_INFO, "ifindex of lo: %u, ifindex of lan: %u", loindex,
         lanindex);
  assert_int_not_equal(loindex, 0);
  assert_int_not_equal(lanindex, 0);

  p = ifindex2name(sfd, loindex, ifname);
  syslog(LOG_INFO, "ifindex %u: %s", loindex, p);
  assert_non_null(p);
  assert_string_equal(p, "lo");

  p = ifindex2name(sfd, lanindex, ifname);
  syslog(LOG_INFO, "ifindex %u: %s", lanindex, p);
  assert_non_null(p);
  assert_string_equal(p, lanname);

  /* ret = strncpy_s(gdefaultnetif, sizeof(gdefaultnetif) -1, p,
   * sizeof(gdefaultnetif) - 1);
   */

  ret_s = strncpy(gdefaultnetif, p, sizeof(gdefaultnetif) - 1);
  assert_non_null(ret_s);
  gdefaultnetif[sizeof(gdefaultnetif) - 1] = '\0';

  ret = (int)ifname2index(sfd, p);
  syslog(LOG_INFO, "index of %s: %d", p, ret);
  assert_int_not_equal(ret, 0);
  assert_int_equal(ret, lanindex);

  ifr.ifr_ifindex = lanindex;
  ret = ioctl(sfd, siocgifname, &ifr);
  assert_int_equal(ret, 0);
  syslog(LOG_INFO, "name of ifindex %u: %s", lanindex, ifr.ifr_name);
  assert_string_equal(ifr.ifr_name, lanname);

  initifreq(&ifr);
  ret = ioctl(sfd, SIOCGIFINDEX, &ifr);
  assert_int_equal(ret, 0);
  syslog(LOG_INFO, "index of ifname %s: %d", ifr.ifr_name,
         ifr.ifr_ifindex);
  assert_int_equal(ifr.ifr_ifindex, lanindex);

  initifreq(&ifr);
  ret = ioctl(sfd, SIOCGIFHWADDR, &ifr);
  assert_int_equal(ret, 0);
  syslog(LOG_INFO, "hwaddr: %02hhX:%02hhX:%02hhX:XX:XX:XX",
         ifr.ifr_hwaddr.sa_data[0], ifr.ifr_hwaddr.sa_data[1],
         ifr.ifr_hwaddr.sa_data[2]);

  initifreq(&ifr);
  ret = ioctl(sfd, SIOCGIFFLAGS, &ifr);
  assert_int_equal(ret, 0);
  syslog(LOG_INFO, "FLAGS of ifname %s: %#" PRIx32 ", IFF_PROMISC: %d",
         ifr.ifr_name, ifr.ifr_flags, !!(ifr.ifr_flags & IFF_PROMISC));

  if (ifr.ifr_flags & IFF_PROMISC)
    {
      ifr.ifr_flags &= ~(IFF_PROMISC);
    }

  else
    {
      ifr.ifr_flags |= IFF_PROMISC;
    }

  syslog(LOG_INFO, "SIOCSIFFLAGS FLAGS: %#" PRIx32, ifr.ifr_flags);
  ret = ioctl(sfd, SIOCSIFFLAGS, &ifr);
  if (ret == -1)
    {
      assert_int_equal(errno, EPERM);
    }

  else
    {
      assert_int_equal(ret, 0);
    }

  initifreq(&ifr);
  ret = ioctl(sfd, SIOCGIFFLAGS, &ifr);
  assert_int_equal(ret, 0);
  syslog(LOG_INFO, "FLAGS of ifname %s: %#" PRIx32 ", IFF_PROMISC: %d",
         ifr.ifr_name, ifr.ifr_flags, !!(ifr.ifr_flags & IFF_PROMISC));

  ret = fcntl(sfd, F_GETFL, 0);
  assert_int_equal(ret < 0, 0);

  ret = fcntl(sfd, F_SETFL, ret | O_NONBLOCK);
  assert_int_equal(ret < 0, 0);

  return 0;
}

void test_nuttx_net_socket_test10(FAR void **state)
{
  int sfd;

  sfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  syslog(LOG_INFO, "socket(PF_INET, SOCK_STREAM, IPPROTO_TCP): %d", sfd);
  assert_int_not_equal(sfd, -1);

  (void)ioctltestinternal(sfd);

  (void)close(sfd);
}
