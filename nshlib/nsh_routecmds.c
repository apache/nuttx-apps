/****************************************************************************
 * apps/nshlib/nsh_routecmds.c
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "netutils/netlib.h"

#include "nsh.h"
#include "nsh_console.h"

#if defined(CONFIG_NET) && defined(CONFIG_NET_ROUTE)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int slash_notation(FAR char *arg)
{
  FAR char *sptr;
  FAR char *ptr;

  /* If an address contains a /, then the netmask is imply by the following
   * numeric value.
   */

  sptr = strchr(arg, '/');
  if (sptr != NULL)
    {
      /* Make sure that everything following the slash is a decimal digit. */

      ptr = sptr + 1;
      while (isdigit(*ptr))
        {
          ptr++;
        }

      /* There should be nothing be digits after the slash and up to the
       * NULL terminator.
       */

      if (*ptr == '\0')
        {
          *sptr++ = '\0';
          return atoi(sptr);
        }
    }

  return ERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_addroute
 *
 * nsh> addroute <target> [<netmask>] <router>
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_ADDROUTE
int cmd_addroute(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  union
  {
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
    struct sockaddr_storage ipx;
  } target;

  union
  {
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
    struct sockaddr_storage ipx;
  } netmask;

  union
  {
#ifdef CONFIG_NET_IPv4
  struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
  struct sockaddr_in6 ipv6;
#endif
  struct sockaddr_storage ipx;
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
  int rtrndx;
  int shift;
  int sockfd;
  int ret;
  FAR char *sptr;

  /* First, check if we are setting the default route */

  if (strcmp(argv[1], "default") == 0)
    {
      /* We are expecting an ip and an interface name to follow. */

      if (argc < 4)
        {
          nsh_error(vtbl, g_fmtargrequired, argv[0]);
          goto errout;
        }
      else if (argc > 4)
        {
          nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
          goto errout;
        }

      /* Convert the target IP address string into its binary form */

#ifdef CONFIG_NET_IPv4
      family = PF_INET;

      ret = inet_pton(AF_INET, argv[2], &inaddr.ipv4);
      if (ret != 1)
#endif
        {
#ifdef CONFIG_NET_IPv6
          family = PF_INET6;

          ret = inet_pton(AF_INET6, argv[2], &inaddr.ipv6);
          if (ret != 1)
#endif
            {
              nsh_error(vtbl, g_fmtarginvalid, argv[0]);
              goto errout;
            }
        }

#ifdef CONFIG_NET_IPv4
      if (family == PF_INET)
        {
          ret = netlib_set_dripv4addr(argv[3], &inaddr.ipv4);
          if (ret != 0)
            {
              nsh_error(vtbl, g_fmtcmdfailed, argv[0]);
              goto errout;
            }
        }
#endif

#ifdef CONFIG_NET_IPv6
      if (family == PF_INET6)
        {
          ret = netlib_set_dripv6addr(argv[3], &inaddr.ipv6);
          if (ret != 0)
            {
              nsh_error(vtbl, g_fmtcmdfailed, argv[0]);
              goto errout;
            }
        }
#endif

      return OK;
    }

  /* Check if slash notation is used with the target address */

  shift = slash_notation(argv[1]);

  /* There will be 2 or 3 arguments, depending on if slash notation is
   * used.
   */

  if (shift > 0 && argc  != 3)
    {
      nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
      goto errout;
    }
  else if (shift < 0 && argc != 4)
    {
      nsh_error(vtbl, g_fmtargrequired, argv[0]);
      goto errout;
    }

  /* We need to remove the slash notation before passing it to inet_pton */

  if (shift > 0)
    {
      sptr = strchr(argv[1], '/');
      if (sptr != NULL)
        {
          *sptr = '\0';
        }
    }

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
          nsh_error(vtbl, g_fmtarginvalid, argv[0]);
          goto errout;
        }
    }

  /* We need to have a socket (any socket) in order to perform the ioctl */

  sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
  if (sockfd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "socket", NSH_ERRNO);
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

  if (shift >= 0)
    {
#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
      if (family == PF_INET)
#endif
        {
          /* /0  -> 0x00000000
           * /8  -> 0xff000000
           * /24 -> 0xffffff00
           * /32 -> 0xffffffff
           */

          if (shift > 32)
            {
              nsh_error(vtbl, g_fmtarginvalid, argv[0]);
              goto errout_with_sockfd;
            }

          inaddr.ipv4.s_addr = htonl(0xffffffff << (32 - shift));
        }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
      else
#endif
        {
          int i;

          /* /0   -> 0000:0000:0000:0000:0000:0000:0000:0000
           * /16  -> ffff:0000:0000:0000:0000:0000:0000:0000
           * /32  -> ffff:ffff:0000:0000:0000:0000:0000:0000
           * ...
           * /128 -> ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff
           */

          memset(&inaddr.ipv6, 0, sizeof(inaddr.ipv6));
          for (i = 0; i < 8 && shift >= 16; i++, shift -= 16)
            {
              inaddr.ipv6.s6_addr16[i] = 0xffff;
            }

          if (shift > 16 || (shift > 0 && i >= 8))
            {
              nsh_error(vtbl, g_fmtarginvalid, argv[0]);
              goto errout_with_sockfd;
            }

          /* /0   -> 0x0000
           * /1   -> 0x8000
           * /2   -> 0xc000
           * ...
           * /16  -> 0xffff
           */

          if (shift > 0)
            {
              inaddr.ipv6.s6_addr16[i] = htons(0xffff << (16 - shift));
            }
        }
#endif

      rtrndx = 2;
    }
  else
    {
      /* Slash notation not used.. mask should follow the target address */

      ret = inet_pton(family, argv[2], &inaddr);
      if (ret != 1)
        {
          nsh_error(vtbl, g_fmtarginvalid, argv[0]);
          goto errout_with_sockfd;
        }

      rtrndx = 3;
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

  ret = inet_pton(family, argv[rtrndx], &inaddr);
  if (ret != 1)
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
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

  ret = addroute(sockfd, &target.ipx, &netmask.ipx, &router.ipx);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "addroute", NSH_ERRNO);
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
 * nsh> delroute <target> [<netmask>]
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_DELROUTE
int cmd_delroute(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  union
  {
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
    struct sockaddr_storage ipx;
  } target;

  union
  {
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
    struct sockaddr_storage ipx;
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
  int shift;
  int sockfd;
  int ret;

  /* Check if slash notation is used with the target address */

  shift = slash_notation(argv[1]);

  /* There will be 1 or 2 arguments, depending on if slash notation is
   * used.
   */

  if (shift > 0 && argc != 2)
    {
      nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
      goto errout;
    }
  else if (shift < 0 && argc != 3)
    {
      nsh_error(vtbl, g_fmtargrequired, argv[0]);
      goto errout;
    }

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
          nsh_error(vtbl, g_fmtarginvalid, argv[0]);
          goto errout;
        }
    }

  /* We need to have a socket (any socket) in order to perform the ioctl */

  sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
  if (sockfd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "socket", NSH_ERRNO);
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

  if (shift >= 0)
    {
#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
      if (family == PF_INET)
#endif
        {
          /* /0  -> 0x00000000
           * /8  -> 0xff000000
           * /24 -> 0xffffff00
           * /32 -> 0xffffffff
           */

          if (shift > 32)
            {
              nsh_error(vtbl, g_fmtarginvalid, argv[0]);
              goto errout_with_sockfd;
            }

          inaddr.ipv4.s_addr = htonl(0xffffffff << (32 - shift));
        }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
      else
#endif
        {
          int i;

          /* /0   -> 0000:0000:0000:0000:0000:0000:0000:0000
           * /16  -> ffff:0000:0000:0000:0000:0000:0000:0000
           * /32  -> ffff:ffff:0000:0000:0000:0000:0000:0000
           * ...
           * /128 -> ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff
           */

          memset(&inaddr.ipv6, 0, sizeof(inaddr.ipv6));
          for (i = 0; i < 8 && shift >= 16; i++, shift -= 16)
            {
              inaddr.ipv6.s6_addr16[i] = 0xffff;
            }

          if (shift > 16 || (shift > 0 && i >= 8))
            {
              nsh_error(vtbl, g_fmtarginvalid, argv[0]);
              goto errout_with_sockfd;
            }

          /* /0   -> 0x0000
           * /1   -> 0x8000
           * /2   -> 0xc000
           * ...
           * /16  -> 0xffff
           */

          if (shift > 0)
            {
              inaddr.ipv6.s6_addr16[i] = htons(0xffff << (16 - shift));
            }
        }
#endif
    }
  else
    {
      /* Slash notation not used.. mask should follow the target address */

      ret = inet_pton(family, argv[2], &inaddr);
      if (ret != 1)
        {
          nsh_error(vtbl, g_fmtarginvalid, argv[0]);
          goto errout_with_sockfd;
        }
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

  ret = delroute(sockfd, &target.ipx, &netmask.ipx);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "delroute", NSH_ERRNO);
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
 * nsh> route <ipv4|ipv6>
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_ROUTE
int cmd_route(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
  bool ipv6 = false;
#endif

  /* If both IPv4 and IPv6 and defined then there will be exactly one
   * argument.  Otherwise no arguments are required, but an addition
   * argument is accepted.
   */

#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
  if (strcmp(argv[1], "ipv4") == 0)
    {
      ipv6 = false;
    }
  else if (strcmp(argv[1], "ipv6") == 0)
    {
      ipv6 = true;
    }

  else
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }
#elif defined(CONFIG_NET_IPv4)
  if (argc == 2 && strcmp(argv[1], "ipv4") != 0)
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }
#else
  if (argc == 2 && strcmp(argv[1], "ipv6") != 0)
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }
#endif

  /* Then just cat the procfs file containing the routing table info */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  if (!ipv6)
#endif
    {
      return nsh_catfile(vtbl, argv[0],
                         CONFIG_NSH_PROC_MOUNTPOINT "/net/route/ipv4");
    }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  else
#endif
    {
      return nsh_catfile(vtbl, argv[0],
                         CONFIG_NSH_PROC_MOUNTPOINT "/net/route/ipv6");
    }
#endif

  return ERROR; /* Shouldn't get here */
}
#endif

#endif /* CONFIG_NET && CONFIG_NET_ROUTE */
