/****************************************************************************
 * examples/nxterm/nxterm_main.c
 *
 *   Copyright (C) 2012, 2016-2017 Gregory Nutt. All rights reserved.
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

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/boardctl.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#ifdef CONFIG_NX_LCDDRIVER
#  include <nuttx/lcd/lcd.h>
#else
#  include <nuttx/video/fb.h>
#  ifdef CONFIG_VNCSERVER
#    include <nuttx/video/vnc.h>
#  endif
#endif

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>
#include <nuttx/nx/nxterm.h>

#include "platform/cxxinitialize.h"
#include "nshlib/nshlib.h"

#include "nxterm_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The NSH telnet console requires networking support (and TCP/IP) */

#ifndef CONFIG_NET
#  undef CONFIG_NSH_TELNET
#endif

/* If Telnet is used and both IPv6 and IPv4 are enabled, then we need to
 * pick one.
 */

#ifdef CONFIG_NET_IPv6
#  define ADDR_FAMILY AF_INET6
#else
#  define ADDR_FAMILY AF_INET
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* All example global variables are retained in a structure to minimize
 * the chance of name collisions.
 */

struct nxterm_state_s g_nxterm_vars;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxterm_initialize
 ****************************************************************************/

static int nxterm_initialize(void)
{
  struct sched_param param;
  pthread_t thread;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_NXTERM_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      printf("nxterm_initialize: sched_setparam failed: %d\n" , ret);
      return ERROR;
    }

  /* Start the NX server kernel thread */

  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      printf("nxterm_initialize: Failed to start the NX server: %d\n", errno);
      return ERROR;
    }

  /* Connect to the server */

  g_nxterm_vars.hnx = nx_connect();
  if (g_nxterm_vars.hnx)
    {
       pthread_attr_t attr;

#ifdef CONFIG_VNCSERVER
       /* Setup the VNC server to support keyboard/mouse inputs */

      ret = vnc_default_fbinitialize(0, g_nxterm_vars.hnx);
      if (ret < 0)
        {
          printf("vnc_default_fbinitialize failed: %d\n", ret);
          return ERROR;
        }
#endif
       /* Start a separate thread to listen for server events.  This is probably
        * the least efficient way to do this, but it makes this example flow more
        * smoothly.
        */

       (void)pthread_attr_init(&attr);
       param.sched_priority = CONFIG_EXAMPLES_NXTERM_LISTENERPRIO;
       (void)pthread_attr_setschedparam(&attr, &param);
       (void)pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NXTERM_STACKSIZE);

       ret = pthread_create(&thread, &attr, nxterm_listener, NULL);
       if (ret != 0)
         {
            printf("nxterm_initialize: pthread_create failed: %d\n", ret);
            return ERROR;
         }

       /* Don't return until we are connected to the server */

       while (!g_nxterm_vars.connected)
         {
           /* Wait for the listener thread to wake us up when we really
            * are connected.
            */

           (void)sem_wait(&g_nxterm_vars.eventsem);
         }
    }
  else
    {
      printf("nxterm_initialize: nx_connect failed: %d\n", errno);
      return ERROR;
    }
  return OK;
}

/****************************************************************************
 * Name: nxterm_task
 ****************************************************************************/

static int nxterm_task(int argc, char **argv)
{
  /* If the console front end is selected, then run it on this thread */

#ifdef CONFIG_NSH_CONSOLE
  (void)nsh_consolemain(0, NULL);
#endif

  printf("nxterm_task: Unregister the NX console device\n");
  (void)nxterm_unregister(g_nxterm_vars.hdrvr);

  printf("nxterm_task: Close the window\n");
  (void)nxtk_closewindow(g_nxterm_vars.hwnd);

  /* Disconnect from the server */

  printf("nxterm_task: Disconnect from the server\n");
  nx_disconnect(g_nxterm_vars.hnx);

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxterm_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int nxterm_main(int argc, char **argv)
#endif
{
  nxgl_mxpixel_t color;
  int fd;
  int ret;

  /* General Initialization *************************************************/
  /* Reset all global data */

  printf("nxterm_main: Started\n");
  memset(&g_nxterm_vars, 0, sizeof(struct nxterm_state_s));

  /* Call all C++ static constructors */

#if defined(CONFIG_HAVE_CXX) && defined(CONFIG_HAVE_CXXINITIALIZE)
  up_cxxinitialize();
#endif

  /* NSH Initialization *****************************************************/
  /* Initialize the NSH library */

  printf("nxterm_main: Initialize NSH\n");
  nsh_initialize();

  /* If the Telnet console is selected as a front-end, then start the
   * Telnet daemon.
   */

#ifdef CONFIG_NSH_TELNET
  ret = nsh_telnetstart(ADDR_FAMILY);
  if (ret < 0)
    {
     /* The daemon is NOT running.  Report the error then fail...
      * either with the serial console up or just exiting.
      */

     fprintf(stderr, "ERROR: Failed to start TELNET daemon: %d\n", ret);
   }
#endif
  /* NX Initialization ******************************************************/
  /* Initialize NX */

  printf("nxterm_main: Initialize NX\n");
  ret = nxterm_initialize();
  printf("nxterm_main: NX handle=%p\n", g_nxterm_vars.hnx);
  if (!g_nxterm_vars.hnx || ret < 0)
    {
      printf("nxterm_main: Failed to get NX handle: %d\n", errno);
      goto errout;
    }

  /* Set the background to the configured background color */

  printf("nxterm_main: Set background color=%d\n", CONFIG_EXAMPLES_NXTERM_BGCOLOR);
  color = CONFIG_EXAMPLES_NXTERM_BGCOLOR;
  ret = nx_setbgcolor(g_nxterm_vars.hnx, &color);
  if (ret < 0)
    {
      printf("nxterm_main: nx_setbgcolor failed: %d\n", errno);
      goto errout_with_nx;
    }

  /* Window Configuration ***************************************************/
  /* Create a window */

  printf("nxterm_main: Create window\n");
  g_nxterm_vars.hwnd = nxtk_openwindow(g_nxterm_vars.hnx, &g_nxtermcb, NULL);
  if (!g_nxterm_vars.hwnd)
    {
      printf("nxterm_main: nxtk_openwindow failed: %d\n", errno);
      goto errout_with_nx;
    }
  printf("nxterm_main: hwnd=%p\n", g_nxterm_vars.hwnd);

  /* Wait until we have the screen resolution.  */

  while (!g_nxterm_vars.haveres)
    {
      (void)sem_wait(&g_nxterm_vars.eventsem);
    }
  printf("nxterm_main: Screen resolution (%d,%d)\n", g_nxterm_vars.xres, g_nxterm_vars.yres);

  /* Determine the size and position of the window */

  g_nxterm_vars.wndo.wsize.w = g_nxterm_vars.xres / 2 + g_nxterm_vars.xres / 4;
  g_nxterm_vars.wndo.wsize.h = g_nxterm_vars.yres / 2 + g_nxterm_vars.yres / 4;

  g_nxterm_vars.wpos.x       = g_nxterm_vars.xres / 8;
  g_nxterm_vars.wpos.y       = g_nxterm_vars.yres / 8;

  /* Set the window position */

  printf("nxterm_main: Set window position to (%d,%d)\n",
         g_nxterm_vars.wpos.x, g_nxterm_vars.wpos.y);

  ret = nxtk_setposition(g_nxterm_vars.hwnd, &g_nxterm_vars.wpos);
  if (ret < 0)
    {
      printf("nxterm_main: nxtk_setposition failed: %d\n", errno);
      goto errout_with_hwnd;
    }

  /* Set the window size */

  printf("nxterm_main: Set window size to (%d,%d)\n",
         g_nxterm_vars.wndo.wsize.w, g_nxterm_vars.wndo.wsize.h);

  ret = nxtk_setsize(g_nxterm_vars.hwnd, &g_nxterm_vars.wndo.wsize);
  if (ret < 0)
    {
      printf("nxterm_main: nxtk_setsize failed: %d\n", errno);
      goto errout_with_hwnd;
    }

  /* Open the toolbar */

  printf("nxterm_main: Add toolbar to window\n");
  ret = nxtk_opentoolbar(g_nxterm_vars.hwnd, CONFIG_EXAMPLES_NXTERM_TOOLBAR_HEIGHT, &g_nxtoolcb, NULL);
  if (ret < 0)
    {
      printf("nxterm_main: nxtk_opentoolbar failed: %d\n", errno);
      goto errout_with_hwnd;
    }

  /* Sleep a little bit to allow the server to catch up */

  sleep(2);

  /* NxTerm Configuration ************************************************/
  /* Use the window to create an NX console */

  g_nxterm_vars.wndo.wcolor[0] = CONFIG_EXAMPLES_NXTERM_WCOLOR;
  g_nxterm_vars.wndo.fcolor[0] = CONFIG_EXAMPLES_NXTERM_FONTCOLOR;
  g_nxterm_vars.wndo.fontid    = CONFIG_EXAMPLES_NXTERM_FONTID;

  g_nxterm_vars.hdrvr = nxtk_register(g_nxterm_vars.hwnd, &g_nxterm_vars.wndo, CONFIG_EXAMPLES_NXTERM_MINOR);
  if (!g_nxterm_vars.hdrvr)
    {
      printf("nxterm_main: nxtk_register failed: %d\n", errno);
      goto errout_with_hwnd;
    }

  /* Open the NxTerm driver */

  fd = open(CONFIG_EXAMPLES_NXTERM_DEVNAME, O_WRONLY);
  if (fd < 0)
    {
      printf("nxterm_main: open %s read-only failed: %d\n",
             CONFIG_EXAMPLES_NXTERM_DEVNAME, errno);
      goto errout_with_driver;
    }

  /* Start Console Task *****************************************************/
  /* Now re-direct stdout and stderr so that they use the NX console driver.
   * Note that stdin is retained (file descriptor 0, probably the serial console).
    */

   printf("nxterm_main: Starting the console task\n");
   fflush(stdout);

  (void)fflush(stdout);
  (void)fflush(stderr);

  (void)fclose(stdout);
  (void)fclose(stderr);

  (void)dup2(fd, 1);
  (void)dup2(fd, 2);

   /* And we can close our original driver file descriptor */

   close(fd);

   /* And start the console task.  It will inherit stdin, stdout, and stderr
    * from this task.
    */

   g_nxterm_vars.pid = task_create("NxTerm", CONFIG_EXAMPLES_NXTERM_PRIO,
                                  CONFIG_EXAMPLES_NXTERM_STACKSIZE,
                                  nxterm_task, NULL);
   DEBUGASSERT(g_nxterm_vars.pid > 0);
   return EXIT_SUCCESS;

  /* Error Exits ************************************************************/

errout_with_driver:
  (void)nxterm_unregister(g_nxterm_vars.hdrvr);

errout_with_hwnd:
  (void)nxtk_closewindow(g_nxterm_vars.hwnd);

errout_with_nx:
  /* Disconnect from the server */

  nx_disconnect(g_nxterm_vars.hnx);
errout:
  return EXIT_FAILURE;
}
