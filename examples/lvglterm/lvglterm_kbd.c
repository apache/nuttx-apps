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
 * Any keyboard registered with keyboard_register() works:  USB HID, a
 * matrix, the simulator, virtio.  Which one it is makes no difference here.
 *
 * The device delivers struct keyboard_event_s events unless the kernel was
 * built with INPUT_KEYBOARD_BYTESTREAM, in which case it delivers the byte
 * stream that the keyboard codec defines.  That is a property of the build
 * rather than of the hardware, so it is what selects the reader below.
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

#include <nuttx/input/keyboard.h>

#ifdef CONFIG_INPUT_KEYBOARD_BYTESTREAM
#  include <nuttx/streams.h>
#  include <nuttx/input/kbd_codec.h>
#endif

#include <lvgl/lvgl.h>

#include "lvglterm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

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
 * Name: handle_key
 *
 * Description:
 *   Act on one key press.  Cursor Up and Down scroll the terminal, anything
 *   else goes to the shell.
 *
 * Input Parameters:
 *   code    - The character, or a value from enum kbd_keycode_e
 *   special - True if the code is a keycode rather than a character.  The
 *             two ranges overlap, so this is what tells them apart.
 *
 ****************************************************************************/

static void handle_key(uint32_t code, bool special)
{
  if (special)
    {
      if (code == KEYCODE_UP)
        {
          scroll_terminal(true);
        }
      else if (code == KEYCODE_DOWN)
        {
          scroll_terminal(false);
        }

      /* Any other special key has no meaning to a terminal */

      return;
    }

  feed_char(code);
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
  char buf[64];
  ssize_t nread;
#ifdef CONFIG_INPUT_KEYBOARD_BYTESTREAM
  struct lib_meminstream_s stream;
  struct kbd_getstate_s state;
  uint8_t ch;
  int ret;
#else
  FAR struct keyboard_event_s *evt;
#endif

  if (g_kfd < 0)
    {
      return;
    }

  nread = read(g_kfd, buf, sizeof(buf));
  if (nread <= 0)
    {
      return;
    }

#ifdef CONFIG_INPUT_KEYBOARD_BYTESTREAM
  memset(&state, 0, sizeof(state));
  lib_meminstream(&stream, buf, nread);

  for (; ; )
    {
      ret = kbd_decode(&stream.common, &state, &ch);
      if (ret == KBD_ERROR)
        {
          break;
        }

      if (ret == KBD_PRESS)
        {
          handle_key(ch, false);
        }
      else if (ret == KBD_SPECPRESS)
        {
          handle_key(ch, true);
        }
    }
#else
  evt = (FAR struct keyboard_event_s *)buf;

  while (nread >= (ssize_t)sizeof(struct keyboard_event_s))
    {
      if (evt->type == KEYBOARD_PRESS)
        {
          handle_key(evt->code, false);
        }
      else if (evt->type == KEYBOARD_SPECPRESS)
        {
          handle_key(evt->code, true);
        }

      nread -= sizeof(struct keyboard_event_s);
      evt++;
    }
#endif
}
