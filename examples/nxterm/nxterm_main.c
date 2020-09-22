/****************************************************************************
 * examples/nxterm/nxterm_main.c
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
#endif

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>
#include <nuttx/nx/nxterm.h>

#include "nshlib/nshlib.h"

#include "nxterm_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The NSH telnet console requires networking support (and TCP/IP) */

#ifndef CONFIG_NET
#  undef CONFIG_NSH_TELNET
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
      printf("nxterm_initialize: Failed to start the NX server: %d\n",
             errno);
      return ERROR;
    }

  /* Connect to the server */

  g_nxterm_vars.hnx = nx_connect();
  if (g_nxterm_vars.hnx)
    {
       pthread_attr_t attr;

#ifdef CONFIG_VNCSERVER
      /* Setup the VNC server to support keyboard/mouse inputs */

      struct boardioc_vncstart_s vnc =
      {
        0, g_nxterm_vars.hnx
      };

      ret = boardctl(BOARDIOC_VNC_START, (uintptr_t)&vnc);
      if (ret < 0)
        {
          printf("boardctl(BOARDIOC_VNC_START) failed: %d\n", ret);
          nx_disconnect(g_nxterm_vars.hnx);
          return ERROR;
        }
#endif

      /* Start a separate thread to listen for server events.  This is
       * probably the least efficient way to do this, but it makes this
       * example flow more smoothly.
       */

      pthread_attr_init(&attr);
      param.sched_priority = CONFIG_EXAMPLES_NXTERM_LISTENERPRIO;
      pthread_attr_setschedparam(&attr, &param);
      pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NXTERM_STACKSIZE);

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

          sem_wait(&g_nxterm_vars.eventsem);
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
  nsh_consolemain(argc, argv);
#endif

  printf("nxterm_task: Unlinking the NX console device\n");
  unlink(CONFIG_EXAMPLES_NXTERM_DEVNAME);

  printf("nxterm_task: Close the window\n");
  nxtk_closewindow(g_nxterm_vars.hwnd);

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

int main(int argc, FAR char *argv[])
{
  struct boardioc_nxterm_create_s nxcreate;
  nxgl_mxpixel_t color;
  int fd;
  int ret;

  /* General Initialization *************************************************/

  /* Reset all global data */

  printf("nxterm_main: Started\n");
  memset(&g_nxterm_vars, 0, sizeof(struct nxterm_state_s));

  /* NSH Initialization *****************************************************/

  /* Initialize the NSH library */

  printf("nxterm_main: Initialize NSH\n");
  nsh_initialize();

  /* If the Telnet console is selected as a front-end, then start the
   * Telnet daemon.
   */

#ifdef CONFIG_NSH_TELNET
  ret = nsh_telnetstart(AF_UNSPEC);
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

  printf("nxterm_main: Set background color=%d\n",
         CONFIG_EXAMPLES_NXTERM_BGCOLOR);

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
  g_nxterm_vars.hwnd = nxtk_openwindow(g_nxterm_vars.hnx, 0, &g_nxtermcb,
                                       NULL);
  if (!g_nxterm_vars.hwnd)
    {
      printf("nxterm_main: nxtk_openwindow failed: %d\n", errno);
      goto errout_with_nx;
    }

  printf("nxterm_main: hwnd=%p\n", g_nxterm_vars.hwnd);

  /* Wait until we have the screen resolution.  */

  while (!g_nxterm_vars.haveres)
    {
      sem_wait(&g_nxterm_vars.eventsem);
    }

  printf("nxterm_main: Screen resolution (%d,%d)\n",
         g_nxterm_vars.xres, g_nxterm_vars.yres);

  /* Determine the size and position of the window */

  g_nxterm_vars.wndo.wsize.w = g_nxterm_vars.xres / 2 +
                               g_nxterm_vars.xres / 4;
  g_nxterm_vars.wndo.wsize.h = g_nxterm_vars.yres / 2 +
                               g_nxterm_vars.yres / 4;

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
  ret = nxtk_opentoolbar(g_nxterm_vars.hwnd,
                         CONFIG_EXAMPLES_NXTERM_TOOLBAR_HEIGHT,
                         &g_nxtoolcb, NULL);
  if (ret < 0)
    {
      printf("nxterm_main: nxtk_opentoolbar failed: %d\n", errno);
      goto errout_with_hwnd;
    }

  /* Sleep a little bit to allow the server to catch up */

  sleep(2);

  /* NxTerm Configuration ***************************************************/

  /* Use the window to create an NX console */

  g_nxterm_vars.wndo.wcolor[0] = CONFIG_EXAMPLES_NXTERM_WCOLOR;
  g_nxterm_vars.wndo.fcolor[0] = CONFIG_EXAMPLES_NXTERM_FONTCOLOR;
  g_nxterm_vars.wndo.fontid    = CONFIG_EXAMPLES_NXTERM_FONTID;

  nxcreate.nxterm              = NULL;
  nxcreate.hwnd                = g_nxterm_vars.hwnd;
  nxcreate.wndo                = g_nxterm_vars.wndo;
  nxcreate.type                = BOARDIOC_XTERM_FRAMED;
  nxcreate.minor               = CONFIG_EXAMPLES_NXTERM_MINOR;

  /* BOARDIOC_NXTERM wants the size of the NxTK main sub-window */

  nxcreate.wndo.wsize.w       -= (2 * CONFIG_NXTK_BORDERWIDTH);
  nxcreate.wndo.wsize.h       -= (CONFIG_EXAMPLES_NXTERM_TOOLBAR_HEIGHT +
                                  2 * CONFIG_NXTK_BORDERWIDTH);

  ret = boardctl(BOARDIOC_NXTERM, (uintptr_t)&nxcreate);
  if (ret < 0)
    {
      printf("nxterm_main: boardctl(BOARDIOC_NXTERM) failed: %d\n", errno);
      goto errout_with_hwnd;
    }

  g_nxterm_vars.hdrvr = nxcreate.nxterm;
  DEBUGASSERT(g_nxterm_vars.hdrvr != NULL);

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
   * Note that stdin is retained (file descriptor 0, probably the serial
   * console).
   */

  printf("nxterm_main: Starting the console task\n");

  fflush(stdout);
  fflush(stderr);

  fclose(stdout);
  fclose(stderr);

  dup2(fd, 1);
  dup2(fd, 2);

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
  unlink(CONFIG_EXAMPLES_NXTERM_DEVNAME);

errout_with_hwnd:
  nxtk_closewindow(g_nxterm_vars.hwnd);

errout_with_nx:

  /* Disconnect from the server */

  nx_disconnect(g_nxterm_vars.hnx);

errout:
  return EXIT_FAILURE;
}
