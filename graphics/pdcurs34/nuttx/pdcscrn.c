/****************************************************************************
 * apps/graphics/nuttx/pdcscrn.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <graphics/curses.h>
#include "pdcnuttx.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_TERMCURSES
#ifndef CONFIG_PDCURSES_MULTITHREAD
bool graphic_screen = false;
#endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_scr_free_term
 *
 * Description:
 *   Frees the memory for SP allocated by PDC_scr_open(). Called by
 *   delscreen().
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_TERMCURSES
static void PDC_scr_free_term(FAR SCREEN *sp)
{
  FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *)sp;
  FAR struct pdc_termstate_s  *termstate = &termscreen->termstate;

  /* Deinitialize termcurses */

  termcurses_deinitterm(termstate->tcurs);

  /* Free the memory */

  free(termscreen);
#ifdef CONFIG_PDCURSES_MULTITHREAD
  PDC_ctx_free();
#endif
}
#endif

/****************************************************************************
 * Name: PDC_scr_open_term
 *
 * Description:
 *   The platform-specific part of initscr().  It's actually called from
 *   Xinitscr(); the arguments, if present, correspond to those used with
 *   main(), and may be used to set the title of the terminal window, or for
 *   other, platform-specific purposes.  This opens a terminal type screen
 *   backed by the NuttX termcurses interface.)  PDC_scr_open() must allocate
 *   memory for SP, and must initialize acs_map[] (unless it's preset) and
 *   several members of SP, *   including lines, cols, mouse_wait, orig_attr
 *   (and if orig_attr is true, orig_fore and orig_back), mono, _restore and
 *   _preserve.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_TERMCURSES
static int PDC_scr_open_term(int argc, char **argv)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  int             ret;
  int             i;
  struct winsize  winsz;
  FAR struct pdc_termscreen_s *termscreen;
  FAR struct pdc_termstate_s  *termstate;
  FAR char *term_type;

  /* Determine the terminal type */

  if (argc > 1)
    {
      term_type = argv[1];
    }
  else
    {
      term_type = NULL;
    }

  /* Allocate the SP */

  termscreen = (FAR struct pdc_termscreen_s *)
    zalloc(sizeof(struct pdc_termscreen_s));

  if (termscreen == NULL)
    {
      PDC_LOG(("ERROR: Failed to allocate SP\n"));
      return ERR;
    }

  termstate = &termscreen->termstate;

  /* Try to initialize termcurses on stdout */

  ret = termcurses_initterm(term_type, 0, 1, &termstate->tcurs);
  if (ret != OK)
    {
      PDC_LOG(("ERROR: Failed to initialize termcurses based SP\n"));
      free(termscreen);
      return ERR;
    }

  /* Try to get the terminal size in rows / cols */

  ret = termcurses_getwinsize(termstate->tcurs, &winsz);
  if (ret != OK)
    {
      PDC_LOG(("ERROR: Terminal termcurses driver doesn't support size reporting\n"));
      free(termscreen);
      return ERR;
    }

  graphic_screen               = false;
  SP                           = &termscreen->screen;
  SP->lines                    = winsz.ws_row;
  SP->cols                     = winsz.ws_col;
  termscreen->termstate.out_fd = 1;
  termscreen->termstate.in_fd  = 0;
  termscreen->termstate.fg_red = 0xFFFE;
  termscreen->termstate.bg_red = 0xFFFE;
  termstate                    = &termscreen->termstate;

  /* Setup initial RGB colors */

  for (i = 0; i < 8; i++)
    {
#ifdef PDCURSES_MONOCHROME
      uint8_t greylevel;

      greylevel                        = (i & COLOR_RED)   ? 0x40 : 0;
      greylevel                       += (i & COLOR_GREEN) ? 0x40 : 0;
      greylevel                       += (i & COLOR_BLUE)  ? 0x40 : 0;

      termstate->greylevel[i]          = greylevel;
      termstate->greylevel[i+8]        = greylevel | 0x3f;
#else
      termstate->rgbcolor[i].red       = (i & COLOR_RED)   ? 0xc0 : 0;
      termstate->rgbcolor[i].green     = (i & COLOR_GREEN) ? 0xc0 : 0;
      termstate->rgbcolor[i].blue      = (i & COLOR_BLUE)  ? 0xc0 : 0;

      termstate->rgbcolor[i + 8].red   = (i & COLOR_RED)   ? 0xff : 0x40;
      termstate->rgbcolor[i + 8].green = (i & COLOR_GREEN) ? 0xff : 0x40;
      termstate->rgbcolor[i + 8].blue  = (i & COLOR_BLUE)  ? 0xff : 0x40;
#endif
    }

  COLORS = 16;

#ifdef CONFIG_PDCURSES_HAVE_INPUT
  ret = PDC_input_open(NULL);
  if (ret != OK)
    {
      /* Free the memory ... can't open input */

#ifdef CONFIG_PDCURSES_MULTITHREAD
      PDC_ctx_free();
#endif
      free(termscreen);
    }
#endif

  return ret;
}
#endif /* CONFIG_SYSTEM_TERMCURSES */

/****************************************************************************
 * Name: PDC_init_pair_term
 *
 * Description:
 *   The core of init_pair().  This does all the work of that function, except
 *   checking for values out of range.  The values passed to this function
 *   should be returned by a call to PDC_pair_content() with the same pair
 *   number.  PDC_transform_line() should use the specified colors when
 *   rendering a chtype with the given pair number.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_TERMCURSES
static void PDC_init_pair_term(FAR SCREEN *sp, short pair, short fg, short bg)
{
  FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *)sp;

  termscreen->termstate.colorpair[pair].fg = fg;
  termscreen->termstate.colorpair[pair].bg = bg;
}
#endif /* CONFIG_SYSTEM_TERMCURSES */

/****************************************************************************
 * Name: PDC_init_color
 *
 * Description:
 *   The core of init_color().  This does all the work of that function,
 *   except checking for values out of range.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_TERMCURSES
int PDC_init_color_term(FAR SCREEN *sp, short color, short red, short green,
                        short blue)
{
  FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *)sp;
  FAR struct pdc_termstate_s  *termstate;

  DEBUGASSERT(termscreen != NULL);
  termstate = &termscreen->termstate;

#ifdef PDCURSES_MONOCHROME
  greylevel                        = (DIVROUND(red * 255, 1000) +
                                      DIVROUND(green * 255, 1000) +
                                      DIVROUND(blue * 255, 1000)) / 3;
  termstate->greylevel[color]      = (uint8_t)greylevel;
#else
  termstate->rgbcolor[color].red   = DIVROUND(red * 255, 1000);
  termstate->rgbcolor[color].green = DIVROUND(green * 255, 1000);
  termstate->rgbcolor[color].blue  = DIVROUND(blue * 255, 1000);
#endif

  return OK;
}
#endif   /* CONFIG_SYSTEM_TERMCURSES */

/****************************************************************************
 * Name: PDC_color_content_term
 *
 * Description:
 *   The core of color_content().  This does all the work of that function,
 *   except checking for values out of range and null pointers.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_TERMCURSES
int PDC_color_content_term(FAR SCREEN *sp, short color,
                           FAR short *red, FAR short *green, FAR short *blue)
{
  FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *)sp;
  FAR struct pdc_termstate_s  *termstate;

  DEBUGASSERT(termscreen != NULL);
  termstate = &termscreen->termstate;

#ifdef PDCURSES_MONOCHROME
  greylevel = DIVROUND(termstate->greylevel[color] * 1000, 255);
  *red      = greylevel;
  *green    = greylevel;
  *blue     = greylevel;
#else
  *red      = DIVROUND(termstate->rgbcolor[color].red * 1000, 255);
  *green    = DIVROUND(termstate->rgbcolor[color].green * 1000, 255);
  *blue     = DIVROUND(termstate->rgbcolor[color].blue * 1000, 255);
#endif

  return OK;
}
#endif /* CONFIG_SYSTEM_TERMCURSES */

/****************************************************************************
 * Name: PDC_pair_content_term
 *
 * Description:
 *   The core of pair_content().  This does all the work of that function,
 *   except checking for values out of range and null pointers.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_TERMCURSES
static int PDC_pair_content_term(FAR SCREEN *sp, short pair, short *fg, short *bg)
{
  FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *)sp;

  *fg = termscreen->termstate.colorpair[pair].fg;
  *bg = termscreen->termstate.colorpair[pair].bg;
  return OK;
}
#endif /* CONFIG_SYSTEM_TERMCURSES */

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
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("PDC_scr_close() - called\n"));

  /* Ensure cursor is visible */

  curs_set(1);
  attrset(COLOR_PAIR(0));

  /* Delete the screen */

  delscreen(SP);
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
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;

#ifdef CONFIG_SYSTEM_TERMCURSES
  if (!graphic_screen)
    {
      PDC_scr_free_term(SP);
      return;
    }

#endif

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

  close(fbstate->fbfd);
#ifdef CONFIG_PDCURSES_HAVE_INPUT
  PDC_input_close(fbstate);
#endif
  free(fbscreen);
  SP = NULL;

#ifdef CONFIG_PDCURSES_MULTITHREAD
  PDC_ctx_free();
#endif
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
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
#ifdef CONFIG_SYSTEM_TERMCURSES
  FAR char *env_display;
#endif
  FAR struct pdc_fbscreen_s *fbscreen;
  FAR const struct nx_font_s *fontset;
  FAR struct pdc_fbstate_s *fbstate;
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  int ret;
  int i;

  PDC_LOG(("PDC_scr_open() - called\n"));

#ifdef CONFIG_SYSTEM_TERMCURSES
  /* When termcurses is compiled in, we must allow opening terminal
   * type screens also.  Check if the DISPLAY environment variable is
   * set to ":0" indicating we should open the graphic screen.
   */

  env_display = getenv("DISPLAY");
  if (!env_display || strcmp(env_display, ":0") != 0)
    {
      /* Perform terminal open operation */

      return PDC_scr_open_term(argc, argv);
    }
#endif

  /* Allocate the global instance of SP */

  fbscreen = (FAR struct pdc_fbscreen_s *)zalloc(sizeof(struct pdc_fbscreen_s));
  if (fbscreen == NULL)
    {
      PDC_LOG(("ERROR: Failed to allocate SP\n"));
      return ERR;
    }

  SP      = &fbscreen->screen;
  fbstate = &fbscreen->fbstate;
#ifdef CONFIG_SYSTEM_TERMCURSES
  graphic_screen = true;
#endif

  /* Number of RGB colors/greyscale levels.  This is the same as the
   * dimension of rgbcolor[] or greylevel[]).
   */

  COLORS = 16;

  /* Setup initial RGB colors */

  for (i = 0; i < 8; i++)
    {
#ifdef PDCURSES_MONOCHROME
      uint8_t greylevel;

      greylevel                      = (i & COLOR_RED)   ? 0x40 : 0;
      greylevel                     += (i & COLOR_GREEN) ? 0x40 : 0;
      greylevel                     += (i & COLOR_BLUE)  ? 0x40 : 0;

      fbstate->greylevel[i]          = greylevel;
      fbstate->greylevel[i+8]        = greylevel | 0x3f;
#else
      fbstate->rgbcolor[i].red       = (i & COLOR_RED)   ? 0xc0 : 0;
      fbstate->rgbcolor[i].green     = (i & COLOR_GREEN) ? 0xc0 : 0;
      fbstate->rgbcolor[i].blue      = (i & COLOR_BLUE)  ? 0xc0 : 0;

      fbstate->rgbcolor[i + 8].red   = (i & COLOR_RED)   ? 0xff : 0x40;
      fbstate->rgbcolor[i + 8].green = (i & COLOR_GREEN) ? 0xff : 0x40;
      fbstate->rgbcolor[i + 8].blue  = (i & COLOR_BLUE)  ? 0xff : 0x40;
#endif
    }

  /* Open the framebuffer driver */

  fbstate->fbfd = open(CONFIG_PDCURSES_FBDEV, O_RDWR);
  if (fbstate->fbfd < 0)
    {
      PDC_LOG(("ERROR: Failed to open %s: %d\n",
               CONFIG_PDCURSES_FBDEV, errno));
      goto errout_with_sp;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(fbstate->fbfd, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&vinfo));
  if (ret < 0)
    {
      PDC_LOG(("ERROR: ioctl(FBIOGET_VIDEOINFO) failed: %d\n", errno));
      goto errout_with_fbfd;
    }

  PDC_LOG(("VideoInfo:\n"));
  PDC_LOG(("      fmt: %u\n", vinfo.fmt));
  PDC_LOG(("     xres: %u\n", vinfo.xres));
  PDC_LOG(("     yres: %u\n", vinfo.yres));
  PDC_LOG(("  nplanes: %u\n", vinfo.nplanes));

  fbstate->xres = vinfo.xres;  /* Horizontal resolution in pixel columns */
  fbstate->yres = vinfo.yres;  /* Vertical resolution in pixel rows */

  /* Only one color format supported. */

  if (vinfo.fmt != PDCURSES_COLORFMT)
    {
      PDC_LOG(("ERROR: color format=%u not supported\n", vinfo.fmt));
      goto errout_with_fbfd;
    }

#ifdef PDCURSES_MONOCHROME
  /* Remember if we are using a monochrome framebuffer */

  SP->mono = true;
#endif

  /* Get characteristics of the color plane */

  ret = ioctl(fbstate->fbfd, FBIOGET_PLANEINFO,
              (unsigned long)((uintptr_t)&pinfo));
  if (ret < 0)
    {
      PDC_LOG(("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n", errno));
      goto errout_with_fbfd;
    }

  PDC_LOG(("PlaneInfo (plane 0):\n"));
  PDC_LOG(("    fbmem: %p\n", pinfo.fbmem));
  PDC_LOG(("    fblen: %lu\n", (unsigned long)pinfo.fblen));
  PDC_LOG(("   stride: %u\n", pinfo.stride));
  PDC_LOG(("  display: %u\n", pinfo.display));
  PDC_LOG(("      bpp: %u\n", pinfo.bpp));

  fbstate->stride = pinfo.stride; /* Length of a line in bytes */

  /* Only one pixel depth is supported. */

  if (pinfo.bpp != PDCURSES_BPP)
    {
      PDC_LOG(("ERROR: bpp=%u not supported\n", pinfo.bpp));
      goto errout_with_fbfd;
    }

  /* mmap() the framebuffer.
   *
   * NOTE: In the FLAT build the frame buffer address returned by the
   * FBIOGET_PLANEINFO IOCTL command will be the same as the framebuffer
   * address.  mmap(), however, is the preferred way to get the framebuffer
   * address because in the KERNEL build, it will perform the necessary
   * address mapping to make the memory accessible to the application.
   */

  fbstate->fbmem = mmap(NULL, pinfo.fblen, PROT_READ|PROT_WRITE,
                        MAP_SHARED|MAP_FILE, fbstate->fbfd, 0);
  if (fbstate->fbmem == MAP_FAILED)
    {
      PDC_LOG(("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n", errno));
      goto errout_with_fbfd;
    }

  PDC_LOG(("Mapped FB: %p\n", fbstate->fbmem));

  /* Get the handle for the selected font */

  fbstate->hfont = nxf_getfonthandle(PDCURSES_FONTID);
  if (fbstate->hfont == NULL)
    {
      PDC_LOG(("ERROR: Failed to get font handle: %d\n", errno));
      goto errout_with_fbfd;
    }

#ifdef HAVE_BOLD_FONT
  /* Get the handle for the matching bold font.  It is assume that the
   * geometry of the bold font is exactly the same as the geometry of
   * the "normal" font.
   */

  fbstate->hbold = nxf_getfonthandle(PDCURSES_BOLD_FONTID);
  if (fbstate->hbold == NULL)
    {
      PDC_LOG(("ERROR: Failed to get font handle: %d\n", errno));
      goto errout_with_font;
    }
#endif

  /* Get information about the fontset */

  fontset = nxf_getfontset(fbstate->hfont);
  if (fontset == NULL)
    {
      PDC_LOG(("ERROR: Failed to get font handle: %d\n", errno));
      goto errout_with_boldfont;
    }

  PDC_LOG(("Fontset (ID=%d):\n", PDCURSES_FONTID));
  PDC_LOG((" mxheight: %u\n", fontset->mxheight));
  PDC_LOG(("  mxwidth: %u\n", fontset->mxwidth));
  PDC_LOG(("   mxbits: %u\n", fontset->mxbits));
  PDC_LOG(("  spwidth: %u\n", fontset->spwidth));

  fbstate->fheight = fontset->mxheight;
  fbstate->fwidth  = fontset->mxwidth;

#if PDCURSES_BPP < 8
  /* Allocate the font buffer for < 8-bit display */

  fbstate->fstride =
    (fbstate->fwidth + PDCURSES_PPB_MASK) >> PDCURSES_PPB_SHIFT;
  fbstate->fbuffer  = (FAR uint8_t *)
    malloc(fbstate->fstride * fbstate->fheight);

  if (fbstate->fbuffer == NULL)
    {
      PDC_LOG(("ERROR: Failed to allocate fstride: %d\n", errno));
      goto errout_with_boldfont;
    }
#endif

  /* Calculate the drawable region */

  SP->lines        = fbstate->yres / fbstate->fheight;
  SP->cols         = fbstate->xres / fbstate->fwidth;

  fbstate->hoffset = (fbstate->xres - fbstate->fwidth * SP->cols) / 2;
  fbstate->voffset = (fbstate->yres - fbstate->fheight * SP->lines) / 2;

  /* Set the framebuffer to a known state */

  PDC_clear_screen(fbstate);

#ifdef CONFIG_PDCURSES_HAVE_INPUT
  /* Open and configure any input devices */

  ret = PDC_input_open(fbstate);
  if (ret == ERR)
    {
      goto errout_with_fbuffer;
    }
#endif

  return OK;

#ifdef CONFIG_PDCURSES_HAVE_INPUT
errout_with_fbuffer:
#if PDCURSES_BPP < 8
  free(fbstate->fbuffer);
#endif
#endif

errout_with_boldfont:
#ifdef HAVE_BOLD_FONT
errout_with_font:
#endif

errout_with_fbfd:
  close(fbstate->fbfd);
  fbstate->fbfd = -1;

errout_with_sp:
  free(SP);
  SP = NULL;
  return ERR;
}

/****************************************************************************
 * Name: PDC_resize_screen
 *
 * Description:
 *   This does the main work of resize_term().  It may respond to non-zero
 *   parameters, by setting the screen to the specified size; to zero
 *   parameters, by setting the screen to a size chosen by the user at
 *   runtime, in an unspecified way (e.g., by dragging the edges of the
 *   window); or both.  It may also do nothing, if there's no appropriate
 *   action for the platform.
 *
 ****************************************************************************/

int PDC_resize_screen(int nlines, int ncols)
{
  PDC_LOG(("PDC_resize_screen() - called. Lines: %d Cols: %d\n",
           nlines, ncols));

  /* We cannot resize the hardware framebuffer */

  return ERR;
}

/****************************************************************************
 * Name: PDC_reset_prog_mode
 *
 * Description:
 *   The non-portable functionality of reset_prog_mode() is handled here --
 *   whatever's not done in _restore_mode().  In current ports:  In OS/2, this
 *   sets the keyboard to binary mode; in Win32, it enables or disables the
 *   mouse pointer to match the saved mode; in others it does nothing.
 *
 ****************************************************************************/

void PDC_reset_prog_mode(void)
{
  PDC_LOG(("PDC_reset_prog_mode() - called.\n"));
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
}

/****************************************************************************
 * Name: PDC_restore_screen_mode
 *
 * Description:
 *  Called from _restore_mode() in pdc_kernel.c, this function does the
 *  actual mode changing, if applicable.  Currently used only in DOS and
 *  OS/2.
 *
 ****************************************************************************/

void PDC_restore_screen_mode(int i)
{
  PDC_LOG(("PDC_restore_screen_mode().  i=%d.\n", i));
}

/****************************************************************************
 * Name: PDC_save_screen_mode
 *
 * Description:
 *   Called from _save_mode() in pdc_kernel.c, this function saves the actual
 *   screen mode, if applicable.  Currently used only in DOS and OS/2.
 *
 ****************************************************************************/

void PDC_save_screen_mode(int i)
{
  PDC_LOG(("PDC_save_screen_mode().  i=%d.\n", i));
}

/****************************************************************************
 * Name: PDC_init_pair
 *
 * Description:
 *   The core of init_pair().  This does all the work of that function, except
 *   checking for values out of range.  The values passed to this function
 *   should be returned by a call to PDC_pair_content() with the same pair
 *   number.  PDC_transform_line() should use the specified colors when
 *   rendering a chtype with the given pair number.
 *
 ****************************************************************************/

void PDC_init_pair(short pair, short fg, short bg)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;

  PDC_LOG(("PDC_init_pair().  pair=%d, fg=%d, bg=%d\n", pair, fg, bg));

#ifdef CONFIG_SYSTEM_TERMCURSES
  if (!graphic_screen)
    {
      /* Process as terminal mode screen */

      PDC_init_pair_term(SP, pair, fg, bg);
      return;
    }
#endif /* CONFIG_SYSTEM_TERMCURSES */

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

  fbstate->colorpair[pair].fg = fg;
  fbstate->colorpair[pair].bg = bg;
}

/****************************************************************************
 * Name: PDC_pair_content
 *
 * Description:
 *   The core of pair_content().  This does all the work of that function,
 *   except checking for values out of range and null pointers.
 *
 ****************************************************************************/

int PDC_pair_content(short pair, short *fg, short *bg)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;

  PDC_LOG(("PDC_pair_content().  pair=%d\n", pair));

#ifdef CONFIG_SYSTEM_TERMCURSES
  if (!graphic_screen)
    {
      /* Process as terminal mode screen */

      return PDC_pair_content_term(SP, pair, fg, bg);
    }
#endif /* CONFIG_SYSTEM_TERMCURSES */

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

  *fg = fbstate->colorpair[pair].fg;
  *bg = fbstate->colorpair[pair].bg;
  return OK;
}

/****************************************************************************
 * Name: PDC_can_change_color
 *
 * Description:
 *   Returns true if init_color() and color_content() give meaningful
 *   results, false otherwise.  Called from can_change_color().
 *
 ****************************************************************************/

bool PDC_can_change_color(void)
{
  return true;
}

/****************************************************************************
 * Name: PDC_color_content
 *
 * Description:
 *   The core of color_content().  This does all the work of that function,
 *   except checking for values out of range and null pointers.
 *
 ****************************************************************************/

int PDC_color_content(short color, short *red, short *green, short *blue)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;
#ifdef PDCURSES_MONOCHROME
  short greylevel;
#endif

  PDC_LOG(("PDC_init_color().  color=%d\n", color));

#ifdef CONFIG_SYSTEM_TERMCURSES
  if (!graphic_screen)
    {
      /* Process as terminal mode screen */

      return PDC_color_content_term(SP, color, red, green, blue);
    }
#endif

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

#ifdef PDCURSES_MONOCHROME
  greylevel = DIVROUND(fbstate->greylevel[color] * 1000, 255);
  *red      = greylevel;
  *green    = greylevel;
  *blue     = greylevel;
#else
  *red      = DIVROUND(fbstate->rgbcolor[color].red * 1000, 255);
  *green    = DIVROUND(fbstate->rgbcolor[color].green * 1000, 255);
  *blue     = DIVROUND(fbstate->rgbcolor[color].blue * 1000, 255);
#endif

  return OK;
}

/****************************************************************************
 * Name: PDC_init_color
 *
 * Description:
 *   The core of init_color().  This does all the work of that function,
 *   except checking for values out of range.
 *
 ****************************************************************************/

int PDC_init_color(short color, short red, short green, short blue)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;
#ifdef PDCURSES_MONOCHROME
  uint16_t greylevel;
#endif

  PDC_LOG(("PDC_init_color().  color=%d, red=%d, green=%d, blue=%d\n",
           color, red, green, blue));

#ifdef CONFIG_SYSTEM_TERMCURSES
  if (!graphic_screen)
    {
      /* Process as terminal mode screen */

      return PDC_init_color_term(SP, color, red, green, blue);
    }
#endif

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

#ifdef PDCURSES_MONOCHROME
  greylevel                      = (DIVROUND(red * 255, 1000) +
                                    DIVROUND(green * 255, 1000) +
                                    DIVROUND(blue * 255, 1000)) / 3;
  fbstate->greylevel[color]      = (uint8_t)greylevel;
#else
  fbstate->rgbcolor[color].red   = DIVROUND(red * 255, 1000);
  fbstate->rgbcolor[color].green = DIVROUND(green * 255, 1000);
  fbstate->rgbcolor[color].blue  = DIVROUND(blue * 255, 1000);
#endif

  return OK;
}
