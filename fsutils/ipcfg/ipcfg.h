/****************************************************************************
 * apps/fsutils/ipcfg/ipcfg.h
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

#ifndef __APPS_FSUTILS_IPCFG_IPCFG_H
#define __APPS_FSUTILS_IPCFG_IPCFG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <netinet/in.h>

#include "fsutils/ipcfg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IPCFG_VERSION   1  /* Increment this if the data/structure changes */
#define MAX_LINESIZE    80
#define MAX_IPv4PROTO   IPv4PROTO_FALLBACK
#define MAX_IPv6PROTO   IPv6PROTO_FALLBACK

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* IP Configuration record header. */

struct ipcfg_header_s
{
  uint8_t     next;     /* Offset to the next IP configuration record */
  uint8_t     version;  /* For binary compatibility */
  sa_family_t type;     /* Either AF_INET or AF_INET6 */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_read_binary_ipv4
 *
 * Description:
 *   Read IPv4 configuration from a binary IP Configuration file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   ipv4cfg - Location to read IPv4 configuration to
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_BINARY) && defined(CONFIG_NET_IPv4)
int ipcfg_read_binary_ipv4(FAR const char *path,
                           FAR struct ipv4cfg_s *ipv4cfg);
#endif

/****************************************************************************
 * Name: ipcfg_read_binary_ipv6
 *
 * Description:
 *   Read IPv6 configuration from a binary IP Configuration file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   ipv6cfg - Location to read IPv6 configuration to
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_BINARY) && defined(CONFIG_NET_IPv6)
int ipcfg_read_binary_ipv6(FAR const char *path,
                           FAR struct ipv6cfg_s *ipv6cfg);
#endif

/****************************************************************************
 * Name: ipcfg_write_binary_ipv4
 *
 * Description:
 *   Write the IPv4 configuration to a binary IP Configuration file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   ipv4cfg - The IPv4 configuration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_IPCFG_BINARY) && \
    defined(CONFIG_NET_IPv4)
int ipcfg_write_binary_ipv4(FAR const char *path,
                            FAR const struct ipv4cfg_s *ipv4cfg);
#endif

/****************************************************************************
 * Name: ipcfg_write_binary_ipv6
 *
 * Description:
 *   Write the IPv6 configuration to a binary IP Configuration file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   ipv6cfg - The IPv6 configuration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_IPCFG_BINARY) && \
    defined(CONFIG_NET_IPv6)
int ipcfg_write_binary_ipv6(FAR const char *path,
                            FAR const struct ipv6cfg_s *ipv6cfg);
#endif

/****************************************************************************
 * Name: ipcfg_read_text_ipv4
 *
 * Description:
 *   Read IPv4 configuration from a human-readable, text IP Configuration
 *   file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   netdev  - Network device name string
 *   ipv4cfg - Location to read IPv4 configuration to
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
int ipcfg_read_text_ipv4(FAR const char *path, FAR const char *netdev,
                         FAR struct ipv4cfg_s *ipv4cfg);
#endif

/****************************************************************************
 * Name: ipcfg_read_text_ipv6
 *
 * Description:
 *   Read IPv6 configuration from a human-readable, text IP Configuration
 *   file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   netdev  - Network device name string
 *   ipv6cfg - Location to read IPv6 configuration to
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
int ipcfg_read_text_ipv6(FAR const char *path, FAR const char *netdev,
                         FAR struct ipv6cfg_s *ipv6cfg);
#endif

/****************************************************************************
 * Name: ipcfg_write_text_ipv4
 *
 * Description:
 *   Write the IPv4 configuration to a human-readable, text IP Configuration
 *   file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   netdev  - Network device name string
 *   ipv4cfg - The IPv4 configuration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv4)
int ipcfg_write_text_ipv4(FAR const char *path, FAR const char *netdev,
                          FAR const struct ipv4cfg_s *ipv4cfg);
#endif

/****************************************************************************
 * Name: ipcfg_write_text_ipv6
 *
 * Description:
 *   Write the IPv6 configuration to a human-readable, text IP Configuration
 *   file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   netdev  - Network device name string
 *   ipv6cfg - The IPv6 configuration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv6)
int ipcfg_write_text_ipv6(FAR const char *path, FAR const char *netdev,
                          FAR const struct ipv6cfg_s *ipv6cfg);
#endif

#endif /* __APPS_FSUTILS_IPCFG_IPCFG_H */
