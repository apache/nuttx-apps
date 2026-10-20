/****************************************************************************
 * apps/games/NXDoom/src/i_input_uartkbd.c
 *
 * SPDX-License-Identifier: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *   Keyboard events from a serial terminal, for a board without a keyboard
 *   driver.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <nuttx/input/kbd_codec.h>
#include <nuttx/input/keyboard.h>

#include "i_input.h"
#include "i_timer.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define UARTKBD_MAXHELD 8
#define UARTKBD_HOLDMS  250 /* Release a key this long after its last byte */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct uartkbd_held
{
  bool     active;
  uint32_t type;
  uint32_t code;
  int      deadline;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct uartkbd_held g_held[UARTKBD_MAXHELD];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: uartkbd_hold
 *
 * Description:
 *   A serial terminal reports a key press only, never its release, so a
 *   press is turned into a press event now and a release event once the
 *   terminal has stopped repeating the key for UARTKBD_HOLDMS.  Returns
 *   true when the key was not already held, so that the caller emits the
 *   press.  A key that finds no free slot is ignored, because its release
 *   could never be reported.
 *
 ****************************************************************************/

static bool uartkbd_hold(uint32_t type, uint32_t code)
{
  int now = i_get_time_ms();
  int i;
  int freeslot = -1;

  for (i = 0; i < UARTKBD_MAXHELD; i++)
    {
      if (g_held[i].active && g_held[i].type == type &&
          g_held[i].code == code)
        {
          g_held[i].deadline = now + UARTKBD_HOLDMS;
          return false;
        }

      if (!g_held[i].active && freeslot < 0)
        {
          freeslot = i;
        }
    }

  if (freeslot < 0)
    {
      return false;
    }

  g_held[freeslot].active   = true;
  g_held[freeslot].type     = type;
  g_held[freeslot].code     = code;
  g_held[freeslot].deadline = now + UARTKBD_HOLDMS;
  return true;
}

/****************************************************************************
 * Name: uartkbd_decode
 *
 * Description:
 *   Turns one byte from the terminal into a key, tracking the arrow key
 *   escape sequences.  Returns true when *type and *code are valid.
 *
 *     w a s d, arrow keys   move / turn
 *     j                     fire (Ctrl)
 *     k                     run (Shift)
 *     l                     strafe (Alt)
 *     space, e              use
 *     Esc, q                menu / back
 *     everything else       passed through as the typed character
 *
 ****************************************************************************/

static bool uartkbd_decode(int fd, uint8_t ch, uint32_t *type,
                           uint32_t *code)
{
  static bool in_csi; /* Inside an ESC [ or ESC O sequence */

  if (in_csi)
    {
      in_csi = false;
      *type = KEYBOARD_SPECPRESS;
      switch (ch)
        {
          case 'A':
            *code = KEYCODE_UP;
            return true;
          case 'B':
            *code = KEYCODE_DOWN;
            return true;
          case 'C':
            *code = KEYCODE_RIGHT;
            return true;
          case 'D':
            *code = KEYCODE_LEFT;
            return true;
          default:
            return false;
        }
    }

  if (ch == 0x1b)
    {
      char next;

      /* A lone Esc cannot be told from the start of an arrow sequence
       * without waiting, so the sequence is only assumed when the next
       * byte of it is already there.
       */

      if (read(fd, &next, 1) == 1 && (next == '[' || next == 'O'))
        {
          in_csi = true;
          return false;
        }

      *type = KEYBOARD_PRESS;
      *code = 0x1b;
      return true;
    }

  *type = KEYBOARD_SPECPRESS;
  switch (ch)
    {
      case 'w':
      case 'W':
        *code = KEYCODE_UP;
        return true;
      case 's':
      case 'S':
        *code = KEYCODE_DOWN;
        return true;
      case 'a':
      case 'A':
        *code = KEYCODE_LEFT;
        return true;
      case 'd':
      case 'D':
        *code = KEYCODE_RIGHT;
        return true;
      case 'j':
      case 'J':
        *code = KEYCODE_LCTRL;
        return true;
      case 'k':
      case 'K':
        *code = KEYCODE_LSHIFT;
        return true;
      case 'l':
      case 'L':
        *code = KEYCODE_LALT;
        return true;
      case 0x7f:
        *code = KEYCODE_BACKDEL;
        return true;
    }

  *type = KEYBOARD_PRESS;
  switch (ch)
    {
      case 'e':
      case 'E':
        *code = ' ';
        return true;
      case 'q':
      case 'Q':
        *code = 0x1b;
        return true;
      case '\r':
        *code = '\n';
        return true;
      default:
        *code = ch;
        return true;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: uartkbd_get_event
 *
 * Description:
 *   Read a single synthesized keyboard event from the serial terminal
 *   that fd refers to.  Returns 0 on success, EAGAIN if there is no event.
 *
 ****************************************************************************/

int uartkbd_get_event(int fd, struct keyboard_event_s *sample)
{
  int now;
  uint8_t ch;
  int i;

  /* Release any key that the terminal has stopped repeating */

  now = i_get_time_ms();
  for (i = 0; i < UARTKBD_MAXHELD; i++)
    {
      if (g_held[i].active && now - g_held[i].deadline >= 0)
        {
          g_held[i].active = false;
          sample->type = (g_held[i].type == KEYBOARD_SPECPRESS) ?
                         KEYBOARD_SPECREL : KEYBOARD_RELEASE;
          sample->code = g_held[i].code;
          return 0;
        }
    }

  /* Then take new bytes until one of them is a new key press */

  while (read(fd, &ch, 1) == 1)
    {
      uint32_t type;
      uint32_t code;

      if (uartkbd_decode(fd, ch, &type, &code) && uartkbd_hold(type, code))
        {
          sample->type = type;
          sample->code = code;
          return 0;
        }
    }

  return EAGAIN;
}
