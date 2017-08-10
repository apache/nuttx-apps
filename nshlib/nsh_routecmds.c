/****************************************************************************
 * apps/nshlib/nsh_routecmds.c
 *
 *   Copyright (C) 2013, 2017 Gregory Nutt. All rights reserved.
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

#include <stdlib.h>
#include <string.h>
#include <net/route.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "netutils/netlib.h"

#include "nsh.h"
#include "nsh_console.h"

#if defined(CONFIG_NET) && defined(CONFIG_NET_ROUTE)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_addroute
 *
 * nsh> addroute <target> <netmask> <router>
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_ADDROUTE
int cmd_addroute(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  union
  {
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
  } target;

  union
  {
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
  } netmask;

  union
  {
#ifdef CONFIG_NET_IPv4
  struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
  struct sockaddr_in6 ipv6;
#endif
  } router;

  union
  {
#ifdef CONFIG_NET_IPv4
    struct in_addr ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct in6_addr ipv6;
#endif
  } inaddr;

  sa_family_t family;
  int sockfd;
  int ret;

  /* NSH has already verified that there are exactly three arguments, so
   * we don't have to look at the argument list.
   */

  /* Convert the target IP address string into its binary form */

#ifdef CONFIG_NET_IPv4
  family = PF_INET;

  ret = inet_pton(AF_INET, argv[1], &inaddr.ipv4);
  if (ret != 1)
#endif
    {
#ifdef CONFIG_NET_IPv6
      family = PF_INET6;

      ret = inet_pton(AF_INET6, argv[1], &inaddr.ipv6);
      if (ret != 1)
#endif
        {
          nsh_output(vtbl, g_fmtarginvalid, argv[0]);
          goto errout;
        }
    }

  /* We need to have a socket (any socket) in order to perform the ioctl */

  sockfd = socket(family, NETLIB_SOCK_IOCTL, 0);
  if (sockfd < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "socket", NSH_ERRNO);
      goto errout;
    }

  /* Format the target sockaddr instance */

  memset(&target, 0, sizeof(target));

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  if (family == PF_INET)
#endif
    {
      target.ipv4.sin_family  = AF_INET;
      target.ipv4.sin_addr    = inaddr.ipv4;
    }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  else
#endif
    {
      target.ipv6.sin6_family = AF_INET6;
      memcpy(&target.ipv6.sin6_addr, &inaddr.ipv6,
             sizeof(struct in6_addr));
    }
#endif

   /* Convert the netmask IP address string into its binary form */

  ret = inet_pton(family, argv[2], &inaddr);
  if (ret != 1)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      goto errout_with_sockfd;
    }

  /* Format the netmask sockaddr instance */

  memset(&netmask, 0, sizeof(netmask));

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  if (family == PF_INET)
#endif
    {
      netmask.ipv4.sin_family  = AF_INET;
      netmask.ipv4.sin_addr    = inaddr.ipv4;
    }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  else
#endif
    {
      netmask.ipv6.sin6_family = AF_INET6;
      memcpy(&netmask.ipv6.sin6_addr, &inaddr.ipv6,
             sizeof(struct in6_addr));
    }
#endif

   /* Convert the router IP address string into its binary form */

  ret = inet_pton(family, argv[3], &inaddr);
  if (ret != 1)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      goto errout_with_sockfd;
    }

  /* Format the router sockaddr instance */

  memset(&router, 0, sizeof(router));

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  if (family == PF_INET)
#endif
    {
      router.ipv4.sin_family  = AF_INET;
      router.ipv4.sin_addr    = inaddr.ipv4;
    }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  else
#endif
    {
      router.ipv6.sin6_family = AF_INET6;
      memcpy(&router.ipv6.sin6_addr, &inaddr.ipv6,
             sizeof(struct in6_addr));
    }
#endif

  /* Then add the route */

  ret = addroute(sockfd,
                 (FAR struct sockaddr_storage *)&target,
                 (FAR struct sockaddr_storage *)&netmask,
                 (FAR struct sockaddr_storage *)&router);
  if (ret < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "addroute", NSH_ERRNO);
      goto errout_with_sockfd;
    }

  close(sockfd);
  return OK;

errout_with_sockfd:
  close(sockfd);
errout:
  return ERROR;
}
#endif /* !CONFIG_NSH_DISABLE_ADDROUTE */

/****************************************************************************
 * Name: cmd_delroute
 *
 * nsh> delroute <target> <netmask>
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_DELROUTE
int cmd_delroute(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  union
  {
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
  } target;

  union
  {
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
  } netmask;

  union
  {
#ifdef CONFIG_NET_IPv4
    struct in_addr ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct in6_addr ipv6;
#endif
  } inaddr;

  sa_family_t family;
  int sockfd;
  int ret;

  /* NSH has already verified that there are exactly two arguments, so
   * we don't have to look at the argument list.
   */

  /* Convert the target IP address string into its binary form */

#ifdef CONFIG_NET_IPv4
  family = PF_INET;

  ret = inet_pton(AF_INET, argv[1], &inaddr.ipv4);
  if (ret != 1)
#endif
    {
#ifdef CONFIG_NET_IPv6
      family = PF_INET6;

      ret = inet_pton(AF_INET6, argv[1], &inaddr.ipv6);
      if (ret != 1)
#endif
        {
          nsh_output(vtbl, g_fmtarginvalid, argv[0]);
          goto errout;
        }
    }

  /* We need to have a socket (any socket) in order to perform the ioctl */

  sockfd = socket(family, NETLIB_SOCK_IOCTL, 0);
  if (sockfd < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "socket", NSH_ERRNO);
      goto errout;
    }

  /* Format the target sockaddr instance */

  memset(&target, 0, sizeof(target));

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  if (family == PF_INET)
#endif
    {
      target.ipv4.sin_family  = AF_INET;
      target.ipv4.sin_addr    = inaddr.ipv4;
    }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  else
#endif
    {
      target.ipv6.sin6_family = AF_INET6;
      memcpy(&target.ipv6.sin6_addr, &inaddr.ipv6,
             sizeof(struct in6_addr));
    }
#endif

   /* Convert the netmask IP address string into its binary form */

  ret = inet_pton(family, argv[2], &inaddr);
  if (ret != 1)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      goto errout_with_sockfd;
    }

  /* Format the netmask sockaddr instance */

  memset(&netmask, 0, sizeof(netmask));

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  if (family == PF_INET)
#endif
    {
      netmask.ipv4.sin_family  = AF_INET;
      netmask.ipv4.sin_addr    = inaddr.ipv4;
    }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  else
#endif
    {
      netmask.ipv6.sin6_family = AF_INET6;
      memcpy(&netmask.ipv6.sin6_addr, &inaddr.ipv6,
             sizeof(struct in6_addr));
    }
#endif

  /* Then delete the route */

  ret = delroute(sockfd,
                 (FAR struct sockaddr_storage *)&target,
                 (FAR struct sockaddr_storage *)&netmask);
  if (ret < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "addroute", NSH_ERRNO);
      goto errout_with_sockfd;
    }

  close(sockfd);
  return OK;

errout_with_sockfd:
  close(sockfd);
errout:
  return ERROR;
}
#endif /* !CONFIG_NSH_DISABLE_DELROUTE */

/****************************************************************************
 * Name: cmd_route
 *
 * nsh> route
 *
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_DELROUTE) && \
    !defined(CONFIG_BUILD_PROTECTED) && \
    !defined(CONFIG_BUILD_KERNEL)

/* Perhaps... someday.  This would current depend on using the internal
 * OS interface net_foreachroute and internal OS data structures defined
 * in nuttx/net/net_route.h
 */
#endif /* !CONFIG_NSH_DISABLE_DELROUTE && !CONFIG_BUILD_PROTECTED && !CONFIG_BUILD_KERNEL */

#endif /* CONFIG_NET && CONFIG_NET_ROUTE */
