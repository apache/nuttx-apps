/****************************************************************************
 * apps/examples/telnetd/telnetd.h
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

#ifndef __APPS_EXAMPLES_TELNETD_TELNETD_H
#define __APPS_EXAMPLES_TELNETD_TELNETD_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* CONFIG_EXAMPLES_TELNETD_DAEMONPRIO - Priority of the Telnet daemon.
 *   Default: SCHED_PRIORITY_DEFAULT
 * CONFIG_EXAMPLES_TELNETD_DAEMONSTACKSIZE - Stack size allocated for the
 *   Telnet daemon. Default: 2048
 * CONFIG_EXAMPLES_TELNETD_CLIENTPRIO- Priority of the Telnet client.
 *   Default: SCHED_PRIORITY_DEFAULT
 * CONFIG_EXAMPLES_TELNETD_CLIENTSTACKSIZE - Stack size allocated for the
 *   Telnet client. Default: 2048
 * CONFIG_EXAMPLES_TELNETD_NOMAC - If the hardware has no MAC address of its
 *   own, define this =y to provide a bogus address for testing.
 * CONFIG_EXAMPLES_TELNETD_IPADDR - The target IP address.  Default 10.0.0.2
 * CONFIG_EXAMPLES_TELNETD_DRIPADDR - The default router address. Default
 *   10.0.0.1
 * CONFIG_EXAMPLES_TELNETD_NETMASK - The network mask.  Default: 255.255.255.0
 */

#ifndef CONFIG_EXAMPLES_TELNETD_DAEMONPRIO
#  define CONFIG_EXAMPLES_TELNETD_DAEMONPRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_EXAMPLES_TELNETD_DAEMONSTACKSIZE
#  define CONFIG_EXAMPLES_TELNETD_DAEMONSTACKSIZE 2048
#endif

#ifndef CONFIG_EXAMPLES_TELNETD_CLIENTPRIO
#  define CONFIG_EXAMPLES_TELNETD_CLIENTPRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_EXAMPLES_TELNETD_CLIENTSTACKSIZE
#  define CONFIG_EXAMPLES_TELNETD_CLIENTSTACKSIZE 2048
#endif

#ifndef CONFIG_EXAMPLES_TELNETD_IPADDR
#  define CONFIG_EXAMPLES_TELNETD_IPADDR 0x0a000002
#endif
#ifndef CONFIG_EXAMPLES_TELNETD_DRIPADDR
#  define CONFIG_EXAMPLES_TELNETD_DRIPADDR 0x0a000002
#endif
#ifndef CONFIG_EXAMPLES_TELNETD_NETMASK
#  define CONFIG_EXAMPLES_TELNETD_NETMASK 0xffffff00
#endif

/* Other definitions ********************************************************/

#define SHELL_PROMPT "uIP 1.0> "

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_EXAMPLES_TELNETD_TELNETD_H */
