/****************************************************************************
 * apps/system/fbvnc/fbvnc_main.c
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

/* A VNC server for any framebuffer, no application cooperation required.
 *
 *   fbvnc start [/dev/fb0] [--diff | --full]
 *   fbvnc stop
 *   fbvnc status
 *
 * It is a service rather than part of an application:  it serves whatever
 * is on the framebuffer, and the application that draws there neither
 * knows nor needs to.  Starting it before or after that application makes
 * no difference, and it survives the application being restarted.
 *
 * The framebuffer is mapped and served as-is;  what changed comes from the
 * kernel's dirty-area reporting (FBIOC_WATCHAREA), which every application
 * that issues FBIO_UPDATE feeds, LVGL's fbdev driver among them. An
 * An application that writes to the framebuffer without saying so, a game
 * rendering straight into the mapping, say, is served with --diff, which
 * keeps a copy of the last frame sent and compares against it, or with
 * --full, a whole frame per update request.
 *
 * Remote input goes to the uinput devices when they exist:  the pointer as
 * touch samples on /dev/utouch, keys as keyboard events on /dev/ukeyboard.
 *
 * TODO:  known limitation, with LVGL as the application.
 *
 *   A key of LVGL's on-screen keyboard, clicked once, is typed twice, and
 *   holding the click types the key over and over.  Everything up to the
 *   device is known good:  one click writes exactly one TOUCH_DOWN and one
 *   TOUCH_UP here, and the samples arrive at /dev/utouch.
 *
 *   The suspect is lv_nuttx_touchscreen.c, which remembers the state of
 *   the last sample but not its position, while LVGL clears the sample
 *   structure before every read.  A pointer that reports only when
 *   something changes, this one, or any touch controller with an
 *   interrupt, therefore reads as pressed at the origin between its
 *   samples, which is a press leaving the widget and coming back.
 *   Repeating the position while the button is held was tried and did not
 *   change the doubling, so this is not established, only where to look
 *   next:  count what the driver delivers per click against what
 *   lv_buttonmatrix acts on.
 *
 *   Typing into LVGL widgets from a remote keyboard does not work at all,
 *   for a plainer reason:  LVGL's NuttX port has a touchscreen driver and
 *   no keyboard one, so nothing reads /dev/ukeyboard into an indev.  An
 *   application that reads the keyboard itself, lvglterm does, is
 *   unaffected, and works.
 * Whatever reads those, an LVGL touchscreen driver, an lvglterm,
 * receives the remote user exactly as it would a local one.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <string.h>
#include <unistd.h>

#include <sched.h>
#include <signal.h>

#include <nuttx/input/keyboard.h>
#include <nuttx/input/kbd_codec.h>
#include <nuttx/input/touchscreen.h>
#include <nuttx/input/x11_keysym.h>
#include <nuttx/video/fb.h>

#include <netutils/fbvnc.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FBVNC_RFB_BUTTON1  (1 << 0)

/* Rows compared as a unit in --diff.  Larger bands mean fewer memcmp calls
 * and coarser rectangles;  16 is the height of a Hextile tile, so a band
 * that changed costs whole tiles either way.
 */

#define FBVNC_BAND         16

/* The only pixel format served;  the framebuffer is refused at startup
 * if it is anything else.
 */

#define FBVNC_BYTESPP      2

/* Keys held down at once, tracked so that a client that vanishes mid-game
 * does not leave the application with Ctrl still down.  Enough for the
 * modifiers plus a couple of ordinary keys.
 */

#define FBVNC_MAXHELD      8

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fbvnc_key_s
{
  uint32_t code;
  uint32_t type;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Everything about one served framebuffer.  The server hands this back
 * to every callback, so nothing here has to be reachable through a
 * global.
 */

struct fbvnc_daemon_s
{
  int fbfd;
  int touchfd;
  int kbdfd;
  FAR const uint8_t *fb;
  uint16_t width;
  uint16_t height;
  bool fullonly;
  bool diff;
  bool watching;
  FAR uint8_t *shadow;
  uint32_t stride;
  uint32_t fblen;
  volatile bool invalid;
  uint8_t buttons;

  /* Keys currently down, with the event type that releases each:  a
   * special key is released with KEYBOARD_SPECREL, and releasing it as
   * an ordinary key would report a character instead.
   */

  struct fbvnc_key_s held[FBVNC_MAXHELD];
  uint8_t nheld;
};

/* The daemon's task id, and the only thing the start and stop commands
 * share.  Stopping is a signal to that task rather than a write into its
 * state, which is what makes it work when the two commands do not share
 * an address space.
 */

static pid_t g_daemon = -1;

/* The running server, reachable from the signal handler.  A POSIX
 * handler takes no argument of its own, so this is the only way it can
 * name the instance;  it lives inside the daemon, not across the two
 * commands.
 */

static FAR struct fbvnc_s *g_server;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fbvnc_snapshot
 *
 * Description:
 *   Runs on the server thread.  Drains the kernel's dirty queue;  a full
 *   screen when asked to start over or when running --full;  in --diff,
 *   the bands that differ from the last frame sent.
 *
 ****************************************************************************/

static FAR const uint8_t *
fbvnc_snapshot(FAR void *ctx, FAR struct fbvnc_rect_s *rects,
               uint32_t maxrects, FAR uint32_t *nrects)
{
  FAR struct fbvnc_daemon_s *dev = ctx;
  struct fb_area_s area;
  uint32_t total = 0;
  uint32_t n = 0;
  uint32_t idx;

  if (dev->invalid || dev->fullonly)
    {
      /* Drop anything stale first */

      while (ioctl(dev->fbfd, FBIOC_GETDIRTY, (unsigned long)&area) == OK);

      rects[0].x = 0;
      rects[0].y = 0;
      rects[0].w = dev->width;
      rects[0].h = dev->height;
      *nrects = 1;
      dev->invalid = false;

      if (dev->shadow != NULL)
        {
          memcpy(dev->shadow, dev->fb, dev->fblen);
        }

      return dev->fb;
    }

  if (dev->diff && !dev->watching)
    {
      uint32_t bandlen = dev->stride * FBVNC_BAND;
      uint32_t y;

      for (y = 0; y < dev->height && n < maxrects; y += FBVNC_BAND)
        {
          uint32_t rows = MIN(FBVNC_BAND, dev->height - y);
          uint32_t off  = y * dev->stride;
          uint32_t len  = rows == FBVNC_BAND ? bandlen : rows * dev->stride;

          if (memcmp(dev->shadow + off, dev->fb + off, len) == 0)
            {
              continue;
            }

          memcpy(dev->shadow + off, dev->fb + off, len);

          /* Bands that changed together are one rectangle */

          if (n > 0 && rects[n - 1].y + rects[n - 1].h == y)
            {
              rects[n - 1].h += rows;
              continue;
            }

          rects[n].x = 0;
          rects[n].y = y;
          rects[n].w = dev->width;
          rects[n].h = rows;
          n++;
        }

      *nrects = n;
      return dev->fb;
    }

  while (ioctl(dev->fbfd, FBIOC_GETDIRTY, (unsigned long)&area) == OK)
    {
      uint32_t best = maxrects;
      int32_t bestcost = 0;
      uint32_t i;

      /* Areas reported for one redraw overlap:  a widget, then the panel
       * it sits on, then the screen behind that.  Sending each of them
       * sends the same pixels several times over. A tab change was
       * measured at fifteen rectangles covering four and a half
       * screens, and a burst that large empties the network buffers
       * and stalls the send for seconds.
       *
       * So areas are joined, but only where joining is not itself
       * expensive:  the rectangle around two of them can be far larger
       * than the two, and answering a change in two corners by sending
       * everything between them is how a small edit becomes a whole
       * screen.  The cost of a join is what the surrounding rectangle
       * covers beyond the pair;  a join that costs nothing is taken, and
       * the cheapest one wins.
       */

      for (i = 0; i < n; i++)
        {
          uint16_t x1 = MIN(rects[i].x, area.x);
          uint16_t y1 = MIN(rects[i].y, area.y);
          uint16_t x2 = MAX(rects[i].x + rects[i].w, area.x + area.w);
          uint16_t y2 = MAX(rects[i].y + rects[i].h, area.y + area.h);
          int32_t cost = (int32_t)((uint32_t)(x2 - x1) * (y2 - y1)) -
                         (int32_t)((uint32_t)rects[i].w * rects[i].h) -
                         (int32_t)((uint32_t)area.w * area.h);

          if (best == maxrects || cost < bestcost)
            {
              best = i;
              bestcost = cost;
            }
        }

      /* Somewhere to put it:  a join worth making, or room for one more.
       * With neither, the cheapest join is taken anyway, a rectangle
       * too many is worse than a rectangle too large.
       */

      if (best < n && (bestcost <= 0 || n == maxrects))
        {
          uint16_t x2 = MAX(rects[best].x + rects[best].w, area.x + area.w);
          uint16_t y2 = MAX(rects[best].y + rects[best].h, area.y + area.h);

          rects[best].x = MIN(rects[best].x, area.x);
          rects[best].y = MIN(rects[best].y, area.y);
          rects[best].w = x2 - rects[best].x;
          rects[best].h = y2 - rects[best].y;
        }
      else
        {
          rects[n].x = area.x;
          rects[n].y = area.y;
          rects[n].w = area.w;
          rects[n].h = area.h;
          n++;
        }
    }

  /* What is going to be sent, which is not what arrived:  the areas
   * overlap, and counting them as they came calls a fraction of the
   * screen a screenful and sends everything for no reason.
   */

  for (idx = 0; idx < n; idx++)
    {
      total += (uint32_t)rects[idx].w * rects[idx].h;
    }

  /* What the application reported is what it redrew, which is not the
   * same as what changed:  a toolkit that animates one element inside a
   * panel invalidates the panel, and half a screen arrives as dirty for
   * the sake of a moving handle.  Where there is a shadow to compare
   * against, each reported area is narrowed to the bands inside it that
   * actually differ, bounded by the area, not by the screen, so it
   * costs a fraction of comparing everything.
   */

  if (dev->shadow != NULL)
    {
      uint32_t out = 0;

      for (idx = 0; idx < n; idx++)
        {
          uint32_t rowlen = (uint32_t)rects[idx].w * FBVNC_BYTESPP;
          int32_t first = -1;
          int32_t last = -1;
          uint16_t y;

          for (y = 0; y < rects[idx].h; y++)
            {
              uint32_t off = (uint32_t)(rects[idx].y + y) * dev->stride +
                             (uint32_t)rects[idx].x * FBVNC_BYTESPP;

              if (memcmp(dev->shadow + off, dev->fb + off, rowlen) != 0)
                {
                  if (first < 0)
                    {
                      first = y;
                    }

                  last = y;
                  memcpy(dev->shadow + off, dev->fb + off, rowlen);
                }
            }

          if (first < 0)
            {
              continue;      /* reported, but nothing in it changed */
            }

          rects[out]    = rects[idx];
          rects[out].y += first;
          rects[out].h  = last - first + 1;
          out++;
        }

      n = out;
      total = 0;

      for (idx = 0; idx < n; idx++)
        {
          total += (uint32_t)rects[idx].w * rects[idx].h;
        }
    }

  /* Past a whole screen's worth there is nothing to be gained by being
   * careful about which parts changed
   */

  if (total >= (uint32_t)dev->width * dev->height)
    {
      rects[0].x = 0;
      rects[0].y = 0;
      rects[0].w = dev->width;
      rects[0].h = dev->height;
      n = 1;
    }

  *nrects = n;
  return dev->fb;
}

/****************************************************************************
 * Name: fbvnc_invalidate
 ****************************************************************************/

static void fbvnc_invalidate(FAR void *ctx)
{
  FAR struct fbvnc_daemon_s *dev = ctx;

  dev->invalid = true;
}

/****************************************************************************
 * Name: fbvnc_track_key
 *
 * Description:
 *   Remember what is down, so that it can be let go of if the client
 *   disappears while holding it.
 *
 ****************************************************************************/

static void fbvnc_track_key(FAR struct fbvnc_daemon_s *dev, uint32_t code,
                            uint32_t type, bool pressed)
{
  uint8_t i;

  for (i = 0; i < dev->nheld; i++)
    {
      if (dev->held[i].code == code)
        {
          if (!pressed)
            {
              dev->held[i] = dev->held[--dev->nheld];
            }

          return;
        }
    }

  if (pressed && dev->nheld < FBVNC_MAXHELD)
    {
      dev->held[dev->nheld].code = code;
      dev->held[dev->nheld].type = type == KEYBOARD_SPECPRESS ?
                             KEYBOARD_SPECREL : KEYBOARD_RELEASE;
      dev->nheld++;
    }
}

/****************************************************************************
 * Name: fbvnc_release_input
 *
 * Description:
 *   A client that vanishes mid-press must not leave the UI with a finger
 *   or a key stuck down.
 *
 ****************************************************************************/

static void fbvnc_release_input(FAR struct fbvnc_daemon_s *dev)
{
  if ((dev->buttons & FBVNC_RFB_BUTTON1) != 0 && dev->touchfd >= 0)
    {
      struct touch_sample_s sample;

      memset(&sample, 0, sizeof(sample));
      sample.npoints        = 1;
      sample.point[0].flags = TOUCH_UP | TOUCH_ID_VALID;
      write(dev->touchfd, &sample, sizeof(sample));
    }

  while (dev->nheld > 0 && dev->kbdfd >= 0)
    {
      struct keyboard_event_s ev;

      dev->nheld--;
      ev.code = dev->held[dev->nheld].code;
      ev.type = dev->held[dev->nheld].type;
      write(dev->kbdfd, &ev, sizeof(ev));
    }

  dev->buttons = 0;
  dev->nheld   = 0;
}

/****************************************************************************
 * Name: fbvnc_on_disconnect
 ****************************************************************************/

static void fbvnc_on_disconnect(FAR void *ctx)
{
  FAR struct fbvnc_daemon_s *dev = ctx;

  fbvnc_release_input(dev);
}

/****************************************************************************
 * Name: fbvnc_pointer
 *
 * Description:
 *   Button edges and drags become touch samples;  hover is nothing, which
 *   is the contract a touch consumer expects.
 *
 ****************************************************************************/

static void fbvnc_pointer(FAR void *ctx, uint16_t x, uint16_t y,
                          uint8_t buttons)
{
  FAR struct fbvnc_daemon_s *dev = ctx;

  struct touch_sample_s sample;
  uint8_t pressed    = buttons & FBVNC_RFB_BUTTON1;
  uint8_t waspressed = dev->buttons & FBVNC_RFB_BUTTON1;

  dev->buttons = buttons;

  if (dev->touchfd < 0 || (!pressed && !waspressed))
    {
      return;
    }

  memset(&sample, 0, sizeof(sample));
  sample.npoints        = 1;
  sample.point[0].x     = x;
  sample.point[0].y     = y;
  sample.point[0].flags = TOUCH_ID_VALID | TOUCH_POS_VALID;

  if (pressed && !waspressed)
    {
      sample.point[0].flags |= TOUCH_DOWN;
    }
  else if (pressed)
    {
      sample.point[0].flags |= TOUCH_MOVE;
    }
  else
    {
      sample.point[0].flags |= TOUCH_UP;
    }

  write(dev->touchfd, &sample, sizeof(sample));
}

/****************************************************************************
 * Name: fbvnc_key
 *
 * Description:
 *   Printables pass through, RFB sends them already shifted. Enter is
 *   a line feed, the arrows and their kin go as SPEC events with kbd_codec
 *   keycodes:  all four event types, so nothing is dropped silently.
 *
 ****************************************************************************/

static void fbvnc_key(FAR void *ctx, uint32_t keysym, bool pressed)
{
  FAR struct fbvnc_daemon_s *dev = ctx;

  struct keyboard_event_s ev;
  uint32_t type = pressed ? KEYBOARD_PRESS : KEYBOARD_RELEASE;
  uint32_t code;

  if (dev->kbdfd < 0)
    {
      return;
    }

  if (keysym >= 0x20 && keysym <= 0x7e)
    {
      code = keysym;
    }
  else
    {
      switch (keysym)
        {
          case XK_Return:
          case XK_KP_Enter:
            code = '\n';
            break;

          case XK_BackSpace:
            code = '\b';
            break;

          case XK_Tab:
            code = '\t';
            break;

          case XK_Escape:
            code = 0x1b;
            break;

          case XK_Up:
            code = KEYCODE_UP;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          case XK_Down:
            code = KEYCODE_DOWN;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          case XK_Left:
            code = KEYCODE_LEFT;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          case XK_Right:
            code = KEYCODE_RIGHT;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          case XK_Delete:
            code = KEYCODE_FWDDEL;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          case XK_Insert:
            code = KEYCODE_INSERT;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          case XK_Page_Up:
            code = KEYCODE_PAGEUP;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          case XK_Page_Down:
            code = KEYCODE_PAGEDOWN;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          /* The modifiers are keys in their own right to whatever is
           * reading:  a game binds fire to Ctrl and strafe to Alt, and
           * dropping them as decoration leaves it unplayable.
           */

          case XK_Control_L:
          case XK_Control_R:
            code = KEYCODE_LCTRL;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          case XK_Shift_L:
          case XK_Shift_R:
            code = KEYCODE_LSHIFT;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          case XK_Alt_L:
          case XK_Alt_R:
            code = KEYCODE_LALT;
            type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
            break;

          default:
            if (keysym >= XK_F1 && keysym <= XK_F12)
              {
                code = KEYCODE_F1 + (keysym - XK_F1);
                type = pressed ? KEYBOARD_SPECPRESS : KEYBOARD_SPECREL;
                break;
              }

            return;
        }
    }

  fbvnc_track_key(dev, code, type, pressed);

  ev.code = code;
  ev.type = type;
  write(dev->kbdfd, &ev, sizeof(ev));
}

/****************************************************************************
 * Name: fbvnc_sigterm
 *
 * Description:
 *   Nothing to do:  the signal itself is what makes the server loop
 *   leave, through fbvnc_stop() below.  The handler exists so that the
 *   default action, killing the task outright, does not run instead.
 *
 ****************************************************************************/

static void fbvnc_sigterm(int signo)
{
  UNUSED(signo);
  fbvnc_stop(g_server);
}

static int fbvnc_daemon(int argc, FAR char *argv[])
{
  FAR struct fbvnc_daemon_s dev_s;
  FAR struct fbvnc_daemon_s *dev = &dev_s;
  FAR struct fbvnc_s *server;
  struct sigaction act;
  struct fbvnc_cfg_s cfg;
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  FAR const char *fbdev = "/dev/fb0";
  int i;

  memset(dev, 0, sizeof(*dev));
  dev->fbfd    = -1;
  dev->touchfd = -1;
  dev->kbdfd   = -1;
  dev->invalid = true;

  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] != '-')
        {
          /* The framebuffer to serve, said plainly rather than behind a
           * flag:  which screen this is about is the one thing the
           * command is always about.
           */

          fbdev = argv[i];
        }
      else if (strcmp(argv[i], "--full") == 0)
        {
          dev->fullonly = true;
        }
      else if (strcmp(argv[i], "--diff") == 0)
        {
          dev->diff = true;
        }
      else
        {
          fprintf(stderr, "fbvnc: unknown option %s\n", argv[i]);
          g_daemon = -1;
          return EXIT_FAILURE;
        }
    }

  dev->fbfd = open(fbdev, O_RDWR);
  if (dev->fbfd < 0)
    {
      perror("fbvnc: cannot open framebuffer");
      g_daemon = -1;
      return EXIT_FAILURE;
    }

  if (ioctl(dev->fbfd, FBIOGET_VIDEOINFO, (unsigned long)&vinfo) < 0 ||
      ioctl(dev->fbfd, FBIOGET_PLANEINFO, (unsigned long)&pinfo) < 0)
    {
      perror("fbvnc: cannot query framebuffer");
      g_daemon = -1;
      return EXIT_FAILURE;
    }

  if (pinfo.bpp != 16)
    {
      fprintf(stderr, "fbvnc: %u bpp framebuffer; only 16 is served\n",
              pinfo.bpp);
      g_daemon = -1;
      return EXIT_FAILURE;
    }

  dev->fb = mmap(NULL, pinfo.fblen, PROT_READ, MAP_SHARED | MAP_FILE,
              dev->fbfd, 0);
  if (dev->fb == MAP_FAILED)
    {
      perror("fbvnc: mmap");
      g_daemon = -1;
      return EXIT_FAILURE;
    }

  dev->width  = vinfo.xres;
  dev->height = vinfo.yres;
  dev->stride = pinfo.stride;
  dev->fblen  = pinfo.fblen;

  if (dev->diff)
    {
      dev->shadow = malloc(dev->fblen);
      if (dev->shadow == NULL)
        {
          fprintf(stderr, "fbvnc: no memory for a %lu byte shadow "
                          "frame\n", (unsigned long)dev->fblen);
          return EXIT_FAILURE;
        }
    }

  if (!dev->fullonly && ioctl(dev->fbfd, FBIOC_WATCHAREA, 1) == OK)
    {
      dev->watching = true;
    }
  else if (!dev->fullonly && !dev->diff)
    {
      printf("fbvnc: no dirty reporting; falling back to full frames\n");
      dev->fullonly = true;
    }

  /* Never blocking:  the thread that writes these is the one reading the
   * client, and a device whose buffer is full would stop it reading,
   * which is the whole connection stalled by a fast drag.  A sample
   * dropped costs nothing:  the next one carries where the pointer is
   * now, which is what the application wants anyway.
   */

  dev->touchfd = open("/dev/utouch", O_WRONLY | O_NONBLOCK);
  dev->kbdfd   = open("/dev/ukeyboard", O_WRONLY | O_NONBLOCK);

  memset(&cfg, 0, sizeof(cfg));
  cfg.ctx           = dev;
  cfg.snapshot      = fbvnc_snapshot;
  cfg.on_connect    = fbvnc_invalidate;
  cfg.on_invalidate = fbvnc_invalidate;
  cfg.on_disconnect = fbvnc_on_disconnect;
  cfg.on_pointer    = fbvnc_pointer;
  cfg.on_key        = fbvnc_key;
  cfg.width         = vinfo.xres;
  cfg.height        = vinfo.yres;
  cfg.stride        = pinfo.stride;

  g_server = server = fbvnc_start(&cfg);
  if (server == NULL)
    {
      fprintf(stderr, "fbvnc: fbvnc_start failed: %d\n", errno);
      g_daemon = -1;
      return EXIT_FAILURE;
    }

  printf("fbvnc: serving %s (%ux%u) on port %d%s\n", fbdev,
         vinfo.xres, vinfo.yres, CONFIG_NETUTILS_FBVNC_PORT,
         dev->fullonly ? " (full frames)" :
         dev->diff ? " (compared frames)" : "");

  /* Stop on a signal rather than on a flag another task writes:  the two
   * commands do not necessarily share an address space.
   */

  memset(&act, 0, sizeof(act));
  act.sa_handler = fbvnc_sigterm;
  sigaction(SIGTERM, &act, NULL);

  /* This task is the server.  Waiting here for a thread to do the work
   * would cost a second stack and buy nothing.
   */

  fbvnc_run(server);
  munmap((FAR void *)dev->fb, dev->fblen);
  close(dev->fbfd);

  if (dev->touchfd >= 0)
    {
      close(dev->touchfd);
    }

  if (dev->kbdfd >= 0)
    {
      close(dev->kbdfd);
    }

  free(dev->shadow);
  dev->shadow = NULL;
  printf("fbvnc: stopped\n");
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: fbvnc_usage
 ****************************************************************************/

static int fbvnc_usage(FAR const char *progname)
{
  fprintf(stderr,
          "Usage: %s start [<framebuffer>] [--diff | --full]\n"
          "       %s stop\n"
          "       %s status\n"
          "\n"
          "  <framebuffer>  what to serve, /dev/fb0 by default\n"
          "  --diff         work out what changed by comparing frames, for\n"
          "                 an application that redraws without saying so\n"
          "  --full         a whole frame per update request\n",
          progname, progname, progname);
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc < 2)
    {
      return fbvnc_usage(argv[0]);
    }

  if (strcmp(argv[1], "start") == 0)
    {
      if (g_daemon >= 0)
        {
          fprintf(stderr, "fbvnc: already running as task %d\n",
                  g_daemon);
          return EXIT_FAILURE;
        }

      /* The daemon owns the server thread, so it has to outlive this
       * command rather than run inside it.
       */

      g_daemon = task_create("fbvnc",
                             CONFIG_SYSTEM_FBVNC_PRIORITY,
                             CONFIG_SYSTEM_FBVNC_STACKSIZE,
                             fbvnc_daemon,
                             argc > 2 ? &argv[1] : NULL);
      if (g_daemon < 0)
        {
          fprintf(stderr, "fbvnc: cannot start: %d\n", errno);
          g_daemon = -1;
          return EXIT_FAILURE;
        }

      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "stop") == 0)
    {
      if (g_daemon < 0)
        {
          fprintf(stderr, "fbvnc: not running\n");
          return EXIT_FAILURE;
        }

      /* Signal the daemon rather than write into its state:  the two
       * commands do not necessarily share an address space.
       */

      if (kill(g_daemon, SIGTERM) < 0)
        {
          fprintf(stderr, "fbvnc: cannot stop %d: %d\n", g_daemon, errno);
          return EXIT_FAILURE;
        }

      g_daemon = -1;
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "status") == 0)
    {
      if (g_daemon < 0)
        {
          printf("fbvnc: not running\n");
        }
      else
        {
          printf("fbvnc: running as task %d\n", g_daemon);
        }

      return EXIT_SUCCESS;
    }

  return fbvnc_usage(argv[0]);
}
