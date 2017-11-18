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

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "pdcnuttx.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This singleton represents the state of frame buffer device.  pdcurses
 * depends on global variables and, hence, can never support more than a
 * single framebuffer instance in the same address space.
 */

struct pdc_fbstate_s g_pdc_fbstate;

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
 *   The platform-specific part of initscr().  It's actually called from
 *   Xinitscr(); the arguments, if present, correspond to those used with
 *   main(), and may be used to set the title of the terminal window, or for
 *   other, platform-specific purposes. (The arguments are currently used
 *   only in X11.)  PDC_scr_open() must allocate memory for SP, and must
 *   initialize acs_map[] (unless it's preset) and several members of SP,
 *   including lines, cols, mouse_wait, orig_attr (and if orig_attr is true,
 *   orig_fore and orig_back), mono, _restore and _preserve. (Although SP is
 *   used the same way in all ports, it's allocated here in order to allow
 *   the X11 port to map it to a block of shared memory.)  If using an
 *   existing terminal, and the environment variable PDC_RESTORE_SCREEN is
 *   set, this function may also store the existing screen image for later
 *   restoration by PDC_scr_close().
 *
 ****************************************************************************/

int PDC_scr_open(int argc, char **argv)
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  FAR const struct nx_font_s *fontset;
  uint32_t bitwidth;
  int ret;

  PDC_LOG(("PDC_scr_open() - called\n"));

  /* Allocate the global instance of SP */

  SP = (SCREEN *)zalloc(sizeof(SCREEN));
  if (SP == NULL)
    {
      PDC_LOG(("ERROR: Failed to allocate SP\n"));
      return ERR;
    }

  /* Open the framebuffer driver */

  g_pdc_fbstate.fd = open(CONFIG_PDCURSES_FBDEV, O_RDWR);
  if (g_pdc_fbstate.fd < 0)
    {
      PDC_LOG(("ERROR: Failed to open %s: %d\n",
               CONFIG_PDCURSES_FBDEV, errno));
      goto errout_with_sp;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(g_pdc_fbstate.fd, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&vinfo));
  if (ret < 0)
    {
      PDC_LOG(("ERROR: ioctl(FBIOGET_VIDEOINFO) failed: %d\n", errno));
      goto errout_with_fd;
    }

  PDC_LOG(("VideoInfo:\n"));
  PDC_LOG(("      fmt: %u\n", vinfo.fmt));
  PDC_LOG(("     xres: %u\n", vinfo.xres));
  PDC_LOG(("     yres: %u\n", vinfo.yres));
  PDC_LOG(("  nplanes: %u\n", vinfo.nplanes));

  g_pdc_fbstate.xres = vinfo.xres;  /* Horizontal resolution in pixel columns */
  g_pdc_fbstate.yres = vinfo.yres;  /* Vertical resolution in pixel rows */

  ret = ioctl(g_pdc_fbstate.fd, FBIOGET_PLANEINFO,
              (unsigned long)((uintptr_t)&pinfo));
  if (ret < 0)
    {
      PDC_LOG(("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n", errno));
      goto errout_with_fd;
    }

  PDC_LOG(("PlaneInfo (plane 0):\n"));
  PDC_LOG(("    fbmem: %p\n", pinfo.fbmem));
  PDC_LOG(("    fblen: %lu\n", (unsigned long)pinfo.fblen));
  PDC_LOG(("   stride: %u\n", pinfo.stride));
  PDC_LOG(("  display: %u\n", pinfo.display));
  PDC_LOG(("      bpp: %u\n", pinfo.bpp));

  g_pdc_fbstate.stride = pinfo.stride; /* Length of a line in bytes */
  g_pdc_fbstate.bpp    = pinfo.bpp;    /* Bits per pixel */

  /* Only these pixel depths are supported.  vinfo.fmt is ignored, only
   * certain color formats are supported.
   */

  if (pinfo.bpp != 32 && pinfo.bpp != 16 &&
      pinfo.bpp != 8  && pinfo.bpp != 1)
    {
      PDC_LOG(("ERROR: bpp=%u not supported\n", pinfo.bpp));
      goto errout_with_fd;
    }

  /* mmap() the framebuffer.
   *
   * NOTE: In the FLAT build the frame buffer address returned by the
   * FBIOGET_PLANEINFO IOCTL command will be the same as the framebuffer
   * address.  mmap(), however, is the preferred way to get the framebuffer
   * address because in the KERNEL build, it will perform the necessary
   * address mapping to make the memory accessible to the application.
   */

  g_pdc_fbstate.fbmem = mmap(NULL, pinfo.fblen, PROT_READ|PROT_WRITE,
                             MAP_SHARED|MAP_FILE, g_pdc_fbstate.fd, 0);
  if (g_pdc_fbstate.fbmem == MAP_FAILED)
    {
      PDC_LOG(("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n", errno));
      goto errout_with_fd;
    }

  PDC_LOG(("Mapped FB: %p\n", g_pdc_fbstate.fbmem));

  /* The the handle for the selected font */

  g_pdc_fbstate.hfont = nxf_getfonthandle(PDCURSES_FONTID);
  if (g_pdc_fbstate.hfont == NULL)
    {
      PDC_LOG(("ERROR: Failed to get font handle: %d\n", errno);)
      goto errout_with_fd;
    }

  /* Get information about the fontset */

  fontset = nxf_getfontset(g_pdc_fbstate.hfont);
  if (fontset == NULL)
    {
      PDC_LOG(("ERROR: Failed to get font handle: %d\n", errno);)
      goto errout_with_font;
    }

  PDC_LOG(("Fonset (ID=%d):\n", PDCURSES_FONTID));
  PDC_LOG((" mxheight: %u\n", fontset->mxheight));
  PDC_LOG(("  mxwidth: %u\n", fontset->mxwidth));
  PDC_LOG(("   mxbits: %u\n", fontset->mxbits));
  PDC_LOG(("  spwidth: %u\n", fontset->spwidth));

  g_pdc_fbstate.fheight = fontset->mxheight;
  g_pdc_fbstate.fwidth  = fontset->mxwidth;

  /* Calculate the drawable region */

  SP->lines             = g_pdc_fbstate.yres / g_pdc_fbstate.fheight;
  SP->cols              = g_pdc_fbstate.xres / g_pdc_fbstate.fwidth;

  g_pdc_fbstate.hoffset = (g_pdc_fbstate.yres - g_pdc_fbstate.fheight * SP->lines) / 2;
  g_pdc_fbstate.hoffset = (g_pdc_fbstate.yres - g_pdc_fbstate.fheight * SP->lines) / 2;

  return OK;

errout_with_font:

errout_with_fd:
  close(g_pdc_fbstate.fd);
  g_pdc_fbstate.fd = -1;

errout_with_sp:
  free(SP);
  SP = NULL;
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
