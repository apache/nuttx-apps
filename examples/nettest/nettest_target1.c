/****************************************************************************
 * apps/examples/nettest/nettest_target1.c
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

#include <sys/wait.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <debug.h>

#include "nettest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NETTEST_LOOPBACK
static int server_child(int argc, char *argv[])
{
  nettest_server();
  return EXIT_SUCCESS;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * nettest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_EXAMPLES_NETTEST_LOOPBACK
  pid_t child;
#ifdef CONFIG_SCHED_WAITPID
  int statloc;
#endif
#endif

  /* Parse any command line options */

  nettest_cmdline(argc, argv);

#ifdef CONFIG_EXAMPLES_NETTEST_INIT
  /* Initialize the network */

  nettest_initialize();
#endif

#if defined(CONFIG_EXAMPLES_NETTEST_LOOPBACK)
  /* Then perform the server side of the test on a child task */

  child = task_create("Nettest Child",
                      CONFIG_EXAMPLES_NETTEST_DAEMON_PRIORITY,
                      CONFIG_EXAMPLES_NETTEST_DAEMON_STACKSIZE,
                      server_child, NULL);
  if (child < 0)
    {
      fprintf(stderr, "ERROR: Failed to server daemon\n");
      return EXIT_FAILURE;
    }

  usleep(500 * 1000);

#elif defined(CONFIG_EXAMPLES_NETTEST_SERVER)
  /* Then perform the server side of the test on this thread */

  nettest_server();
#endif

#if !defined(CONFIG_EXAMPLES_NETTEST_SERVER) || \
    defined(CONFIG_EXAMPLES_NETTEST_LOOPBACK)
  /* Then perform the client side of the test on this thread */

  nettest_client();
#endif

#if defined(CONFIG_EXAMPLES_NETTEST_LOOPBACK) && defined(CONFIG_SCHED_WAITPID)
  printf("main: Waiting for the server to exit\n");
  waitpid(child, &statloc, 0);
#endif

  return EXIT_SUCCESS;
}
