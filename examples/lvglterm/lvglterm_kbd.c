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

/* Physical-keyboard input variant of the LVGL terminal: the keyboard device
 * is polled (non-blocking) from the LVGL thread and key presses are streamed
 * to NSH; the shell output fills the whole screen.  Works on any board with
 * a keyboard driver; the device defaults to CONFIG_EXAMPLES_LVGLTERM_KBD_DEV
 * and can be overridden by the first command-line argument.
 *
 * Two device flavours are supported, selected at build time:
 *
 *   - Upper-half keyboards (INPUT_KEYBOARD, e.g. the M5Stack Cardputer
 *     matrix on /dev/kbd0) deliver struct keyboard_event_s events; the Fn
 *     Up/Down keys are handled locally as scroll requests.
 *   - USB HID keyboards (EXAMPLES_LVGLTERM_INPUT_KBD_USB, e.g. /dev/kbda)
 *     deliver a byte stream decoded with the NuttX keyboard codec: normal
 *     keys are forwarded to the shell and, when the driver is built with
 *     CONFIG_HIDKBD_ENCODED, the Up/Down cursor keys scroll the terminal.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#ifdef CONFIG_EXAMPLES_LVGLTERM_INPUT_KBD_MATRIX
#  include <nuttx/input/keyboard.h>
#endif

#ifdef CONFIG_EXAMPLES_LVGLTERM_INPUT_KBD_USB
#  include <nuttx/streams.h>
#  include <nuttx/input/kbd_codec.h>
#endif

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

static int g_kfd = -1;             /* Keyboard device fd (opened O_NONBLOCK) */
static FAR const char *g_kbddev;   /* Keyboard device path (/dev/kbdN) */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: feed_char
 *
 * Description:
 *   Forward one character to NSH stdin and, for printable characters and
 *   newline, echo it on screen.  Control keys such as backspace are echoed
 *   by the shell's own line editor (via its stdout) to avoid handling them
 *   twice.  Carriage return is normalised to newline.  Runs in the LVGL
 *   thread, so it may touch the LVGL widgets directly.
 *
 ****************************************************************************/

static void feed_char(char ch)
{
  if (ch == '\r')
    {
      ch = '\n';
    }

  write(g_nsh_stdin[WRITE_PIPE], &ch, 1);

  if (ch == '\n' || ((uint8_t)ch >= 0x20 && (uint8_t)ch < 0x7f))
    {
      lvglterm_add_output(&ch, 1);
    }
}

/****************************************************************************
 * Name: scroll_terminal
 *
 * Description:
 *   Scroll the output text area up (older output) or down by one step.  Runs
 *   in the LVGL thread.
 *
 ****************************************************************************/

static void scroll_terminal(bool up)
{
  int32_t dy = up ? SCROLL_STEP : -SCROLL_STEP;

  lv_obj_scroll_by(g_output, 0, dy, LV_ANIM_OFF);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lvglterm_input_create
 *
 * Description:
 *   Resolve the keyboard device (first command-line argument, else the
 *   configured default) and open it non-blocking so that it can be polled
 *   from the LVGL thread.
 *
 ****************************************************************************/

void lvglterm_input_create(int argc, FAR char *argv[])
{
  g_kbddev = (argc > 1) ? argv[1] : CONFIG_EXAMPLES_LVGLTERM_KBD_DEV;

  g_kfd = open(g_kbddev, O_RDONLY | O_NONBLOCK);
  if (g_kfd < 0)
    {
      gerr("ERROR: open %s failed: %d\n", g_kbddev, errno);
    }
}

/****************************************************************************
 * Name: lvglterm_input_poll
 *
 * Description:
 *   Drain any pending key presses from the keyboard device and forward them
 *   to NSH (echoing what is typed on screen).  Runs in the LVGL thread from
 *   the terminal's periodic timer, so the read must not block.
 *
 ****************************************************************************/

void lvglterm_input_poll(void)
{
  if (g_kfd < 0)
    {
      return;
    }

#ifdef CONFIG_EXAMPLES_LVGLTERM_INPUT_KBD_USB
  /* USB HID keyboard: read() returns a byte stream that is decoded with the
   * keyboard codec.  Normal keys are fed to the shell; the Up/Down cursor
   * keys (only emitted when the driver is built with CONFIG_HIDKBD_ENCODED)
   * scroll the terminal.  On a plain-ASCII stream every byte simply decodes
   * to a normal key press, so this also works without encoding.
   */

  struct lib_meminstream_s stream;
  struct kbd_getstate_s state;
  char buf[64];
  ssize_t nread;
  uint8_t ch;
  int ret;

  nread = read(g_kfd, buf, sizeof(buf));
  if (nread <= 0)
    {
      return;
    }

  memset(&state, 0, sizeof(state));
  lib_meminstream(&stream, buf, nread);

  for (; ; )
    {
      ret = kbd_decode((FAR struct lib_instream_s *)&stream, &state, &ch);
      if (ret == KBD_ERROR)
        {
          break;
        }

      if (ret == KBD_PRESS)
        {
          feed_char((char)ch);
        }
      else if (ret == KBD_SPECPRESS)
        {
          if (ch == KEYCODE_UP)
            {
              scroll_terminal(true);
            }
          else if (ch == KEYCODE_DOWN)
            {
              scroll_terminal(false);
            }
        }
    }
#else
  /* Upper-half keyboard: read() returns keyboard_event_s events */

  struct keyboard_event_s evt;

  while (read(g_kfd, &evt, sizeof(evt)) == (ssize_t)sizeof(evt))
    {
      if (evt.type != KEYBOARD_PRESS)
        {
          continue;
        }

      /* Fn navigation keys are handled locally: Up/Down scroll the terminal;
       * Left/Right are reserved and simply swallowed for now.
       */

      if (evt.code >= KEY_UP && evt.code <= KEY_RIGHT)
        {
          if (evt.code == KEY_UP)
            {
              scroll_terminal(true);
            }
          else if (evt.code == KEY_DOWN)
            {
              scroll_terminal(false);
            }

          continue;
        }

      feed_char((char)evt.code);
    }
#endif
}
