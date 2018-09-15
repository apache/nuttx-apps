/****************************************************************************
 * apps/wireless/ieee802154/libmac/sixlowpan_getreq.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author:  Gregory Nutt <gnutt@nuttx.org>
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <nuttx/fs/ioctl.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int sixlowpan_get_req(int sock, FAR const char *ifname,
                      FAR const struct ieee802154_get_req_s *req)
{
  struct ieee802154_netmac_s arg;
  int ret;

  strncpy(arg.ifr_name, ifname, IFNAMSIZ);

  /* We must use a shadow arg to perform the operation as we must add the
   * network interface name to the front of the argument.
   */

  arg.u.getreq.attr = req->attr;

  ret = ioctl(sock, MAC802154IOC_MLME_GET_REQUEST,
              (unsigned long)((uintptr_t)&arg));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "MAC802154IOC_MLME_GET_REQUEST failed: %d\n", ret);
    }

  memcpy((void *)req, &arg.u.getreq, sizeof(struct ieee802154_get_req_s));

  return ret;
}

/* Wrappers around get_req to make it even easier to get common attributes */

int sixlowpan_getchan(int sock, FAR const char *ifname, FAR uint8_t *chan)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_PHY_CHAN;
  ret = sixlowpan_get_req(sock, ifname, &req);

  *chan = req.attrval.phy.chan;

  return ret;
}

int sixlowpan_geteaddr(int sock, FAR const char *ifname, FAR uint8_t *eaddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_EADDR;
  ret = sixlowpan_get_req(sock, ifname, &req);

  IEEE802154_EADDRCOPY(eaddr, req.attrval.mac.eaddr);

  return ret;
}

int sixlowpan_getsaddr(int sock, FAR const char *ifname, FAR uint8_t *saddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_SADDR;
  ret = sixlowpan_get_req(sock, ifname, &req);

  IEEE802154_SADDRCOPY(saddr, req.attrval.mac.saddr);

  return ret;
}

int sixlowpan_getpanid(int sock, FAR const char *ifname, FAR uint8_t *panid)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_PANID;
  ret = sixlowpan_get_req(sock, ifname, &req);

  IEEE802154_PANIDCOPY(panid, req.attrval.mac.panid);

  return ret;
}

int sixlowpan_getcoordeaddr(int sock, FAR const char *ifname, FAR uint8_t *eaddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_COORD_EADDR;
  ret = sixlowpan_get_req(sock, ifname, &req);

  IEEE802154_EADDRCOPY(eaddr, req.attrval.mac.eaddr);

  return ret;
}

int sixlowpan_getcoordsaddr(int sock, FAR const char *ifname, FAR uint8_t *saddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_COORD_SADDR;
  ret = sixlowpan_get_req(sock, ifname, &req);

  IEEE802154_SADDRCOPY(saddr, req.attrval.mac.saddr);

  return ret;
}

int sixlowpan_getrxonidle(int sock, FAR const char *ifname, FAR bool *rxonidle)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_RX_ON_WHEN_IDLE;
  ret = sixlowpan_get_req(sock, ifname, &req);

  *rxonidle = req.attrval.mac.rxonidle;

  return ret;
}

int sixlowpan_getdevmode(int sock, FAR const char *ifname,
                         FAR enum ieee802154_devmode_e *devmode)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_DEVMODE;
  ret = sixlowpan_get_req(sock, ifname, &req);

  *devmode = req.attrval.mac.devmode;

  return ret;
}

int sixlowpan_gettxpwr(int sock, FAR const char *ifname, FAR int32_t *txpwr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_PHY_TX_POWER;
  ret = sixlowpan_get_req(sock, ifname, &req);

  *txpwr = req.attrval.phy.txpwr;

  return ret;
}
