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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>

#include "pdcnuttx.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Currently only this bits are used.  Others are simply masked out. */

#define DJOY_BITSET (DJOY_UP_BIT | DJOY_DOWN_BIT | DJOY_LEFT_BIT | \
                     DJOY_RIGHT_BIT | DJOY_BUTTON_SELECT_BIT)

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

#ifndef CONFIG_PDCURSES_MULTITHREAD
unsigned long pdc_key_modifiers;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_djoy_sample
 *
 * Description:
 *   Keyboard/mouse event check, called from wgetch(). Returns true if
 *   there's an event ready to process. This function must be non-blocking.
 *
 ****************************************************************************/

#ifdef CONFIG_PDCURSES_DJOYSTICK
static djoy_buttonset_t PDC_djoy_sample(FAR struct pdc_fbstate_s *fbstate)
{
  djoy_buttonset_t sample;
  ssize_t nread;

  PDC_LOG(("PDC_check_key() - called: DJoystick\n"));

  nread = read(fbstate->djfd, &sample, sizeof(djoy_buttonset_t));
  if (nread < 0)
    {
      PDC_LOG(("ERROR: read() failed: %d\n", errno));
      return 0;
    }
  else if (nread != sizeof(djoy_buttonset_t))
    {
      PDC_LOG(("ERROR: read() unexpected size: %ld vs %d\n",
              (long)nread, sizeof(djoy_buttonset_t)));
      return 0;
    }

  return sample & DJOY_BITSET;
}
#endif

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
#if defined(CONFIG_PDCURSES_KEYBOARD)
#  warning Missing logic
  return false;

#elif defined(CONFIG_PDCURSES_MOUSE)
#  warning Missing logic
  return false;

#elif defined(CONFIG_PDCURSES_DJOYSTICK)
  {
#ifdef CONFIG_PDCURSES_MULTITHREAD
    FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
    FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
    FAR struct pdc_fbstate_s *fbstate;
    djoy_buttonset_t newset;

    PDC_LOG(("PDC_check_key() - called: DJoystick\n"));

    DEBUGASSERT(fbscreen != NULL);
    fbstate = &fbscreen->fbstate;

    /* Sample the discrete joystick bits and return true of any of them
     * are now in a different state.
     */

    newset = PDC_djoy_sample(fbstate);
    return (fbstate->djlast ^ newset) != 0;
  }

#elif defined(CONFIG_PDCURSES_TERMINPUT)
  {
#ifdef CONFIG_PDCURSES_MULTITHREAD
    FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
    FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *) SP;
    FAR struct pdc_termstate_s  *termstate = &termscreen->termstate;
    int ret;
    fd_set rfds;
    struct timeval tv;

    /* Test the registered tcurs interface for cached characters */

    ret = termcurses_checkkey(termstate->tcurs);
    if (ret)
      {
        return true;
      }

    /* Watch stdin (fd 0) to see when it has input. */

    FD_ZERO(&rfds);
    FD_SET(termstate->in_fd, &rfds);

    /* Do not wait. */

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    ret = select(1, &rfds, NULL, NULL, &tv);
    if (ret > 0)
      {
        return true;
      }

    return false;
  }
#else
  return false;
#endif
}

/****************************************************************************
 * Name: PDC_get_key
 *
 * Description:
 *   Get the next available key, or mouse event (indicated by a return of
 *   KEY_MOUSE), and remove it from the OS' input queue, if applicable. This
 *   function is called from wgetch().  This function may be blocking, and
 *   traditionally is; but it need not be.  If a valid key or mouse event
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
#if defined(CONFIG_PDCURSES_KEYBOARD)
#  warning Missing logic
  return false;

#elif defined(CONFIG_PDCURSES_MOUSE)
#  warning Missing logic
  return false;

#elif defined(CONFIG_PDCURSES_DJOYSTICK)
  {
#ifdef CONFIG_PDCURSES_MULTITHREAD
    FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
    FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
    FAR struct pdc_fbstate_s *fbstate;
    djoy_buttonset_t sample;
    djoy_buttonset_t newbits;

    PDC_LOG(("PDC_get_key() - called: DJoystick\n"));

    DEBUGASSERT(fbscreen != NULL);
    fbstate = &fbscreen->fbstate;

    /* Sample the discrete joystick bits, get the bits that have changed
     * state, then update the settings for the next time we get here.
     */

    sample          = PDC_djoy_sample(fbstate);
    newbits         = sample & (sample ^ fbstate->djlast);
    fbstate->djlast = sample;

    if (newbits == 0)
      {
        /* Nothing has changed... we should:
         *
         * 1) Check for repeat keys.  Up, Down, Left and Right should repeat
         *    at a most rate if held down.
         * 2) Otherwise, block, polling for a change in button state.  But,
         *    apparently, we can just return ERR in this case.
         *
         *    The joystick driver also supports a notification via signal
         *    when a button change occurs.  This could be an option to poll().
         */

        return ERR;
      }

    /* Return a key code for any button that has just been pressed (button
     * releases are not reported)
     */

    if ((newbits & DJOY_BUTTON_SELECT_BIT) != 0)
      {
        return '\n';
      }
    else if ((newbits & DJOY_UP_BIT) != 0)
      {
        return KEY_UP;
      }
    else if ((newbits & DJOY_DOWN_BIT) != 0)
      {
        return KEY_DOWN;
      }
    else if ((newbits & DJOY_LEFT_BIT) != 0)
      {
        return KEY_LEFT;
      }
    else /* if ((newbits & DJOY_RIGHT_BIT) != 0) */
      {
        return KEY_RIGHT;
      }

    return ERR;
  }

#elif defined(CONFIG_PDCURSES_TERMINPUT)
  {
#ifdef CONFIG_PDCURSES_MULTITHREAD
    FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
    FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *)SP;
    FAR struct pdc_termstate_s  *termstate = &termscreen->termstate;
    int specialkey;
    int keymodifiers;
    int keycode;

    /* Call termcurses library routine to get keycode */

    keycode = termcurses_getkeycode(termstate->tcurs, &specialkey, &keymodifiers);
    SP->key_code = specialkey;
    pdc_key_modifiers = keymodifiers;

    return keycode;
  }
#else
  return false;
#endif
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
#ifdef CONFIG_PDCURSES_HAVE_INPUT
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
#ifdef CONFIG_PDCURSES_DJOYSTICK
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;
#endif

#ifdef CONFIG_SYSTEM_TERMCURSES
  if (!graphic_screen)
    {
      FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *)SP;
      return termscreen->termstate.in_fd;
    }
#endif

#if defined(CONFIG_PDCURSES_KEYBOARD)
#  warning Missing logic
  return -1;

#elif defined(CONFIG_PDCURSES_MOUSE)
#  warning Missing logic
  return -1;

#elif defined(CONFIG_PDCURSES_DJOYSTICK)
  PDC_LOG(("PDC_get_input_fd() - called\n"));

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

  return fbstate->djfd;

#else
  return -1;
#endif
#else
  PDC_LOG(("PDC_get_input_fd() - called:  No input device\n"));

  /* No input device */

  return -1;
#endif
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

#ifdef CONFIG_PDCURSES_KEYBOARD
#  warning Missing logic
#endif

#ifdef CONFIG_PDCURSES_MOUSE
#  warning Missing logic
#endif
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
#ifdef CONFIG_PDCURSES_MOUSE
#  warning Missing logic
#endif
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
#ifdef CONFIG_PDCURSES_KEYBOARD
#  warning Missing logic
#endif
  return OK;
}

/****************************************************************************
 * Name: PDC_input_open
 *
 * Description:
 *   Open and configure any input devices
 *
 ****************************************************************************/

#ifdef CONFIG_PDCURSES_HAVE_INPUT
int PDC_input_open(FAR struct pdc_fbstate_s *fbstate)
{
#ifdef CONFIG_PDCURSES_DJOYSTICK
  /* Initialize discrete joystick state. */

  fbstate->djlast = 0;

  /* Open the djoystick device */

  fbstate->djfd = open(CONFIG_PDCURSES_DJOYDEV, O_RDONLY);
  if (fbstate->djfd < 0)
    {
      PDC_LOG(("ERROR: Failed to open %s: %d\n",
              CONFIG_PDCURSES_DJOYDEV, errno));
      return ERR;
    }
#endif

  return OK;
}
#endif

/****************************************************************************
 * Name: PDC_input_close
 *
 * Description:
 *   Close any input devices and release any resources committed by
 *   PDC_input_open()
 *
 ****************************************************************************/

#ifdef CONFIG_PDCURSES_HAVE_INPUT
void PDC_input_close(FAR struct pdc_fbstate_s *fbstate)
{
#ifdef CONFIG_PDCURSES_DJOYSTICK
  close(fbstate->djfd);
#endif
}
#endif
