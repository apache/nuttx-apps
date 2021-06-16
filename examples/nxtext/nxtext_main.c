/****************************************************************************
 * apps/examples/nxtext/nxtext_main.c
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
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>

#ifdef CONFIG_NX_LCDDRIVER
#  include <nuttx/lcd/lcd.h>
#else
#  include <nuttx/video/fb.h>
#endif

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

#include "nxtext_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/
/* If not specified, assume that the hardware supports one video plane */

#ifndef CONFIG_EXAMPLES_NXTEXT_VPLANE
#  define CONFIG_EXAMPLES_NXTEXT_VPLANE 0
#endif

/* If not specified, assume that the hardware supports one LCD device */

#ifndef CONFIG_EXAMPLES_NXTEXT_DEVNO
#  define CONFIG_EXAMPLES_NXTEXT_DEVNO 0
#endif

#define BGMSG_LINES 24

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static const uint8_t g_pumsg[] = "Pop-Up!";
#endif

static const char *g_bgmsg[BGMSG_LINES] =
{
  "\nJULIET\n",                           /* Line 1 */
  "Wilt thou be gone?\n",                 /* Line 2 */
  "  It is not yet near day:\n",          /* Line 3 */
  "It was the nightingale,\n",            /* Line 4 */
  "  and not the lark,\n",                /* Line 5 */
  "That pierced the fearful hollow\n",    /* Line 6 */
  "  of thine ear;\n",                    /* Line 7 */
  "Nightly she sings\n",                  /* Line 8 */
  "  on yon pomegranate-tree:\n",         /* Line 9 */
  "Believe me, love,\n",                  /* Line 10 */
  "  it was the nightingale.\n",          /* Line 11 */
  "\nROMEO\n",                            /* Line 12 */
  "It was the lark,\n",                   /* Line 13 */
  "  the herald of the morn,\n",          /* Line 14 */
  "No nightingale:\n",                    /* Line 15 */
  "  look, love, what envious streaks\n", /* Line 16 */
  "Do lace the severing clouds\n",        /* Line 17 */
  "  in yonder east:\n",                  /* Line 18 */
  "Night's candles are burnt out,\n",     /* Line 19 */
  "  and jocund day\n",                   /* Line 20 */
  "Stands tiptoe\n",                      /* Line 21 */
  "  on the misty mountain tops.\n",      /* Line 22 */
  "I must be gone and live,\n",           /* Line 23 */
  "  or stay and die.\n"                  /* Line 24 */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The connection handler */

NXHANDLE g_hnx = NULL;

/* The font handles */

NXHANDLE g_bghfont = NULL;
NXHANDLE g_puhfont = NULL;

/* The screen resolution */

nxgl_coord_t g_xres;
nxgl_coord_t g_yres;

bool b_haveresolution = false;
bool g_connected = false;
sem_t g_semevent = {0};

int g_exitcode = NXEXIT_SUCCESS;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxtext_initialize
 ****************************************************************************/

static int nxtext_initialize(void)
{
  struct sched_param param;
  pthread_t thread;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_NXTEXT_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      printf("nxtext_initialize: sched_setparam failed: %d\n" , ret);
      g_exitcode = NXEXIT_SCHEDSETPARAM;
      return ERROR;
    }

  /* Start the NX server kernel thread */

  printf("nxtext_initialize: Starting NX server\n");
  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      printf("nxtext_initialize: Failed to start the NX server: %d\n", errno);
      g_exitcode = NXEXIT_SERVERSTART;
      return ERROR;
    }

  /* Connect to the server */

  g_hnx = nx_connect();
  if (g_hnx)
    {
      pthread_attr_t attr;

#ifdef CONFIG_VNCSERVER
      /* Setup the VNC server to support keyboard/mouse inputs */

       struct boardioc_vncstart_s vnc =
       {
         0, g_hnx
       };

       ret = boardctl(BOARDIOC_VNC_START, (uintptr_t)&vnc);
       if (ret < 0)
         {
           printf("boardctl(BOARDIOC_VNC_START) failed: %d\n", ret);
           nx_disconnect(g_hnx);
           g_exitcode = NXEXIT_FBINITIALIZE;
           return ERROR;
         }
#endif

      /* Start a separate thread to listen for server events.  This is probably
       * the least efficient way to do this, but it makes this example flow more
       * smoothly.
       */

      pthread_attr_init(&attr);
      param.sched_priority = CONFIG_EXAMPLES_NXTEXT_LISTENERPRIO;
      pthread_attr_setschedparam(&attr, &param);
      pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NXTEXT_STACKSIZE);

      ret = pthread_create(&thread, &attr, nxtext_listener, NULL);
      if (ret != 0)
        {
           printf("nxtext_initialize: pthread_create failed: %d\n", ret);

           g_exitcode = NXEXIT_PTHREADCREATE;
           return ERROR;
        }

      /* Don't return until we are connected to the server */

      while (!g_connected)
        {
          /* Wait for the listener thread to wake us up when we really
           * are connected.
           */

          sem_wait(&g_semevent);
        }
    }
  else
    {
      printf("nxtext_initialize: nx_connect failed: %d\n", errno);

      g_exitcode = NXEXIT_NXCONNECT;
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxtext_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct nxtext_state_s *bgstate;
  NXWINDOW hwnd = NULL;
  nxgl_mxpixel_t color;
  int popcnt;
  int bkgndx;
  int ret;

  /* Initialize NX */

  ret = nxtext_initialize();
  printf("nxtext_main: NX handle=%p\n", g_hnx);
  if (!g_hnx || ret < 0)
    {
      printf("nxtext_main: Failed to get NX handle: %d\n", errno);
      g_exitcode = NXEXIT_NXOPEN;
      goto errout;
    }

  /* Get the configured font handles */

  g_bghfont = nxf_getfonthandle(CONFIG_EXAMPLES_NXTEXT_BGFONTID);
  if (!g_bghfont)
    {
      printf("nxtext_main: Failed to get background font handle: %d\n", errno);
      g_exitcode = NXEXIT_FONTOPEN;
      goto errout;
    }

  g_puhfont = nxf_getfonthandle(CONFIG_EXAMPLES_NXTEXT_PUFONTID);
  if (!g_puhfont)
    {
      printf("nxtext_main: Failed to get pop-up font handle: %d\n", errno);
      g_exitcode = NXEXIT_FONTOPEN;
      goto errout;
    }

  /* Set the background to the configured background color */

  printf("nxtext_main: Set background color=%d\n", CONFIG_EXAMPLES_NXTEXT_BGCOLOR);
  color = CONFIG_EXAMPLES_NXTEXT_BGCOLOR;
  ret = nx_setbgcolor(g_hnx, &color);
  if (ret < 0)
    {
      printf("nxtext_main: nx_setbgcolor failed: %d\n", errno);
      g_exitcode = NXEXIT_NXSETBGCOLOR;
      goto errout_with_nx;
    }

  /* Get the background window */

  bgstate = nxbg_getstate();
  ret = nx_requestbkgd(g_hnx, &g_nxtextcb, bgstate);
  if (ret < 0)
    {
      printf("nxtext_main: nx_requestbkgd failed: %d\n", errno);
      g_exitcode = NXEXIT_NXREQUESTBKGD;
      goto errout_with_nx;
    }

  /* Wait until we have the screen resolution.  */

  while (!b_haveresolution)
    {
      sem_wait(&g_semevent);
    }

  printf("nxtext_main: Screen resolution (%d,%d)\n", g_xres, g_yres);

  /* Now loop, adding text to the background and periodically presenting
   * a pop-up window.
   */

  popcnt = 0;
  bkgndx = 0;
  for (;;)
    {
      /* Sleep for one second */

      sleep(1);
      popcnt++;

      /* Each three seconds, create a pop-up window.  Destroy the pop-up
       * window after two more seconds.
       */

      if (popcnt == 3)
        {
          /* Create a pop-up window */

          hwnd = nxpu_open();

#ifdef CONFIG_NX_KBD
          /* Give keyboard input to the top window (which should be the pop-up) */

          printf("nxtext_main: Send keyboard input: %s\n", g_pumsg);
          ret = nx_kbdin(g_hnx, strlen((FAR const char *)g_pumsg), g_pumsg);
          if (ret < 0)
           {
             printf("nxtext_main: nx_kbdin failed: %d\n", errno);
             goto errout_with_hwnd;
           }
#endif
        }
      else if (popcnt == 5)
        {
          /* Destroy the pop-up window and restart the sequence */

          printf("nxtext_main: Close pop-up\n");
          nxpu_close(hwnd);
          popcnt = 0;
        }

      /* Give another line of text to the background window.  Force this
       * text to go the background by calling the background window interfaces
       * directly.
       */

      nxbg_write(g_bgwnd, (FAR const uint8_t *)g_bgmsg[bkgndx], strlen(g_bgmsg[bkgndx]));
      if (++bkgndx >= BGMSG_LINES)
        {
          bkgndx = 0;
        }
    }

  /* Close the pop-up window */

#ifdef CONFIG_NX_KBD
errout_with_hwnd:
#endif
  if (popcnt >= 3)
    {
      printf("nxtext_main: Close pop-up\n");
     nxpu_close(hwnd);
    }

//errout_with_bkgd:
  nx_releasebkgd(g_bgwnd);

errout_with_nx:
  /* Disconnect from the server */

  printf("nxtext_main: Disconnect from the server\n");
  nx_disconnect(g_hnx);

errout:
  return g_exitcode;
}
