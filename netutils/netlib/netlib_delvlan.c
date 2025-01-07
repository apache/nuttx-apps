/****************************************************************************
 * apps/netutils/netlib/netlib_delvlan.c
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
 * Name: netlib_del_vlan
 *
 * Description:
 *   Remove a VLAN interface from an existing network device.
 *
 * Parameters:
 *   vlanif - The name of the VLAN network device
 *
 * Return:
 *   0 on success; -1 on failure
 *
 ****************************************************************************/

int netlib_del_vlan(FAR const char *vlanif)
{
  int ret = ERROR;

  if (vlanif)
    {
      int sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          struct vlan_ioctl_args ifv;

          strlcpy(ifv.device1, vlanif, sizeof(ifv.device1));
          ifv.cmd = DEL_VLAN_CMD;

          ret = ioctl(sockfd, SIOCSIFVLAN, &ifv);
          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET_VLAN */
