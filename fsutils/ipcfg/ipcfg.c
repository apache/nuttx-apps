/****************************************************************************
 * apps/fsutils/ipcfg/ipcfg.c
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

#include <stdio.h>
#include <stdlib.h>
#include <debug.h>

#include "fsutils/ipcfg.h"
#include "ipcfg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_allocpath
 *
 * Description:
 *   Allocate memory for and construct the full path to the IPv4
 *   Configuration file.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *
 * Returned Value:
 *   A pointer to the allocated path is returned.  NULL is returned only on
 *  on a failure to allocate.
 *
 ****************************************************************************/

static inline FAR char *ipcfg_allocpath(FAR const char *netdev)
{
#ifndef CONFIG_IPCFG_CHARDEV
  FAR char *path = NULL;
  int ret;

  /* Create the full path to the ipcfg file for the network device.
   * the path of CONFIG_IPCFG_PATH returns to a directory containing
   * multiple files of the form ipcfg-<dev> where <dev> is the network
   * device name.  For example, ipcfg-eth0.
   */

  ret = asprintf(&path, CONFIG_IPCFG_PATH "/ipcfg-%s", netdev);
  if (ret < 0 || path == NULL)
    {
      /* Assume that asprintf failed to allocate memory */

      ferr("ERROR: Failed to create path to ipcfg file: %d\n", ret);
      return NULL;
    }

  return path;

#else
  /* In this case CONFIG_IPCFG_PATH is the full path to a character device
   * that describes the configuration of a single network device.
   *
   * It is dup'ed to simplify typing (const vs non-const) and to make the
   * free operation unconditional.
   */

  return strdup(CONFIG_IPCFG_PATH);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_read
 *
 * Description:
 *   Read and parse the IP configuration file for the specified network
 *   device.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *   ipcfg  - Pointer to a user provided location to receive the IP
 *            configuration.  Refers to either struct ipv4cfg_s or
 *            ipv6cfg_s, depending on the value of af.
 *   af     - Identifies the address family whose IP configuration is
 *            requested.  May be either AF_INET or AF_INET6.
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

int ipcfg_read(FAR const char *netdev, FAR void *ipcfg, sa_family_t af)
{
#ifdef CONFIG_IPCFG_BINARY
  FAR char *path;
  int ret;

  DEBUGASSERT(netdev != NULL && ipcfg != NULL);

  /* Create the full path to the ipcfg file for the network device */

  path = ipcfg_allocpath(netdev);
  if (path == NULL)
    {
      /* Assume failure to allocate memory */

      ferr("ERROR: Failed to create path to ipcfg file\n");
      return -ENOMEM;
    }

#ifdef CONFIG_NET_IPv4
  if (af == AF_INET)
    {
      ret = ipcfg_read_binary_ipv4(path, (FAR struct ipv4cfg_s *)ipcfg);
    }
  else
#endif
#ifdef CONFIG_NET_IPv6
  if (af == AF_INET6)
    {
      ret = ipcfg_read_binary_ipv6(path, (FAR struct ipv6cfg_s *)ipcfg);
    }
  else
#endif
    {
      ferr("ERROR: Unsupported address family: %d\n", af);
      ret = -EAFNOSUPPORT;
    }

  free(path);
  return ret;

#else
  FAR char *path;
  int ret;

  DEBUGASSERT(netdev != NULL && ipcfg != NULL);

  /* Create the full path to the ipcfg file for the network device */

  path = ipcfg_allocpath(netdev);
  if (path == NULL)
    {
      /* Assume failure to allocate memory */

      ferr("ERROR: Failed to create path to ipcfg file\n");
      return -ENOMEM;
    }

#ifdef CONFIG_NET_IPv4
  if (af == AF_INET)
    {
      ret = ipcfg_read_text_ipv4(path, netdev,
                                 (FAR struct ipv4cfg_s *)ipcfg);
    }
  else
#endif
#ifdef CONFIG_NET_IPv6
  if (af == AF_INET6)
    {
      ret = ipcfg_read_text_ipv6(path, netdev,
                                 (FAR struct ipv6cfg_s *)ipcfg);
    }
  else
#endif
    {
      ferr("ERROR: Unsupported address family: %d\n", af);
      ret = -EAFNOSUPPORT;
    }

  free(path);
  return ret;
#endif
}

/****************************************************************************
 * Name: ipcfg_write
 *
 * Description:
 *   Write the IP configuration file for the specified network device.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *   ipcfg  - The IP configuration to be written.  Refers to either struct
 *            ipv4cfg_s or ipv6cfg_s, depending on the value of af.
 *   af     - Identifies the address family whose IP configuration is
 *            to be written.  May be either AF_INET or AF_INET6.
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_IPCFG_WRITABLE
int ipcfg_write(FAR const char *netdev, FAR const void *ipcfg,
                sa_family_t af)
{
#ifdef CONFIG_IPCFG_BINARY
  FAR char *path;
  int ret;

  DEBUGASSERT(netdev != NULL && ipcfg != NULL);

  /* Create the full path to the ipcfg file for the network device */

  path = ipcfg_allocpath(netdev);
  if (path == NULL)
    {
      /* Assume failure to allocate memory */

      ferr("ERROR: Failed to create path to ipcfg file\n");
      return -ENOMEM;
    }

#ifdef CONFIG_NET_IPv4
  if (af == AF_INET)
    {
      ret = ipcfg_write_binary_ipv4(path,
                                    (FAR const struct ipv4cfg_s *)ipcfg);
    }
  else
#endif
#ifdef CONFIG_NET_IPv6
  if (af == AF_INET6)
    {
      ret = ipcfg_write_binary_ipv6(path,
                                    (FAR const struct ipv6cfg_s *)ipcfg);
    }
  else
#endif
    {
      ferr("ERROR: Unsupported address family: %d\n", af);
      ret = -EAFNOSUPPORT;
    }

  free(path);
  return ret;

#else
  FAR char *path;
  int ret;

  DEBUGASSERT(netdev != NULL && ipcfg != NULL);

  /* Create the full path to the ipcfg file for the network device */

  path = ipcfg_allocpath(netdev);
  if (path == NULL)
    {
      /* Assume failure to allocate memory */

      ferr("ERROR: Failed to create path to ipcfg file\n");
      return -ENOMEM;
    }

#ifdef CONFIG_NET_IPv4
  if (af == AF_INET)
    {
      ret = ipcfg_write_text_ipv4(path, netdev,
                                  (FAR const struct ipv4cfg_s *)ipcfg);
    }
  else
#endif
#ifdef CONFIG_NET_IPv6
  if (af == AF_INET6)
    {
      ret = ipcfg_write_text_ipv6(path, netdev,
                                  (FAR const struct ipv6cfg_s *)ipcfg);
    }
  else
#endif
    {
      ferr("ERROR: Unsupported address family: %d\n", af);
      ret = -EAFNOSUPPORT;
    }

  free(path);
  return ret;
#endif
}
#endif
