/****************************************************************************
 * apps/games/NXDoom/src/i_video.c
 *
 * SPDX-License-Identifier: GPLv2
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
 * DOOM graphics stuff for SDL.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "config.h"
#include "d_loop.h"
#include "deh_str.h"
#include "doomtype.h"
#include "i_input.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "tables.h"
#include "v_diskicon.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define RESIZE_DELAY 500

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct graphics_state_s
{
  int fd; /* File descriptor handle to frame buffer */

  /* The 320x200x32 RGBA intermediate buffer is what we blit the former
   * buffer to. On NuttX, this is the frame buffer memory `fbmem`. It may not
   * have 32-bit depth, but if it doesn't, the code is adjusted accordingly.
   */

  FAR void *fbmem;

  /* 8-bit depth screen buffer (320x200x8) that we draw to (i.e. the one that
   * holds i_video_buffer)
   */

  pixel_t *scrnbuf;

  /* Information about the frame buffer needed for rendering. */

  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;

  /* Scale multiplier for rendering large image */

  uint8_t scale;

  /* Byte offset into the frame buffer of the top left corner of the scaled
   * image, so that it is centred on displays it does not fill.
   */

  size_t origin;

  /* Size of the scaled image in pixels.  This is an integer multiple of the
   * DOOM resolution unless the image is stretched to fill the display.
   */

  unsigned outw;
  unsigned outh;

  /* Maps an output column onto the source column it is drawn from, so that
   * the inner loop needs neither a division nor a separate case for
   * fractional scaling.
   */

  FAR uint16_t *colmap;

  bool inited; /* Track initialization */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* NuttX graphics state */

static struct graphics_state_s g_graphics_state =
{
  0
};

#ifdef CONFIG_GAMES_NXDOOM_STATIC_SCRNBUF
/* The buffer DOOM renders into.  Keeping it in .bss rather than on the heap
 * puts it in internal RAM on boards whose heap is mostly external memory.
 */

static pixel_t g_scrnbuf[SCREENWIDTH * SCREENHEIGHT];
#endif

#ifdef CONFIG_GAMES_NXDOOM_ROWSTAGE
/* Staging area for one output row.  Expanding a row here and then copying it
 * out keeps the per-pixel writes in internal RAM and turns every frame
 * buffer access into a burst-friendly copy, which is what an external frame
 * buffer wants.  Declared as uint32_t for alignment; it holds one row at the
 * largest scale and pixel size it can serve.
 */

#define BLIT_MAX_STAGED_SCALE 4

static uint32_t g_rowbuf[SCREENWIDTH * BLIT_MAX_STAGED_SCALE];
#endif

/* Window title */

static const char *g_window_title = "";

/* Colour palette map from 8-bit colour to 32-bit */

static struct argbcolor_s g_palette[256];

/* The same palette, pre-converted to the pixel format of the frame buffer.
 * Doing the conversion once per palette change instead of once per pixel
 * keeps blit_screen() down to a lookup and a store.
 */

static uint32_t g_palette_native[256];

static boolean palette_to_set;

/* disable mouse? */

static boolean nomouse = false;

/* Maximum number of pixels to use for intermediate scale buffer. */

static int max_scaling_buffer_pixels = 16000000;

/* Time to wait for the screen to settle on startup before starting the game
 * (ms)
 */

static int startup_delay = 1000;

/* Grab the mouse? (int type for config code). nograbmouse_override allows
 * this to be temporarily disabled via the command line.
 */

static int grabmouse = true;
static boolean nograbmouse_override = false;

/* If true, we display dots at the bottom of the screen to
 * indicate FPS.
 */

static boolean display_fps_dots;

/* If this is true, the screen is rendered but not blitted to the
 * video buffer.
 */

static boolean noblit;

/* Callback function to invoke to determine whether to grab the
 * mouse pointer.
 */

static grabmouse_callback_t grabmouse_callback = NULL;

/* Does the window currently have focus? */

static boolean window_focused = true;

/* Window resize state. */

#if 0
static boolean need_resize = false;
static unsigned int last_resize_time;
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

int usemouse = 1;

/* Save screenshots in PNG format. */

int png_screenshots = 0;

/* SDL video driver name */

char *video_driver = "";

/* Window position: */

char *window_position = "center";

/* SDL display number on which to run. */

int video_display = 0;

/* Screen width and height, from configuration file. */

int window_width = 320;
int window_height = 200;

/* Fullscreen mode, 0x0 for SDL_WINDOW_FULLSCREEN_DESKTOP. */

int fullscreen_width = 0;
int fullscreen_height = 0;

/* Run in full screen mode?  (int type for config code) */

int fullscreen = true;

/* Smooth pixel scaling */

int smooth_pixel_scaling = true;

/* Force integer scales for resolution-independent rendering */

int integer_scaling = false;

/* VGA Porch palette change emulation */

int vga_porch_flash = false;

/* Force software rendering, for systems which lack effective hardware
 * acceleration
 */

int force_software_renderer = false;

/* The screen buffer; this is modified to draw things to the screen */

pixel_t *i_video_buffer = NULL;

/* If true, game is running as a screensaver */

boolean screensaver_mode = false;

/* Flag indicating whether the screen is currently visible:
 * when the screen isn't visible, don't render the screen
 */

boolean screenvisible = true;

/* Gamma correction level to use */

int usegamma = 0;

/* Joystick/gamepad hysteresis */

unsigned int joywait = 0;

/* TODO: I'm sure more of the variables above can be private */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: update_native_palette
 *
 * Description:
 *   Convert the ARGB palette into the pixel format of the frame buffer so
 *   that blitting is a plain table lookup.
 *
 ****************************************************************************/

static void update_native_palette(void)
{
  int i;

#ifdef CONFIG_GAMES_NXDOOM_FB_CMAP
  uint8_t red[256];
  uint8_t green[256];
  uint8_t blue[256];
#ifdef CONFIG_FB_TRANSPARENCY
  uint8_t transp[256];
#endif
  struct fb_cmap_s cmap;

  /* Hand the palette to the display and leave the lookup an identity, so
   * that blitting writes the palette index and the hardware converts it
   * while it scans the display out.
   */

  for (i = 0; i < 256; i++)
    {
      red[i]              = g_palette[i].r;
      green[i]            = g_palette[i].g;
      blue[i]             = g_palette[i].b;
#ifdef CONFIG_FB_TRANSPARENCY
      transp[i]           = 0xff;
#endif
      g_palette_native[i] = i;
    }

  cmap.first = 0;
  cmap.len   = 256;
  cmap.red   = red;
  cmap.green = green;
  cmap.blue  = blue;
#ifdef CONFIG_FB_TRANSPARENCY
  cmap.transp = transp;
#endif

  if (ioctl(g_graphics_state.fd, FBIOPUT_CMAP,
            (unsigned long)((uintptr_t)&cmap)) < 0)
    {
      i_error("ioctl(FBIOPUT_CMAP) failed: %d\n", errno);
    }
#else
  for (i = 0; i < 256; i++)
    {
      switch (g_graphics_state.pinfo.bpp)
        {
          case 8:
            g_palette_native[i] = RGBTO8(g_palette[i].r, g_palette[i].g,
                                         g_palette[i].b);
            break;

          case 16:
            g_palette_native[i] = RGBTO16(g_palette[i].r, g_palette[i].g,
                                          g_palette[i].b);
            break;

          case 24:
            g_palette_native[i] = RGBTO24(g_palette[i].r, g_palette[i].g,
                                          g_palette[i].b);
            break;

          default:
            g_palette_native[i] = ARGBTO32(g_palette[i].a, g_palette[i].r,
                                           g_palette[i].g, g_palette[i].b);
            break;
        }
    }
#endif
}

/****************************************************************************
 * Name: blit_screen
 *
 * Description:
 *   Blit the 8-bit depth buffer that DOOM renders to onto the frame buffer,
 *   converting to the pixel format of the frame buffer and scaling up by an
 *   integer factor.
 *
 ****************************************************************************/

static void blit_screen(void)
{
  FAR uint8_t *fbbase = (FAR uint8_t *)g_graphics_state.fbmem +
                        g_graphics_state.origin;
  FAR const uint16_t *colmap = g_graphics_state.colmap;
  unsigned stride = g_graphics_state.pinfo.stride;
  unsigned pixlen = g_graphics_state.pinfo.bpp >> 3;
  unsigned outw = g_graphics_state.outw;
  unsigned outh = g_graphics_state.outh;
  unsigned rowbytes = outw * pixlen;
#ifdef CONFIG_GAMES_NXDOOM_ROWSTAGE
  bool staged = rowbytes <= sizeof(g_rowbuf);
#else
  const bool staged = false;
#endif
  FAR uint8_t *prevrow = NULL;
  unsigned prevsy = SCREENHEIGHT;
  unsigned x;
  unsigned oy;
  unsigned sy;

  for (oy = 0; oy < outh; oy++)
    {
      FAR uint8_t *dst = fbbase + oy * stride;
#ifdef CONFIG_GAMES_NXDOOM_ROWSTAGE
      FAR uint8_t *out = staged ? (FAR uint8_t *)g_rowbuf : dst;
#else
      FAR uint8_t *out = dst;
#endif
      FAR const pixel_t *src;
      FAR uint8_t *dest8 = out;
      FAR uint16_t *dest16 = (FAR uint16_t *)out;
      FAR uint32_t *dest32 = (FAR uint32_t *)out;
      uint32_t pixel;

      /* Which source row this output row comes from.  Several output rows
       * map to the same source row whenever the image is scaled up.
       */

      sy = oy * SCREENHEIGHT / outh;
      if (sy == prevsy)
        {
          /* Identical to the row just built, so copy it rather than
           * converting the same pixels again.
           */

          memcpy(dst, prevrow, rowbytes);
          continue;
        }

      /* Expand one source row.  The column map turns this into a lookup per
       * output pixel, with no division and no special case for a scale
       * factor that is not a whole number.
       */

      src = &g_graphics_state.scrnbuf[sy * SCREENWIDTH];

      switch (g_graphics_state.pinfo.bpp)
        {
          case 8:
            for (x = 0; x < outw; x++)
              {
                *dest8++ = g_palette_native[src[colmap[x]]];
              }
            break;

          case 16:
            for (x = 0; x < outw; x++)
              {
                *dest16++ = g_palette_native[src[colmap[x]]];
              }
            break;

          case 24:
            for (x = 0; x < outw; x++)
              {
                pixel = g_palette_native[src[colmap[x]]];

                *dest8++ = pixel;
                *dest8++ = pixel >> 8;
                *dest8++ = pixel >> 16;
              }
            break;

          default:
            for (x = 0; x < outw; x++)
              {
                *dest32++ = g_palette_native[src[colmap[x]]];
              }
            break;
        }

      if (staged)
        {
          memcpy(dst, out, rowbytes);
        }

      prevsy = sy;
      prevrow = out;
    }
}

static void update_grab(void)
{
}

static void set_video_mode(void)
{
}

static void i_get_event(void)
{
  int err;
#if CONFIG_GAMES_NXDOOM_KEYBOARD
  struct keyboard_event_s kbdevent;

  while ((err = get_kbd_event(&kbdevent)) == 0)
    {
      /* All four event types are handled:  dropping the special ones here
       * would silently lose the arrow keys.
       */

      i_handle_keyboard_event(&kbdevent);
    }
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void i_set_grab_mouse_callback(grabmouse_callback_t func)
{
  grabmouse_callback = func;
}

/* Set the variable controlling FPS dots. */

void i_display_fps_dots(boolean dots_on)
{
  display_fps_dots = dots_on;
}

void i_shutdown_graphics(void)
{
  if (!g_graphics_state.inited)
    {
      return;
    }

  close(g_graphics_state.fd);
  munmap(g_graphics_state.fbmem, g_graphics_state.pinfo.fblen);
#ifndef CONFIG_GAMES_NXDOOM_STATIC_SCRNBUF
  free(g_graphics_state.scrnbuf);
#endif
  free(g_graphics_state.colmap);
  g_graphics_state.inited = false;
}

void i_start_frame(void)
{
  /* er? */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void i_start_tic(void)
{
  if (!g_graphics_state.inited)
    {
      return;
    }

  i_get_event();

  if (usemouse && !nomouse && window_focused)
    {
      i_read_mouse();
    }

  if (joywait < i_get_time())
    {
      i_update_joystick();
    }
}

void i_update_no_blit(void)
{
  /* what is this? */
}

void i_finish_update(void)
{
  static int lasttic;
  int tics;
  int i;

  if (!g_graphics_state.inited) return;

  if (noblit) return;

  /* draws little dots on the bottom of the screen */

  if (display_fps_dots)
    {
      i = i_get_time();
      tics = i - lasttic;
      lasttic = i;
      if (tics > 20) tics = 20;

      for (i = 0; i < tics * 4; i += 4)
        i_video_buffer[(SCREENHEIGHT - 1) * SCREENWIDTH + i] = 0xff;
      for (; i < 20 * 4; i += 4)
        i_video_buffer[(SCREENHEIGHT - 1) * SCREENWIDTH + i] = 0x0;
    }

  /* Draw disk icon before blit, if necessary. */

  v_draw_disk_icon();

  if (palette_to_set)
    {
      palette_to_set = false;
      update_native_palette();
    }

  blit_screen();

  /* Draw! */

  /* Restore background and undo the disk indicator, if it was drawn. */

  v_restore_disk_background();
}

void i_read_screen(pixel_t *scr)
{
  memcpy(scr, i_video_buffer, SCREENWIDTH * SCREENHEIGHT * sizeof(*scr));
}

/****************************************************************************
 * Name: i_set_palette
 ****************************************************************************/

void i_set_palette(byte *doompalette)
{
  for (int i = 0; i < 256; ++i)
    {
      /* Zero out the bottom two bits of each channel - the PC VGA
       * controller only supports 6 bits of accuracy.
       */

      g_palette[i].a = 0xffu;
      g_palette[i].r = gammatable[usegamma][*doompalette++] & ~3;
      g_palette[i].g = gammatable[usegamma][*doompalette++] & ~3;
      g_palette[i].b = gammatable[usegamma][*doompalette++] & ~3;
    }

  palette_to_set = true;
}

/****************************************************************************
 * Name: i_get_palette_index
 *
 * Description:
 *  Given an RGB value, find the closest matching palette index.
 *
 * Return:
 *   An index into the palette lookup table for the best match.
 *
 ****************************************************************************/

int i_get_palette_index(int r, int g, int b)
{
  int best = 0;
  int best_diff = INT_MAX;
  int diff;

  for (int i = 0; i < 256; ++i)
    {
      diff = (r - g_palette[i].r) * (r - g_palette[i].r) +
             (g - g_palette[i].g) * (g - g_palette[i].g) +
             (b - g_palette[i].b) * (b - g_palette[i].b);

      if (diff < best_diff)
        {
          best = i;
          best_diff = diff;
        }

      if (diff == 0)
        {
          break;
        }
    }

  return best;
}

/****************************************************************************
 * Name: i_set_window_title
 *
 * Description:
 *  Set the window title internally.
 *
 ****************************************************************************/

void i_set_window_title(const char *title)
{
  g_window_title = title;
}

/****************************************************************************
 * Name: i_init_window_title
 *
 * Description:
 *   Actually cause the window title to update with whatever window title was
 *   last set via i_set_window_title.
 *
 ****************************************************************************/

void i_init_window_title(void)
{
}

/****************************************************************************
 * Name: i_set_window_title
 ****************************************************************************/

void i_graphics_check_commandline(void)
{
  int i;

  /* @category video
   * @vanilla
   *
   * Disable blitting the screen.
   */

  noblit = m_check_parm("-noblit");

  /* @category video
   *
   * Don't grab the mouse when running in windowed mode.
   */

  nograbmouse_override = m_parm_exists("-nograbmouse");

  /* default to fullscreen mode, allow override with command line
   * nofullscreen because we love prboom
   */

  /* @category video
   *
   * Run in a window.
   */

  if (m_check_parm("-window") || m_check_parm("-nofullscreen"))
    {
      fullscreen = false;
    }

  /* @category video
   *
   * Run in fullscreen mode.
   */

  if (m_check_parm("-fullscreen"))
    {
      fullscreen = true;
    }

  /* @category video
   *
   * Disable the mouse.
   */

  nomouse = m_check_parm("-nomouse") > 0;

  /* @category video
   * @arg <W>
   *
   * Specify the screen width, in pixels.  Implies -window.
   */

  i = m_check_parm_with_args("-width", 1);

  if (i > 0)
    {
      window_width = atoi(myargv[i + 1]);
      fullscreen = false;
    }

  /* @category video
   * @arg <H>
   *
   * Specify the screen height, in pixels.  Implies -window.
   */

  i = m_check_parm_with_args("-height", 1);

  if (i > 0)
    {
      window_height = atoi(myargv[i + 1]);
      fullscreen = false;
    }

  /* @category video
   * @arg <WxH>
   *
   * Specify the dimensions of the window.  Implies -window.
   */

  i = m_check_parm_with_args("-geometry", 1);

  if (i > 0)
    {
      int w;
      int h;
      int s;

      s = sscanf(myargv[i + 1], "%ix%i", &w, &h);
      if (s == 2)
        {
          window_width = w;
          window_height = h;
          fullscreen = false;
        }
    }

  /* @category video
   * @arg <x>
   *
   * Specify the display number on which to show the screen.
   */

  i = m_check_parm_with_args("-display", 1);

  if (i > 0)
    {
      int display = atoi(myargv[i + 1]);
      if (display >= 0)
        {
          video_display = display;
        }
    }
}

/* Check if we have been invoked as a screensaver by xscreensaver. */

void i_check_is_screensaver(void)
{
  char *env;

  env = getenv("XSCREENSAVER_WINDOW");

  if (env != NULL)
    {
      screensaver_mode = true;
    }
}

/* Check the display bounds of the display referred to by 'video_display' and
 * set x and y to a location that places the window in the center of that
 * display.
 */

static void center_window(int *x, int *y, int w, int h)
{
  *x = MAX((g_graphics_state.vinfo.xres - w) / 2, 0);
  *y = MAX((g_graphics_state.vinfo.yres - h) / 2, 0);
}

void i_get_window_position(int *x, int *y, int w, int h)
{
  /* in fullscreen mode, the window "position" still matters, because
   * we use it to control which display we run fullscreen on.
   */

  if (fullscreen)
    {
      center_window(x, y, w, h);
      return;
    }
}

void i_init_graphics(void)
{
  uint8_t xscale;
  uint8_t yscale;
  unsigned i;
  int err;
  byte *doompal;

  /* Open frame buffer */

  g_graphics_state.fd = open(CONFIG_GAMES_NXDOOM_FBPATH, O_RDWR | O_CLOEXEC);
  if (g_graphics_state.fd < 0)
    {
      i_error("Failed to open frame buffer: %d", errno);
    }

  /* Get frame buffer characteristics */

  err = ioctl(g_graphics_state.fd, FBIOGET_VIDEOINFO,
              (unsigned long)(uintptr_t)&g_graphics_state.vinfo);
  if (err < 0)
    {
      close(g_graphics_state.fd);
      i_error("Failed to get video info: %d", errno);
    }

  /* Here, we check the dimensions of the frame buffer. If we have enough
   * space to scale up the rendered image in both width and height, record
   * that so we can make use of it elsewhere.
   *
   * If we don't have enough frame buffer space for the game, quit!
   */

  if (g_graphics_state.vinfo.xres < SCREENWIDTH)
    {
      i_error("Resolution width of %u px < minimum of %u px\n",
              g_graphics_state.vinfo.xres, SCREENWIDTH);
    }

  if (g_graphics_state.vinfo.yres < SCREENHEIGHT)
    {
      i_error("Resolution height of %u px < minimum of %u px\n",
              g_graphics_state.vinfo.yres, SCREENHEIGHT);
    }

  xscale = g_graphics_state.vinfo.xres / SCREENWIDTH;
  yscale = g_graphics_state.vinfo.yres / SCREENHEIGHT;
  g_graphics_state.scale = xscale > yscale ? yscale : xscale;

#ifdef CONFIG_GAMES_NXDOOM_FILLSCREEN
  /* Stretch to the whole display.  The scale factor is then generally not an
   * integer, which the column map below takes care of.
   */

  g_graphics_state.outw = g_graphics_state.vinfo.xres;
  g_graphics_state.outh = g_graphics_state.vinfo.yres;
#else
  g_graphics_state.outw = SCREENWIDTH * g_graphics_state.scale;
  g_graphics_state.outh = SCREENHEIGHT * g_graphics_state.scale;
#endif

  /* Centre the scaled image in the frame buffer */

  g_graphics_state.origin =
      (g_graphics_state.vinfo.yres - g_graphics_state.outh) / 2 *
      g_graphics_state.pinfo.stride +
      (g_graphics_state.vinfo.xres - g_graphics_state.outw) / 2 *
      (g_graphics_state.pinfo.bpp >> 3);

  /* Build the output column to source column map once */

  g_graphics_state.colmap = malloc(g_graphics_state.outw * sizeof(uint16_t));
  if (g_graphics_state.colmap == NULL)
    {
      i_error("Couldn't allocate column map: %d\n", errno);
    }

  for (i = 0; i < g_graphics_state.outw; i++)
    {
      g_graphics_state.colmap[i] = i * SCREENWIDTH / g_graphics_state.outw;
    }

  /* Get frame buffer plane info */

  if (ioctl(g_graphics_state.fd, FBIOGET_PLANEINFO,
            (unsigned long)((uintptr_t)&g_graphics_state.pinfo)) < 0)
    {
      i_error("ioctl(FBIOGET_PLANEINFO) failed: %d\n", errno);
    }

  /* Initialize frame buffer memory for actual rendering */

  g_graphics_state.fbmem =
      mmap(NULL, g_graphics_state.pinfo.fblen, PROT_READ | PROT_WRITE,
           MAP_SHARED | MAP_FILE, g_graphics_state.fd, 0);
  if (g_graphics_state.fbmem == MAP_FAILED)
    {
      i_error("mmap() of frame buffer failed: %d\n", errno);
    }

  /* Create an 8-bit depth screen buffer for DOOM to render to */

#ifdef CONFIG_GAMES_NXDOOM_STATIC_SCRNBUF
  g_graphics_state.scrnbuf = g_scrnbuf;
#else
  g_graphics_state.scrnbuf = malloc(SCREENWIDTH * SCREENHEIGHT);
#endif
  if (g_graphics_state.scrnbuf == NULL)
    {
      i_error("Couldn't allocate screen buffer: %d\n", errno);
    }

  /* Create the game window; this may switch graphic modes depending
   * on configuration.
   * AdjustWindowSize();
   */

  set_video_mode();

  /* Start with a clear black screen
   * (screen will be flipped after we set the palette)
   */

  memset(g_graphics_state.scrnbuf, 0, SCREENHEIGHT * SCREENWIDTH);

  /* Set the palette */

  doompal = w_cache_lump_name(("PLAYPAL"), PU_CACHE);
  i_set_palette(doompal);

  update_grab();

  /* On some systems, it takes a second or so for the screen to settle
   * after changing modes.  We include the option to add a delay when
   * setting the screen mode, so that the game doesn't start immediately
   * with the player unable to see anything.
   */

  if (fullscreen && !screensaver_mode)
    {
      usleep(startup_delay * 1000);
    }

  /* The actual 320x200 canvas that we draw to. This is the pixel buffer of
   * the 8-bit paletted screen buffer that gets blit on an intermediate
   * 32-bit RGBA screen buffer that gets loaded into a texture that gets
   * finally rendered into our window or full screen in i_finish_update().
   */

  i_video_buffer = g_graphics_state.scrnbuf;
  v_restore_buffer();

  /* Clear the screen to black. */

  memset(i_video_buffer, 0,
         SCREENWIDTH * SCREENHEIGHT * sizeof(*i_video_buffer));

  /* clear out any events waiting at the start and center the mouse */

  g_graphics_state.inited = true;

  /* Call i_shutdown_graphics on quit */

  i_at_exit(i_shutdown_graphics, true);
}

/* Bind all variables controlling video options into the configuration
 * file system.
 */

void i_bind_video_variables(void)
{
  m_bind_int_variable("use_mouse", &usemouse);
  m_bind_int_variable("fullscreen", &fullscreen);
  m_bind_int_variable("video_display", &video_display);
  m_bind_int_variable("integer_scaling", &integer_scaling);
  m_bind_int_variable("smooth_pixel_scaling", &smooth_pixel_scaling);
  m_bind_int_variable("vga_porch_flash", &vga_porch_flash);
  m_bind_int_variable("startup_delay", &startup_delay);
  m_bind_int_variable("fullscreen_width", &fullscreen_width);
  m_bind_int_variable("fullscreen_height", &fullscreen_height);
  m_bind_int_variable("force_software_renderer", &force_software_renderer);
  m_bind_int_variable("max_scaling_buffer_pixels",
                      &max_scaling_buffer_pixels);
  m_bind_int_variable("window_width", &window_width);
  m_bind_int_variable("window_height", &window_height);
  m_bind_int_variable("grabmouse", &grabmouse);
  m_bind_string_variable("video_driver", &video_driver);
  m_bind_string_variable("window_position", &window_position);
  m_bind_int_variable("usegamma", &usegamma);
  m_bind_int_variable("png_screenshots", &png_screenshots);
}
