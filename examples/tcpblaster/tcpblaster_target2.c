/****************************************************************************
 * apps/examples/tcpblaster/tcpblaster_target2.c
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

#include "config.h"
#include <stdlib.h>
#include "tcpblaster.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * tcpblaster_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Parse any command line options */

  tcpblaster_cmdline(argc, argv);

#ifdef CONFIG_EXAMPLES_TCPBLASTER_INIT
  /* Initialize the network */

  tcpblaster_initialize();
#endif

#if defined(CONFIG_EXAMPLES_TCPBLASTER_SERVER)
  /* Then perform the client side of the test on this thread */

  tcpblaster_client();
#else
  /* Then perform the server side of the test on this thread */

  tcpblaster_server();
#endif

  return EXIT_SUCCESS;
}
