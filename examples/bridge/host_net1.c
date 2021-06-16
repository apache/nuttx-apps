/****************************************************************************
 * apps/examples/bridge/host_net1.c
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

#include "bridge_config.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Send on network 1 */

#define EXAMPLES_BRIDGE_SEND_IFNAME    CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME
#define EXAMPLES_BRIDGE_SEND_RECVPORT  CONFIG_EXAMPLES_BRIDGE_NET1_RECVPORT
#define EXAMPLES_BRIDGE_SEND_IOBUFIZE  CONFIG_EXAMPLES_BRIDGE_NET1_IOBUFIZE
#ifdef CONFIG_EXAMPLES_BRIDGE_NET1_NOMAC
#  define EXAMPLES_BRIDGE_SEND_NOMAC
#endif
#define EXAMPLES_BRIDGE_SEND_MACADDR   CONFIG_EXAMPLES_BRIDGE_NET1_MACADDR
#define EXAMPLES_BRIDGE_SEND_IPADDR    CONFIG_EXAMPLES_BRIDGE_NET1_IPADDR
#define EXAMPLES_BRIDGE_SEND_DRIPADDR  CONFIG_EXAMPLES_BRIDGE_NET1_DRIPADDR
#define EXAMPLES_BRIDGE_SEND_NETMASK   CONFIG_EXAMPLES_BRIDGE_NET1_NETMASK
#define EXAMPLES_BRIDGE_SEND_IPHOST    CONFIG_EXAMPLES_BRIDGE_NET1_IPHOST
#define EXAMPLES_BRIDGE_SEND_HOSTPORT  CONFIG_EXAMPLES_BRIDGE_NET1_HOSTPORT
#define EXAMPLES_BRIDGE_SEND_STACKSIZE CONFIG_EXAMPLES_BRIDGE_NET1_STACKSIZE
#define EXAMPLES_BRIDGE_SEND_PRIORITY  CONFIG_EXAMPLES_BRIDGE_NET1_PRIORITY

/* Receive on network 2 */

#define EXAMPLES_BRIDGE_RECV_IFNAME    CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME
#define EXAMPLES_BRIDGE_RECV_RECVPORT  CONFIG_EXAMPLES_BRIDGE_NET2_RECVPORT
#define EXAMPLES_BRIDGE_RECV_IOBUFIZE  CONFIG_EXAMPLES_BRIDGE_NET2_IOBUFIZE
#ifdef CONFIG_EXAMPLES_BRIDGE_NET2_NOMAC
#  define EXAMPLES_BRIDGE_RECV_NOMAC
#endif
#define EXAMPLES_BRIDGE_RECV_MACADDR   CONFIG_EXAMPLES_BRIDGE_NET2_MACADDR
#define EXAMPLES_BRIDGE_RECV_IPADDR    CONFIG_EXAMPLES_BRIDGE_NET2_IPADDR
#define EXAMPLES_BRIDGE_RECV_DRIPADDR  CONFIG_EXAMPLES_BRIDGE_NET2_DRIPADDR
#define EXAMPLES_BRIDGE_RECV_NETMASK   CONFIG_EXAMPLES_BRIDGE_NET2_NETMASK
#define EXAMPLES_BRIDGE_RECV_IPHOST    CONFIG_EXAMPLES_BRIDGE_NET2_IPHOST
#define EXAMPLES_BRIDGE_RECV_HOSTPORT  CONFIG_EXAMPLES_BRIDGE_NET2_HOSTPORT
#define EXAMPLES_BRIDGE_RECV_STACKSIZE CONFIG_EXAMPLES_BRIDGE_NET2_STACKSIZE
#define EXAMPLES_BRIDGE_RECV_PRIORITY  CONFIG_EXAMPLES_BRIDGE_NET2_PRIORITY

#define LABEL "NET1->2: "

#define MESSAGE \
  "Great ambition is the passion of a great character. Those endowed" \
  "with it may perform very good or very bad acts. All depends on the" \
  "principles which direct them. -- Napoleon Bonaparte"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

#include "host_main.c"
