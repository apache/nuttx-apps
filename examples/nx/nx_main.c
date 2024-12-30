/****************************************************************************
 * apps/examples/nx/nx_main.c
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

#include <sys/types.h>
#include <sys/boardctl.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxfonts.h>

#include "nx_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/
/* If not specified, assume that the hardware supports one video plane */

#ifndef CONFIG_EXAMPLES_NX_VPLANE
#  define CONFIG_EXAMPLES_NX_VPLANE 0
#endif

/* If not specified, assume that the hardware supports one LCD device */

#ifndef CONFIG_EXAMPLES_NX_DEVNO
#  define CONFIG_EXAMPLES_NX_DEVNO 0
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_exitcode = NXEXIT_SUCCESS;

static struct nxeg_state_s g_wstate[2];

#ifdef CONFIG_NX_KBD
static const uint8_t g_kbdmsg1[] = "NuttX is cool!";
static const uint8_t g_kbdmsg2[] = "NuttX is fun!";
#endif

/* The font handle */

NXHANDLE g_fonthandle;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The connection handler */

NXHANDLE g_hnx = NULL;

/* The screen resolution */

nxgl_coord_t g_xres;
nxgl_coord_t g_yres;

bool b_haveresolution = false;
bool g_connected = false;
sem_t g_semevent = {0};

/* Colors used to fill window 1 & 2 */

nxgl_mxpixel_t g_color1[CONFIG_NX_NPLANES];
nxgl_mxpixel_t g_color2[CONFIG_NX_NPLANES];
#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
nxgl_mxpixel_t g_tbcolor[CONFIG_NX_NPLANES];
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxeg_drivemouse
 ****************************************************************************/

#ifdef CONFIG_NX_XYINPUT
static void nxeg_drivemouse(void)
{
  nxgl_coord_t x;
  nxgl_coord_t y;
  nxgl_coord_t xstep = g_xres / 8;
  nxgl_coord_t ystep = g_yres / 8;

  for (x = 0; x < g_xres; x += xstep)
    {
      for (y = 0; y < g_yres; y += ystep)
        {
          printf("nxeg_drivemouse: Mouse left button at (%d,%d)\n", x, y);
          nx_mousein(g_hnx, x, y, NX_MOUSE_LEFTBUTTON);
        }
    }
}
#endif

/****************************************************************************
 * Name: nxeg_initstate
 ****************************************************************************/

static void nxeg_initstate(FAR struct nxeg_state_s *st, int wnum,
                           nxgl_mxpixel_t color)
{
#ifdef CONFIG_NX_KBD
  FAR const struct nx_font_s *fontset;
#endif

  /* Initialize the window number (used for debug output only) and color
   * (used for redrawing the window)
   */

  st->wnum     = wnum;
  st->color[0] = color;

  /* Get information about the font set being used and save this in the
   * state structure
   */

#ifdef CONFIG_NX_KBD
  fontset      = nxf_getfontset(g_fonthandle);
  st->nchars   = 0;
  st->nglyphs  = 0;
  st->height   = fontset->mxheight;
  st->width    = fontset->mxwidth;
  st->spwidth  = fontset->spwidth;
#endif
}

/****************************************************************************
 * Name: nxeg_freestate
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
static void nxeg_freestate(FAR struct nxeg_state_s *st)
{
#ifdef CONFIG_NX_KBD
  int i;

  if (st)
    {
      for (i = 0; i < st->nglyphs; i++)
        {
           if (st->glyph[i].bitmap)
              {
                free(st->glyph[i].bitmap);
              }
           st->glyph[i].bitmap = NULL;
        }
      st->nchars = 0;
     }
#endif
}
#endif

/****************************************************************************
 * Name: nxeg_openwindow
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NX_RAWWINDOWS
static inline NXEGWINDOW nxeg_openwindow(FAR const struct nx_callback_s *cb,
                                       FAR struct nxeg_state_s *state)
{
  NXEGWINDOW hwnd;

  hwnd = nx_openwindow(g_hnx, 0, cb, (FAR void *)state);
  if (!hwnd)
    {
      printf("nxeg_openwindow: nx_openwindow failed: %d\n", errno);
      g_exitcode = NXEXIT_NXOPENWINDOW;
    }
  return hwnd;
}
#else
static inline NXEGWINDOW nxeg_openwindow(FAR const struct nx_callback_s *cb,
                                         FAR struct nxeg_state_s *state)
{
  NXEGWINDOW hwnd;

  hwnd = nxtk_openwindow(g_hnx, 0, cb, (FAR void *)state);
  if (!hwnd)
    {
      printf("nxeg_openwindow: nxtk_openwindow failed: %d\n", errno);
      g_exitcode = NXEXIT_NXOPENWINDOW;
    }
  return hwnd;
}
#endif

/****************************************************************************
 * Name: nxeg_closewindow
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NX_RAWWINDOWS
static inline int nxeg_closewindow(NXEGWINDOW hwnd, FAR struct nxeg_state_s *state)
{
  int ret = nx_closewindow(hwnd);
  if (ret < 0)
    {
      printf("nxeg_closewindow: nx_closewindow failed: %d\n", errno);
      g_exitcode = NXEXIT_NXCLOSEWINDOW;
    }
  return ret;
}
#else
static inline int nxeg_closewindow(NXEGWINDOW hwnd, FAR struct nxeg_state_s *state)
{
  int ret = nxtk_closewindow(hwnd);
  if (ret < 0)
    {
      printf("nxeg_closewindow: nxtk_closewindow failed: %d\n", errno);
      g_exitcode = NXEXIT_NXCLOSEWINDOW;
    }
  nxeg_freestate(state);
  return ret;
}
#endif

/****************************************************************************
 * Name: nxeg_setsize
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NX_RAWWINDOWS
static inline int nxeg_setsize(NXEGWINDOW hwnd, FAR struct nxgl_size_s *size)
{
  int ret = nx_setsize(hwnd, size);
  if (ret < 0)
    {
      printf("nxeg_setsize: nx_setsize failed: %d\n", errno);
      g_exitcode = NXEXIT_NXSETSIZE;
    }
  return ret;
}
#else
static inline int nxeg_setsize(NXEGWINDOW hwnd, FAR struct nxgl_size_s *size)
{
  int ret = nxtk_setsize(hwnd, size);
  if (ret < 0)
    {
      printf("nxeg_setsize: nxtk_setsize failed: %d\n", errno);
      g_exitcode = NXEXIT_NXSETSIZE;
    }
  return ret;
}
#endif

/****************************************************************************
 * Name: nxeg_setposition
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NX_RAWWINDOWS
static inline int nxeg_setposition(NXEGWINDOW hwnd, FAR struct nxgl_point_s *pos)
{
  int ret = nx_setposition(hwnd, pos);
  if (ret < 0)
    {
      printf("nxeg_setposition: nx_setposition failed: %d\n", errno);
      g_exitcode = NXEXIT_NXSETPOSITION;
    }
  return ret;
}
#else
static inline int nxeg_setposition(NXEGWINDOW hwnd, FAR struct nxgl_point_s *pos)
{
  int ret = nxtk_setposition(hwnd, pos);
  if (ret < 0)
    {
      printf("nxeg_setposition: nxtk_setposition failed: %d\n", errno);
      g_exitcode = NXEXIT_NXSETPOSITION;
    }
  return ret;
}
#endif

/****************************************************************************
 * Name: nxeq_opentoolbar
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
static inline int nxeq_opentoolbar(NXEGWINDOW hwnd, nxgl_coord_t height,
                                   FAR const struct nx_callback_s *cb,
                                   FAR struct nxeg_state_s *state)
{
  int ret;
  ret = nxtk_opentoolbar(hwnd, height, cb, (FAR void *)state);
  if (ret < 0)
    {
      printf("nxeq_opentoolbar: nxtk_opentoolbar failed: %d\n", errno);
      g_exitcode = NXEXIT_NXOPENTOOLBAR;
    }
  return ret;
}
#endif

/****************************************************************************
 * Name: nxeg_lower
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NX_RAWWINDOWS
static inline int nxeg_lower(NXEGWINDOW hwnd)
{
  int ret = nx_lower(hwnd);
  if (ret < 0)
    {
      printf("nxeg_lower: nx_lower failed: %d\n", errno);
      g_exitcode = NXEXIT_NXLOWER;
    }
  return ret;
}
#else
static inline int nxeg_lower(NXEGWINDOW hwnd)
{
  int ret = nxtk_lower(hwnd);
  if (ret < 0)
    {
      printf("nxeg_lower: nxtk_lower failed: %d\n", errno);
      g_exitcode = NXEXIT_NXLOWER;
    }
  return ret;
}
#endif

/****************************************************************************
 * Name: nxeg_raise
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NX_RAWWINDOWS
static inline int nxeg_raise(NXEGWINDOW hwnd)
{
  int ret = nx_raise(hwnd);
  if (ret < 0)
    {
      printf("nxeg_raise: nx_raise failed: %d\n", errno);
      g_exitcode = NXEXIT_NXRAISE;
    }
  return ret;
}
#else
static inline int nxeg_raise(NXEGWINDOW hwnd)
{
  int ret = nxtk_raise(hwnd);
  if (ret < 0)
    {
      printf("nxeg_raise: nxtk_raise failed: %d\n", errno);
      g_exitcode = NXEXIT_NXRAISE;
    }
  return ret;
}
#endif

/****************************************************************************
 * Name: nxeg_initialize
 ****************************************************************************/

static int nxeg_initialize(void)
{
  struct sched_param param;
  pthread_t thread;
  int ret;
  int i;

  /* Initialize window colors */

  for (i = 0; i < CONFIG_NX_NPLANES; i++)
    {
      g_color1[i]  = CONFIG_EXAMPLES_NX_COLOR1;
      g_color2[i]  = CONFIG_EXAMPLES_NX_COLOR2;
#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
      g_tbcolor[i] = CONFIG_EXAMPLES_NX_TBCOLOR;
#endif
    }

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_NX_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      printf("nxeg_initialize: sched_setparam failed: %d\n" , ret);
      g_exitcode = NXEXIT_SCHEDSETPARAM;
      return ERROR;
    }

  /* Start the NX server kernel thread */

  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      printf("nxeg_initialize: Failed to start the NX server: %d\n", errno);
      g_exitcode = NXEXIT_TASKCREATE;
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
       param.sched_priority = CONFIG_EXAMPLES_NX_LISTENERPRIO;
       pthread_attr_setschedparam(&attr, &param);
       pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NX_STACKSIZE);

       ret = pthread_create(&thread, &attr, nx_listenerthread, NULL);
       if (ret != 0)
         {
            printf("nxeg_initialize: pthread_create failed: %d\n", ret);
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
      printf("nxeg_initialize: nx_connect failed: %d\n", errno);
      g_exitcode = NXEXIT_NXCONNECT;
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nx_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  NXEGWINDOW hwnd1;
  NXEGWINDOW hwnd2;
  struct nxgl_size_s size;
  struct nxgl_point_s pt;
  nxgl_mxpixel_t color;
  int ret;

  /* Initialize */

  ret = nxeg_initialize();
  printf("nx_main: NX handle=%p\n", g_hnx);
  if (!g_hnx || ret < 0)
    {
      printf("nx_main: Failed to get NX handle: %d\n", errno);
      g_exitcode = NXEXIT_NXOPEN;
      goto errout;
    }

  /* Get the default font handle */

  g_fonthandle = nxf_getfonthandle(CONFIG_EXAMPLES_NX_FONTID);
  if (!g_fonthandle)
    {
      printf("nx_main: Failed to get font handle: %d\n", errno);
      g_exitcode = NXEXIT_FONTOPEN;
      goto errout;
    }

  /* Set the background to the configured background color */

  printf("nx_main: Set background color=%d\n", CONFIG_EXAMPLES_NX_BGCOLOR);
  color = CONFIG_EXAMPLES_NX_BGCOLOR;
  ret = nx_setbgcolor(g_hnx, &color);
  if (ret < 0)
    {
      printf("nx_main: nx_setbgcolor failed: %d\n", errno);
      g_exitcode = NXEXIT_NXSETBGCOLOR;
      goto errout_with_nx;
    }

  /* Create window #1 */

  printf("nx_main: Create window #1\n");
  nxeg_initstate(&g_wstate[0], 1, CONFIG_EXAMPLES_NX_COLOR1);
  hwnd1 = nxeg_openwindow(&g_nxcb, &g_wstate[0]);
  printf("nx_main: hwnd1=%p\n", hwnd1);
  if (!hwnd1)
    {
      goto errout_with_nx;
    }

  /* Wait until we have the screen resolution */

  while (!b_haveresolution)
    {
      sem_wait(&g_semevent);
    }
  printf("nx_main: Screen resolution (%d,%d)\n", g_xres, g_yres);

  /* Set the size of the window 1 */

  size.w = g_xres / 2;
  size.h = g_yres / 2;

  printf("nx_main: Set window #1 size to (%d,%d)\n", size.w, size.h);
  ret = nxeg_setsize(hwnd1, &size);
  if (ret < 0)
    {
      goto errout_with_hwnd1;
    }

  /* Sleep a bit -- both so that we can see the result of the above operations
   * but also, in the multi-user case, so that the server can get a chance to
   * actually do them!
   */

  printf("nx_main: Sleeping\n\n");
  sleep(1);

  /* Set the position of window #1 */

  pt.x = g_xres / 8;
  pt.y = g_yres / 8;

  printf("nx_main: Set window #1 position to (%d,%d)\n", pt.x, pt.y);
  ret = nxeg_setposition(hwnd1, &pt);
  if (ret < 0)
    {
      goto errout_with_hwnd1;
    }

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);

  /* Open the toolbar */

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
  printf("nx_main: Add toolbar to window #1\n");
  ret = nxeq_opentoolbar(hwnd1, CONFIG_EXAMPLES_NX_TOOLBAR_HEIGHT, &g_tbcb, &g_wstate[0]);
  if (ret < 0)
    {
      goto errout_with_hwnd1;
    }

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);
#endif

  /* Create window #2 */

  printf("nx_main: Create window #2\n");
  nxeg_initstate(&g_wstate[1], 2, CONFIG_EXAMPLES_NX_COLOR2);
  hwnd2 = nxeg_openwindow(&g_nxcb, &g_wstate[1]);
  printf("nx_main: hwnd2=%p\n", hwnd2);
  if (!hwnd2)
    {
      goto errout_with_hwnd1;
    }

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);

  /* Set the size of the window 2 == size of window 1*/

  printf("nx_main: Set hwnd2 size to (%d,%d)\n", size.w, size.h);
  ret = nxeg_setsize(hwnd2, &size);
  if (ret < 0)
    {
      goto errout_with_hwnd2;
    }

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);

  /* Set the position of window #2 */

  pt.x = g_xres - size.w - pt.x;
  pt.y = g_yres - size.h - pt.y;

  printf("nx_main: Set hwnd2 position to (%d,%d)\n", pt.x, pt.y);
  ret = nxeg_setposition(hwnd2, &pt);
  if (ret < 0)
    {
      goto errout_with_hwnd2;
    }

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
  printf("nx_main: Add toolbar to window #2\n");
  ret = nxeq_opentoolbar(hwnd2, CONFIG_EXAMPLES_NX_TOOLBAR_HEIGHT, &g_tbcb, &g_wstate[1]);
  if (ret < 0)
    {
      goto errout_with_hwnd2;
    }

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);
#endif

  /* Give keyboard input to the top window -- should be window #2 */

#ifdef CONFIG_NX_KBD
  printf("nx_main: Send keyboard input: %s\n", g_kbdmsg1);
  ret = nx_kbdin(g_hnx, strlen((FAR const char *)g_kbdmsg1), g_kbdmsg1);
  if (ret < 0)
    {
      printf("nx_main: nx_kbdin failed: %d\n", errno);
      goto errout_with_hwnd2;
    }

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);
#endif

  /* Lower window 2 */

  printf("nx_main: Lower window #2\n");
  ret = nxeg_lower(hwnd2);
  if (ret < 0)
    {
      goto errout_with_hwnd2;
    }

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);

  /* Put mouse left-button clicks all over the screen and see who responds */

#ifdef CONFIG_NX_XYINPUT
  nxeg_drivemouse();

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);
#endif

  /* Give keyboard input to the top window -- should be window #1 */

#ifdef CONFIG_NX_KBD
  printf("nx_main: Send keyboard input: %s\n", g_kbdmsg2);
  ret = nx_kbdin(g_hnx, strlen((FAR const char *)g_kbdmsg2), g_kbdmsg2);
  if (ret < 0)
    {
      printf("nx_main: nx_kbdin failed: %d\n", errno);
      goto errout_with_hwnd2;
    }

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(1);
#endif

  /* Raise window 2 */

  printf("nx_main: Raise window #2\n");
  ret = nxeg_raise(hwnd2);
  if (ret < 0)
    {
      goto errout_with_hwnd2;
    }

  /* Put mouse left-button clicks all over the screen and see who responds */

#ifdef CONFIG_NX_XYINPUT
  nxeg_drivemouse();
#endif

  /* Sleep a bit */

  printf("nx_main: Sleeping\n\n");
  sleep(2);

  /* Close the window 2 */

errout_with_hwnd2:
  printf("nx_main: Close window #2\n");
  nxeg_closewindow(hwnd2, &g_wstate[1]);

  /* Close the window1 */

errout_with_hwnd1:
  printf("nx_main: Close window #1\n");
  nxeg_closewindow(hwnd1, &g_wstate[0]);

errout_with_nx:
  /* Disconnect from the server */

  printf("nx_main: Disconnect from the server\n");
  nx_disconnect(g_hnx);

errout:
  return g_exitcode;
}
