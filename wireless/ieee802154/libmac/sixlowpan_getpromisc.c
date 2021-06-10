/****************************************************************************
 * apps/wireless/ieee802154/libmac/sixlowpan_getpromisc.c
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

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

#include "wireless/ieee802154.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int sixlowpan_getpromisc(int sock, FAR const char *ifname, FAR bool *promisc)
{
  struct ieee802154_get_req_s req;
  int ret;

  req.attr = IEEE802154_ATTR_MAC_PROMISCUOUS_MODE;
  ret = sixlowpan_get_req(sock, ifname, &req);

  *promisc = req.attrval.mac.promisc_mode;

  return ret;
}
