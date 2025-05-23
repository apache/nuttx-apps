/****************************************************************************
 * apps/netutils/mdns/mdnsd.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <limits.h>
#include <semaphore.h>
#include <debug.h>                 /* For nerr, info */

#include "netutils/netlib.h"
#include "netutils/mdnsd.h"

#include <spawn.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This enumeration describes the state of the MDNSD daemon */

enum mdnsd_daemon_e
{
  MDNSD_NOT_RUNNING = 0,
  MDNSD_STARTING,
  MDNSD_RUNNING,
  MDNSD_STOP_REQUESTED,
};

struct mdnsd_config_s
{
  pid_t               pid;   /* Task ID of the spawned MDNSD daemon         */
  enum mdnsd_daemon_e state; /* State of the daemon                         */
  sem_t               lock;  /* Used to protect the whole structure         */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct mdnsd_config_s g_mdnsd_config =
{
  -1,
  MDNSD_NOT_RUNNING,
  SEM_INITIALIZER(1),
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mdnsd_stop(void)
{
  int ret;

  sem_wait(&g_mdnsd_config.lock);
  if (g_mdnsd_config.state == MDNSD_RUNNING)
    {
      g_mdnsd_config.state = MDNSD_STOP_REQUESTED;
      do
        {
          ret = (kill(g_mdnsd_config.pid,
                      CONFIG_NETUTILS_MDNS_DAEMON_STOP_SIGNAL));
          if (ret < 0)
            {
              nerr("ERROR: kill pid %d failed: %d\n",
                    g_mdnsd_config.pid, errno);
              break;
            }
        }
      while (g_mdnsd_config.state == MDNSD_STOP_REQUESTED);
    }

  g_mdnsd_config.state = MDNSD_NOT_RUNNING;
  sem_post(&g_mdnsd_config.lock);
  return OK;
}

int mdnsd_start(FAR char *service_name, FAR char *service_port)
{
  int ret;
  char hostname_buffer[HOST_NAME_MAX];
  const char *hostname = "nuttx";
  FAR char *argv[7];

  /* Are we in a non-running state? */

  sem_wait(&g_mdnsd_config.lock);
  if (g_mdnsd_config.state == MDNSD_NOT_RUNNING)
    {
      g_mdnsd_config.state = MDNSD_STARTING;

      /* Configure our mDNS service */

      /* The mdns library function will check the hostname too, but this
       * ensures the default is our, not the library author's, choice.
       */

      if (gethostname(hostname_buffer, HOST_NAME_MAX) == 0)
        {
          if (strlen(hostname_buffer) != 0)
            {
              hostname = hostname_buffer;
            }
        }

      argv[0] = "--service";
      argv[1] = service_name;
      argv[2] = "--hostname";
      argv[3] = (char *)hostname;
      argv[4] = "--port";
      argv[5] = service_port;
      argv[6] = NULL;

      /* Spawn mdns app */

      ret = posix_spawn(&g_mdnsd_config.pid, /* Returned Task ID            */
                        "mdns",              /* Task Path                   */
                        NULL,                /* File actions                */
                        NULL,                /* Attributes                  */
                        argv,                /* Arguments                   */
                        NULL);               /* No                          */

      if (ret < 0)
        {
          int errval = errno;
          syslog(LOG_ERR, "Failed to spawn mdns daemon\n");
          g_mdnsd_config.state = MDNSD_NOT_RUNNING;
          return -errval;
        }

      ninfo("Started\n");

      g_mdnsd_config.state = MDNSD_RUNNING;
    }

  sem_post(&g_mdnsd_config.lock);
  return OK;
}

