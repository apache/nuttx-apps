/****************************************************************************
 * apps/examples/pwlines/pwlines_main.c
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
#include <fixedmath.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxbe.h>

#include "pwlines_internal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwlines_server_initialize
 ****************************************************************************/

static bool pwlines_server_initialize(FAR struct pwlines_state_s *st)
{
  struct sched_param param;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_PWLINES_CLIENT_PRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      printf("pwlines_server_initialize: ERROR: "
             "sched_setparam failed: %d\n" ,
             ret);
      return false;
    }

  /* Start the NX server kernel thread */

  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      printf("pwlines_server_initialize: ERROR: "
             "Failed to start the NX server: %d\n",
             errno);
      return false;
    }

  /* Connect to the server */

  st->hnx = nx_connect();
  if (st->hnx)
    {
#ifdef CONFIG_VNCSERVER
      /* Setup the VNC server to support keyboard/mouse inputs */

       struct boardioc_vncstart_s vnc =
       {
         0, st->hnx
       };

       ret = boardctl(BOARDIOC_VNC_START, (uintptr_t)&vnc);
       if (ret < 0)
         {
           printf("pwlines_server_initialize: ERROR: "
                  "boardctl(BOARDIOC_VNC_START) failed: %d\n",
                  ret);

           nx_disconnect(st->hnx);
           return false;
         }
#endif
    }
  else
    {
      printf("pwlines_server_initialize: ERROR: "
             "nx_connect failed: %d\n",
             errno);
      return false;
    }

  return true;
}

/****************************************************************************
 * Name: pwlines_listener_initialize
 ****************************************************************************/

static bool pwlines_listener_initialize(FAR struct pwlines_state_s *st)
{
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t thread;
  int ret;

  /* Start a separate thread to listen for server events.  This is probably
   * the least efficient way to do this, but it makes this example flow more
   * smoothly.
   */

  pthread_attr_init(&attr);
  param.sched_priority = CONFIG_EXAMPLES_PWLINES_LISTENER_PRIO;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_PWLINES_LISTENER_STACKSIZE);

  ret = pthread_create(&thread, &attr, pwlines_listener, st);
  if (ret != 0)
    {
       printf("pwlines_listener_initialize: ERROR: "
              "pthread_create failed: %d\n",
              ret);
       return false;
    }

  /* Don't return until we are connected to the server */

  while (!st->connected)
    {
      /* Wait for the listener thread to wake us up when we really
       * are connected.
       */

      sem_wait(&st->semevent);
    }

  return true;
}

/****************************************************************************
 * Name: pwlines_state_initialize
 ****************************************************************************/

static bool pwlines_state_initialize(FAR struct pwlines_state_s *st)
{
  /* Initialize semaphores */

  sem_init(&st->semevent, 0, 0);

  /* Initialize color information (only a single color plane supported) */

  st->wndo[0].color[0] = CONFIG_EXAMPLES_PWLINES_COLOR1;
  st->wndo[1].color[0] = CONFIG_EXAMPLES_PWLINES_COLOR2;
  st->wndo[2].color[0] = CONFIG_EXAMPLES_PWLINES_COLOR3;

  st->facecolor[0]     = CONFIG_EXAMPLES_PWLINES_FACECOLOR;
  st->bordercolor[0]   = CONFIG_EXAMPLES_PWLINES_BORDERCOLOR;
  st->linecolor[0]     = CONFIG_EXAMPLES_PWLINES_LINECOLOR;

  return true;
}

/****************************************************************************
 * Name: pwlines_configure_window
 ****************************************************************************/

static bool pwlines_configure_window(FAR struct pwlines_state_s *st, int wndx,
                                     FAR struct nxgl_size_s *size,
                                     FAR struct nxgl_point_s *pos,
                                     double deltax, double deltay)
{
  FAR struct pwlines_window_s *wndo = &st->wndo[wndx];
  struct nxgl_rect_s rect;
  int ret;

  /* Set the total size of the window */

  printf("pwlines_configure_window: Set window %d size to (%d,%d)\n",
         wndx + 1, size->w, size->h);

  ret = nxtk_setsize(wndo->hwnd, size);
  if (ret < 0)
    {
      printf("pwlines_configure_window: ERROR: "
             "nxtk_setsize failed: %d\n", errno);
      goto errout_with_hwnd;
    }

  /* Set the drawable size (excludes borders) */

  wndo->size.h = size->h - 2 * CONFIG_NXTK_BORDERWIDTH;
  wndo->size.w = size->w - 2 * CONFIG_NXTK_BORDERWIDTH;

  /* Set the position of window */

  printf("pwlines_configure_window: Set window %d position to (%d,%d)\n",
         wndx + 1, pos->x, pos->y);

  ret = nxtk_setposition(wndo->hwnd, pos);
  if (ret < 0)
    {
      printf("pwlines_configure_window: ERROR: "
             "nxtk_setposition failed: %d\n",
             errno);
      goto errout_with_hwnd;
    }

  /* Create a bounding box.  This is actually too large because it does not
   * account for the boarder widths.  However, NX should clip the fill to
   * stay within the frame.
   *
   * There is a race condition here we resolve by making the main thread
   * lowest in priority.  In order for the size and position to take effect,
   * a command is sent to server which responds with an event.  So we need
   * to be synchronized at this point or the following fill will fail because
   * it depends on current knowlede of the size and position.
   * REVISIT: Use nx_synch()!
   */

  rect.pt1.x = 0;
  rect.pt1.y = 0;
  rect.pt2.x = size->w - 1;
  rect.pt2.y = size->h - 1;

  /* Fill the window with the selected color */

  ret = nxtk_fillwindow(wndo->hwnd, &rect, wndo->color);
  if (ret < 0)
    {
      printf("pwlines_configure_window: ERROR: "
             "nxtk_fillwindow failed: %d\n",
             errno);
      goto errout_with_hwnd;
    }

  /* Set up for motion */

  wndo->xmax   = itob16(st->xres - size->w - 1);
  wndo->ymax   = itob16(st->yres - size->h - 1);
  wndo->ypos   = itob16(pos->y);
  wndo->xpos   = itob16(pos->x);
  wndo->deltax = dtob16(deltax);
  wndo->deltay = dtob16(deltay);

  return true;

errout_with_hwnd:
  printf("pwlines_configure_window: Close window %d\n", wndx + 1);

  ret = nxtk_closewindow(wndo->hwnd);
  if (ret < 0)
    {
      printf("pwlines_configure_window: ERROR: "
             "nxtk_closewindow failed: %d\n",
             errno);
    }

  return false;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwlines_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct pwlines_state_s wstate;
  struct nxgl_size_s size;
  struct nxgl_point_s pos;
  nxgl_coord_t mindim;
  nxgl_mxpixel_t color;
  unsigned int elapsed;
  int errcode = EXIT_SUCCESS;
  int ret;

  /* Connect to the NX server */

  memset(&wstate, 0, sizeof(struct pwlines_state_s));
  if (!pwlines_server_initialize(&wstate) || wstate.hnx == NULL)
    {
      printf("pwlines_main: ERROR: "
             "Failed to get NX handle\n");
      goto errout;
    }

  printf("pwlines_main: NX handle=%p\n", wstate.hnx);

  /* Start the listener thread */

  if (!pwlines_listener_initialize(&wstate))
    {
      printf("pwlines_main: ERROR: "
             "pwlines_listener_initialize failed\n");
      goto errout_with_nx;
    }

  /* Initialize the window state, colors, cache, etc. */

  if (!pwlines_state_initialize(&wstate))
    {
      printf("pwlines_main: ERROR: "
             "pwlines_state_initialize failed\n");
      goto errout_with_nx;
    }

  /* Set the background to the configured background color */

  printf("pwlines_main: Set background color=%d\n",
         CONFIG_EXAMPLES_PWLINES_BGCOLOR);

  color = CONFIG_EXAMPLES_PWLINES_BGCOLOR;
  ret = nx_setbgcolor(wstate.hnx, &color);
  if (ret < 0)
    {
      printf("pwlines_main: nx_setbgcolor failed: %d\n", errno);
      goto errout_with_nx;
    }

  /* Open window 1 */

  printf("pwlines_main: Open window 1\n");

  wstate.wndo[0].hwnd = nxtk_openwindow(wstate.hnx, NXBE_WINDOW_RAMBACKED,
                                        &g_pwlines_wncb, (FAR void *)&wstate);
  if (wstate.wndo[0].hwnd == NULL)
    {
      printf("pwlines_main: ERROR: "
             "nxtk_openwindow failed: %d\n",
             errno);
      goto errout_with_nx;
    }

  printf("pwlines_main: hwnd1=%p\n", wstate.wndo[0].hwnd);

  /* Wait until we receive the screen resolution from the server.  We only
   * need to do this once after opening the first window.
   */

  while (!wstate.haveres)
    {
      sem_wait(&wstate.semevent);
    }

  printf("pwlines_main: Screen resolution (%d,%d)\n",
         wstate.xres, wstate.yres);

  /* Configure window 1 */

  mindim = MIN(wstate.xres, wstate.yres);
  size.w = mindim / 2;
  size.h = size.w;

  pos.x  = wstate.xres / 8;
  pos.y  = wstate.yres / 8;

  if (!pwlines_configure_window(&wstate, 0, &size, &pos, 4.200, 4.285))
    {
      printf("pwlines_main: ERROR: "
             "pwlines_configure_window failed for window 1\n");
      goto errout_with_hwnd1;
    }

  /* Open window 2 */

  printf("pwlines_main: Open window 2\n");

  wstate.wndo[1].hwnd = nxtk_openwindow(wstate.hnx, NXBE_WINDOW_RAMBACKED,
                                        &g_pwlines_wncb, (FAR void *)&wstate);
  if (wstate.wndo[1].hwnd == NULL)
    {
      printf("pwlines_main: ERROR: "
             "nxtk_openwindow failed: %d\n",
             errno);
      goto errout_with_hwnd1;
    }

  printf("pwlines_main: hwnd1=%p\n", wstate.wndo[1].hwnd);

  /* Configure window 2 (same size) */

  pos.x  = wstate.xres / 4;
  pos.y  = wstate.yres / 4;

  if (!pwlines_configure_window(&wstate, 1, &size, &pos, -3.317, 5.0))
    {
      printf("pwlines_main: ERROR: "
             "pwlines_configure_window failed for window 2\n");
      goto errout_with_hwnd2;
    }

  /* Open window 3 */

  printf("pwlines_main: Open window 3\n");

  wstate.wndo[2].hwnd = nxtk_openwindow(wstate.hnx, NXBE_WINDOW_RAMBACKED,
                                        &g_pwlines_wncb, (FAR void *)&wstate);
  if (wstate.wndo[2].hwnd == NULL)
    {
      printf("pwlines_main: ERROR: "
             "nxtk_openwindow failed: %d\n",
             errno);
      goto errout_with_hwnd2;
    }

  printf("pwlines_main: hwnd2=%p\n", wstate.wndo[2].hwnd);

  /* Configure window 3 (same size) */

  pos.x = (3 * wstate.xres) / 8;
  pos.y = (3 * wstate.yres) / 8;

  if (!pwlines_configure_window(&wstate, 2, &size, &pos, 4.600, -3.852))
    {
      printf("pwlines_main: ERROR: "
             "pwlines_configure_window failed for window 2\n");
      goto errout_with_hwnd3;
    }

  /* Apply the initial graphics */

  pwlines_circle(&wstate);
  pwlines_update(&wstate);

  /* Now loop animating the windows */

  elapsed = 0;
  for (; ; )
    {
      usleep(CONFIG_EXAMPLES_PWLINES_RATECONTROL * 1000);

      if (!pwlines_motion(&wstate))
        {
          printf("pwlines_main: ERROR:"
                 "pwlines_motion failed\n");
          goto errout_with_hwnd3;
        }

      elapsed += CONFIG_EXAMPLES_PWLINES_RATECONTROL;
      if (elapsed >= 1000)
        {
          pwlines_update(&wstate);
          elapsed = 0;
        }
    }

  errcode = EXIT_SUCCESS;

  /* Close window 3 */

errout_with_hwnd3:
  printf("pwlines_main: Close window #2\n");

  ret = nxtk_closewindow(wstate.wndo[2].hwnd);
  if (ret < 0)
    {
      printf("pwlines_main: ERROR: nxtk_closewindow failed: %d\n", errno);
    }

  /* Close window 2 */

errout_with_hwnd2:
  printf("pwlines_main: Close window #2\n");

  ret = nxtk_closewindow(wstate.wndo[1].hwnd);
  if (ret < 0)
    {
      printf("pwlines_main: ERROR: nxtk_closewindow failed: %d\n", errno);
    }

  /* Close window1 */

errout_with_hwnd1:
  printf("pwlines_main: Close window #1\n");

  ret = nxtk_closewindow(wstate.wndo[0].hwnd);
  if (ret < 0)
    {
      printf("pwlines_main: ERROR: nxtk_closewindow failed: %d\n", errno);
    }

errout_with_nx:
  /* Disconnect from the server */

  printf("pwlines_main: Disconnect from the server\n");
  nx_disconnect(wstate.hnx);

errout:
  return errcode;
}
