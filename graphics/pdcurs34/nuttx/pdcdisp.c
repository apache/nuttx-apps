/****************************************************************************
 * apps/graphics/nuttx/pdcdisp.c
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

/* A port of PDCurses must provide acs_map[], a 128-element array of chtypes,
 * with values laid out based on the Alternate Character Set of the VT100
 * (see curses.h).  PDC_transform_line() must use this table; when it
 * encounters a chtype with the A_ALTCHARSET flag set, and an A_CHARTEXT
 * value in the range 0-127, it must render it using the A_CHARTEXT portion
 * of the corresponding value from this table, instead of the original
 * value.  Also, values may be read from this table by apps, and passed
 * through functions such as waddch(), which does no special processing on
 * control characters (0-31 and 127) when the A_ALTCHARSET flag is set.
 * Thus, any control characters used in acs_map[] should also have the
 * A_ALTCHARSET flag set. Implementations should provide suitable values
 * for all the ACS_ macros defined in curses.h; other values in the table
 * should be filled with their own indices (e.g., acs_map['E'] == 'E'). The
 * table can be either hardwired, or filled by PDC_scr_open(). Existing
 * ports define it in pdcdisp.c, but this is not required.
 */

#ifdef CONFIG_PDCURSES_CHTYPE_LONG

# define A(x) ((chtype)x | A_ALTCHARSET)

chtype acs_map[128] =
{
  A(0), A(1), A(2), A(3), A(4), A(5), A(6), A(7), A(8), A(9),
  A(10), A(11), A(12), A(13), A(14), A(15), A(16), A(17), A(18),
  A(19), A(20), A(21), A(22), A(23), A(24), A(25), A(26), A(27),
  A(28), A(29), A(30), A(31), ' ', '!', '"', '#', '$', '%', '&',
  '\'', '(', ')', '*',

  A(0x1a), A(0x1b), A(0x18), A(0x19),

  '/',

  0xdb,

  '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=',
  '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
  'X', 'Y', 'Z', '[', '\\', ']', '^', '_',

  A(0x04), 0xb1,

  'b', 'c', 'd', 'e',

  0xf8, 0xf1, 0xb0, A(0x0f), 0xd9, 0xbf, 0xda, 0xc0, 0xc5, 0x2d,
  0x2d, 0xc4, 0x2d, 0x5f, 0xc3, 0xb4, 0xc1, 0xc2, 0xb3, 0xf3,
  0xf2, 0xe3, 0xd8, 0x9c, 0xf9,

  A(127)
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_pixel_[x|y]
 *
 * Description:
 *   Convert row or column text position to framebuffer x/y positions
 *   in pixels.
 *
 ****************************************************************************/

static inline fb_coord_t PDC_pixel_x(FAR struct pdc_fbstate_s *fbstate,
                                     int col)
{
  return col * fbstate->fwidth + fbstate->hoffset;
}

static inline fb_coord_t PDC_pixel_y(FAR struct pdc_fbstate_s *fbstate,
                                     int row)
{
  return row * fbstate->fheight + fbstate->voffset;
}

/****************************************************************************
 * Name: PDC_fbmem_[x|y]
 *
 * Description:
 *   Convert row or column pixel position to framebuffer x/y byte offsets.
 *
 ****************************************************************************/

static inline uintptr_t PDC_fbmem_x(FAR struct pdc_fbstate_s *fbstate,
                                   int col)
{
  return (PDC_pixel_x(fbstate, col) * PDCURSES_BPP + 7) >> 3;
}

static inline uintptr_t PDC_fbmem_y(FAR struct pdc_fbstate_s *fbstate,
                                    int row)
{
  return PDC_pixel_y(fbstate, row) * fbstate->stride;
}

/****************************************************************************
 * Name: PDC_color
 *
 * Description:
 *   Convert a pixel code to a RGB device pixel.
 *
 ****************************************************************************/

static inline pdc_color_t PDC_color(FAR struct pdc_fbstate_s *fbstate,
                                    short color)
{
#if defined(CONFIG_PDCURSES_COLORFMT_RGB332)
  return  RGBTO8(fbstate->rgbcolor[color].red,
                 fbstate->rgbcolor[color].green,
                 fbstate->rgbcolor[color].blue);
#elif defined(CONFIG_PDCURSES_COLORFMT_RGB565)
  return RGBTO16(fbstate->rgbcolor[color].red,
                 fbstate->rgbcolor[color].green,
                 fbstate->rgbcolor[color].blue);
#elif defined(CONFIG_PDCURSES_COLORFMT_RGB888)
  return RGBTO24(fbstate->rgbcolor[color].red,
                 fbstate->rgbcolor[color].green,
                 fbstate->rgbcolor[color].blue);
#else
#  error No color format selected
  return 0;
#endif
}

/****************************************************************************
 * Name: PDC_set_bg
 *
 * Description:
 *   Set the glyph memory to the device background RGB color.
 *
 ****************************************************************************/

static inline void PDC_set_bg(FAR struct pdc_fbstate_s *fbstate,
                              FAR uint8_t *fbstart, short bg)
{
  pdc_color_t bgcolor = PDC_color(fbstate, bg);
  int row;

  /* Set the glyph to the background color. */

  for (row = 0; row < fbstate->fheight; row++, fbstart += fbstate->stride)
    {
      FAR pdc_color_t *fbdest;
      int col;

      for (col = 0, fbdest = (FAR pdc_color_t *)fbstart;
           col < fbstate->fwidth;
           col++)
        {
          *fbdest++ = bgcolor;
        }
    }
}

/****************************************************************************
 * Name: PDC_render_gyph
 *
 * Description:
 *   Render the font into the glyph memory using the foreground RGB color.
 *
 ****************************************************************************/

static inline void PDC_render_gyph(FAR struct pdc_fbstate_s *fbstate,
                                   FAR const struct nx_fontbitmap_s *fbm,
                                   FAR uint8_t *fbstart, short fg)
{
  pdc_color_t fgcolor = PDC_color(fbstate, fg);
  int ret;

  /* Then render the glyph into the allocated memory
   *
   * REVISIT:  The case where visibility==1 is not yet handled.  In that
   * case, only the lowe quarter of the glyph should be reversed.
   */

  ret = RENDERER((FAR pdc_color_t *)fbstart,
                  fbstate->fheight, fbstate->fwidth, fbstate->stride,
                  fbm, fgcolor);
  if (ret < 0)
    {
      /* Actually, the RENDERER never returns a failure */

      PDC_LOG(("ERROR:  RENDERER failed: %d\n", ret));
    }
}

/****************************************************************************
 * Name: PDC_update
 *
 * Description:
 *   Update the LCD display is necessary.
 *
 ****************************************************************************/

#ifdef CONFIG_LCD_UPDATE
static void PDC_update(FAR struct pdc_fbstate_s *fbstate, int row, int col,
                       int nchars)
{
  struct nxgl_rect_s rect;
  int ret;

  if (nchars > 0)
    {
      /* Setup the bounding rectangle */

      rect.pt1.x = PDC_pixel_x(FAR fbstate, col);
      rect.pt1.y = PDC_pixel_x(FAR fbstate, row);
      rect.pt2.x = rect.pt1.x + nchars * fbstate->fwidth - 1;
      rect.pt2.y = y + fbstate->fheight - 1;

      /* Then perfom the update via IOCTL */

      ret = ioctl(fbstate->fd, FBIO_UPDATE,
                  (unsigned long)((uintptr_t)rect));
      if (ret < 0)
        {
          PDC_LOG(("ERROR:  ioctl(FBIO_UPDATE) failed: %d\n", errno));
        }
    }
}
#else
#  define PDC_update(f,r,c,n)
#endif

/****************************************************************************
 * Name: PDC_putc
 *
 * Description:
 *   Put one character with selected attributes at the selected drawing
 *   position.
 *
 ****************************************************************************/

static void PDC_putc(FAR struct pdc_fbstate_s *fbstate, int row, int col,
                     chtype ch)
{
  FAR const struct nx_fontbitmap_s *fbm;
  FAR uint8_t *dest;
  short fg;
  short bg;
#ifdef HAVE_BOLD_FONT
  bool bold = ((ch & A_BOLD) != 0);
#endif

  /* Clip */

  if (row < 0 || row >= SP->lines || col < 0 || col >= SP->cols)
    {
      PDC_LOG(("ERROR: Position out of range: row=%d col=%d\n", row, col));
      return;
    }

 /* Get the forground and background colors of the character */

 PDC_pair_content(PAIR_NUMBER(ch), &fg, &bg);

 /* Handle the A_REVERSE attribute. */

 if ((ch & A_REVERSE) != 0)
   {
     /* Swap the foreground and background colors if reversed */

     short tmp = fg;
     fg = bg;
     bg = tmp;
   }

#ifdef CONFIG_PDCURSES_CHTYPE_LONG
  /* Translate characters 0-127 via acs_map[], if they're flagged with
   * A_ALTCHARSET in the attribute portion of the chtype.
   */

  if (ch & A_ALTCHARSET && !(ch & 0xff80))
    {
      ch = (ch & (A_ATTRIBUTES ^ A_ALTCHARSET)) | acs_map[ch & 0x7f];
    }
#endif

  /* Calculation the destination address in the framebuffer */

  dest = (FAR uint8_t *)fbstate->fbmem +
                        PDC_fbmem_y(fbstate, row) +
                        PDC_fbmem_x(fbstate, col);

  /* Initialize the glyph to the (possibly reversed) background color */

  PDC_set_bg(fbstate, dest, bg);

  /* Does the code map to a font? */

#ifdef HAVE_BOLD_FONT
  fbm = nxf_getbitmap(bold ? fbstate->hfont : fbstate->hbold,
                      ch & A_CHARTEXT);
#else
  fbm = nxf_getbitmap(fbstate->hfont, ch & A_CHARTEXT);
#endif

  if (fbm != NULL)
    {
      /* Yes.. render the glyph */

      PDC_render_gyph(fbstate, fbm, dest, fg);
    }

  /* Apply more attributes */

  if ((ch & (A_UNDERLINE | A_LEFTLINE | A_RIGHTLINE)) != 0)
    {
#warning Missing logic
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_gotoyx
 *
 * Description:
 *   Move the physical cursor (as opposed to the logical cursor affected by
 *   wmove()) to the given location. T his is called mainly from doupdate().
 *   In general, this function need not compare the old location with the
 *   new one, and should just move the cursor unconditionally.
 *
 ****************************************************************************/

void PDC_gotoyx(int row, int col)
{
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;
  int oldrow;
  int oldcol;
  chtype ch;

  PDC_LOG(("PDC_gotoyx() - called: row %d col %d\n", row, col));

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

  /* Clear the old cursor */

  oldrow = SP->cursrow;
  oldcol = SP->curscol;

  PDC_putc(fbstate, oldrow, oldcol, curscr->_y[oldrow][oldcol]);
  PDC_update(fbstate, oldrow, oldcol, 1);

  if (SP->visibility != 0)
    {
      /* Draw a new cursor by overprinting the existing character in
       * reverse, either the full cell (when visibility == 2) or the
       * lowest quarter of it (when visibility == 1)
       */

      ch = curscr->_y[row][col] ^ A_REVERSE;
      PDC_putc(fbstate, row, col, ch);
      PDC_update(fbstate, row, col, 1);
    }
}

/****************************************************************************
 * Name: PDC_transform_line
 *
 * Description:
 *   The core output routine.  It takes len chtype entities from srcp (a
 *   pointer into curscr) and renders them to the physical screen at line
 *   lineno, column x.  It must also translate characters 0-127 via acs_map[],
 *   if they're flagged with A_ALTCHARSET in the attribute portion of the
 *   chtype.
 *
 ****************************************************************************/

void PDC_transform_line(int lineno, int x, int len, const chtype *srcp)
{
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;
  int nextx;
  int i;

  PDC_LOG(("PDC_transform_line() - called: lineno=%d x=%d len=%d\n",
           lineno, x, len));

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

  /* Add each character to the framebuffer at the current position,
   * incrementing the horizontal position after each character.
   */

  for (i = 0, nextx = x; i < len; i++, nextx++)
    {
      if (nextx >= SP->cols)
        {
          PDC_LOG(("ERROR:  Write past end of line\n"));
          break;
        }

      /* Render the font glyph into the framebuffer */

      PDC_putc(fbstate, lineno, nextx, srcp[i]);
    }

  PDC_update(fbstate, lineno, x, nextx - x);
}
