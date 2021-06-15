/****************************************************************************
 * apps/examples/ftpd/ftpd.h
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

#ifndef __APPS_EXAMPLES_FTPD_FTPD_H
#define __APPS_EXAMPLES_FTPD_FTPD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* CONFIG_EXAMPLES_FTPD_PRIO - Priority of the FTP daemon.
 *   Default: SCHED_PRIORITY_DEFAULT
 * CONFIG_EXAMPLES_FTPD_STACKSIZE - Stack size allocated for the
 *   FTP daemon. Default: 2048
 * CONFIG_EXAMPLES_FTPD_NONETINIT - Define to suppress configuration of the
 *   network by apps/examples/ftpd.  You would need to suppress network
 *   configuration if the network is configuration prior to running the
 *   example.
 *
 * If CONFIG_EXAMPLES_FTPD_NONETINIT is not defined, then the following may
 * be specified to customized the network configuration:
 *
 * CONFIG_EXAMPLES_FTPD_NOMAC - If the hardware has no MAC address of its
 *   own, define this =y to provide a bogus address for testing.
 * CONFIG_EXAMPLES_FTPD_IPADDR - The target IP address.  Default 10.0.0.2
 * CONFIG_EXAMPLES_FTPD_DRIPADDR - The default router address. Default
 *   10.0.0.1
 * CONFIG_EXAMPLES_FTPD_NETMASK - The network mask.  Default: 255.255.255.0
 */

#ifndef CONFIG_EXAMPLES_FTPD_PRIO
#  define CONFIG_EXAMPLES_FTPD_PRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_EXAMPLES_FTPD_STACKSIZE
#  define CONFIG_EXAMPLES_FTPD_STACKSIZE 2048
#endif

#ifndef CONFIG_EXAMPLES_FTPD_CLIENTPRIO
#  define CONFIG_EXAMPLES_FTPD_CLIENTPRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_EXAMPLES_FTPD_CLIENTSTACKSIZE
#  define CONFIG_EXAMPLES_FTPD_CLIENTSTACKSIZE 2048
#endif

/* NSH always initializes the network */

#if defined(CONFIG_NSH_NETINIT) && !defined(CONFIG_EXAMPLES_FTPD_NONETINIT)
#  define CONFIG_EXAMPLES_FTPD_NONETINIT 1
#endif

#ifdef CONFIG_EXAMPLES_FTPD_NONETINIT
#  undef CONFIG_EXAMPLES_FTPD_IPADDR
#  undef CONFIG_EXAMPLES_FTPD_DRIPADDR
#  undef CONFIG_EXAMPLES_FTPD_NETMASK
#else
#  ifndef CONFIG_EXAMPLES_FTPD_IPADDR
#    define CONFIG_EXAMPLES_FTPD_IPADDR 0x0a000002
#  endif
#  ifndef CONFIG_EXAMPLES_FTPD_DRIPADDR
#    define CONFIG_EXAMPLES_FTPD_DRIPADDR 0x0a000001
#  endif
#  ifndef CONFIG_EXAMPLES_FTPD_NETMASK
#    define CONFIG_EXAMPLES_FTPD_NETMASK 0xffffff00
#  endif
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure describes one entry in a table of accounts */

struct fptd_account_s
{
  uint8_t         flags;
  FAR const char *user;
  FAR const char *password;
  FAR const char *home;
};

/* To minimize the probability of name collisitions, all FTPD example
 * global data is maintained in single structure.
 */

struct ftpd_globals_s
{
  bool          initialized; /* True: Networking is initialized.  The
                              * network must be initialized only once.
                              */
  volatile bool stop;        /* True: Request daemon to exit */
  volatile bool running;     /* True: The daemon is running */
  pid_t         pid;         /* Task ID of the FTPD daemon.  The value
                              * -1 is a redundant indication that the
                              * daemon is not running.
                              */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* To minimize the probability of name collisitions, all FTPD example
 * global data is maintained in a single instance of a structure.
 */

extern struct ftpd_globals_s g_ftpdglob;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_EXAMPLES_FTPD_FTPD_H */
