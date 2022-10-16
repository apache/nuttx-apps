/****************************************************************************
 * apps/include/netutils/telnetd.h
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

#ifndef __APPS_INCLUDE_NETUTILS_TELNETD_H
#define __APPS_INCLUDE_NETUTILS_TELNETD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* CONFIG_TELNETD_CONSOLE - Use the first Telnet session as the default
 *   console.
 */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* An instance of the struct telnetd_config_s structure must be passed to
 * telnetd_start in order to configure the new Telnet daemon.
 */

struct telnetd_config_s
{
  /* These fields describe the telnet daemon */

  uint8_t     d_priority;  /* The execution priority of the Telnet daemon task */
  size_t      d_stacksize; /* The stack size needed by the Telnet daemon task */

  /* These fields describe the network connection */

  uint16_t    d_port;      /* The port to listen on (in network byte order) */
  sa_family_t d_family;    /* Address family */

  /* These fields describe the priority of each thread created by the Telnet
   * daemon.
   */

  uint8_t     t_priority;  /* The execution priority of the spawned task, */
  size_t      t_stacksize; /* The stack size needed by the spawned task */
  main_t      t_entry;     /* The entrypoint of the task to spawn when a new
                            * connection is accepted. */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: telnetd_start
 *
 * Description:
 *   Start the Telnet daemon.
 *
 * Parameters:
 *   config    A pointer to a configuration structure that characterizes the
 *             Telnet daemon.  This configuration structure may be defined
 *             on the caller's stack because it is not retained by the
 *             daemon.
 *
 * Return:
 *   The process ID (pid) of the new Telnet daemon is returned on
 *   success; A negated errno is returned if the daemon was not successfully
 *   started.
 *
 ****************************************************************************/

int telnetd_start(FAR struct telnetd_config_s *config);

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif /* __APPS_INCLUDE_NETUTILS_TELNETD_H */
