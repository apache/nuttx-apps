/****************************************************************************
 * apps/examples/nettest/nettest_target2.c
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
#include "nettest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * nettest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Parse any command line options */

  nettest_cmdline(argc, argv);

#ifdef CONFIG_EXAMPLES_NETTEST_INIT
  /* Initialize the network */

  nettest_initialize();
#endif

#if defined(CONFIG_EXAMPLES_NETTEST_SERVER)
  /* Then perform the client side of the test on this thread */

  nettest_client();
#else
  /* Then perform the server side of the test on this thread */

  nettest_server();
#endif

  return EXIT_SUCCESS;
}
