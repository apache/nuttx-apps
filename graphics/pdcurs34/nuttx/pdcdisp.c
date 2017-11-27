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

#include <sys/ioctl.h>
#include <errno.h>

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
 *   NOTE: For BPP={8,16,32} this function returns the exact positions.  For
 *   the case of BPP={1,2,4}, PDC_fbmem_x() aligns down to the address of
 *   the byte Aligns down to the byte that contains the 'col' pixel.
 *
 ****************************************************************************/

static inline uintptr_t PDC_fbmem_x(FAR struct pdc_fbstate_s *fbstate,
                                   int col)
{
  return (PDC_pixel_x(fbstate, col) * PDCURSES_BPP) >> 3;
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
#if defined (CONFIG_PDCURSES_COLORFMT_Y1)
  /* Returns 8 pixels packed into a byte */

  return (fbstate->greylevel[color] & 0xc0) == 0 ? 0x00 : 0xff;

#elif defined (CONFIG_PDCURSES_COLORFMT_Y2)
  uint8_t color8;

  /* Returns 4 pixels packed into a byte */

  color8 = fbstate->greylevel[color] >> 6;
  color8 = color8 << 2 | color8;
  color8 = color8 << 4 | color8;

  return color8;

#elif defined (CONFIG_PDCURSES_COLORFMT_Y4)
  uint8_t color8;

  /* Returns 2 pixels packed into a byte */

  color8 = fbstate->greylevel[color] >> 4;
  color8 = color8 << 4 | color8;

  return color8;

#elif defined(CONFIG_PDCURSES_COLORFMT_RGB332)
  /* Returns 8-bit RGB332 */

  return  RGBTO8(fbstate->rgbcolor[color].red,
                 fbstate->rgbcolor[color].green,
                 fbstate->rgbcolor[color].blue);

#elif defined(CONFIG_PDCURSES_COLORFMT_RGB565)
  /* Returns 16-bit RGB565 */

  return RGBTO16(fbstate->rgbcolor[color].red,
                 fbstate->rgbcolor[color].green,
                 fbstate->rgbcolor[color].blue);

#elif defined(CONFIG_PDCURSES_COLORFMT_RGB888)
  /* Returns 32-bit RGB888 */

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

#if PDCURSES_BPP < 8
static inline void PDC_set_bg(FAR struct pdc_fbstate_s *fbstate,
                              FAR uint8_t *fbstart, int col, short bg)
{
  uint8_t color8;
  uint8_t lmask;
  uint8_t rmask;
  int startcol;
  int endcol;
  int row;

  /* Get a byte that packs multiple pixels into one byte */

  color8   = PDC_color(fbstate, bg);

  /* Handle the case where the first or last bytes may require read, modify,
   * write operations.
   *
   * Get the start and end column in pixels (relative to the start position)
   */

  startcol = col & PDCURSES_PPB_MASK;
  endcol   = startcol + fbstate->fwidth - 1;

  /* Get the masks that we will need to perform the read-modify-write
   * operations.
   */

#ifdef CONFIG_NXFONTS_PACKEDMSFIRST
  lmask    = 0xff << ((PDCURSES_PPB - startcol) << PDCURSES_BPP_SHIFT);
  rmask    = 0xff >> ((endcol & PDCURSES_PPB_MASK) << PDCURSES_BPP_SHIFT);
#else
  lmask    = 0xff >> ((PDCURSES_PPB - startcol) << PDCURSES_BPP_SHIFT);
  rmask    = 0xff << ((endcol & PDCURSES_PPB_MASK) << PDCURSES_BPP_SHIFT);
#endif

  /* Convert endcol to a byte offset (taking the ceiling so that includes
   * the final byte than may have fewer than 8 pixels in it).
   */

  endcol   = (endcol + PDCURSES_PPB_MASK) >> PDCURSES_PPB_SHIFT;

  /* Now copy the color into the entire glyph region */

  for (row = 0; row < fbstate->fheight; row++, fbstart += fbstate->stride)
    {
      FAR pdc_color_t *fbdest;

      fbdest = (FAR pdc_color_t *)fbstart;

      /* Special case: The row is less no more than one byte wide */

      if (endcol == 0)
        {
          uint8_t mask = lmask | rmask;

          *fbdest = (*fbdest & mask) | (color8 & ~mask);
        }
      else
        {
          /* Special case the first byte of the row */

          *fbdest = (*fbdest & lmask) | (color8 & ~lmask);
          fbdest++;

          /* Handle all middle bytes */

          for (col = 1; col < endcol; col++)
            {
              *fbdest++ = color8;
            }

          /* Handle the final byte of the row */

          *fbdest = (*fbdest & rmask) | (color8 & ~rmask);
       }
    }
}
#else
static inline void PDC_set_bg(FAR struct pdc_fbstate_s *fbstate,
                              FAR uint8_t *fbstart, int col, short bg)
{
  pdc_color_t bgcolor = PDC_color(fbstate, bg);
  int row;

  /* Set the glyph to the background color. */

  for (row = 0; row < fbstate->fheight; row++, fbstart += fbstate->stride)
    {
      FAR pdc_color_t *fbdest;

      for (col = 0, fbdest = (FAR pdc_color_t *)fbstart;
           col < fbstate->fwidth;
           col++)
        {
          *fbdest++ = bgcolor;
        }
    }
}
#endif

/****************************************************************************
 * Name: PDC_render_glyph
 *
 * Description:
 *   Render the font into the glyph memory using the foreground RGB color.
 *
 ****************************************************************************/

static inline void PDC_render_glyph(FAR struct pdc_fbstate_s *fbstate,
                                    FAR const struct nx_fontbitmap_s *fbm,
                                    FAR uint8_t *fbstart, short fg)
{
  pdc_color_t fgcolor = PDC_color(fbstate, fg);
  int ret;

  /* Then render the glyph into the allocated memory
   *
   * REVISIT:  The case where visibility==1 is not yet handled.  In that
   * case, only the lower quarter of the glyph should be reversed.
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
 * Name: PDC_copy_glyph
 *
 * Description:
 *   Copy the font from the the font buffer into the correct location in the
 *   the frame buffer.
 *
 *   For the case of pixel depth less then 1-byte, we will need to rend the
 *   font into a font buffer first, then copy it into the frame buffer at
 *   the correct position when the font is completely rendered.
 *
 ****************************************************************************/

#if PDCURSES_BPP < 8
static inline void  PDC_copy_glyph(FAR struct pdc_fbstate_s *fbstate,
                                   FAR uint8_t *dest, unsigned int xpos)
{
  FAR const uint8_t *srcrow;
  FAR const uint8_t *sptr;
  FAR uint8_t *destrow;
  FAR uint8_t *dptr;
  unsigned int startcol;
  unsigned int endcol;
  unsigned int row;
  unsigned int col;
  uint8_t lmask;
  uint8_t rmask;
  uint8_t mask;

  /* Handle the case where the first or last bytes may require read, modify,
   * write operations.
   *
   * Get the start and end column in pixels (relative to the start position)
   */

  startcol = xpos & PDCURSES_PPB_MASK;
  endcol   = startcol + fbstate->fwidth - 1;

#ifdef CONFIG_NXFONTS_PACKEDMSFIRST
  /* Get the mask for pixels that are ordered so that they pack from the
   * MS byte down.
   */

  lmask    = 0xff << ((PDCURSES_PPB - startcol) << PDCURSES_BPP_SHIFT);
  rmask    = 0xff >> ((endcol & PDCURSES_PPB_MASK) << PDCURSES_BPP_SHIFT);
#else
  /* Get the mask for pixels that are ordered so that they pack from the
   * LS byte up.
   */
  lmask    = 0xff >> ((PDCURSES_PPB - startcol) << PDCURSES_BPP_SHIFT);
  rmask    = 0xff << ((endcol & PDCURSES_PPB_MASK) << PDCURSES_BPP_SHIFT);
#endif

  /* Convert endcol to a byte offset (taking the ceiling so that includes
   * the final byte than may have fewer than 8 pixels in it).
   */

  startcol = 0;
  endcol   = (endcol + PDCURSES_PPB_MASK) >> PDCURSES_PPB_SHIFT;

  /* Then copy the image */

  srcrow  = fbstate->fbuffer;
  destrow = dest;

  for (row = 0; row < fbstate->fheight; row++)
    {
     /* Handle masking of the fractional initial byte */

     mask = lmask;           /* Start with the left mask */
     sptr = srcrow;          /* Set to the beginning of the src row */
     dptr = destrow;         /* Set to the beginning of the dest row */

     /* Handle masking of the left, leading byte */

     if (mask != 0x00)
        {
          /* Perform the read/modify/write */

          dptr[0] = (dptr[0] & ~mask) | (sptr[0] & mask);

          mask = 0xff;       /* Reset the mask */
          dptr++;            /* Skip to the next src byte */
          sptr++;            /* Skip to the next destination byte */
          startcol++;        /* Filled bytes may start on the next column */
        }

      /* Handle masking of the fractional final byte */

      mask &= rmask;
      if (mask != 0xff)
        {
          /* Perform the read/modify/write */

          dptr[endcol] = (dptr[endcol] & ~mask) | (sptr[endcol] & mask);

          endcol--;          /* Filled bytes may end on this column */
        }

      /* Handle all of the unmasked bytes in-between */

      for (col = startcol; col <= endcol; col++)
        {
         *dptr++ = *sptr++;
        }

      /* Move to the next row */

      destrow += fbstate->stride;
      srcrow  += fbstate->fstride;
    }
}
#endif

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

      rect.pt1.x = PDC_pixel_x(fbstate, col);
      rect.pt1.y = PDC_pixel_y(fbstate, row);
      rect.pt2.x = rect.pt1.x + nchars * fbstate->fwidth - 1;
      rect.pt2.y = rect.pt1.y + fbstate->fheight - 1;

      /* Then perfom the update via IOCTL */

      ret = ioctl(fbstate->fbfd, FBIO_UPDATE,
                  (unsigned long)((uintptr_t)&rect));
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

#if PDCURSES_BPP < 8
  /* For the case of pixel depth less then 1-byte, we will need to rend the
   * font into a font buffer first, then copy it into the frame buffer at
   * the correct position when the font is completely rendered.
   */

  dest = fbstate->fbuffer;
#else
  /* Otherwise, we can rend directly into the frame buffer.  Calculate the
   * destination address in the framebuffer.
   */

  dest = (FAR uint8_t *)fbstate->fbmem +
                        PDC_fbmem_y(fbstate, row) +
                        PDC_fbmem_x(fbstate, col);
#endif

  /* Initialize the glyph to the (possibly reversed) background color */

  PDC_set_bg(fbstate, dest, col, bg);

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

      PDC_render_glyph(fbstate, fbm, dest, fg);
    }

  /* Apply more attributes */

  if ((ch & (A_UNDERLINE | A_LEFTLINE | A_RIGHTLINE)) != 0)
    {
#warning Missing logic
    }

#if PDCURSES_BPP < 8
  /* Font is now completely rendered into the font buffer and can be copied
   * to the framebuffer.
   *
   * Calculate the destination address in the framebuffer.
   */

  dest = (FAR uint8_t *)fbstate->fbmem +
                        PDC_fbmem_y(fbstate, row) +
                        PDC_fbmem_x(fbstate, col);

  /* Then copy the glyph */

  PDC_copy_glyph(fbstate, dest, col);
#endif
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
       * reverse.  NOTE: visibility {1, 2} are treated the same.
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

/****************************************************************************
 * Name: PDC_clear_screen
 *
 * Description:
 *   Set the framebuffer content to a single color
 *
 ****************************************************************************/

void PDC_clear_screen(FAR struct pdc_fbstate_s *fbstate)
{
  FAR pdc_color_t *dest;
  FAR pdc_color_t bgcolor;
  FAR uint8_t *line;
  int width;
  int row;
  int col;

#ifdef CONFIG_LCD_UPDATE
  struct nxgl_rect_s rect;
  int ret;
#endif

  /* Get the background color and display width */

  bgcolor = PDCURSES_INIT_COLOR;      /* Background color for one pixel */
  width   = fbstate->xres;            /* Width in units of pixels */

#if PDCURSES_BPP < 8
  /* Pack multiple pixels into one byte.  Works for BPP={1,2,4} */

#if PDCURSES_BPP == 1                 /* BPP = 1 */
  bgcolor &= 1;                       /* Isolate 0 */
  bgcolor  = bgcolor << 1 | bgcolor;  /* Replicate 0 to 1 */
  bgcolor  = bgcolor << 2 | bgcolor;  /* Replicate 0-1 to 2-3 */
  bgcolor  = bgcolor << 4 | bgcolor;  /* Replicate 0-3 to 4-7 */
#elif PDCURSES_BPP == 2               /* BPP = 2 */
  bgcolor &= 3;                       /* Isolate 0-1 */
  bgcolor  = bgcolor << 2 | bgcolor;  /* Replicate 0-1 to 2-3 */
  bgcolor  = bgcolor << 4 | bgcolor;  /* Replicate 0-3 to 4-7 */
#else                                 /* BPP = 4 */
  bgcolor &= 15;                      /* Isolate 0-3 */
  bgcolor  = bgcolor << 4 | bgcolor;  /* Replicate 0-3 to 4-7 */
#endif

  /* Convert the width of the display to units of bytes. */

  width    = (width + PDCURSES_PPB - 1) / PDCURSES_PPB;
#endif

  /* Write the initial color into the entire framebuffer */

  for (row = 0, line = (FAR uint8_t *)fbstate->fbmem;
       row < fbstate->yres;
       row++, line += fbstate->stride)
    {
       for (col = 0, dest = (FAR pdc_color_t *)line;
            col < width;
            col++)
         {
           *dest++ = bgcolor;
         }
    }

#ifdef CONFIG_LCD_UPDATE
  /* Update the entire display */
  /* Setup the bounding rectangle */

  rect.pt1.x = 0;
  rect.pt1.y = 0;
  rect.pt2.x = fbstate->xres - 1;
  rect.pt2.y = fbstate->yres - 1;

  /* Then perfom the update via IOCTL */

  ret = ioctl(fbstate->fbfd, FBIO_UPDATE, (unsigned long)((uintptr_t)&rect));
  if (ret < 0)
    {
      PDC_LOG(("ERROR:  ioctl(FBIO_UPDATE) failed: %d\n", errno));
    }
#endif
}
