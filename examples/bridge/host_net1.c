/****************************************************************************
 * examples/bridge/host_net1.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
