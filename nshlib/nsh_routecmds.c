/****************************************************************************
 * apps/nshlib/nsh_routecmds.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
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

#include <apps/netutils/netlib.h>

#include "nsh.h"
#include "nsh_console.h"

#if defined(CONFIG_NET) && defined(CONFIG_NET_ROUTE)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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
#ifdef CONFIG_NET_IPv6
  struct sockaddr_in6 target;
  struct sockaddr_in6 netmask;
  struct sockaddr_in6 router;
  struct in6_addr inaddr;
#else
  struct sockaddr_in target;
  struct sockaddr_in netmask;
  struct sockaddr_in router;
  struct in_addr inaddr;
#endif
  int sockfd;
  int ret;

  /* NSH has already verified that there are exactly three arguments, so
   * we don't have to look at the argument list.
   */

  /* We need to have a socket (any socket) in order to perform the ioctl */

  sockfd = socket(PF_INET, NETLIB_SOCK_IOCTL, 0);
  if (sockfd < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "socket", NSH_ERRNO);
      return ERROR;
    }

  /* Convert the target IP address string into its binary form */

#ifdef CONFIG_NET_IPv6
  ret = inet_pton(AF_INET6, argv[1], &inaddr);
#else
  ret = inet_pton(AF_INET, argv[1], &inaddr);
#endif
  if (ret != 1)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      goto errout;
    }

  /* Format the target sockaddr instance */

  memset(&target, 0, sizeof(target));
#ifdef CONFIG_NET_IPv6
  target.sin6_family = AF_INET6;
  memcpy(&target.sin6_addr, &inaddr, sizeof(struct in6_addr));
#else
  target.sin_family = AF_INET;
  target.sin_addr   = inaddr;
#endif

   /* Convert the netmask IP address string into its binary form */

#ifdef CONFIG_NET_IPv6
  ret = inet_pton(AF_INET6, argv[2], &inaddr);
#else
  ret = inet_pton(AF_INET, argv[2], &inaddr);
#endif
  if (ret != 1)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      goto errout;
    }

  /* Format the netmask sockaddr instance */

  memset(&netmask, 0, sizeof(netmask));
#ifdef CONFIG_NET_IPv6
  netmask.sin6_family = AF_INET6;
  memcpy(&netmask.sin6_addr, &inaddr, sizeof(struct in6_addr));
#else
  netmask.sin_family  = AF_INET;
  netmask.sin_addr    = inaddr;
#endif

   /* Convert the router IP address string into its binary form */

#ifdef CONFIG_NET_IPv6
  ret = inet_pton(AF_INET6, argv[3], &inaddr);
#else
  ret = inet_pton(AF_INET, argv[3], &inaddr);
#endif
  if (ret != 1)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      goto errout;
    }

  /* Format the router sockaddr instance */

  memset(&router, 0, sizeof(router));
#ifdef CONFIG_NET_IPv6
  router.sin6_family = AF_INET6;
  memcpy(&router.sin6_addr, &inaddr, sizeof(struct in6_addr));
#else
  router.sin_family  = AF_INET;
  router.sin_addr    = inaddr;
#endif

  /* Then add the route */

  ret = addroute(sockfd,
                 (FAR struct sockaddr_storage *)&target,
                 (FAR struct sockaddr_storage *)&netmask,
                 (FAR struct sockaddr_storage *)&router);
  if (ret < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "addroute", NSH_ERRNO);
      goto errout;
    }

  close(sockfd);
  return OK;

errout:
  close(sockfd);
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
#ifdef CONFIG_NET_IPv6
  struct sockaddr_in6 target;
  struct sockaddr_in6 netmask;
  struct in6_addr inaddr;
#else
  struct sockaddr_in target;
  struct sockaddr_in netmask;
  struct in_addr inaddr;
#endif
  int sockfd;
  int ret;

  /* NSH has already verified that there are exactly two arguments, so
   * we don't have to look at the argument list.
   */

  /* We need to have a socket (any socket) in order to perform the ioctl */

  sockfd = socket(PF_INET, NETLIB_SOCK_IOCTL, 0);
  if (sockfd < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "socket", NSH_ERRNO);
      return ERROR;
    }

  /* Convert the target IP address string into its binary form */

#ifdef CONFIG_NET_IPv6
  ret = inet_pton(AF_INET6, argv[1], &inaddr);
#else
  ret = inet_pton(AF_INET, argv[1], &inaddr);
#endif
  if (ret != 1)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      goto errout;
    }

  /* Format the target sockaddr instance */

  memset(&target, 0, sizeof(target));
#ifdef CONFIG_NET_IPv6
  target.sin6_family = AF_INET6;
  memcpy(&target.sin6_addr, &inaddr, sizeof(struct in6_addr));
#else
  target.sin_family  = AF_INET;
  target.sin_addr    = inaddr;
#endif

   /* Convert the netmask IP address string into its binary form */

#ifdef CONFIG_NET_IPv6
  ret = inet_pton(AF_INET6, argv[2], &inaddr);
#else
  ret = inet_pton(AF_INET, argv[2], &inaddr);
#endif
  if (ret != 1)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      goto errout;
    }

  /* Format the netmask sockaddr instance */

  memset(&netmask, 0, sizeof(netmask));
#ifdef CONFIG_NET_IPv6
  netmask.sin6_family = AF_INET6;
  memcpy(&netmask.sin6_addr, &inaddr, sizeof(struct in6_addr));
#else
  netmask.sin_family  = AF_INET;
  netmask.sin_addr    = inaddr;
#endif

  /* Then delete the route */

  ret = delroute(sockfd,
                 (FAR struct sockaddr_storage *)&target,
                 (FAR struct sockaddr_storage *)&netmask);
  if (ret < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "addroute", NSH_ERRNO);
      goto errout;
    }

  close(sockfd);
  return OK;

errout:
  close(sockfd);
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
