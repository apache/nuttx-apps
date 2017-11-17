/****************************************************************************
 * apps/graphics/nuttx/pdcscrn.c
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_scr_close
 *
 * Description:
 *   The platform-specific part of endwin(). It may restore the image of the
 *   original screen saved by PDC_scr_open(), if the PDC_RESTORE_SCREEN
 *   environment variable is set; either way, if using an existing terminal,
 *   this function should restore it to the mode it had at startup, and move
 *   the cursor to the lower left corner. (The X11 port does nothing.)
 *
 ****************************************************************************/

void PDC_scr_close(void)
{
  PDC_LOG(("PDC_scr_close() - called\n"));
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_scr_free
 *
 * Description:
 *   Frees the memory for SP allocated by PDC_scr_open(). Called by
 *   delscreen().
 *
 ****************************************************************************/

void PDC_scr_free(void)
{
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_scr_open
 *
 * Description:
 *   The platform-specific part of initscr(). It's actually called from
 *   Xinitscr(); the arguments, if present, correspond to those used with
 *   main(), and may be used to set the title of the terminal window, or for
 *   other, platform-specific purposes. (The arguments are currently used
 *   only in X11.) PDC_scr_open() must allocate memory for SP, and must
 *   initialize acs_map[] (unless it's preset) and several members of SP,
 *   including lines, cols, mouse_wait, orig_attr (and if orig_attr is true,
 *   orig_fore and orig_back), mono, _restore and _preserve. (Although SP is
 *   used the same way in all ports, it's allocated here in order to allow
 *   the X11 port to map it to a block of shared memory.) If using an
 *   existing terminal, and the environment variable PDC_RESTORE_SCREEN is
 *   set, this function may also store the existing screen image for later
 *   restoration by PDC_scr_close().
 *
 ****************************************************************************/

int PDC_scr_open(int argc, char **argv)
{
  PDC_LOG(("PDC_scr_open() - called\n"));
#warning Missing logic
  return ERR;
}

/****************************************************************************
 * Name: PDC_resize_screen
 *
 * Description:
 *   This does the main work of resize_term(). It may respond to non-zero
 *   parameters, by setting the screen to the specified size; to zero
 *   parameters, by setting the screen to a size chosen by the user at
 *   runtime, in an unspecified way (e.g., by dragging the edges of the
 *   window); or both. It may also do nothing, if there's no appropriate
 *   action for the platform.
 *
 ****************************************************************************/

int PDC_resize_screen(int nlines, int ncols)
{
  PDC_LOG(("PDC_resize_screen() - called. Lines: %d Cols: %d\n",
           nlines, ncols));
#warning Missing logic
  return ERR;
}

/****************************************************************************
 * Name: PDC_reset_prog_mode
 *
 * Description:
 *   The non-portable functionality of reset_prog_mode() is handled here --
 *   whatever's not done in _restore_mode().  In current ports: In OS/2, this
 *   sets the keyboard to binary mode; in Win32, it enables or disables the
 *   mouse pointer to match the saved mode; in others it does nothing.
 *
 ****************************************************************************/

void PDC_reset_prog_mode(void)
{
  PDC_LOG(("PDC_reset_prog_mode() - called.\n"));
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_reset_shell_mode
 *
 * Description:
 *   The same thing for reset_shell_mode() as PDC_reset_prog_mode(). In OS/2
 *   and Win32, it restores the default console mode; in others it does
 *   nothing.
 *
 ****************************************************************************/

void PDC_reset_shell_mode(void)
{
  PDC_LOG(("PDC_reset_shell_mode() - called.\n"));
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_restore_screen_mode
 *
 * Description:
 *  Called from _restore_mode() in pdc_kernel.c, this function does the
 *  actual mode changing, if applicable. Currently used only in DOS and OS/2.
 *
 ****************************************************************************/

void PDC_restore_screen_mode(int i)
{
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_save_screen_mode
 *
 * Description:
 *   Called from _save_mode() in pdc_kernel.c, this function saves the actual
 *   screen mode, if applicable. Currently used only in DOS and OS/2.
 *
 ****************************************************************************/

void PDC_save_screen_mode(int i)
{
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_init_pair
 *
 * Description:
 *   The core of init_pair(). This does all the work of that function, except
 *   checking for values out of range. The values passed to this function
 *   should be returned by a call to PDC_pair_content() with the same pair
 *   number. PDC_transform_line() should use the specified colors when
 *   rendering a chtype with the given pair number.
 *
 ****************************************************************************/

void PDC_init_pair(short pair, short fg, short bg)
{
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_pair_content
 *
 * Description:
 *   The core of pair_content(). This does all the work of that function,
 *   except checking for values out of range and null pointers.
 *
 ****************************************************************************/

int PDC_pair_content(short pair, short *fg, short *bg)
{
#warning Missing logic
  return ERR;
}

/****************************************************************************
 * Name: PDC_can_change_color
 *
 * Description:
 *   Returns true if init_color() and color_content() give meaningful
 *   results, false otherwise. Called from can_change_color().
 *
 ****************************************************************************/

bool PDC_can_change_color(void)
{
#warning Missing logic
  return false;
}

/****************************************************************************
 * Name: PDC_color_content
 *
 * Description:
 *   The core of color_content(). This does all the work of that function,
 *   except checking for values out of range and null pointers.
 *
 ****************************************************************************/

int PDC_color_content(short color, short *red, short *green, short *blue)
{
#warning Missing logic
  return ERR;
}

/****************************************************************************
 * Name: PDC_init_color
 *
 * Description:
 *   The core of init_color(). This does all the work of that function,
 *   except checking for values out of range.
 *
 ****************************************************************************/

int PDC_init_color(short color, short red, short green, short blue)
{
#warning Missing logic
  return ERR;
}
