/****************************************************************************
 * apps/graphics/nuttx/pdcnuttx.h
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

#ifndef __APPS_GRAPHICS_PDCURS34_NUTTX_PDCNUTTX_H
#define __APPS_GRAPHICS_PDCURS34_NUTTX_PDCNUTTX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "nuttx/config.h"

#include <stdint.h>

#include "nuttx/input/djoystick.h"
#include "nuttx/nx/nx.h"
#include "nuttx/nx/nxfonts.h"
#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

#include "curspriv.h"
#ifdef CONFIG_SYSTEM_TERMCURSES
#include <system/termcurses.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef HAVE_BOLD_FONT

#if defined(CONFIG_PDCURSES_FONT_4X6)
#  define PDCURSES_FONTID       FONTID_X11_MISC_FIXED_4X6
#  define PDCURSES_BOLD_FONTID  FONTID_X11_MISC_FIXED_4X6
#elif defined(CONFIG_PDCURSES_FONT_5X7)
#  define PDCURSES_FONTID       FONTID_X11_MISC_FIXED_5X7
#  define PDCURSES_BOLD_FONTID  FONTID_X11_MISC_FIXED_5X7
#elif defined(CONFIG_PDCURSES_FONT_5X8)
#  define PDCURSES_FONTID       FONTID_X11_MISC_FIXED_5X8
#  define PDCURSES_BOLD_FONTID  FONTID_X11_MISC_FIXED_5X8
#elif defined(CONFIG_PDCURSES_FONT_6X9)
#  define PDCURSES_FONTID       FONTID_X11_MISC_FIXED_6X9
#  define PDCURSES_BOLD_FONTID  FONTID_X11_MISC_FIXED_6X9
#elif defined(CONFIG_PDCURSES_FONT_6X10)
#  define PDCURSES_FONTID       FONTID_X11_MISC_FIXED_6X10
#  define PDCURSES_BOLD_FONTID  FONTID_X11_MISC_FIXED_6X10
#elif defined(CONFIG_PDCURSES_FONT_6X12)
#  define PDCURSES_FONTID       FONTID_X11_MISC_FIXED_6X12
#  define PDCURSES_BOLD_FONTID  FONTID_X11_MISC_FIXED_6X12
#elif defined(CONFIG_PDCURSES_FONT_6X13)
#  define PDCURSES_FONTID       FONTID_X11_MISC_FIXED_6X13
#  define PDCURSES_BOLD_FONTID  FONTID_X11_MISC_FIXED_6X13B
#  define HAVE_BOLD_FONT        1
#elif defined(CONFIG_PDCURSES_FONT_7X13)
#  define PDCURSES_FONTID       FONTID_X11_MISC_FIXED_7X13
#  define PDCURSES_BOLD_FONTID  FONTID_X11_MISC_FIXED_7X13B
#  define HAVE_BOLD_FONT        1
#elif defined(CONFIG_PDCURSES_FONT_7X14)
#  define PDCURSES_FONTID       FONTID_X11_MISC_FIXED_7X14
#  define PDCURSES_BOLD_FONTID  FONTID_X11_MISC_FIXED_7X14B
#  define HAVE_BOLD_FONT        1
#elif defined(CONFIG_PDCURSES_FONT_8X13)
#  define PDCURSES_FONTID        FONTID_X11_MISC_FIXED_8X13
#  define PDCURSES_BOLD_FONTID   FONTID_X11_MISC_FIXED_8X13B
#  define HAVE_BOLD_FONT        1
#elif defined(CONFIG_PDCURSES_FONT_9X15)
#  define PDCURSES_FONTID        FONTID_X11_MISC_FIXED_9X15
#  define PDCURSES_FONTID        FONTID_X11_MISC_FIXED_9X15B
#  define HAVE_BOLD_FONT        1
#elif defined(CONFIG_PDCURSES_FONT_9X18)
#  define PDCURSES_FONTID        FONTID_X11_MISC_FIXED_9X18
#  define PDCURSES_BOLD_FONTID   FONTID_X11_MISC_FIXED_9X18B
#  define HAVE_BOLD_FONT        1
#elif defined(CONFIG_PDCURSES_FONT_10X20)
#  define PDCURSES_FONTID        FONTID_X11_MISC_FIXED_10X20
#  define PDCURSES_BOLD_FONTID   FONTID_X11_MISC_FIXED_10X20
#else
#  error No fixed width font selected
#endif

#undef PDCURSES_MONOCHROME

#if defined(CONFIG_PDCURSES_COLORFMT_Y1)
#  define PDCURSES_COLORFMT      FB_FMT_Y1
#  define PDCURSES_BPP           1
#  define PDCURSES_BPP_SHIFT     0
#  define PDCURSES_PPB           8
#  define PDCURSES_PPB_MASK      (PDCURSES_PPB - 1)
#  define PDCURSES_PPB_SHIFT     3
#  define PDCURSES_INIT_COLOR    CONFIG_PDCURSES_BGCOLOR_GREYLEVEL
#  define PDCURSES_MONOCHROME    1
#elif defined(CONFIG_PDCURSES_COLORFMT_Y2)
#  define PDCURSES_COLORFMT      FB_FMT_Y2
#  define PDCURSES_BPP           2
#  define PDCURSES_BPP_SHIFT     1
#  define PDCURSES_PPB           4
#  define PDCURSES_PPB_MASK      (PDCURSES_PPB - 1)
#  define PDCURSES_PPB_SHIFT     2
#  define PDCURSES_INIT_COLOR    CONFIG_PDCURSES_BGCOLOR_GREYLEVEL
#  define PDCURSES_MONOCHROME    1
#elif defined(CONFIG_PDCURSES_COLORFMT_Y4)
#  define PDCURSES_COLORFMT      FB_FMT_Y4
#  define PDCURSES_BPP           4
#  define PDCURSES_BPP_SHIFT     2
#  define PDCURSES_PPB           2
#  define PDCURSES_PPB_MASK      (PDCURSES_PPB - 1)
#  define PDCURSES_PPB_SHIFT     1
#  define PDCURSES_INIT_COLOR    CONFIG_PDCURSES_BGCOLOR_GREYLEVEL
#  define PDCURSES_MONOCHROME    1
#elif defined(CONFIG_PDCURSES_COLORFMT_RGB332)
#  define PDCURSES_COLORFMT      FB_FMT_RGB8_332
#  define PDCURSES_BPP           8
#  define PDCURSES_BPP_MASK      7
#  define PDCURSES_BPP_SHIFT     3
#  define PDCURSES_INIT_COLOR    RGBTO8(CONFIG_PDCURSES_BGCOLOR_RED, \
                                        CONFIG_PDCURSES_BGCOLOR_GREEN, \
                                        CONFIG_PDCURSES_BGCOLOR_BLUE)
#elif defined(CONFIG_PDCURSES_COLORFMT_RGB565)
#  define PDCURSES_COLORFMT      FB_FMT_RGB16_565
#  define PDCURSES_BPP           16
#  define PDCURSES_BPP_MASK      15
#  define PDCURSES_BPP_SHIFT     4
#  define PDCURSES_INIT_COLOR    RGBTO16(CONFIG_PDCURSES_BGCOLOR_RED, \
                                         CONFIG_PDCURSES_BGCOLOR_GREEN, \
                                         CONFIG_PDCURSES_BGCOLOR_BLUE)
#elif defined(CONFIG_PDCURSES_COLORFMT_RGB888)
#  define PDCURSES_COLORFMT      FB_FMT_RGB24  /* RGB24 at 32-BPP */
#  define PDCURSES_BPP           32
#  define PDCURSES_BPP_MASK      31
#  define PDCURSES_BPP_SHIFT     5
#  define PDCURSES_INIT_COLOR    RGBTO24(CONFIG_PDCURSES_BGCOLOR_RED, \
                                         CONFIG_PDCURSES_BGCOLOR_GREEN, \
                                         CONFIG_PDCURSES_BGCOLOR_BLUE)
#else
#  error No color format selected
#endif

/* Select renderer -- Some additional logic would be required to support
 * pixel depths that are not directly addressable (1,2,4, and 24).
 */

#if PDCURSES_BPP == 1
#  define RENDERER nxf_convert_1bpp
#elif PDCURSES_BPP == 2
#  define RENDERER nxf_convert_2bpp
#elif PDCURSES_BPP == 4
#  define RENDERER nxf_convert_4bpp
#elif PDCURSES_BPP == 8
#  define RENDERER nxf_convert_8bpp
#elif PDCURSES_BPP == 16
#  define RENDERER nxf_convert_16bpp
#elif PDCURSES_BPP == 24
#  define RENDERER nxf_convert_24bpp
#elif PDCURSES_BPP == 32
#  define RENDERER nxf_convert_32bpp
#else
#  error "Unsupported bits-per-pixel"
#endif

/* Convert bits to bytes to hold an even number of pixels */

#define PDCURSES_ALIGN_UP(n)   (((n) + PDCURSES_BPP_MASK) >> 3)
#define PDCURSES_ALIGN_DOWN(n) (((n) & ~PDCURSES_BPP_MASK) >> 3)

/****************************************************************************
 * Private Types
 ****************************************************************************/

#if defined(__cplusplus)
extern "C"
{
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif

/* Describes one color pair */

struct pdc_colorpair_s
{
  short fg;
  short bg;
};

/* Describes one RGB color */

struct pdc_rgbcolor_s
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

/* Holds one framebuffer pixel */

#if PDCURSES_BPP <= 8
typedef uint8_t  pdc_color_t;
#elif PDCURSES_BPP <= 16
typedef uint16_t pdc_color_t;
#elif PDCURSES_BPP <= 32
typedef uint32_t pdc_color_t;
#endif

/* This structure provides the overall state of the framebuffer device */

struct pdc_fbstate_s
{
  /* Framebuffer */

  int fbfd;                /* Open framebuffer driver file descriptor */
  FAR void *fbmem;         /* Start of framebuffer memory */
  fb_coord_t xres;         /* Horizontal resolution (pixels) */
  fb_coord_t yres;         /* Vertical resolution (rows) */
  fb_coord_t stride;       /* Length of a line (bytes) */

#ifdef CONFIG_PDCURSES_DJOYSTICK
  /* Discrete joystick */

  int djfd;                /* Open discrete joystick driver file descriptor */
  djoy_buttonset_t djlast; /* Last sampled joystick state */
#endif

  /* Font */

  NXHANDLE hfont;          /* Handle used to access selected font */
#ifdef HAVE_BOLD_FONT
  NXHANDLE hbold;          /* Handle used to access bold font */
#endif
  uint8_t fheight;         /* Height of the font (rows) */
  uint8_t fwidth;          /* Width of the font (pixels) */
#if PDCURSES_BPP < 8
  uint8_t fstride;         /* Width of the font buffer (bytes) */
  FAR uint8_t *fbuffer;    /* Allocated font buffer */
#endif

  /* Drawable area (See also SP->lines and SP->cols) */

  fb_coord_t xpos;         /* Drawing X position (pixels) */
  fb_coord_t ypos;         /* Drawing Y position (rows) */
  uint8_t hoffset;         /* Offset from left of display (pixels) */
  uint8_t voffset;         /* Offset from top of display (rows) */

  /* Colors */

  struct pdc_colorpair_s colorpair[PDC_COLOR_PAIRS];
#ifdef PDCURSES_MONOCHROME
  uint8_t greylevel[16];
#else
  struct pdc_rgbcolor_s rgbcolor[16];
#endif
};

/* This structure contains the framebuffer device structure and is a cast
 * compatible with type SCREEN.
 */

struct pdc_fbscreen_s
{
  SCREEN screen;
  struct pdc_fbstate_s fbstate;
};

#ifdef CONFIG_SYSTEM_TERMCURSES

/* This structure provides the overall state of the termcurses device */

struct pdc_termstate_s
{
  /* Terminal fd numbers (typically 0 and 1) */

  int    out_fd;
  int    in_fd;

  /* Colors */

  short  fg_red;
  short  fg_green;
  short  fg_blue;
  short  bg_red;
  short  bg_green;
  short  bg_blue;

#ifdef CONFIG_PDCURSES_CHTYPE_LONG
  long   attrib;
#else
  short  attrib;
#endif

  struct pdc_colorpair_s colorpair[PDC_COLOR_PAIRS];
#ifdef PDCURSES_MONOCHROME
  uint8_t greylevel[16];
#else
  struct pdc_rgbcolor_s rgbcolor[16];
#endif

  FAR struct termcurses_s *tcurs;
};

/* This structure contains the termstate structure and is a cast
 * compatible with type SCREEN.
 */

struct pdc_termscreen_s
{
  SCREEN screen;
  struct pdc_termstate_s termstate;
};
#endif   /* CONFIG_SYSTEM_TERMCURSES */

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This singleton represents the state of frame buffer device.  pdcurses
 * depends on global variables and, hence, can never support more than a
 * single framebuffer instance in the same address space.
 */

EXTERN struct pdc_fbstate_s g_pdc_fbstate;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_clear_screen
 *
 * Description:
 *   Set the framebuffer content to a single color
 *
 ****************************************************************************/

void PDC_clear_screen(FAR struct pdc_fbstate_s *fbstate);

/****************************************************************************
 * Name: PDC_input_open
 *
 * Description:
 *   Open and configure any input devices
 *
 ****************************************************************************/

#ifdef CONFIG_PDCURSES_HAVE_INPUT
int PDC_input_open(FAR struct pdc_fbstate_s *fbstate);
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
void PDC_input_close(FAR struct pdc_fbstate_s *fbstate);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_GRAPHICS_PDCURS34_NUTTX_PDCNUTTX_H */
