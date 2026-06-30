/****************************************************************************
 * apps/games/NXDoom/src/i_input.c
 *
 * SPDX-License-Identifer: GPLv2
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
 *   SDL implementation of system-specific input interface.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#ifdef CONFIG_GAMES_NXDOOM_KEYBOARD
#include <nuttx/input/kbd_codec.h>
#endif

#include "d_event.h"
#include "doomkeys.h"
#include "doomtype.h"
#include "i_input.h"
#include "m_argv.h"
#include "m_config.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct keyboard_dev
{
  int fd;
  bool inited;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_GAMES_NXDOOM_KEYBOARD
struct keyboard_dev g_kbd_dev =
{
  .fd = -1,
  .inited = false,
};
#endif

#if 0
static const int g_scancode_translate_table[] = SCANCODE_TO_KEYS_ARRAY;

/* Lookup table for mapping ASCII characters to their equivalent when
 * shift is pressed on a US layout keyboard. This is the original table
 * as found in the Doom sources, comments and all.
 */

static const char shiftxform[] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, ' ', '!', '"', '#', '$', '%', '&',
    '"', /* shift-' */
    '(', ')', '*', '+',
    '<', /* shift-, */
    '_', /* shift-- */
    '>', /* shift-. */
    '?', /* shift-/ */
    ')', /* shift-0 */
    '!', /* shift-1 */
    '@', /* shift-2 */
    '#', /* shift-3 */
    '$', /* shift-4 */
    '%', /* shift-5 */
    '^', /* shift-6 */
    '&', /* shift-7 */
    '*', /* shift-8 */
    '(', /* shift-9 */
    ':',
    ':', /* shift-; */
    '<',
    '+', /* shift-= */
    '>', '?', '@',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '[', /* shift-[ */
    '!', /* shift-backslash - OH MY GOD DOES WATCOM SUCK */
    ']', /* shift-] */
    '"', '_',
    '\'', /* shift-` */
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '{', '|', '}', '~', 127
};

/* If true, i_start_text_input() has been called, and we are populating
 * the data3 field of ev_keydown events.
 */

static boolean text_input_enabled = true;

/* Bit mask of mouse button state. */

static unsigned int mouse_button_state = 0;
#endif

/* Disallow mouse and joystick movement to cause forward/backward
 * motion.  Specified with the '-novert' command line parameter.
 * This is an int to allow saving to config file
 */

int novert = 0;

/* If true, keyboard mapping is ignored, like in Vanilla Doom.
 * The sensible thing to do is to disable this if you have a non-US
 * keyboard.
 */

int vanilla_keyboard_mapping = true;

/* Mouse acceleration
 *
 * This emulates some of the behavior of DOS mouse drivers by increasing
 * the speed when the mouse is moved fast.
 *
 * The mouse input values are input directly to the game, but when
 * the values exceed the value of mouse_threshold, they are multiplied
 * by mouse_acceleration to increase the speed.
 */

float mouse_acceleration = 2.0;
int mouse_threshold = 10;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: init_kbd_dev
 *
 * Description:
 *   Set up the keyboard device for getting keyboard events.
 *
 * Return:
 *   0 on success, error code otherwise.
 *
 ****************************************************************************/

static int init_kbd_dev(struct keyboard_dev *dev)
{
  if (dev->inited)
    {
      return 0;
    }

  dev->fd = open(CONFIG_GAMES_NXDOOM_KBDPATH, O_RDONLY | O_NONBLOCK);
  if (dev->fd < 0)
    {
      return errno;
    }

  dev->inited = true;
  return 0;
}

/****************************************************************************
 * Name: translate_key
 *
 * Description:
 *   Translates a NuttX key code into one from doomkeys.h.
 *
 * Return:
 *   The translated key code.
 *
 ****************************************************************************/

static int translate_key(uint32_t keycode)
{
  switch (keycode)
    {
    case KEYCODE_LEFT:
      return KEY_LEFTARROW;
    case KEYCODE_RIGHT:
      return KEY_RIGHTARROW;
    case KEYCODE_UP:
      return KEY_UPARROW;
    case KEYCODE_DOWN:
      return KEY_DOWNARROW;
    case KEYCODE_ENTER:
      return KEY_ENTER;
    default:
      return keycode;
    }
}

/****************************************************************************
 * Name: get_localized_key
 *
 * Description:
 *  Get the localized version of the key press. This takes into account the
 *  keyboard layout, but does not apply any changes due to modifiers, (eg.
 *  shift-, alt-, etc.)
 *
 ****************************************************************************/

static int get_localized_key(uint32_t sym)
{
  /* NOTE: Argument was SDL_Keysym *sym */

  /* When using Vanilla mapping, we just base everything off the scancode
   * and always pretend the user is using a US layout keyboard.
   */

  if (vanilla_keyboard_mapping)
    {
      return translate_key(sym);
    }
  else
    {
      int result = sym;

      if (result < 0 || result >= 128)
        {
          result = 0;
        }

      return result;
    }
}

/****************************************************************************
 * Name: get_typed_char
 *
 * Description:
 *  Get the equivalent ASCII (Unicode?) character for a keypress.
 *
 ****************************************************************************/

static int get_typed_char(uint32_t sym)
{
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_GAMES_NXDOOM_KEYBOARD

/****************************************************************************
 * Name: get_kbd_event
 *
 * Description:
 *   Read a single keyboard event from the keyboard device.
 *
 * Return:
 *   0 on success, error code otherwise.
 *
 ****************************************************************************/

int get_kbd_event(struct keyboard_event_s *sample)
{
  int err;
  ssize_t nbytes;

  /* Initialize the keyboard device if it isn't already */

  err = init_kbd_dev(&g_kbd_dev);
  if (err)
    {
      return err;
    }

  /* Read events until we're out of them */

  nbytes = read(g_kbd_dev.fd, sample, sizeof(*sample));
  if (nbytes < 0)
    {
      err = errno;
      if (err != EINTR)
        {
          return err;
        }
    }
  else if (nbytes != sizeof(*sample))
    {
      return EIO;
    }
  else if (nbytes == 0)
    {
      return EAGAIN; /* No event */
    }

  return 0;
}
#endif

void i_handle_keyboard_event(struct keyboard_event_s *kevent)
{
  /* XXX: passing pointers to event for access after this function
   * has terminated is undefined behaviour
   */

  event_t event;

  switch (kevent->type)
    {
    case KEYBOARD_PRESS:
      event.type = ev_keydown;
      event.data1 = translate_key(kevent->code);
      event.data2 = get_localized_key(kevent->code);
      event.data3 = get_typed_char(kevent->code);

      if (event.data1 != 0)
        {
          d_post_event(&event);
        }
      break;

    case KEYBOARD_RELEASE:
      event.type = ev_keyup;
      event.data1 = translate_key(kevent->code);

      /* data2/data3 are initialized to zero for ev_keyup.
       * For ev_keydown it's the shifted Unicode character
       * that was typed, but if something wants to detect
       * key releases it should do so based on data1
       * (key ID), not the printable char.
       */

      event.data2 = 0;
      event.data3 = 0;

      if (event.data1 != 0)
        {
          d_post_event(&event);
        }
      break;

    default:
      break;
    }
}

void i_start_text_input(int x1, int y1, int x2, int y2)
{
}

void i_stop_text_input(void)
{
}

void i_handle_mouse_event(void)
{
  /* Argument was SDL_Event *sdlevent */
}

/****************************************************************************
 * Name: i_read_mouse
 *
 * Description:
 *  Read the change in mouse state to generate mouse motion events. This is
 *  to combine all mouse movement for a tic into one mouse motion event.
 *
 ****************************************************************************/

void i_read_mouse(void)
{
}

/****************************************************************************
 * Name: i_bind_input_variables
 *
 * Description:
 *  Bind all variables controlling input options.
 *
 ****************************************************************************/

void i_bind_input_variables(void)
{
  m_bind_float_variable("mouse_acceleration", &mouse_acceleration);
  m_bind_int_variable("mouse_threshold", &mouse_threshold);
  m_bind_int_variable("vanilla_keyboard_mapping", &vanilla_keyboard_mapping);
  m_bind_int_variable("novert", &novert);
}
