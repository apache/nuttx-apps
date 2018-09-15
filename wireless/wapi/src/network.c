/****************************************************************************
 * apps/wireless/wapi/examples/network.c
 *
 *   Copyright (C) 2011, 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for Nuttx from WAPI:
 *
 *   Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of  source code must  retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of  conditions and the  following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

#include <net/route.h>
#include <netinet/in.h>

#include "util.h"
#include "wireless/wapi.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int wapi_get_addr(int sock, FAR const char *ifname, int cmd,
                         FAR struct in_addr *addr)
{
  struct ifreq ifr;
  int ret;

  WAPI_VALIDATE_PTR(addr);

  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  if ((ret = ioctl(sock, cmd, (unsigned long)((uintptr_t)&ifr))) >= 0)
    {
      struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
      memcpy(addr, &sin->sin_addr, sizeof(struct in_addr));
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(cmd, errcode);
      ret = -errcode;
    }

  return ret;
}

static int wapi_set_addr(int sock, FAR const char *ifname, int cmd,
                         FAR const struct in_addr *addr)
{
  struct sockaddr_in sin;
  struct ifreq ifr;
  int ret;

  WAPI_VALIDATE_PTR(addr);

  sin.sin_family = AF_INET;
  memcpy(&sin.sin_addr, addr, sizeof(struct in_addr));
  memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr_in));
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  if ((ret = ioctl(sock, cmd, (unsigned long)((uintptr_t)&ifr))) < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(cmd, errcode);
      ret = -errcode;
    }

  return ret;
}

#ifdef CONFIG_NET_ROUTE
static int wapi_act_route_gw(int sock, int act,
                             enum wapi_route_target_e targettype,
                             FAR const struct in_addr *target,
                             FAR const struct in_addr *netmask,
                             FAR const struct in_addr *gw)
{
  int ret;
  struct rtentry rt;
  FAR struct sockaddr_in *sin;

  /* Clean out rtentry. */

  bzero(&rt, sizeof(struct rtentry));

  /* Set target. */

  sin = (struct sockaddr_in *)&rt.rt_dst;
  sin->sin_family = AF_INET;
  memcpy(&sin->sin_addr, target, sizeof(struct in_addr));

  /* Set netmask. */

  sin = (struct sockaddr_in *)&rt.rt_genmask;
  sin->sin_family = AF_INET;
  memcpy(&sin->sin_addr, netmask, sizeof(struct in_addr));

  /* Set gateway. */

  sin = (struct sockaddr_in *)&rt.rt_gateway;
  sin->sin_family = AF_INET;
  memcpy(&sin->sin_addr, gw, sizeof(struct in_addr));

  /* Set rt_flags. */

  rt.rt_flags = RTF_UP | RTF_GATEWAY;
  if (targettype == WAPI_ROUTE_TARGET_HOST)
    {
      rt.rt_flags |= RTF_HOST;
    }

  if ((ret = ioctl(sock, act, (unsigned long)((uintptr_t)&rt))) < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(act, errcode);
      ret = -errcode;
    }

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wapi_get_ifup
 *
 * Description:
 *   Gets the interface up status.
 *
 * Input Parameters:
 *   is_up Set to 0, if up; 1, otherwise.
 *
 ****************************************************************************/

int wapi_get_ifup(int sock, FAR const char *ifname, FAR int *is_up)
{
  struct ifreq ifr;
  int ret;

  WAPI_VALIDATE_PTR(is_up);

  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  if ((ret = ioctl(sock, SIOCGIFFLAGS, (unsigned long)((uintptr_t)&ifr))) >= 0)
    {
      *is_up = (ifr.ifr_flags & IFF_UP) == IFF_UP;
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIFFLAGS, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_set_ifup
 *
 * Description:
 *   Activates the interface.
 *
 ****************************************************************************/

int wapi_set_ifup(int sock, FAR const char *ifname)
{
  struct ifreq ifr;
  int ret;

  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  if ((ret = ioctl(sock, SIOCGIFFLAGS, (unsigned long)((uintptr_t)&ifr))) >= 0)
    {
      ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
      ret = ioctl(sock, SIOCSIFFLAGS, (unsigned long)((uintptr_t)&ifr));
      if (ret < 0)
        {
          int errcode = errno;
          WAPI_IOCTL_STRERROR(SIOCSIFFLAGS, errcode);
          ret = -errcode;
        }
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIFFLAGS, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_set_ifdown
 *
 * Description:
 *   Shuts down the interface.
 *
 ****************************************************************************/

int wapi_set_ifdown(int sock, FAR const char *ifname)
{
  struct ifreq ifr;
  int ret;

  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  if ((ret = ioctl(sock, SIOCGIFFLAGS, (unsigned long)((uintptr_t)&ifr))) >= 0)
    {
      ifr.ifr_flags &= ~IFF_UP;
      ret = ioctl(sock, SIOCSIFFLAGS, (unsigned long)((uintptr_t)&ifr));
      if (ret < 0)
        {
          int errcode = errno;
          WAPI_IOCTL_STRERROR(SIOCSIFFLAGS, errcode);
          ret = -errcode;
        }
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIFFLAGS, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_get_ip
 *
 * Description:
 *   Gets IP address of the given network interface.
 *
 ****************************************************************************/

int wapi_get_ip(int sock, FAR const char *ifname, FAR struct in_addr *addr)
{
  return wapi_get_addr(sock, ifname, SIOCGIFADDR, addr);
}

/****************************************************************************
 * Name: wapi_set_ip
 *
 * Description:
 *   Sets IP adress of the given network interface.
 *
 ****************************************************************************/

int wapi_set_ip(int sock, FAR const char *ifname,
                FAR const struct in_addr *addr)
{
  return wapi_set_addr(sock, ifname, SIOCSIFADDR, addr);
}

/****************************************************************************
 * Name: wapi_set_netmask
 *
 * Description:
 *   Sets netmask of the given network interface.
 *
 ****************************************************************************/

int wapi_get_netmask(int sock, FAR const char *ifname,
                     FAR struct in_addr *addr)
{
  return wapi_get_addr(sock, ifname, SIOCGIFNETMASK, addr);
}

/****************************************************************************
 * Name: wapi_set_netmask
 *
 * Description:
 *   Sets netmask of the given network interface.
 *
 ****************************************************************************/

int wapi_set_netmask(int sock, FAR const char *ifname,
                     FAR const struct in_addr *addr)
{
  return wapi_set_addr(sock, ifname, SIOCSIFNETMASK, addr);
}

/****************************************************************************
 * Name: wapi_add_route_gw
 *
 * Description:
 *   Adds gateway for the given target network.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_ROUTE
int wapi_add_route_gw(int sock, enum wapi_route_target_e targettype,
                      FAR const struct in_addr *target,
                      FAR const struct in_addr *netmask,
                      FAR const struct in_addr *gw)
{
  return wapi_act_route_gw(sock, SIOCADDRT, targettype, target, netmask, gw);
}
#endif

/****************************************************************************
 * Name: wapi_del_route_gw
 *
 * Description:
 *   Deletes gateway for the given target network.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_ROUTE
int wapi_del_route_gw(int sock, enum wapi_route_target_e targettype,
                      FAR const struct in_addr *target,
                      FAR const struct in_addr *netmask,
                      FAR const struct in_addr *gw)
{
  return wapi_act_route_gw(sock, SIOCDELRT, targettype, target, netmask, gw);
}
#endif
