/****************************************************************************
 * apps/examples/udp/udp_target2.c
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

#include "config.h"
#include "udp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * udp2_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Parse any command line options */

  udp_cmdline(argc, argv);

#ifdef CONFIG_EXAMPLES_UDP_NETINIT
  /* Initialize the network */

  udp_netinit();
#endif

  /* Run the server or client, depending upon how target1 was configured */

#ifdef CONFIG_EXAMPLES_UDP_SERVER1
  udp_client();
#else
  udp_server();
#endif

  return 0;
}
