/****************************************************************************
 * apps/system/iptables/iptables.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __APPS_SYSTEM_IPTABLES_IPTABLES_H
#define __APPS_SYSTEM_IPTABLES_IPTABLES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/compiler.h>
#include <nuttx/net/netfilter/netfilter.h>
#include <nuttx/net/netfilter/nf_nat.h>
#include <nuttx/net/netfilter/x_tables.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define INV_FLAG_STR(flag) ((flag) ? "!" : "")

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

enum iptables_command_e
{
  COMMAND_INVALID = 0,
  COMMAND_APPEND,
  COMMAND_INSERT,
  COMMAND_DELETE,
  COMMAND_FLUSH,
  COMMAND_LIST,
  COMMAND_POLICY
};

struct iptables_args_s
{
  enum iptables_command_e cmd;
  enum nf_inet_hooks hook;

  FAR const char *table;

  FAR const char *inifname;
  FAR const char *outifname;
  FAR const char *target;

  FAR const char *saddr;
  FAR const char *daddr;
  FAR const char *sport;
  FAR const char *dport;
  FAR const char *icmp_type;

  int8_t  verdict;
  int8_t  rulenum;
  uint8_t protocol;

  /* invert flags */

  uint8_t ipinv;
  uint8_t tcpudpinv;
  uint8_t icmpinv;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: iptables_parse
 *
 * Description:
 *   Parse args from arg list
 *
 * Returned Value:
 *   0 on success, or a negative error code on failure
 *
 ****************************************************************************/

int iptables_parse(FAR struct iptables_args_s *args,
                   int argc, FAR char *argv[]);

/****************************************************************************
 * Name: iptables_parse_ip
 *
 * Description:
 *   Parse ip string into address and mask
 *
 * Returned Value:
 *   0 on success, or a negative error code on failure
 *
 ****************************************************************************/

int iptables_parse_ip(FAR const char *str, FAR void *addr, FAR void *mask,
                      uint8_t family);

/****************************************************************************
 * Name: iptables_parse_ports
 *
 * Description:
 *   Parse port string into min/max port, NULL for ALL
 *
 * Returned Value:
 *   0 on success, or a negative error code on failure
 *
 ****************************************************************************/

int iptables_parse_ports(FAR const char *str, uint16_t ports[2]);

/****************************************************************************
 * Name: iptables_parse_icmp
 *
 * Description:
 *   Parse icmp type string into type number
 *
 * Returned Value:
 *   Type code on success, or a negative error code on failure
 *
 ****************************************************************************/

int iptables_parse_icmp(FAR const char *str);

/****************************************************************************
 * Name: iptables_showusage
 *
 * Description:
 *   Show usage of the iptables program
 *
 ****************************************************************************/

void iptables_showusage(FAR const char *progname);

/****************************************************************************
 * Name: iptables_hook2str
 *
 * Description:
 *   Get hook name from hook number
 *
 ****************************************************************************/

FAR const char *iptables_hook2str(enum nf_inet_hooks hook);

/****************************************************************************
 * Name: iptables_target2str
 *
 * Description:
 *   Get target name from target structure
 *
 ****************************************************************************/

FAR const char *iptables_target2str(FAR const struct xt_entry_target *tgt);

/****************************************************************************
 * Name: iptables_proto2str
 *
 * Description:
 *   Get protocol name from protocol number
 *
 ****************************************************************************/

FAR const char *iptables_proto2str(uint8_t proto);

/****************************************************************************
 * Name: iptables_iface2str
 *
 * Description:
 *   Get interface name from interface string
 *
 ****************************************************************************/

#define iptables_iface2str(iface) ((iface)[0] != '\0' ? (iface) : "any")

/****************************************************************************
 * Name: iptables_match2str
 *
 * Description:
 *   Get match details from match structure
 *
 ****************************************************************************/

FAR const char *iptables_match2str(FAR const struct xt_entry_match *match);

#endif /* __APPS_SYSTEM_IPTABLES_IPTABLES_H */
