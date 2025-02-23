/****************************************************************************
 * apps/examples/fbcon/fbcon_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/boardctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <nuttx/video/fb.h>
#include <nuttx/nx/nxfonts.h>
#include <nuttx/video/fb.h>
#include <ctype.h>
#include <spawn.h>
#include <debug.h>
#include <poll.h>
#include <nuttx/ascii.h>
#include <nuttx/lib/builtin.h>

#ifndef CONFIG_NX
#  error This application requires NX Graphics
#endif

/* NSH Redirection requires Pipes */

#ifndef CONFIG_DEV_PIPE_SIZE
#  error FIFO and Named Pipe Drivers should be enabled in the configuration
#endif

#ifdef CONFIG_NSH_CLE
#  warning FBCON console does not support much VT100/EMACS/CLE type functionality yet
#endif

#if !defined(CONFIG_EXAMPLES_FBCON_PIPE_STDOUT) && \
    !defined(CONFIG_EXAMPLES_FBCON_PIPE_STDOUT)
#  warning FBCON is not configured for either stdout or stderr!
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* stdout and stderr configuration */

#define READ_PIPE           0
#define WRITE_PIPE          1
#define POLL_BUFSIZE       64
#define VT100_MAX_SEQUENCE  6 /* Max length of VT100 sequence to check      */

/* VT100 codes that can be decoded */

#define VT100_CLEAREOL  {ASCII_ESC, '[', 'K'}                /* Clear line from cursor right            */
#define VT100_CLEARLINE {ASCII_ESC, '[', '2', 'K'}           /* Clear entire line                       */
#define VT100_CURSOROFF {ASCII_ESC, '[', '?', '2', '5', 'l'} /* Cursor OFF                              */
#define VT100_CURSORON  {ASCII_ESC, '[', '?', '2', '5', 'h'} /* Cursor ON                               */
#define VT100_CURSORL   {ASCII_ESC, '[', '*', 'D'}           /* Move cursor left (* = [1...99]) columns */

/* Configuration ************************************************************/

/* Pixel depth.  If none provided, pick the smallest enabled pixel depth    */

#ifdef CONFIG_EXAMPLES_FBCON_BPP_NX_DEFAULT
#  if !defined(CONFIG_NX_DISABLE_1BPP)
#    define FBCON_BPP 1
#  elif !defined(CONFIG_NX_DISABLE_2BPP)
#    define FBCON_BPP 2
#  elif !defined(CONFIG_NX_DISABLE_4BPP)
#    define FBCON_BPP 4
#  elif !defined(CONFIG_NX_DISABLE_8BPP)
#    define FBCON_BPP 8
#  elif !defined(CONFIG_NX_DISABLE_16BPP)
#    define FBCON_BPP 16
#  elif !defined(CONFIG_NX_DISABLE_24BPP)
#    define FBCON_BPP 24
#  elif !defined(CONFIG_NX_DISABLE_32BPP)
#    define FBCON_BPP 32
#  else
#    error "No pixel depth enabled"
#  endif
#elif defined(CONFIG_EXAMPLES_FBCON_1BPP)
#  define FBCON_BPP 1
#elif defined(CONFIG_EXAMPLES_FBCON_2BPP)
#  define FBCON_BPP 2
#elif defined(CONFIG_EXAMPLES_FBCON_4BPP)
#  define FBCON_BPP 4
#elif defined(CONFIG_EXAMPLES_FBCON_8BPP)
#  define FBCON_BPP 8
#elif defined(CONFIG_EXAMPLES_FBCON_16BPP)
#  define FBCON_BPP 16
#elif defined(CONFIG_EXAMPLES_FBCON_24BPP)
#  define FBCON_BPP 24
#elif defined(CONFIG_EXAMPLES_FBCON_32BPP)
#  define FBCON_BPP 32
#endif

/* Select renderer */

#if (FBCON_BPP == 1)
#  define RENDERER nxf_convert_1bpp
#elif (FBCON_BPP == 2)
#  define RENDERER nxf_convert_2bpp
#elif (FBCON_BPP == 4)
#  define RENDERER nxf_convert_4bpp
#elif (FBCON_BPP == 8)
#  define RENDERER nxf_convert_8bpp
#elif (FBCON_BPP == 16)
#  define RENDERER nxf_convert_16bpp
#elif (FBCON_BPP == 24)
#  define RENDERER nxf_convert_24bpp
#elif (FBCON_BPP == 32)
#  define RENDERER nxf_convert_32bpp
#else
#  error "Unsupported CONFIG_EXAMPLES_FBCON_BPP"
#endif

/* Background and font color, if defaults chosen */

#ifndef CONFIG_EXAMPLES_FBCON_BGCOLOR
#  if (FBCON_BPP == 24) || (FBCON_BPP == 32)
#    define FBCON_BGCOLOR 0x00000000
#  elif FBCON_BPP == 16
#    define FBCON_BGCOLOR 0x0000
#  else
#    define FBCON_BGCOLOR 0x0
#  endif
#else
#  define FBCON_BGCOLOR CONFIG_EXAMPLES_FBCON_BGCOLOR
#endif

#ifndef CONFIG_EXAMPLES_FBCON_FCOLOR
#  if (FBCON_BPP == 32) || (FBCON_BPP == 24)
#    define FBCON_FCOLOR 0xffffff
#  elif FBCON_BPP == 16
#    define FBCON_FCOLOR 0xffff
#  else
#    define FBCON_FCOLOR 0xff
#  endif
#else
#  define FBCON_FCOLOR CONFIG_EXAMPLES_FBCON_FCOLOR
#endif

/* Console font ID */

#ifndef CONFIG_EXAMPLES_FBCON_FONTID
#  ifndef NXFONT_DEFAULT
#    error NXFONT_DEFAULT not defined
#  endif
#  define FBCON_FONTID NXFONT_DEFAULT
#else
#  define FBCON_FONTID CONFIG_EXAMPLES_FBCON_FONTID
#endif

/* Font glyph caching */

#ifndef CONFIG_EXAMPLES_FBCON_GLCACHE
#  define FBCON_GLCACHE 16
#else
#  define FBCON_GLCACHE CONFIG_EXAMPLES_FBCON_GLCACHE
#endif

/* Bitmap flags */

#define BMFLAGS_NOGLYPH (1 << 0) /* No glyph available, use space */
#define BM_ISSPACE(bm)  (((bm)->flags & BMFLAGS_NOGLYPH) != 0)

/* Sizes and maximums */

#define MAX_USECNT      255  /* Limit to range of a uint8_t */

/* Line spacing.  Space (in rows) between lines. */

#define FBCON_LINESPACING CONFIG_EXAMPLES_FBCON_LINESPACING

/* Cursor character */

#define FBCON_CURSORCHAR CONFIG_EXAMPLES_FBCON_CURSORCHAR

/* Spawn task */

#define SPAWN_TASK CONFIG_EXAMPLES_FBCON_SPAWN_TASK

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum exitcode_e
{
  FBCON_EXIT_SUCCESS = 0,
  FBCON_EXIT_FAIL,
  FBCON_EXIT_FD,
  FBCON_EXIT_GETVINFO,
  FBCON_EXIT_GETPINFO,
  FBCON_EXIT_FBMEM,
  FBCON_EXIT_FBMEM2,
  FBCON_EXIT_FBIO_OVERLAY_INFO,
  FBCON_EXIT_FBIO_SELECT_OVERLAY,
  FBCON_EXIT_FONTOPEN,
  FBCON_EXIT_SETBGCOLOR,
  FBCON_EXIT_STDOUT_PIPE_FAILED,
  FBCON_EXIT_STDERR_PIPE_FAILED,
  FBCON_EXIT_STDIN_PIPE_FAILED,
  FBCON_EXIT_LOCAL_STDERR_PIPE_FAILED,
  FBCON_EXIT_LOCAL_STDOUT_PIPE_FAILED,
  FBCON_EXIT_POSIX_SPAWN_ACTIONS_INIT_FAILED,
  FBCON_EXIT_POSIX_SPAWN_FAILED,
  FBCON_EXIT_APP_INDEX_UNAVAILABLE,
  FBCON_EXIT_APP_POSIX_ATTRIB_INIT_FAILED,
};

/* Describes one cached glyph bitmap */

struct fbcon_glyph_s
{
  uint8_t                    code;       /* Character code                  */
  uint8_t                    height;     /* Height of this glyph (in rows)  */
  uint8_t                    width;      /* Width of this glyph (in pixels) */
  uint8_t                    stride;     /* Width of the glyph row (bytes)  */
  uint8_t                    usecnt;     /* Use count                       */
  FAR uint8_t               *bitmap;     /* Allocated bitmap memory         */
};

/* Describes on character on the display */

struct fbcon_bitmap_s
{
  uint8_t                    code;       /* Character code                  */
  uint8_t                    flags;      /* See BMFLAGS_*                   */
  struct nxgl_point_s        pos;        /* Character position              */
};

/* Describes the state of one text display */

struct fbcon_state_s
{
  int                        fd_fb;
  FAR void                  *fbcon_font;
  struct fb_videoinfo_s     *vinfo;
  struct fb_planeinfo_s     *pinfo;
#ifdef CONFIG_FB_OVERLAY
  struct fb_overlayinfo_s    oinfo;
#endif
  FAR void                  *fbmem;
#if 0
  /* Revisit needed - no support got dual framebuffers as yet */

  FAR void                  *fbmem2;
  FAR void                  *act_fbmem;
#endif
  uint32_t                   mem2_yoffset;

  /* The following describe the console */

  nxgl_mxpixel_t             bcolor;    /* Console background color         */
  nxgl_mxpixel_t             fcolor;    /* Console font color               */
  struct nxgl_size_s         wsize;     /* Console size                     */
  uint8_t                    fheight;   /* Max height of a font in pixels   */
  uint8_t                    fwidth;    /* Max width of a font in pixels    */
  uint8_t                    spwidth;   /* The width of a space             */

  /* These describe all text already added to the display */

  uint8_t                    maxglyphs; /* Size of the glyph[] array        */
  uint16_t                   maxchars;  /* Size of the bm[] array           */
  uint32_t                   nchars;    /* Numb of chars in the bm[] array  */

  FAR struct fbcon_bitmap_s *bm;        /* List of bitmaps on the display   */
  FAR struct fbcon_glyph_s  *glyph;     /* Cache of rendered fonts in use   */
  FAR struct fbcon_bitmap_s  cursor;    /* Character to use for cursor      */

  /* VT100 escape sequence processing                                       */

  char                       seq[VT100_MAX_SEQUENCE]; /* Buffered chars     */
  uint8_t                    nseq;                    /* Num buffered chars */
  int                        nwild;                   /* Num wild collected */
  int                        wildval;                 /* Wildcard value     */
};

typedef void (*seqhandler_t)(FAR struct fbcon_state_s *st);

/* Identifies the state of the VT100 escape sequence processing */

enum fbcon_vt100state_e
{
  VT100_NOT_CONSUMED = 0, /* Character is not part of a VT100 escape sequence */
  VT100_CONSUMED,         /* Character was consumed as part of the VT100 escape processing */
  VT100_PROCESSED,        /* The full VT100 escape sequence was processed */
  VT100_ABORT             /* Invalid/unsupported character in buffered escape sequence */
};

struct vt100_sequence_s
{
  FAR const char            *seq;
  seqhandler_t               handler;
  uint8_t                    size;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
#if 0
/* Revisit needed = no support for dual framebuffers yet */

static int fb_init_mem2(FAR struct fbcon_state_s *st);
#endif
static int fbdev_get_pinfo(int fd, FAR struct fb_planeinfo_s *pinfo);
static int fbcon_initialize(FAR struct fbcon_state_s *st);
static void fbcon_newline(FAR struct fbcon_state_s *st);
static int fbcon_hidechar(FAR FAR struct fbcon_state_s *st,
                          FAR const struct fbcon_bitmap_s *bm);
static enum fbcon_vt100state_e fbcon_vt100(FAR struct fbcon_state_s *st,
                                           char ch);
static void fbcon_fillchar(FAR struct fbcon_state_s *st,
                           FAR const struct nxgl_rect_s *rect,
                           FAR const struct fbcon_bitmap_s *bm);
static int fbcon_fill(FAR struct fbcon_state_s *st,
                      FAR struct nxgl_rect_s *rect,
                      FAR nxgl_mxpixel_t *color);
static void fbcon_fillspace(FAR struct fbcon_state_s *st,
                            FAR const struct nxgl_rect_s *rect,
                            FAR const struct fbcon_bitmap_s *bm);
static int fbcon_bitmap(FAR struct fbcon_state_s     *st,
                        FAR const struct nxgl_rect_s *dest,
                        FAR const uint32_t           *src,
                        unsigned int                 stride);
static int fbcon_backspace(FAR struct fbcon_state_s *st);
static void fbcon_home(FAR struct fbcon_state_s *st);
static inline void fbcon_movedisplay(FAR struct fbcon_state_s *st,
                                     int bottom, int scrollheight);
static inline void fbcon_scroll(FAR struct fbcon_state_s *st,
                                int scrollheight);
static void fbcon_freeglyph(FAR struct fbcon_glyph_s *glyph);
static inline FAR struct fbcon_glyph_s *
                              fbcon_allocglyph(FAR struct fbcon_state_s *st);
static FAR struct fbcon_glyph_s *
                   fbcon_findglyph(FAR struct fbcon_state_s *st, uint8_t ch);
static inline FAR struct fbcon_glyph_s *
                          fbcon_renderglyph(FAR struct fbcon_state_s *st,
                          FAR const struct nx_fontbitmap_s *fbm, uint8_t ch);
static int fbcon_fontsize(FAR void *hfont, uint8_t ch,
                          FAR struct nxgl_size_s *size);
static FAR struct fbcon_glyph_s *
  fbcon_getglyph(FAR struct fbcon_state_s *st, uint8_t ch);
static FAR const struct fbcon_bitmap_s *
  fbcon_addchar(FAR struct fbcon_state_s *st, uint8_t ch);
static void fbcon_write(FAR struct fbcon_state_s *st,
                        FAR char *buffer, size_t buflen);
static bool has_input(int fd);
static void poll_std_streams(FAR struct fbcon_state_s *st);
static void fbcon_putc(FAR struct fbcon_state_s *st, uint8_t ch);
static enum fbcon_vt100state_e fbcon_vt100seq(
                                  FAR struct fbcon_state_s *st, int seqsize);
static FAR const struct vt100_sequence_s *
                  fbcon_vt100part(FAR struct fbcon_state_s *st, int seqsize);
static enum fbcon_vt100state_e fbcon_vt100(FAR struct fbcon_state_s *st,
                                           char ch);
static void fbcon_erasetoeol(FAR struct fbcon_state_s *st);
static void fbcon_clearline(FAR struct fbcon_state_s *st);
static void fbcon_showcursor(FAR struct fbcon_state_s *st);
static void fbcon_hidecursor(FAR struct fbcon_state_s *st);
static void fbcon_cursorl(FAR struct fbcon_state_s *st);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Pipes for NSH Shell: stdin, stdout, stderr */

#ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDIN
static int g_nsh_stdin[2];
#endif
static int g_nsh_stdout[2];
static int g_nsh_stderr[2];

/* The VT100 sequences supported by FBCON                                   */

static const char g_erasetoeol[] = VT100_CLEAREOL;
static const char g_clearline[]  = VT100_CLEARLINE;
static const char g_cursoroff[]  = VT100_CURSOROFF;
static const char g_cursoron[]   = VT100_CURSORON;
static const char g_cursorl[]    = VT100_CURSORL;

static const struct vt100_sequence_s g_vt100sequences[] =
{
  {g_erasetoeol, fbcon_erasetoeol, sizeof(g_erasetoeol)},
  {g_clearline,  fbcon_clearline,  sizeof(g_clearline)},
  {g_cursoroff,  fbcon_hidecursor, sizeof(g_cursoroff)},
  {g_cursoron,   fbcon_showcursor, sizeof(g_cursoroff)},
  {g_cursorl,    fbcon_cursorl,    sizeof(g_cursorl)},
  {NULL, NULL, 0}
};

#ifdef CONFIG_EXAMPLES_FBCON_SHOW_WELCOME
#  ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDOUT
static const char g_stdout_hello[] = "Hello FBCON stdout fprintf output!";
#  endif
#  ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDERR
static const char g_stderr_hello[] = "Hello FBCON stderr fprintf output!";
#  endif
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fbcon_showcursor
 *
 * Description:
 *   Render the cursor character at the current display position.
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fbcon_showcursor(FAR struct fbcon_state_s *st)
{
  int lineheight;

  if ((st->cursor.pos.x + st->fwidth) > st->wsize.w)
    {
#ifndef CONFIG_EXAMPLES_FBCON_NOWRAP
      /* No.. move to the next line */

      fbcon_newline(st);
#else
      return OK;
#endif
    }

  /* Check if we need to scroll up */

  lineheight = st->fheight + FBCON_LINESPACING;
  while (st->cursor.pos.y >= st->wsize.h + lineheight)
    {
      fbcon_scroll(st, lineheight);
    }

  /* Render the cursor glyph onto the display. */

  fbcon_fillchar(st, NULL, &st->cursor);
}

/****************************************************************************
 * Name: fbcon_hidecursor
 *
 * Description:
 *   Remove the cursor character at the current display position.
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fbcon_hidecursor(FAR struct fbcon_state_s *st)
{
  fbcon_hidechar(st, &st->cursor);
}

/****************************************************************************
 * Name: fbcon_cursorl
 *
 * Description:
 *   Move the cursor position to the left n characters.
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fbcon_cursorl(FAR struct fbcon_state_s *st)
{
  int i;

  /* Revisit needed. It seems backspace 1 less than calculated.
   * This is illogical and yet to be explained and is perhaps related to
   * nsh itself when nsh is the spawned app/task.
   */

  for (i = 1; i < st->wildval; i++)
    {
      fbcon_backspace(st);
    }
}

/****************************************************************************
 * Name: fbcon_erasetoeol
 *
 * Description:
 *   Handle the erase-to-eol VT100 escape sequence.
 *   Erase from and including cursor position
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fbcon_erasetoeol(FAR struct fbcon_state_s *st)
{
  struct nxgl_rect_s rect;

  /* Create a bounding box the size of the remaining iine */

  rect.pt1.x = st->cursor.pos.x;
  rect.pt2.x = st->wsize.w - 1;
  rect.pt1.y = st->cursor.pos.y;
  rect.pt2.y = rect.pt1.y + st->fheight + FBCON_LINESPACING - 1;

  /* Clear the region */

  if (fbcon_fill(st, &rect, &st->bcolor) < 0)
    {
      gerr("ERROR: fbcon_fill failed: %d\n", errno);
    }

  /* Because we were clearing from the cursor position, there's no need
   * to modify st->nchar as there are no characters after the cursor.
   */
}

/****************************************************************************
 * Name: fbcon_clearline
 *
 * Description:
 *   Handle the clearline VT100 escape sequence
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   The index of the match in g_vt100sequences[]
 *
 ****************************************************************************/

static void fbcon_clearline(FAR struct fbcon_state_s *st)
{
  int                        i;
  FAR struct fbcon_bitmap_s *bm;
  struct nxgl_rect_s         rect;

  /* Create a bounding box the size of the iine */

  rect.pt1.x = 0;
  rect.pt2.x = st->wsize.w - 1;
  rect.pt1.y = st->cursor.pos.y + FBCON_LINESPACING;
  rect.pt2.y = st->cursor.pos.y + st->fheight + FBCON_LINESPACING - 1;

  /* Clear the region */

  if (fbcon_fill(st, &rect, &st->bcolor) < 0)
    {
      gerr("ERROR: fbcon_fill failed: %d\n", errno);
    }

  st->cursor.pos.x = st->spwidth;

  /* Decrement nchar for each character within in the bounding box */

  i = st->nchars;
  while (--i > 0)
    {
      bm = &st->bm[i];
      if (bm->pos.y <= rect.pt1.y &&
          bm->pos.y + st->fheight >= rect.pt2.y)
        {
          st->nchars--;
        }
    }
}

/****************************************************************************
 * Name: fbcon_bitmap
 *
 * Description:
 *   Copy a rectangular region of a larger image into the rectangle in the
 *   specified window.
 *
 * Input Parameters:
 *   st     - pointer to FBCON status structure
 *   dest   - Describes the rectangular region on the display that will
 *            receive the bit map.
 *   src    - The start of the source image.
 *   stride - The width of the full source image in pixels.
 *
 * Returned Value:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

static int fbcon_bitmap(FAR struct fbcon_state_s     *st,
                        FAR const struct nxgl_rect_s *dest,
                        FAR const uint32_t           *src,
                        unsigned int                 stride)
{
  FAR uint32_t *dst;
  FAR uint8_t  *row;
  struct fb_area_s area;
  int x;
  int y;

  area.h = dest->pt2.y - dest->pt1.y + 1;
  area.w = dest->pt2.x - dest->pt1.x + 1;
  area.x = dest->pt1.x;
  area.y = dest->pt1.y;

  row = (FAR uint8_t *)st->fbmem + st->pinfo->stride * area.y;
  for (y = 0; y < area.h; y++)
    {
      dst = ((FAR uint32_t *)row) + area.x;
      for (x = 0; x < area.w; x++)
        {
          *dst++ = *(uint32_t *)src++;
        }

      row += st->pinfo->stride;
    }

  return OK;
}

/****************************************************************************
 * Name: fbcon_movedisplay
 *
 * Description:
 *   This function implements the data movement for the scroll operation.
 *
 * Input Parameters:
 *   st           - pointer to FBCON status structure
 *   bottom       - Start of the vacated area at the bottom to be cleared
 *   scrollheight - The distance the display must be scrolled
 *
 * Returned Value:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

static inline void fbcon_movedisplay(struct fbcon_state_s *st, int bottom,
                                     int scrollheight)
{
  FAR struct fbcon_bitmap_s *bm;
  struct nxgl_rect_s         rect;
  nxgl_coord_t               row;
  int                        ret;
  int                        i;

  /* Move each row, one at a time.  They could all be moved at once but since
   * the region is cleared, then re-written, the effect would not be good.
   * Below the region is also cleared and re-written,
   * however, in much smaller chunks.
   */

  rect.pt1.x = 0;
  rect.pt2.x = st->wsize.w - 1;

  for (row = FBCON_LINESPACING; row < bottom; row += scrollheight)
    {
      /* Create a bounding box the size of one row of characters */

      rect.pt1.y = row;
      rect.pt2.y = row + scrollheight - 1;

      /* Clear the region */

      ret = fbcon_fill(st, &rect, &st->bcolor);
      if (ret < 0)
        {
          gerr("ERROR: fbcon_fill failed: %d\n", errno);
        }

      /* Fill each character that might lie within in the bounding box */

      for (i = 0; i < st->nchars; i++)
        {
          bm = &st->bm[i];
          if (bm->pos.y <= rect.pt1.y &&
              bm->pos.y + st->fheight >= rect.pt2.y)
            {
              fbcon_fillchar(st, &rect, bm);
            }
        }
    }

  /* Finally, clear the vacated part of the display */

  rect.pt1.y = bottom;
  rect.pt2.y = st->wsize.h - 1;

  ret = fbcon_fill(st, &rect, &st->bcolor);
  if (ret < 0)
    {
      fprintf(stderr, "fbcon_movedisplay: fbcon_fill failed: %d\n", errno);
    }
}

/****************************************************************************
 * Name: fbcon_scroll
 *
 * Description:
 *   Scroll the display up by a certain amount
 *
 * Input Parameters:
 *   st           - pointer to FBCON status structure
 *   scrollheight - The distance the display must be scrolled
 *
 * Returned Value:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

static inline void fbcon_scroll(struct fbcon_state_s *st, int scrollheight)
{
  int i;
  int j;

  /* Adjust the vertical position of each character */

  for (i = 0; i < st->nchars; )
    {
      FAR struct fbcon_bitmap_s *bm = &st->bm[i];

      /* Has any part of this character scrolled off the screen? */

      if (bm->pos.y < scrollheight + FBCON_LINESPACING)
        {
          /* Yes... Delete the character by moving all of the data */

          for (j = i; j < st->nchars - 1; j++)
            {
              memcpy(&st->bm[j], &st->bm[j + 1],
                     sizeof(struct fbcon_bitmap_s));
            }

          /* Decrement the number of cached characters ('i' is not
           * incremented in this case because it already points to the next
           * character)
           */

          st->nchars--;
        }

      /* No.. just decrement its vertical position (moving it "up" the
       * display by one line).
       */

      else
        {
          bm->pos.y -= scrollheight;

          /* We are keeping this one so increment to the next character */

          i++;
        }
    }

  /* And move the next display position up by one line as well */

  st->cursor.pos.y -= scrollheight;

  /* Move the display in the range of 0-height up one scrollheight. */

  fbcon_movedisplay(st, st->cursor.pos.y, scrollheight);
}

/****************************************************************************
 * Name: fbcon_freeglyph
 *
 * Description:
 *   Clear the specified glyph structure
 *
 * Input Parameters:
 *   glyph - pointer to glyph structure to be freed
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fbcon_freeglyph(FAR struct fbcon_glyph_s *glyph)
{
  if (glyph->bitmap)
    {
      free(glyph->bitmap);
    }

    memset(glyph, 0, sizeof(struct fbcon_glyph_s));
}

/****************************************************************************
 * Name: fbcon_allocglyph
 *
 * Description:
 *   Allocate a glyph structure
 *
 * Input Parameters:
 *   glyph - pointer to glyph structure to be allocated
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static inline FAR struct fbcon_glyph_s *
                         fbcon_allocglyph(FAR struct fbcon_state_s *st)
{
  FAR struct fbcon_glyph_s *glyph = NULL;
  FAR struct fbcon_glyph_s *luglyph = NULL;
  uint8_t luusecnt;
  int i;

  /* Search through the glyph cache looking for an unused glyph.  Also, keep
   * track of the least used glyph as well.  We need that if we have to
   * replace a glyph in the cache.
   */

  for (i = 0; i < st->maxglyphs; i++)
    {
      /* Is this glyph in use? */

      glyph = &st->glyph[i];
      if (!glyph->usecnt)
        {
          /* No.. return this glyph with a use count of one */

          glyph->usecnt = 1;
          return glyph;
        }

      /* Yes.. check for the least recently used */

      if (!luglyph || glyph->usecnt < luglyph->usecnt)
        {
          luglyph = glyph;
        }
    }

  /* If we get here, the glyph cache is full.  We replace the least used
   * glyph with the one we need now. (luglyph can't be NULL).
   */

  luusecnt = luglyph->usecnt;
  fbcon_freeglyph(luglyph);

  /* But lets decrement all of the usecnts so that the new one one be so
   * far behind in the counts as the older ones.
   */

  if (luusecnt > 1)
    {
      uint8_t decr = luusecnt - 1;

      for (i = 0; i < st->maxglyphs; i++)
        {
          /* Is this glyph in use? */

          glyph = &st->glyph[i];
          if (glyph->usecnt > decr)
            {
              glyph->usecnt -= decr;
            }
        }
    }

  /* Then return the least used glyph */

  luglyph->usecnt = 1;
  return luglyph;
}

/****************************************************************************
 * Name: fbcon_findglyph
 *
 * Description:
 *   Try and find a glyph in the cache for a given character
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *   ch - the character
 *
 * Returned Value:
 *   a pointer to the glyph structure if found or NULL if not
 *
 ****************************************************************************/

static FAR struct fbcon_glyph_s *
                  fbcon_findglyph(FAR struct fbcon_state_s *st, uint8_t ch)
{
  int i;

  /* Try to find the glyph in the cache of pre-rendered glyphs */

  for (i = 0; i < st->maxglyphs; i++)
    {
      FAR struct fbcon_glyph_s *glyph = &st->glyph[i];
      if (glyph->usecnt > 0 && glyph->code == ch)
        {
          /* Increment the use count (unless it is already at the max) */

          if (glyph->usecnt < MAX_USECNT)
            {
               glyph->usecnt++;
            }

          /* And return the glyph that we found */

          return glyph;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: fbcon_renderglyph
 *
 * Description:
 *   Render a character as a glyph based on the font bitmap and metrics
 *
 * Input Parameters:
 *   st  - pointer to FBCON status structure
 *   fbm - pointer to the character bitmap/metrics
 *   ch  - the character to be rendered
 *
 * Returned Value:
 *   a pointer to the rendered glyph structure
 *
 ****************************************************************************/

static inline FAR struct fbcon_glyph_s *
                           fbcon_renderglyph(FAR struct fbcon_state_s *st,
                           FAR const struct nx_fontbitmap_s *fbm, uint8_t ch)
{
  FAR struct fbcon_glyph_s *glyph = NULL;
  FAR nxgl_mxpixel_t *ptr;
#if FBCON_BPP < 8
  nxgl_mxpixel_t pixel;
#endif
  int bmsize;
  int row;
  int col;
  int ret;

  /* Make sure that there is room for another glyph */

  ginfo("ch=%c [%02x]\n", isprint(ch) ? ch : '.', ch);

  /* Allocate the glyph (always succeeds) */

  glyph         = fbcon_allocglyph(st);
  glyph->code   = ch;

  /* Get the dimensions of the glyph */

  glyph->width  = fbm->metric.width + fbm->metric.xoffset;
  glyph->height = fbm->metric.height + fbm->metric.yoffset;

  /* Allocate memory to hold the glyph with its offsets */

  glyph->stride = (glyph->width * FBCON_BPP + 7) / 8;
  bmsize        =  glyph->stride * glyph->height;
  glyph->bitmap = (FAR uint8_t *)malloc(bmsize);

  if (glyph->bitmap)
    {
      /* Initialize the glyph memory to the background color */

#if FBCON_BPP < 8
      pixel  = st->bcolor;
#  if FBCON_BPP == 1
      /* Pack 1-bit pixels into a 2-bits */

      pixel &= 0x01;
      pixel  = (pixel) << 1 | pixel;
#  endif
#  if FBCON_BPP < 4
      /* Pack 2-bit pixels into a nibble */

      pixel &= 0x03;
      pixel  = (pixel) << 2 | pixel;
#  endif

      /* Pack 4-bit nibbles into a byte */

      pixel &= 0x0f;
      pixel  = (pixel) << 4 | pixel;

      ptr    = (FAR nxgl_mxpixel_t *)glyph->bitmap;
      for (row = 0; row < glyph->height; row++)
        {
          for (col = 0; col < glyph->stride; col++)
            {
              /* Transfer the packed bytes into the buffer */

              *ptr++ = pixel;
            }
        }

#elif FBCON_BPP == 24
#  error "Additional logic is needed here for 24bpp support"

#else /* FBCON_BPP = {8,16,32} */

      ptr = (FAR nxgl_mxpixel_t *)glyph->bitmap;
      for (row = 0; row < glyph->height; row++)
        {
          /* Just copy the color value into the glyph memory */

          for (col = 0; col < glyph->width; col++)
            {
              *ptr++ = st->bcolor;
            }
        }
#endif

      /* Then render the glyph into the allocated memory */

      ret = RENDERER((FAR nxgl_mxpixel_t *)glyph->bitmap,
                      glyph->height, glyph->width, glyph->stride,
                      fbm, st->fcolor);
      if (ret < 0)
        {
          /* Actually, the RENDERER never returns a failure */

          gerr("ERROR: fbcon_renderglyph: RENDERER failed\n");
          fbcon_freeglyph(glyph);
          glyph = NULL;
        }
    }

  return glyph;
}

/****************************************************************************
 * Name: fbcon_fontsize
 *
 * Description:
 *   Get the size of a given character bitmap
 *
 * Input Parameters:
 *   font - the font of interest
 *   ch   - the character of interest
 *   size _ pointer to the structure to return the character size
 *
 * Returned Value:
 *   Success or ERROR
 *
 ****************************************************************************/

static int fbcon_fontsize(FAR void *hfont, uint8_t ch,
                          FAR struct nxgl_size_s *size)
{
  FAR const struct nx_fontbitmap_s *fbm;

  /* No, it is not cached... Does the code map to a font? */

  fbm = nxf_getbitmap(hfont, ch);
  if (fbm)
    {
      /* Yes.. return the font size */

      size->w = fbm->metric.width + fbm->metric.xoffset;
      size->h = fbm->metric.height + fbm->metric.yoffset;
      return OK;
    }

  return ERROR;
}

/****************************************************************************
 * Name: fbcon_getglyph
 *
 * Description:
 *   Get rendered glyph data for a given character
 *
 * Input Parameters:
 *   st  - pointer to FBCON status structure
 *   ch  - the character to be rendered
 *
 * Returned Value:
 *   a pointer to the glyph data
 *
 ****************************************************************************/

static FAR struct fbcon_glyph_s *
  fbcon_getglyph(FAR struct fbcon_state_s *st, uint8_t ch)
{
  FAR struct fbcon_glyph_s *glyph;
  FAR const struct nx_fontbitmap_s *fbm;

  /* First, try to find the glyph in the cache of pre-rendered glyphs */

  glyph = fbcon_findglyph(st, ch);
  if (!glyph)
    {
      /* No, it is not cached... Does the code map to a font? */

      fbm = nxf_getbitmap(st->fbcon_font, ch);
      if (fbm)
        {
          /* Yes.. render the glyph */

          glyph = fbcon_renderglyph(st, fbm, ch);
        }
    }

  return glyph;
}

/****************************************************************************
 * Name: fbcon_hidechar
 *
 * Description:
 *   Erase a character from the display.
 *
 * Input Parameters:
 *   st  - pointer to FBCON status structure
 *   bm  - pointer to the character bitmap to be erased
 *
 * Returned Value:
 *   Success or error value
 *
 ****************************************************************************/

static int fbcon_hidechar(FAR struct fbcon_state_s *st,
                          FAR const struct fbcon_bitmap_s *bm)
{
  struct nxgl_rect_s bounds;
  struct nxgl_size_s fsize;
  int ret;

  /* Get the size of the font glyph.  If fbcon_fontsize, then the
   * character will have been rendered as a space, and no display
   * modification is required (not an error).
   */

  ret = fbcon_fontsize(st->fbcon_font, bm->code, &fsize);
  if (ret < 0)
    {
      /* It was rendered as a space. */

      return OK;
    }

  /* Construct a bounding box for the glyph */

  bounds.pt1.x = bm->pos.x;
  bounds.pt1.y = bm->pos.y;
  bounds.pt2.x = bm->pos.x + fsize.w - 1;
  bounds.pt2.y = bm->pos.y + fsize.h - 1;

  /* Fill the bitmap region with the background color, erasing the
   * character from the display.
   */

  return fbcon_fill(st, &bounds, &st->bcolor);
}

/****************************************************************************
 * Name: fbcon_addchar
 *
 * Description:
 *   Find or create a bitmap structurefor a character
 *
 * Input Parameters:
 *   st  - pointer to FBCON status structure
 *   ch  - the character to be rendered
 *
 * Returned Value:
 *   Pointer to the character bitmap
 *
 ****************************************************************************/

static FAR const struct fbcon_bitmap_s *
                        fbcon_addchar(FAR struct fbcon_state_s *st,
                        uint8_t ch)
{
  FAR struct fbcon_bitmap_s *bm = NULL;
  FAR struct fbcon_glyph_s *glyph;

  /* Setup the bitmap information */

  bm        = &st->bm[st->nchars];
  bm->code  = ch;
  bm->flags = 0;
  bm->pos.x = st->cursor.pos.x;
  bm->pos.y = st->cursor.pos.y;

  /* Find (or create) the matching glyph */

  glyph = fbcon_getglyph(st, ch);
  if (!glyph)
    {
      /* No, there is no font for this code.  Just mark this as a space. */

      bm->flags |= BMFLAGS_NOGLYPH;

      /* Set up the next character position */

      st->cursor.pos.x += st->spwidth;
    }
  else
    {
      /* Set up the next character position */

      st->cursor.pos.x += glyph->width;
    }

  /* Increment nchars to retain this character */

  if (st->nchars < st->maxchars)
    {
      st->nchars++;
    }

  return bm;
}

/****************************************************************************
 * Name: fbcon_fill
 *
 * Description:
 *   Fill a region with a colour.
 *
 * Input Parameters:
 *   st    - pointer to FBCON status structure
 *   rect  - Describes the rectangular region on the display to be filled
 *   color - The color to fill the rectangle with
 *
 * Returned Value:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

static int fbcon_fill(FAR struct fbcon_state_s *st,
                      FAR struct nxgl_rect_s *rect,
                      FAR nxgl_mxpixel_t *color)
{
  FAR uint32_t *dest;
  FAR uint8_t *row;
  struct fb_area_s area;
  int x;
  int y;

  area.h = rect->pt2.y - rect->pt1.y + 1;
  area.w = rect->pt2.x - rect->pt1.x + 1;
  area.x = rect->pt1.x;
  area.y = rect->pt1.y;

  row = (FAR uint8_t *)st->fbmem + st->pinfo->stride * area.y;
  for (y = 0; y < area.h; y++)
    {
      dest = (FAR uint32_t *)row + area.x;
      for (x = 0; x < area.w; x++)
        {
          *dest++ = *color;
        }

      row += st->pinfo->stride;
    }

  return OK;
}

/****************************************************************************
 * Name: fbcon_backspace
 *
 * Description:
 *   Remove the last character from the window.
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   OK on success; ERROR on failure with errno set appropriately
 *
 ****************************************************************************/

static int fbcon_backspace(FAR struct fbcon_state_s *st)
{
  FAR struct fbcon_bitmap_s *bm;
  int ndx;
  int ret = -ENOENT;

  /* Is there a character on the display? */

  if (st->nchars > 0)
    {
      /* Yes.. Get the index to the last bitmap on the display */

      ndx = st->nchars - 1;
      bm  = &st->bm[ndx];

      /* Erase the character from the display */

      ret = fbcon_hidechar(st, bm);

      /* The current position to the location where the last character was */

      st->cursor.pos.x = bm->pos.x;
      st->cursor.pos.y = bm->pos.y;

      /* Decrement nchars to discard this character */

      st->nchars = ndx;
    }

  return ret;
}

/****************************************************************************
 * Name: fbcon_home
 *
 * Description:
 *   Set the next character position to the top-left corner of the display.
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fbcon_home(FAR struct fbcon_state_s *st)
{
  /* The first character is one space from the left */

  st->cursor.pos.x = st->spwidth;

  /* And FBCON_LINESPACING lines from the top */

  st->cursor.pos.y = FBCON_LINESPACING;

  /* And reset number of characters in the bm buffer */

  st->nchars = 0;
}

/****************************************************************************
 * Name: fbcon_newline
 *
 * Description:
 *   Set the next character position to the beginning of the next line.
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void fbcon_newline(FAR struct fbcon_state_s *st)
{
  /* Carriage return: The first character is one space from the left */

  st->cursor.pos.x = st->spwidth;

  /* Linefeed: Down the max font height + FBCON_LINESPACING */

  st->cursor.pos.y += (st->fheight + FBCON_LINESPACING);
}

/****************************************************************************
 * Name: fbcon_putc
 *
 * Description:
 *   Render the specified character at the current display position.
 *
 * Input Parameters:
 *   st    - pointer to FBCON status structure
 *   ch  - the character to be rendered
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void fbcon_putc(FAR struct fbcon_state_s *st, uint8_t ch)
{
  FAR const struct fbcon_bitmap_s *bm;
  int lineheight;

  /* Ignore carriage returns */

  if (ch == '\r')
    {
      return;
    }

  /* Handle backspace (treating both BS and DEL as backspace) */

  if (ch == ASCII_BS || ch == ASCII_DEL)
    {
      fbcon_backspace(st);
      return;
    }

  /* Will another character fit on this line? */

  if (st->cursor.pos.x + st->fwidth > st->wsize.w)
    {
#ifndef CONFIG_EXAMPLES_FBCON_NOWRAP
      /* No.. move to the next line */

      fbcon_newline(st);

      /* If we were about to output a newline character, then don't */

      if (ch == ASCII_LF)
        {
          return;
        }
#else
      /* No.. Ignore all further characters until a newline is encountered */

      if (ch != ASCII_LF)
        {
          return;
        }
#endif
    }

  /* If it is a newline character, then just perform the logical newline
   * operation.
   */

  if (ch == ASCII_LF)
    {
      fbcon_newline(st);
      return;
    }

  /* Check if we need to scroll up  */

  lineheight = st->fheight + FBCON_LINESPACING;
  while (st->cursor.pos.y > st->wsize.h - lineheight)
    {
      fbcon_scroll(st, lineheight);
    }

  /* Find the glyph associated with the character and render it
   * onto the display.
   */

  bm = fbcon_addchar(st, ch);
  if (bm)
    {
      fbcon_fillchar(st, NULL, bm);
    }
}

/****************************************************************************
 * Name: fbcon_fillspace
 *
 * Description:
 *   Handle the special case of a space being displayed
 *
 * Input Parameters:
 *   st  - pointer to FBCON status structure
 *   rect- Describes the rectangular region on the display that will
 *         receive the space.
 *   bm  - pointer to the character bitmap structure with the location
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fbcon_fillspace(FAR struct fbcon_state_s *st,
  FAR const struct nxgl_rect_s *rect,
  FAR const struct fbcon_bitmap_s *bm)
{
  struct nxgl_rect_s bounds;
  struct nxgl_rect_s intersection;
  int ret;

  /* Construct a bounding box for the glyph */

  bounds.pt1.x = bm->pos.x;
  bounds.pt1.y = bm->pos.y;
  bounds.pt2.x = bm->pos.x + st->spwidth - 1;
  bounds.pt2.y = bm->pos.y + st->fheight - 1;

  /* Should this also be clipped to a region in the window? */

  if (rect != NULL)
    {
      /* Get the intersection of the redraw region and the character bitmap */

      nxgl_rectintersect(&intersection, rect, &bounds);
    }
  else
    {
      /* The intersection is the whole glyph */

      nxgl_rectcopy(&intersection, &bounds);
    }

  /* Check for empty intersections */

  if (!nxgl_nullrect(&intersection))
    {
      /* Fill the bitmap region with the background color, erasing the
       * character from the display.
       */

      ret = fbcon_fill(st, &intersection, &st->bcolor);
      if (ret < 0)
        {
          gerr("ERROR: fill() method failed: %d\n", ret);
        }
    }
}

/****************************************************************************
 * Name: fbcon_fillchar
 *
 * Description:
 *   Implement the character display
 *
 * Input Parameters:
 *   st  - pointer to FBCON status structure
 *   rect- Describes the rectangular region on the display that will
 *         receive the space.
 *   bm  - pointer to the character bitmap structure with the location
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void fbcon_fillchar(FAR struct fbcon_state_s *st,
                    FAR const struct nxgl_rect_s *rect,
                    FAR const struct fbcon_bitmap_s *bm)
{
  FAR struct fbcon_glyph_s *glyph;
  struct nxgl_rect_s bounds;
  struct nxgl_rect_s intersection;
  struct nxgl_size_s fsize;
  int ret;

  /* Handle the special case of spaces which have no glyph bitmap */

  if (BM_ISSPACE(bm))
    {
      fbcon_fillspace(st, rect, bm);
      return;
    }

  /* Get the size of the font glyph (which may not have been created yet) */

  ret = fbcon_fontsize(st->fbcon_font, bm->code, &fsize);
  if (ret < 0)
    {
      /* This would mean that there is no bitmap for the character code and
       * that the font would be rendered as a space.  But this case should
       * never happen here because the BM_ISSPACE() should have already
       * found all such cases.
       */

      return;
    }

  /* Construct a bounding box for the glyph */

  bounds.pt1.x = bm->pos.x;
  bounds.pt1.y = bm->pos.y;
  bounds.pt2.x = bm->pos.x + fsize.w - 1;
  bounds.pt2.y = bm->pos.y + fsize.h - 1;

  /* Should this also be clipped to a region in the window? */

  if (rect != NULL)
    {
      /* Get the intersection of the redraw region and the character bitmap */

      nxgl_rectintersect(&intersection, rect, &bounds);
    }
  else
    {
      /* The intersection is the whole glyph */

      nxgl_rectcopy(&intersection, &bounds);
    }

  /* Check for empty intersections */

  if (!nxgl_nullrect(&intersection))
    {
      FAR const void *src;

      /* Find (or create) the glyph that goes with this font */

      glyph = fbcon_getglyph(st, bm->code);
      if (!glyph)
        {
          /* Shouldn't happen */

          return;
        }

      /* Blit the font bitmap into the window */

      src = (FAR const void *)glyph->bitmap;

      ret = fbcon_bitmap(st, &intersection, src,
                         (unsigned int)glyph->stride);
      if (ret < 0)
        {
          gerr("ERROR: fbcon_fillchar: fbcon_bitmapwindow failed: %d\n",
               ret);
        }
    }
}

/****************************************************************************
 * Name: fbcon_vt100part
 *
 * Description:
 *   Return the next entry that is a partial match to the sequence.
 *
 * Input Parameters:
 *   st       - Driver data structure
 *   seqsize  - The number of bytes in the sequence
 *
 * Returned Value:
 *   A pointer to the matching sequence in g_vt100sequences[]
 *
 ****************************************************************************/

FAR const struct vt100_sequence_s *
     fbcon_vt100part(FAR struct fbcon_state_s *st, int seqsize)
{
  FAR static const struct vt100_sequence_s *seq;
  static int                                ndx;
  static int                                i;
  static bool                               aborted;
  static int                                numw;
  static bool                               collecting;

  /* Search from the beginning of the sequence table */

  for (ndx = 0; g_vt100sequences[ndx].seq; ndx++)
    {
      seq = &g_vt100sequences[ndx];

      /* Is this sequence big enough? */

      aborted = false;
      numw = 0;
      st->nwild = 0;

      /* compare characters received to those in the decodable sequences    */

      collecting = false;
      for (i = 0; i < seqsize; i++)
        {
          if (seq->seq[i] == '*')
            {
              /* This sequence has a wildcard */

              if ((st->seq[i] < ASCII_0) || (st->seq[i] > ASCII_9))
                {
                  aborted = true;
                  break;
                }

              if (++numw > 1)
                {
                  /* This logic only allows collection of one wildcard
                   *  per VT100 sequence
                   */

                  aborted = true;
                  break;
                }

              collecting = true;
              st->wildval = st->seq[i] - ASCII_0; /* convert from ASCII */
            }
          else if (collecting)
            {
              if ((st->seq[i] >= ASCII_0) && (st->seq[i] <= ASCII_9))
                {
                  st->wildval *= 10;
                  st->wildval += st->seq[i] - ASCII_0;
                  st->nwild++;
                }
              else
                {
                  collecting = false;
                }
            }
          else if (st->seq[i] != seq->seq[i])
            {
              aborted = true;
              break;
            }
        }

      if (!aborted)
        {
          return seq;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: fbcon_vt100seq
 *
 * Description:
 *   Determine if the new sequence is a part of a supported VT100 escape
 *   sequence.
 *
 * Input Parameters:
 *   st      - Driver data structure
 *   seqsize - The number of bytes in the sequence
 *
 * Returned Value:
 *   state - See enum fbcon_vt100state_e;
 *
 ****************************************************************************/

static enum fbcon_vt100state_e fbcon_vt100seq(
                                   FAR struct fbcon_state_s *st, int seqsize)
{
  FAR const struct vt100_sequence_s *seq;
  enum fbcon_vt100state_e            ret;

  /* Is there any VT100 escape sequence that matches what we have
   * buffered so far?
   */

  seq = fbcon_vt100part(st, seqsize);
  if (seq)
    {
      /* Yes.. if the size of that escape sequence is the same as that
       *  buffered, then we have an exact match.
       */

      if (seqsize >= seq->size + st->nwild)
        {
          /* Process the VT100 sequence */

          seq->handler(st);
          st->nseq = 0;
          return VT100_PROCESSED;
        }

      /* The 'seqsize' is still smaller than the potential match(es).  We
       * will need to collect more characters before we can make a decision.
       * Return an indication that we have consumed the character.
       */

      return VT100_CONSUMED;
    }

  /* We get here on a failure.  The buffer sequence is not part of any
   * supported VT100 escape sequence.  If seqsize > 1 then we need to
   * return a special value because we have to re-process the buffered
   * data.
   */

  ret = seqsize > 1 ? VT100_ABORT : VT100_NOT_CONSUMED;
  return ret;
}

/****************************************************************************
 * Name: fbcon_vt100
 *
 * Description:
 *   Test if the newly received byte is part of a VT100 escape sequence
 *
 * Input Parameters:
 *   st - Driver data structure
 *   ch - The newly received character
 *
 * Returned Value:
 *   state - See enum fbcon_vt100state_e;
 *
 ****************************************************************************/

static enum fbcon_vt100state_e fbcon_vt100(FAR struct fbcon_state_s *st,
                                           char ch)
{
  enum fbcon_vt100state_e ret;
  int seqsize;

  /* If we have no buffered characters, then 'ch' must be the first character
   * of an escape sequence.
   */

  if (st->nseq < 1)
    {
      /* The first character of an escape sequence must be an an escape
       * character (!).
       */

      if (ch != ASCII_ESC)
        {
          return VT100_NOT_CONSUMED;
        }

      /* Add the escape character to the buffer but don't bother with any
       * further checking.
       */

      st->seq[0] = ASCII_ESC;
      st->nseq   = 1;

      return VT100_CONSUMED;
    }

  /* Temporarily add the next character to the buffer */

  seqsize = st->nseq;
  st->seq[seqsize] = ch;

  /* Then check if this sequence is part of an a valid escape sequence */

  seqsize++;
  ret = fbcon_vt100seq(st, seqsize);
  if (ret == VT100_CONSUMED)
    {
      /* The newly added character is indeed part of a VT100 escape sequence
       * (which is still incomplete).  Keep it in the buffer.
       */

      st->nseq = seqsize;
    }

  return ret;
}

/****************************************************************************
 * Name: fbcon_write
 *
 * Description:
 *   Put a sequence of characters on the display.
 *
 * Input Parameters:
 *   st     - pointer to FBCON status structure
 *   buffer - pointer to the character array to be displayed
 *   buflen - number of characters to be displayed
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void fbcon_write(FAR struct fbcon_state_s *st,
                        FAR char *buffer, size_t buflen)
{
  char ch;
  enum fbcon_vt100state_e state;
  ssize_t remaining;

  fbcon_hidecursor(st);
  for (remaining = buflen; remaining > 0; remaining--)
    {
      ch = *buffer++;

#if CONFIG_EXAMPLES_FBCON_VT100_DECODE
      /* Check if this character is part of a VT100 escape sequence */

      do
        {
          /* Is the character part of a VT100 escape sequnce? */

          state = fbcon_vt100(st, ch);
          switch (state)
            {
              /* Character is not part of a VT100 escape sequence (and no
               * characters are buffer.
               */

              default:
              case VT100_NOT_CONSUMED:
                {
                  /* We can output the character to the window */

                  fbcon_putc(st, (uint8_t)ch);
                }
              break;

            /* The full VT100 escape sequence was processed (and the new
             * character was consumed)
             */

            case VT100_PROCESSED:

            /* Character was consumed as part of the VT100 escape processing
             * (but the escape sequence is still incomplete.
             */

            case VT100_CONSUMED:
              {
                /* Do nothing... the VT100 logic owns the character */
              }
              break;

            /* Invalid/unsupported character in escape sequence */

            case VT100_ABORT:
              {
                int i;

                /* Add the first unhandled character to the window */

                fbcon_putc(st, (uint8_t)st->seq[0]);

                /* Move all buffer characters down one */

                for (i = 1; i < st->nseq; i++)
                  {
                    st->seq[i - 1] = st->seq[i];
                  }

                st->nseq--;

                /* Then loop again and check if what remains is part of a
                 * VT100 escape sequence.  We could speed this up by
                 * checking if st->seq[0] == ASCII_ESC.
                 */
              }
              break;
            }
        }
      while (state == VT100_ABORT);
#else
    /* Just output the character */

    fbcon_putc(st, ch);
#endif /* CONFIG_EXAMPLES_FBCON_VT100_DECODE */
    }

  fbcon_showcursor(st);
}

/****************************************************************************
 * Name: fbdev_get_pinfo
 *
 * Description:
 *   Get plane information for a framebuffer
 *   Note - does not support dual framebuffer memory
 *
 * Input Parameters:
 *   fd    - file descriptor of the framebuffer
 *   pinfo - pointer to the3 structure for the plane info
 *
 * Returned Value:
 *   OK
 *
 ****************************************************************************/

static int fbdev_get_pinfo(int fd, FAR struct fb_planeinfo_s *pinfo)
{
  if (ioctl(fd, FBIOGET_PLANEINFO, (unsigned long)((uintptr_t)pinfo)) < 0)
    {
      int errcode = errno;
      gerr("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n", errcode);
      return EXIT_FAILURE;
    }

  /* Only these pixel depths are supported.  viinfo.fmt is ignored, only
   * certain color formats are supported.
   */

  if (pinfo->bpp != 32 && pinfo->bpp != 24 &&
      pinfo->bpp != 16 && pinfo->bpp != 8 &&
      pinfo->bpp != 1)
    {
      gerr("ERROR: bpp=%u not supported\n", pinfo->bpp);
      return EXIT_FAILURE;
    }

  return OK;
}

#if 0
/* Revisit needed = no support for dual framebuffers yet */

/****************************************************************************
 * Name: fb_init_mem2
 *
 * Description:
 *   Initialise the memory for the second framebuffer
 *
 * Input Parameters:
 *   st    - pointer to FBCON status structure
 *
 * Returned Value:
 *   OK
 *
 ****************************************************************************/

static int fb_init_mem2(FAR struct fbcon_state_s *st)
{
  int ret;
  uintptr_t buf_offset;
  struct fb_planeinfo_s pinfo;

  memset(&pinfo, 0, sizeof(pinfo));
  pinfo.display = st->pinfo->display + 1;

  if ((ret = fbdev_get_pinfo(st->fd_fb, &pinfo)) < 0)
    {
      return EXIT_FAILURE;
    }

  /* Check bpp */

  if (pinfo.bpp != st->pinfo->bpp)
    {
      gerr("ERROR: fbmem2 is incorrect");
      return -EINVAL;
    }

  /* Check the buffer address offset,
   * It needs to be divisible by pinfo->stride
   */

  buf_offset = pinfo.fbmem - st->fbmem;

  if ((buf_offset % st->pinfo->stride) != 0)
    {
      gerr("ERROR: It is detected that buf_offset(%" PRIuPTR ") "
           "and stride(%d) are not divisible, please ensure "
           "that the driver handles the address offset by itself.\n",
            buf_offset, st->pinfo->stride);
    }

  /* Calculate the address and yoffset of fbmem2 */

  if (buf_offset == 0)
    {
      /* Use consecutive fbmem2. */

      st->mem2_yoffset = st->vinfo->yres;
      st->fbmem2 = pinfo.fbmem + st->mem2_yoffset * pinfo.stride;
      gerr("ERROR: Use consecutive fbmem2 = %p, yoffset = %" PRIu32"\n",
             st->fbmem2, st->mem2_yoffset);
    }
  else
    {
      /* Use non-consecutive fbmem2. */

      st->mem2_yoffset = buf_offset / st->pinfo->stride;
      st->fbmem2 = pinfo.fbmem;
      gerr("ERROR: Use non-consecutive fbmem2 = %p, yoffset = %" PRIu32"\n",
            st->fbmem2, st->mem2_yoffset);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: fbcon_initialize
 *
 * Description:
 *   Initialise the Framebuffer Console
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   Success or failure code
 *
 ****************************************************************************/

static int fbcon_initialize(FAR struct fbcon_state_s *st)
{
  struct nxgl_rect_s         rect;
  FAR const struct nx_font_s *fontset;
  int                        ret;

  /* Get the configured font handles */

  st->fbcon_font = nxf_getfonthandle(FBCON_FONTID);
  if (!st->fbcon_font)
    {
      gerr("ERROR: fbcon_main: Failed to get console font handle: %d\n",
           errno);
      return FBCON_EXIT_FONTOPEN;
    }

  st->bcolor = FBCON_BGCOLOR;
  st->fcolor = FBCON_FCOLOR;

  rect.pt1.x = 0;
  rect.pt1.y = 0;
  rect.pt2.x = st->vinfo->xres - 1;
  rect.pt2.y = st->vinfo->yres - 1;
  ret = fbcon_fill(st, &rect, &st->bcolor);
  if (ret < 0)
    {
      gerr("ERROR: fbcon_main: fbcon_setbgcolor failed: %d\n", errno);
      return FBCON_EXIT_SETBGCOLOR;
    }

  fontset         = nxf_getfontset(st->fbcon_font);
  st->fheight     = fontset->mxheight;
  st->fwidth      = fontset->mxwidth;
  st->spwidth     = fontset->spwidth;

  /* we use the entire LCD area for the console */

  st->wsize.h     = st->vinfo->yres;
  st->wsize.w     = st->vinfo->xres;

  st->cursor.code  = FBCON_CURSORCHAR;
  if (FBCON_CURSORCHAR != ASCII_SPACE)
    {
      st->cursor.flags = 0;
    }
  else
    {
      st->cursor.flags = BMFLAGS_NOGLYPH;
    }

  st->nchars       = 0;
  st->maxglyphs    = FBCON_GLCACHE;

  /* The first character is one space from the left
   * and FBCON_LINESEPARATION lines from the top
   */

  st->cursor.pos.x = st->spwidth;
  st->cursor.pos.y = FBCON_LINESPACING;
  st->maxchars     = (st->wsize.w / st->fwidth) *
                     (st->wsize.h / (st->fheight + FBCON_LINESPACING) - 1);

  st->bm = malloc(st->maxchars * sizeof(struct fbcon_bitmap_s));
  if (st->bm == NULL)
    {
      gerr("ERROR: Unable to allocate display buffer memory");
      return -ENOMEM;
    }

  memset(st->bm, 0, st->maxchars * sizeof(struct fbcon_bitmap_s));

  st->glyph = malloc(FBCON_GLCACHE * (sizeof(struct fbcon_glyph_s)));
  if (st->glyph == NULL)
    {
      gerr("ERROR: Unable to allocate glyph cache memory");
      return -ENOMEM;
    }

  memset(st->glyph, 0, FBCON_GLCACHE * (sizeof(struct fbcon_glyph_s)));

  st->nseq  = 0;

  return OK;
}

/****************************************************************************
 * Name: has_input
 *
 * Description:
 *   Return true if a File Descriptor has data to be read.
 *
 * Input Parameters:
 *   fd - File Descriptor to be checked
 *
 * Returned Value:
 *   True if File Descriptor has data to be read; False otherwise
 *
 ****************************************************************************/

static bool has_input(int fd)
{
  int ret;

  /* Poll the File Descriptor for input */

  struct pollfd fdp;
  fdp.fd = fd;
  fdp.events = POLLIN;
  ret = poll(&fdp,  /* File Descriptors                                     */
            1,      /* Number of File Descriptors                           */
            0);     /* Poll Timeout (Milliseconds)                          */

  if (ret > 0)
    {
      /* If poll is OK and there is input */

      if ((fdp.revents & POLLIN) != 0)
        {
          /* Report that there is input */

          return true;
        }

      /* Else report no input */

      return false;
    }
  else if (ret == 0)
    {
      /* If timeout, report no input */

      return false;
    }
  else if (ret < 0)
    {
      /* Handle error */

      fprintf(stderr, "poll failed: %d, fd=%d\n", ret, fd);
      return false;
    }

  /* Never comes here */

  assert(false);
  return false;
}

/****************************************************************************
 * Name: poll_std_streams
 *
 * Description:
 *   Poll NSH stdout and stderr for output and display the output.
 *
 * Input Parameters:
 *   st - pointer to FBCON status structure
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void poll_std_streams(FAR struct fbcon_state_s *st)
{
  ssize_t     num_ch;
  static char buf[POLL_BUFSIZE];

  assert(g_nsh_stdout[READ_PIPE] != 0);
  assert(g_nsh_stderr[READ_PIPE] != 0);
#ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDIN  
  assert(g_nsh_stdin[READ_PIPE] != 0);
#endif

#ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDERR
  /* Poll NSH stderr to check if there's output to be processed */

  if (has_input(g_nsh_stderr[READ_PIPE]))
    {
      /* Write it to display */

      num_ch = read(g_nsh_stderr[READ_PIPE], buf, POLL_BUFSIZE);
      if (num_ch > 0)
        {
          /* display */

          fbcon_write(st, buf, num_ch);
        }
    }
#endif /* CONFIG_EXAMPLES_FBCON_PIPE_STDERR */

#ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDOUT
  /* Poll NSH stdout to check if there's output to be processed */

  if (has_input(g_nsh_stdout[READ_PIPE]))
    {
      /* Read the output from NSH stdout */

      num_ch = read(g_nsh_stdout[READ_PIPE], buf, POLL_BUFSIZE);
      if (num_ch > 0)
        {
          /* Write it to display */

          fbcon_write(st, buf, num_ch);
        }
    }
#endif /* CONFIG_EXAMPLES_FBCON_PIPE_STDOUT */

#ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDIN
  if (has_input(STDIN_FILENO))
    {
      /* Get and process a character */

      ssize_t num = 0;
      char ch;
      num_ch = read(STDIN_FILENO, &ch, 1);

      if (num_ch != 1)
        {
          fprintf(stderr, "STDIN read failed\n");
          return;
        }

      /* copy it to spawned app/process */

      num = write(g_nsh_stdin[WRITE_PIPE], &ch, 1);
      if (num != num_ch)
        {
          fprintf(stderr, "STDIN write failed\n");
          return;
        }

#  if defined(CONFIG_NSH_READLINE)
      fbcon_write(st, buf, num_ch);
#  elif defined(CONFIG_NSH_CLE)
      if (ch == ASCII_LF)
        {
          fbcon_write(st, &ch, 1);
        }
#  endif
    }
#endif /* CONFIG_EXAMPLES_FBCON_PIPE_STDIN */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fbcon_main
 *
 * Description:
 *   fbcon entry point
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct fbcon_state_s   *st;
  int                         index;
  int                         ret;
  pid_t                       pid;
  posix_spawn_file_actions_t  actions;
  posix_spawnattr_t           attr;
  FAR const struct builtin_s *builtin;
  int                         exitcode = FBCON_EXIT_SUCCESS;
  FAR const char             *fbdev   = CONFIG_EXAMPLES_FBCON_DEF_FB;

  /* There is a single optional argument:  The path to the framebuffer
   * driver.
   */

  if (argc == 2)
    {
      fbdev = argv[1];
    }
  else if (argc != 1)
    {
      fprintf(stderr, "ERROR: Single argument required\n");
      fprintf(stderr, "USAGE: %s [<fb-driver-path>]\n", argv[0]);
      return FBCON_EXIT_FAIL;
    }

  st = malloc(sizeof(struct fbcon_state_s));
  if (st == NULL)
    {
      gerr("ERROR: Unable to allocate fbcon_state_s memory");
      return -ENOMEM;
    }

  st->pinfo = malloc(sizeof(struct fb_planeinfo_s));
  if (st->pinfo == NULL)
    {
      gerr("ERROR: Unable to allocate pinfo memory");
      return -ENOMEM;
    }

  st->vinfo = malloc(sizeof(struct fb_videoinfo_s));
  if (st->vinfo == NULL)
    {
      gerr("ERROR: Unable to allocate vinfo memory");
      return -ENOMEM;
    }

  /* Open the framebuffer driver */

  st->fd_fb = open(fbdev, O_RDWR);
  if (st->fd_fb < 0)
    {
      int errcode = errno;
      gerr("ERROR: Failed to open %s: %d\n", fbdev, errcode);
      return FBCON_EXIT_FD;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(st->fd_fb, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)st->vinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_VIDEOINFO) failed: %d\n",
                       errcode);
      exitcode = FBCON_EXIT_GETVINFO;
      goto errout;
    }

#ifdef CONFIG_FB_OVERLAY
  gerr("ERROR: noverlays: %u\n", st->vinfo->noverlays);

  /* Select the first overlay, which should be the composed framebuffer */

  ret = ioctl(st->fd_fb, FBIO_SELECT_OVERLAY, 0);
  if (ret < 0)
    {
      int errcode = errno;
      gerr("ERROR: ioctl(FBIO_SELECT_OVERLAY) failed: %d\n", errcode);
      ret = FBCON_EXIT_FBIO_SELECT_OVERLAY
      goto errout;
    }

  /* Get the first overlay information */

  st->oinfo.overlay = 0;
  ret = ioctl(st->fd_fb, FBIOGET_OVERLAYINFO,
                        (unsigned long)((uintptr_t)&st->oinfo));
  if (ret < 0)
    {
      int errcode = errno;
      gerr("ERROR: ioctl(FBIOGET_OVERLAYINFO) failed: %d\n", errcode);
      ret = FBCON_EXIT_FBIO_OVERLAY_INFO
      goto errout;
    }

  /* select default framebuffer layer */

  ret = ioctl(st->fd_fb, FBIO_SELECT_OVERLAY, FB_NO_OVERLAY);
  if (ret < 0)
    {
      int errcode = errno;
      gerr("ERROR: ioctl(FBIO_SELECT_OVERLAY) failed: %d\n", errcode);
      ret = FBCON_EXIT_FBIO_SELECT_OVERLAY
      goto errout;
    }

#endif

  if ((ret = fbdev_get_pinfo(st->fd_fb, st->pinfo)) < 0)
    {
      ret = FBCON_EXIT_GETPINFO;
      goto errout;
    }

  /* mmap() the framebuffer.
   *
   * NOTE: In the FLAT build the frame buffer address returned by the
   * FBIOGET_PLANEINFO IOCTL command will be the same as the framebuffer
   * address.  mmap(), however, is the preferred way to get the framebuffer
   * address because in the KERNEL build, it will perform the necessary
   * address mapping to make the memory accessible to the application.
   */

  st->fbmem = mmap(NULL, st->pinfo->fblen, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_FILE, st->fd_fb, 0);
  if (st->fbmem == MAP_FAILED)
    {
      gerr("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n", errno);
      ret = FBCON_EXIT_FBMEM;
      goto errout;
    }

#if 0
  /* Revisit needed = no support for dual framebuffers yet */

  if (st->pinfo->yres_virtual == (st->vinfo->yres * 2))
    {
      if ((ret = fb_init_mem2(st)) < 0)
        {
          ret = FBCON_EXIT_FBMEM2;
          goto errout;
        }
    }

#endif
  ret = fbcon_initialize(st);
  if (ret != OK)
    {
      exitcode = ret;
      goto errout;
    }

  fbcon_home(st);
  fbcon_showcursor(st);

  /* Pipe and duplicate STDOUT and/or STDERR */

#ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDOUT
  ret = pipe(g_nsh_stdout);
  if (ret < 0)
    {
      _err("stdout pipe failed: %d\n", errno);
      return ERROR;
    }

  close(STDOUT_FILENO);
  dup2(g_nsh_stdout[WRITE_PIPE], 1);
#endif
#ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDERR
  ret = pipe(g_nsh_stderr);
  if (ret < 0)
    {
      _err("stderr pipe failed: %d\n", errno);
      return ERROR;
    }

  close(STDERR_FILENO);
  dup2(g_nsh_stderr[WRITE_PIPE], 2);
#endif

  ret = posix_spawn_file_actions_init(&actions);
  if (ret != 0)
    {
      gerr("ERROR: unable to init spawn sctiond for"
           "requested task " "\"""%s""\"" ".\n", SPAWN_TASK);
      exitcode = FBCON_EXIT_POSIX_SPAWN_ACTIONS_INIT_FAILED;
      goto errout;
    }

#ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDIN
  /* Spawned app/task may need to intercept and forward STDIN
   * otherwise any waiting input characters can be missed if read by the
   * spawned app/task first.
   */

  ret = pipe(g_nsh_stdin);
  if (ret < 0)
    {
      gerr("ERROR: stdin pipe failed: %d\n", errno);
      exitcode =  FBCON_EXIT_STDIN_PIPE_FAILED;
      goto errout;
    }

  posix_spawn_file_actions_addclose(&actions, STDIN_FILENO);
  posix_spawn_file_actions_adddup2(&actions, g_nsh_stdin[READ_PIPE],
                                   STDIN_FILENO);

#endif /* CONFIG_EXAMPLES_FBCON_PIPE_STDIN */

  index = builtin_isavail(SPAWN_TASK);
  if (index < 0)
    {
      gerr("ERROR: requested task " "\"""%s""\"" " not available.\n",
            SPAWN_TASK);
      exitcode = FBCON_EXIT_APP_INDEX_UNAVAILABLE;
      goto errout;
    }

  builtin = builtin_for_index(index);
  if (builtin == NULL)
    {
      gerr("ERROR: requested task " "\"""%s""\""
          " has no scheduling parameters.\n", SPAWN_TASK);
      exitcode = FBCON_EXIT_APP_INDEX_UNAVAILABLE;
      goto errout;
    }

  /* Set up for app/task spawn */

  ret = posix_spawnattr_init(&attr);
  if (ret != 0)
    {
      gerr("ERROR: unable to init spawn attributes for"
           "requested task " "\"""%s""\"" ".\n", SPAWN_TASK);
      exitcode = FBCON_EXIT_APP_POSIX_ATTRIB_INIT_FAILED;
      goto errout;
    }

  attr.stacksize = builtin->stacksize;
  attr.priority  = builtin->priority;

  /* Spawn required application                                             */

  ret = posix_spawn(&pid,       /* Returned Task ID                         */
                    SPAWN_TASK, /* Task Path                                */
                    &actions,   /* Replace STDIN and/or STDOUT and/or STDIN */
                    &attr,      /* Attributes of app/task                   */
                    NULL,       /* Arguments                                */
                    NULL);      /* No environment                           */
  if (ret < 0)
    {
      int errcode = errno;
      gerr("ERROR: posix_spawn failed: %d\n", errcode);
      exitcode = FBCON_EXIT_POSIX_SPAWN_FAILED;
    }

#ifdef CONFIG_EXAMPLES_FBCON_SHOW_WELCOME
#  ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDOUT
  fprintf(stdout, "%s\n", g_stdout_hello);
  fflush(stdout);
#  endif
#  ifdef CONFIG_EXAMPLES_FBCON_PIPE_STDERR
  fprintf(stderr, "%s\n", g_stderr_hello);
  fflush(stderr);
#  endif
#endif

  for (; ; )
    {
      poll_std_streams(st);
      usleep(10000);
    }

errout:
  close(st->fd_fb);
  fprintf(stderr, "FBCON exiting with error %d\n", exitcode);
  return exitcode;
}

