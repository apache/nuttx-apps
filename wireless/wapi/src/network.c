/****************************************************************************
 * apps/wireless/wapi/examples/network.c
 *
 *  Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *  All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "util.h"
#include "include/wireless/wapi.h"

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
  if ((ret = ioctl(sock, cmd, &ifr)) >= 0)
    {
      struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
      memcpy(addr, &sin->sin_addr, sizeof(struct in_addr));
    }
  else
    {
      WAPI_IOCTL_STRERROR(cmd);
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
  if ((ret = ioctl(sock, cmd, &ifr)) < 0)
    {
      WAPI_IOCTL_STRERROR(cmd);
    }

  return ret;
}

static int wapi_act_route_gw(int sock, int act,
                             wapi_route_target_t targettype,
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

  if ((ret = ioctl(sock, act, &rt)) < 0)
    {
      WAPI_IOCTL_STRERROR(act);
    }

  return ret;
}

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
  if ((ret = ioctl(sock, SIOCGIFFLAGS, &ifr)) >= 0)
    {
      *is_up = (ifr.ifr_flags & IFF_UP) == IFF_UP;
    }
  else
    {
      WAPI_IOCTL_STRERROR(SIOCGIFFLAGS);
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
  if ((ret = ioctl(sock, SIOCGIFFLAGS, &ifr)) >= 0)
    {
      ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
      ret = ioctl(sock, SIOCSIFFLAGS, &ifr);
    }
  else
    {
      WAPI_IOCTL_STRERROR(SIOCGIFFLAGS);
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
  if ((ret = ioctl(sock, SIOCGIFFLAGS, &ifr)) >= 0)
    {
      ifr.ifr_flags &= ~IFF_UP;
      ret = ioctl(sock, SIOCSIFFLAGS, &ifr);
    }
  else
    {
      WAPI_IOCTL_STRERROR(SIOCGIFFLAGS);
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
 * Name: wapi_get_routes
 *
 * Description:
 *   Parses routing table rows from WAPI_PROC_NET_ROUTE.
 *
 * Input Parameters:
 *   list - Pushes collected wapi_route_info_t into this list.
 *
 ****************************************************************************/

int wapi_get_routes(FAR wapi_list_t *list)
{
  FAR FILE *fp;
  int ret;
  size_t bufsiz = WAPI_PROC_LINE_SIZE * sizeof(char);
  char buf[WAPI_PROC_LINE_SIZE];

  WAPI_VALIDATE_PTR(list);

  /* Open file for reading. */

  fp = fopen(WAPI_PROC_NET_ROUTE, "r");
  if (!fp)
    {
      WAPI_STRERROR("fopen(\"%s\", \"r\")", WAPI_PROC_NET_ROUTE);
      return -1;
    }

  /* Skip header line. */

  if (!fgets(buf, bufsiz, fp))
    {
      WAPI_ERROR("Invalid \"%s\" content!\n", WAPI_PROC_NET_ROUTE);
      return -1;
    }

  /* Read lines. */

  ret = 0;
  while (fgets(buf, bufsiz, fp))
    {
      wapi_route_info_t *ri;
      char ifname[WAPI_PROC_LINE_SIZE];
      int refcnt, use, metric, mtu, window, irtt;
      unsigned int dest, gw, flags, netmask;

      /* Allocate route row buffer. */

      ri = malloc(sizeof(wapi_route_info_t));
      if (!ri)
        {
          WAPI_STRERROR("malloc()");
          ret = -1;
          break;
        }

      /* Read and tokenize fields. */

      sscanf(buf, "%s\t"        /* ifname */
             "%x\t"             /* dest */
             "%x\t"             /* gw */
             "%x\t"             /* flags */
             "%d\t"             /* refcnt */
             "%d\t"             /* use */
             "%d\t"             /* metric */
             "%x\t"             /* mask */
             "%d\t"             /* mtu */
             "%d\t"             /* window */
             "%d\t",            /* irtt */
             ifname, &dest, &gw, &flags, &refcnt, &use, &metric, &netmask, &mtu,
             &window, &irtt);

      /* Allocate "ifname". */

      ri->ifname = malloc((strlen(ifname) + 1) * sizeof(char));
      if (!ri->ifname)
        {
          WAPI_STRERROR("malloc()");
          free(ri);
          ret = -1;
          break;
        }

      /* Copy fields. */

      sprintf(ri->ifname, "%s", ifname);
      ri->dest.s_addr = dest;
      ri->gw.s_addr = gw;
      ri->flags = flags;
      ri->refcnt = refcnt;
      ri->use = use;
      ri->metric = metric;
      ri->netmask.s_addr = netmask;
      ri->mtu = mtu;
      ri->window = window;
      ri->irtt = irtt;

      /* Push parsed node to the list. */

      ri->next = list->head.route;
      list->head.route = ri;
    }

  /* Close file. */

  fclose(fp);
  return 0;
}

/****************************************************************************
 * Name: wapi_add_route_gw
 *
 * Description:
 *   Adds gateway for the given target network.
 *
 ****************************************************************************/

int wapi_add_route_gw(int sock, wapi_route_target_t targettype,
                      FAR const struct in_addr *target,
                      FAR const struct in_addr *netmask,
                      FAR const struct in_addr *gw)
{
  return wapi_act_route_gw(sock, SIOCADDRT, targettype, target, netmask, gw);
}

/****************************************************************************
 * Name: wapi_del_route_gw
 *
 * Description:
 *   Deletes gateway for the given target network.
 *
 ****************************************************************************/

int wapi_del_route_gw(int sock, wapi_route_target_t targettype,
                      FAR const struct in_addr *target,
                      FAR const struct in_addr *netmask,
                      FAR const struct in_addr *gw)
{
  return wapi_act_route_gw(sock, SIOCDELRT, targettype, target, netmask, gw);
}
