/****************************************************************************
 * apps/netutils/netlib/netlib_addvlan.c
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
#ifdef CONFIG_NET_VLAN

#include <nuttx/net/vlan.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_add_vlan
 *
 * Description:
 *   Add a VLAN interface to an existing network device.
 *
 * Parameters:
 *   ifname - The name of the existing network device
 *   vlanid - The VLAN identifier to be added
 *
 * Return:
 *   0 on success; -1 on failure
 *
 ****************************************************************************/

int netlib_add_vlan(FAR const char *ifname, int vlanid)
{
  int ret = ERROR;

  if (ifname && vlanid > 0)
    {
      int sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          struct vlan_ioctl_args ifv;

          strlcpy(ifv.device1, ifname, sizeof(ifv.device1));
          ifv.u.VID = vlanid;
          ifv.cmd = ADD_VLAN_CMD;

          ret = ioctl(sockfd, SIOCSIFVLAN, &ifv);
          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET_VLAN */
