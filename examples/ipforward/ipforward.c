/****************************************************************************
 * examplex/ipforward/ipforward.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include <net/if.h>
#include <arpa/inet.h>

#include <nuttx/net/tun.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_DEVNAME 8

#ifdef CONFIG_NET_IPv6
#  define IPADDR_TYPE FAR const uint16_t *
#else
#  define IPADDR_TYPE uint32_t
#endif

/****************************************************************************
 * Name: Private Types
 ****************************************************************************/

struct ipfwd_tun_s
{
  int  it_fd;
  char it_devname[MAX_DEVNAME];
};

struct ipfwd_state_s
{
  struct ipfwd_tun_s if_tun0;
  struct ipfwd_tun_s if_tun1;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
static const uint16_t g_tun0_ipaddr[8] =
{
  HTONS(0x7c00),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0x0097),
};

static const uint16_t g_tun1_ipaddr[8] =
{
  HTONS(0x7c00),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0x0139),
};

static const uint16_t g_netmask[8] =
{
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0),
};
#else
static const uint32_t g_tun0_ipaddr = HTONL(0x0a000097);
static const uint32_t g_tun1_ipaddr = HTONL(0x0a000039);
static const uint32_t g_netmask     = HTONL(0xffffff00);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipfwd_tun_configure
 ****************************************************************************/

static int ipfwd_tun_configure(FAR struct ipfwd_tun_s *tun)
{
  struct ifreq ifr;
  int errcode;
  int ret;

  tun->it_fd = open("/dev/tun", O_RDWR);
  if (tun->it_fd < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Failed to open /dev/tun: %d\n", errcode);
      return -errcode;
    }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN;

  ret = ioctl(tun->it_fd, TUNSETIFF, (unsigned long)&ifr);
  if (ret < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: ioctl TUNSETIFF failed: %d\n", errcode);
      close(tun->it_fd);
      return -errcode;
    }

  strncpy(tun->it_devname, ifr.ifr_name, MAX_DEVNAME);
  printf("Created TUN device: %s\n", tun->it_devname);
  return 0;
}

/****************************************************************************
 * Name: ipfwd_netconfig
 ****************************************************************************/

static int ipfwd_netconfig(FAR struct ipfwd_tun_s *tun, IPADDR_TYPE ipaddr,
                           IPADDR_TYPE netmask)
{
  int ret;

#ifdef CONFIG_NET_IPv6

  struct in6_addr addr;

  memcpy(addr.s6_addr16, ipaddr, 8 * sizeof(uint16_t));
  ret = netlib_set_ipv6addr(tun->it_devname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: netlib_set_ipv6addr() failed\n", ret);
      return ret;
    }

  memcpy(addr.s6_addr16, netmask, 8 * sizeof(uint16_t));
  ret = netlib_set_ipv6netmask(tun->it_devname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: netlib_set_ipv6netmask() failed\n", ret);
      return ret;
    }

#else /* CONFIG_NET_IPv4 */

  struct in_addr addr;

  addr.s_addr = ipaddr;
  ret = netlib_set_ipv4addr(tun->it_devname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: netlib_set_ipv4addr() failed\n", ret);
      return ret;
    }

  addr.s_addr = netmask;
  ret = netlib_set_ipv4netmask(tun->it_devname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: netlib_set_ipv4netmask() failed\n", ret);
      return ret;
    }
#endif

  netlib_ifup(tun->it_devname);
  return 0;
}

/****************************************************************************
 * Name: ipfwd_receiver
 ****************************************************************************/

/****************************************************************************
 * Name: ipfwd_sender
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fstest_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int ipfwd_main(int argc, char *argv[])
#endif
{
  struct ipfwd_state_s fwd;
  int errcode = EXIT_SUCCESS;
  int ret;

  /* Initialize the first TUN device */

  ret = ipfwd_tun_configure(&fwd.if_tun0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to create tun0: %d\n", ret);
      goto errout;
    }

  ret = ipfwd_netconfig(&fwd.if_tun0, g_tun0_ipaddr, g_netmask);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ipfwd_netconfig failed: %d\n", ret);
      goto errout_with_tun0;
    }

  /* Initialize the second TUN device */

  ret = ipfwd_tun_configure(&fwd.if_tun1);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to create tun1: %d\n", ret);
      goto errout_with_tun0;
    }

  ret = ipfwd_netconfig(&fwd.if_tun0, g_tun0_ipaddr, g_netmask);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ipfwd_netconfig failed: %d\n", ret);
      errcode = EXIT_FAILURE;
      goto errout_with_tun1;
    }

  /* Start receiver thread */

  /* Start sender thread */

  /* Wait for sender thread to terminate */

errout_with_receiver:
  /* Wait for receiver thread to terminate */

errout_with_tun1:
  close(fwd.if_tun1.it_fd);
errout_with_tun0:
  close(fwd.if_tun0.it_fd);
errout:
  return errcode;
}
