/****************************************************************************
 * apps/graphics/nuttx/pdckbd.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "pdcnuttx.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Key modifiers.  The OR of any of:
 *
 *   PDC_KEY_MODIFIER_SHIFT;
 *   PDC_KEY_MODIFIER_CONTROL;
 *   PDC_KEY_MODIFIER_ALT;
 *   PDC_KEY_MODIFIER_NUMLOCK;
 */

unsigned long pdc_key_modifiers;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_check_key
 *
 * Description:
 *   Keyboard/mouse event check, called from wgetch(). Returns true if
 *   there's an event ready to process. This function must be non-blocking.
 *
 ****************************************************************************/

bool PDC_check_key(void)
{
#warning Missing logic
  return false;
}

/****************************************************************************
 * Name: PDC_get_key
 *
 * Description:
 *   Get the next available key, or mouse event (indicated by a return of
 *   KEY_MOUSE), and remove it from the OS' input queue, if applicable. This
 *   function is called from wgetch(). This function may be blocking, and
 *   traditionally is; but it need not be. If a valid key or mouse event
 *   cannot be returned, for any reason, this function returns -1. Valid keys
 *   are those that fall within the appropriate character set, or are in the
 *   list of special keys found in curses.h (KEY_MIN through KEY_MAX). When
 *   returning a special key code, this routine must also set SP->key_code to
 *   true; otherwise it must set it to false. If SP->return_key_modifiers is
 *   true, this function may return modifier keys (shift, control, alt),
 *   pressed alone, as special key codes; if SP->return_key_modifiers is
 *   false, it must not. If modifier keys are returned, it should only happen
 *   if no other keys were pressed in the meantime; i.e., the return should
 *   happen on key up. But if this is not possible, it may return the
 *   modifier keys on key down (if and only if SP->return_key_modifiers is
 *   true).
 *
 ****************************************************************************/

int PDC_get_key(void)
{
#warning Missing logic
  return 0;
}

/****************************************************************************
 * Name: PDC_get_input_fd
 *
 * Description:
 *   PDC_get_input_fd() returns the file descriptor that PDCurses reads its
 *   input from.  It can be used for select().
 *
 ****************************************************************************/

unsigned long PDC_get_input_fd(void)
{
  PDC_LOG(("PDC_get_input_fd() - called\n"));
#warning Missing logic
  return -1;
}

/****************************************************************************
 * Name: PDC_set_keyboard_binary
 *
 * Description:
 *   Set keyboard input to "binary" mode. If you need to do something to keep
 *   the OS from processing ^C, etc. on your platform, do it here. true turns
 *   the mode on; false reverts it. This function is called from raw() and
 *   noraw().
 *
 ****************************************************************************/

void PDC_set_keyboard_binary(bool on)
{
  PDC_LOG(("PDC_set_keyboard_binary() - called\n"));
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_flushinp
 *
 * Description:
 *   This is the core of flushinp(). It discards any pending key or mouse
 *   events, removing them from any internal queue and from the OS queue, if
 *   applicable.
 *
 ****************************************************************************/

void PDC_flushinp(void)
{
  PDC_LOG(("PDC_flushinp() - called\n"));
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_mouse_set
 *
 * Description:
 *   Called by mouse_set(), mouse_on(), and mouse_off() -- all the functions
 *   that modify SP->_trap_mbe. If your platform needs to do anything in
 *   response to a change in SP->_trap_mbe (for example, turning the mouse
 *   cursor on or off), do it here. Returns OK or ERR, which is passed on by
 *   the caller.
 *
 ****************************************************************************/

int PDC_mouse_set(void)
{
#warning Missing logic
  return OK;
}

/****************************************************************************
 * Name: PDC_modifiers_set
 *
 * Description:
 *   Called from PDC_return_key_modifiers(). If your platform needs to do
 *   anything in response to a change in SP->return_key_modifiers, do it
 *   here. Returns OK or ERR, which is passed on by the caller.
 *
 ****************************************************************************/

int PDC_modifiers_set(void)
{
#warning Missing logic
  return OK;
}
