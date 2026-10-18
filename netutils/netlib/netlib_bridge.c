/****************************************************************************
 * apps/netutils/netlib/netlib_bridge.c
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
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>

#include <nuttx/net/ioctl.h>
#include <nuttx/net/netconfig.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_bridge_cmd
 *
 * Description:
 *   Issue a bridge ioctl that takes a bridge name only (add/delete bridge)
 *   or a bridge name and an interface (add/delete port).
 *
 ****************************************************************************/

static int netlib_bridge_cmd(int cmd, FAR const char *bridge,
                             FAR const char *ifname)
{
  struct ifreq req;
  int sockfd;
  int ret;

  if (bridge == NULL)
    {
      return -EINVAL;
    }

  sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
  if (sockfd < 0)
    {
      return -errno;
    }

  if (ifname == NULL)
    {
      ret = ioctl(sockfd, cmd, (unsigned long)(uintptr_t)bridge);
      if (ret < 0)
        {
          ret = -errno;
        }
    }
  else
    {
      memset(&req, 0, sizeof(req));
      strlcpy(req.ifr_name, bridge, IFNAMSIZ);
      req.ifr_ifindex = if_nametoindex(ifname);
      if (req.ifr_ifindex == 0)
        {
          ret = -errno;
        }
      else
        {
          ret = ioctl(sockfd, cmd, (unsigned long)(uintptr_t)&req);
          if (ret < 0)
            {
              ret = -errno;
            }
        }
    }

  close(sockfd);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_add_bridge
 *
 * Description:
 *   Create a bridge.
 *
 * Parameters:
 *   bridge - The name of the bridge to create
 *
 * Return:
 *   0 on success; a negated errno value on failure
 *
 ****************************************************************************/

int netlib_add_bridge(FAR const char *bridge)
{
  return netlib_bridge_cmd(SIOCBRADDBR, bridge, NULL);
}

/****************************************************************************
 * Name: netlib_del_bridge
 *
 * Description:
 *   Delete a bridge.
 *
 * Parameters:
 *   bridge - The name of the bridge to delete
 *
 * Return:
 *   0 on success; a negated errno value on failure
 *
 ****************************************************************************/

int netlib_del_bridge(FAR const char *bridge)
{
  return netlib_bridge_cmd(SIOCBRDELBR, bridge, NULL);
}

/****************************************************************************
 * Name: netlib_bridge_addif
 *
 * Description:
 *   Add a network device to a bridge.
 *
 * Parameters:
 *   bridge - The name of the bridge
 *   ifname - The name of the network device to add
 *
 * Return:
 *   0 on success; a negated errno value on failure
 *
 ****************************************************************************/

int netlib_bridge_addif(FAR const char *bridge, FAR const char *ifname)
{
  return ifname == NULL ? -EINVAL :
         netlib_bridge_cmd(SIOCBRADDIF, bridge, ifname);
}

/****************************************************************************
 * Name: netlib_bridge_delif
 *
 * Description:
 *   Remove a network device from a bridge.
 *
 * Parameters:
 *   bridge - The name of the bridge
 *   ifname - The name of the network device to remove
 *
 * Return:
 *   0 on success; a negated errno value on failure
 *
 ****************************************************************************/

int netlib_bridge_delif(FAR const char *bridge, FAR const char *ifname)
{
  return ifname == NULL ? -EINVAL :
         netlib_bridge_cmd(SIOCBRDELIF, bridge, ifname);
}
