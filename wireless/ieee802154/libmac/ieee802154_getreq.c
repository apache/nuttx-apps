/****************************************************************************
 * apps/wireless/ieee802154/libmac/ieee802154_getreq.c
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
#include <errno.h>

#include <nuttx/fs/ioctl.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ieee802154_get_req(int fd, FAR struct ieee802154_get_req_s *req)
{
  int ret;

  ret = ioctl(fd, MAC802154IOC_MLME_GET_REQUEST,
             (unsigned long)((uintptr_t)req));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "MAC802154IOC_MLME_GET_REQUEST failed: %d\n", ret);
    }

  return ret;
}

/* Wrappers around get_req to make it even easier to get common attributes */

int ieee802154_getchan(int fd, FAR uint8_t *chan)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_PHY_CHAN;
  ret = ieee802154_get_req(fd, &req);

  *chan = req.attrval.phy.chan;

  return ret;
}

int ieee802154_geteaddr(int fd, FAR uint8_t *eaddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_EADDR;
  ret = ieee802154_get_req(fd, &req);

  IEEE802154_EADDRCOPY(eaddr, req.attrval.mac.eaddr);

  return ret;
}

int ieee802154_getsaddr(int fd, FAR uint8_t *saddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_SADDR;
  ret = ieee802154_get_req(fd, &req);

  IEEE802154_SADDRCOPY(saddr, req.attrval.mac.saddr);
  return ret;
}

int ieee802154_getpanid(int fd, FAR uint8_t *panid)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_PANID;
  ret = ieee802154_get_req(fd, &req);

  IEEE802154_PANIDCOPY(panid, req.attrval.mac.panid);
  return ret;
}

int ieee802154_getcoordeaddr(int fd, FAR uint8_t *eaddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_COORD_EADDR;
  ret = ieee802154_get_req(fd, &req);

  IEEE802154_EADDRCOPY(eaddr, req.attrval.mac.eaddr);

  return ret;
}

int ieee802154_getcoordsaddr(int fd, FAR uint8_t *saddr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_COORD_SADDR;
  ret = ieee802154_get_req(fd, &req);

  IEEE802154_SADDRCOPY(saddr, req.attrval.mac.saddr);
  return ret;
}

int ieee802154_getrxonidle(int fd, FAR bool *rxonidle)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_RX_ON_WHEN_IDLE;
  ret = ieee802154_get_req(fd, &req);

  *rxonidle = req.attrval.mac.rxonidle;

  return ret;
}

int ieee802154_getdevmode(int fd, FAR enum ieee802154_devmode_e *devmode)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_DEVMODE;
  ret = ieee802154_get_req(fd, &req);

  *devmode = req.attrval.mac.devmode;

  return ret;
}

int ieee802154_getpromisc(int fd, FAR bool *promisc)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_PROMISCUOUS_MODE;
  ret = ieee802154_get_req(fd, &req);

  *promisc = req.attrval.mac.promisc_mode;

  return ret;
}

int ieee802154_gettxpwr(int fd, FAR int32_t *txpwr)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_PHY_TX_POWER;
  ret = ieee802154_get_req(fd, &req);

  *txpwr = req.attrval.phy.txpwr;

  return ret;
}

int ieee802154_getmaxretries(int fd, FAR uint8_t *retries)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_MAX_FRAME_RETRIES;
  ret = ieee802154_get_req(fd, &req);

  *retries = req.attrval.mac.max_retries;

  return ret;
}

int ieee802154_getfcslen(int fd, FAR uint8_t *fcslen)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_PHY_FCS_LEN;
  ret = ieee802154_get_req(fd, &req);

  *fcslen = req.attrval.phy.fcslen;

  return ret;
}
