/****************************************************************************
 * apps/examples/lvglterm/lvglterm_kbd.c
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

/* Physical-keyboard input variant of the LVGL terminal: a task reads key
 * events from a /dev/kbdN keyboard device and streams them to NSH; the shell
 * output fills the whole screen.  Works on any board with a keyboard driver;
 * the device defaults to CONFIG_EXAMPLES_LVGLTERM_KBD_DEV and can be
 * overridden by the first command-line argument.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/input/keyboard.h>

#include <lvgl/lvgl.h>

#include "lvglterm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Out-of-band key codes for the Fn + navigation cluster (cursor keys),
 * reported by keyboard drivers that follow this convention (for example the
 * M5Stack Cardputer).  Up/Down scroll the terminal instead of going to the
 * shell; drivers that do not emit these codes keep normal behaviour.
 */

#define KEY_UP         0x80
#define KEY_DOWN       0x81
#define KEY_LEFT       0x82
#define KEY_RIGHT      0x83

#define SCROLL_STEP    24          /* Pixels scrolled per Up/Down keypress */

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_echo[2];              /* Keyboard task -> screen (local echo) */
static volatile int g_scroll;      /* Pending scroll (lines), applied by LVGL */
static FAR const char *g_kbddev;   /* Keyboard device path (/dev/kbdN) */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: kbd_task
 *
 * Description:
 *   Read key presses from the keyboard device and forward them to NSH stdin
 *   (and to the echo pipe so the user sees what is typed).  The Fn Up/Down
 *   keys are handled locally as scroll requests.
 *
 ****************************************************************************/

static int kbd_task(int argc, FAR char *argv[])
{
  struct keyboard_event_s evt;
  ssize_t nread;
  int  kfd;
  char ch;

  kfd = open(g_kbddev, O_RDONLY);
  if (kfd < 0)
    {
      gerr("ERROR: open %s failed: %d\n", g_kbddev, errno);
      return EXIT_FAILURE;
    }

  for (; ; )
    {
      nread = read(kfd, &evt, sizeof(evt));
      if (nread != (ssize_t)sizeof(evt))
        {
          if (nread <= 0)
            {
              usleep(10000);
            }

          continue;
        }

      if (evt.type != KEYBOARD_PRESS)
        {
          continue;
        }

      /* Fn navigation keys are handled locally: Up/Down scroll the terminal
       * (applied by the LVGL thread) and are not sent to the shell.
       * Left/Right are reserved and simply swallowed for now.
       */

      if (evt.code >= KEY_UP && evt.code <= KEY_RIGHT)
        {
          if (evt.code == KEY_UP)
            {
              g_scroll -= 1;
            }
          else if (evt.code == KEY_DOWN)
            {
              g_scroll += 1;
            }

          continue;
        }

      ch = (char)evt.code;
      if (ch == '\r')
        {
          ch = '\n';
        }

      /* Always feed the shell.  Echo only printable characters and newline
       * locally; control keys such as backspace are echoed by the shell's
       * own line editor (via its stdout) to avoid handling them twice.
       */

      write(g_nsh_stdin[WRITE_PIPE], &ch, 1);

      if (ch == '\n' || ((uint8_t)ch >= 0x20 && (uint8_t)ch < 0x7f))
        {
          write(g_echo[WRITE_PIPE], &ch, 1);
        }
    }

  close(kfd);
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lvglterm_input_create
 *
 * Description:
 *   Resolve the keyboard device (first command-line argument, else the
 *   configured default) and start the task that streams key presses to NSH.
 *
 ****************************************************************************/

void lvglterm_input_create(int argc, FAR char *argv[])
{
  g_kbddev = (argc > 1) ? argv[1] : CONFIG_EXAMPLES_LVGLTERM_KBD_DEV;

  if (pipe(g_echo) < 0)
    {
      gerr("ERROR: echo pipe failed: %d\n", errno);
      return;
    }

  /* The read end is polled by the LVGL timer and must not block */

  fcntl(g_echo[READ_PIPE], F_SETFL,
        fcntl(g_echo[READ_PIPE], F_GETFL) | O_NONBLOCK);

  task_create("lvgltermkbd", SCHED_PRIORITY_DEFAULT, 2048, kbd_task, NULL);
}

/****************************************************************************
 * Name: lvglterm_input_poll
 *
 * Description:
 *   Apply any pending scroll and drain the local echo pipe onto the screen.
 *   Runs in the LVGL thread.
 *
 ****************************************************************************/

void lvglterm_input_poll(void)
{
  char buf[64];
  int  n;
  int  s;

  /* Apply any pending scroll requested by the keyboard task (Up/Down).
   * A negative value scrolls up (towards older output), positive scrolls
   * down.  Consume only what we read so concurrent keypresses are not lost.
   */

  s = g_scroll;
  if (s != 0)
    {
      g_scroll -= s;
      lv_obj_scroll_by(g_output, 0, -s * SCROLL_STEP, LV_ANIM_OFF);
    }

  if (lvglterm_has_input(g_echo[READ_PIPE]))
    {
      n = read(g_echo[READ_PIPE], buf, sizeof(buf));
      if (n > 0)
        {
          lvglterm_add_output(buf, n);
        }
    }
}
