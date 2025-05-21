/****************************************************************************
 * apps/examples/mdnsd/mdnsd_daemon.c
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

#include "netutils/netlib.h"
#include "netutils/mdnsd.h"
#include "mdnsd_daemon.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/* Configuration Checks *****************************************************/

/* BEWARE:
 * There are other configuration settings needed in netutils/mdnsd/mdnsdc.c,
 * but there are default values for those so we cannot check them here.
 */

#ifndef CONFIG_NET
#  error "You must define CONFIG_NET"
#endif

#ifndef CONFIG_NET_UDP
#  error "You must define CONFIG_NET_UDP"
#endif

#ifndef CONFIG_NET_BROADCAST
#  error "You must define CONFIG_NET_BROADCAST"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * mdnsd_showusage
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mdnsd_daemon
 ****************************************************************************/

int mdnsd_daemon(int argc, FAR char *argv[], bool start)
{
  /* No arguments are needed for the example app */

  if (start)
    {
      return mdnsd_start(CONFIG_EXAMPLES_MDNS_SERVICE,
                        CONFIG_EXAMPLES_MDNS_SERVICE_PORT);
    }
  else
    {
      return mdnsd_stop();
    }
}
