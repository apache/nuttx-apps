/****************************************************************************
 * apps/wireless/ieee802154/libmac/sixlowpan_getreq.c
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

/* Wrappers around get_req to make it even easier to get common
 * attributes
 */

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

int sixlowpan_getcoordeaddr(int sock,
                            FAR const char *ifname, FAR uint8_t *eaddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_COORD_EADDR;
  ret = sixlowpan_get_req(sock, ifname, &req);

  IEEE802154_EADDRCOPY(eaddr, req.attrval.mac.eaddr);

  return ret;
}

int sixlowpan_getcoordsaddr(int sock,
                            FAR const char *ifname, FAR uint8_t *saddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_COORD_SADDR;
  ret = sixlowpan_get_req(sock, ifname, &req);

  IEEE802154_SADDRCOPY(saddr, req.attrval.mac.saddr);

  return ret;
}

int sixlowpan_getrxonidle(int sock,
                          FAR const char *ifname, FAR bool *rxonidle)
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

int sixlowpan_getpromisc(int sock,
                         FAR const char *ifname, FAR bool *promisc)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_PROMISCUOUS_MODE;
  ret = sixlowpan_get_req(sock, ifname, &req);

  *promisc = req.attrval.mac.promisc_mode;

  return ret;
}

int sixlowpan_gettxpwr(int sock,
                       FAR const char *ifname, FAR int32_t *txpwr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_PHY_TX_POWER;
  ret = sixlowpan_get_req(sock, ifname, &req);

  *txpwr = req.attrval.phy.txpwr;

  return ret;
}

int sixlowpan_getmaxretries(int sock,
                            FAR const char *ifname, FAR uint8_t *retries)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_MAX_FRAME_RETRIES;
  ret = sixlowpan_get_req(sock, ifname, &req);

  *retries = req.attrval.mac.max_retries;

  return ret;
}

int sixlowpan_getfcslen(int sock,
                        FAR const char *ifname, FAR uint8_t *fcslen)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_PHY_FCS_LEN;
  ret = sixlowpan_get_req(sock, ifname, &req);

  *fcslen = req.attrval.phy.fcslen;

  return ret;
}
