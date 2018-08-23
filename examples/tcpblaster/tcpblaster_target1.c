/****************************************************************************
 * examples/tcpblaster/tcpblaster_target1.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include "config.h"

#include <sys/wait.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <debug.h>

#include "tcpblaster.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK
static int server_child(int argc, char *argv[])
{
  tcpblaster_server();
  return EXIT_SUCCESS;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * tcpblaster_main
 ****************************************************************************/

#if defined(CONFIG_BUILD_LOADABLE)
int main(int argc, FAR char *argv[])
#elif defined(CONFIG_EXAMPLES_TCPBLASTER_TARGET2)
int tcpblaster1_main(int argc, char *argv[])
#else
int tcpblaster_main(int argc, char *argv[])
#endif
{
#ifdef CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK
  pid_t child;
#ifdef CONFIG_SCHED_WAITPID
  int statloc;
#endif
#endif

  /* Parse any command line options */

  tcpblaster_cmdline(argc, argv);

#ifdef CONFIG_EXAMPLES_TCPBLASTER_INIT
  /* Initialize the network */

  tcpblaster_initialize();
#endif

#if defined(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK)
  /* Then perform the server side of the test on a child task */

  child = task_create("TcpBlaster Child", CONFIG_EXAMPLES_TCPBLASTER_DAEMON_PRIORITY,
                      CONFIG_EXAMPLES_TCPBLASTER_DAEMON_STACKSIZE, server_child,
                      NULL);
  if (child < 0)
    {
      fprintf(stderr, "ERROR: Failed to server daemon\n");
      return EXIT_FAILURE;
    }

  usleep(500*1000);

#elif defined(CONFIG_EXAMPLES_TCPBLASTER_SERVER)
  /* Then perform the server side of the test on this thread */

  tcpblaster_server();
#endif

#if !defined(CONFIG_EXAMPLES_TCPBLASTER_SERVER) || \
    defined(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK)
  /* Then perform the client side of the test on this thread */

  tcpblaster_client();
#endif

#if defined(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK) && defined(CONFIG_SCHED_WAITPID)
  printf("main: Waiting for the server to exit\n");
  (void)waitpid(child, &statloc, 0);
#endif

  return EXIT_SUCCESS;
}
