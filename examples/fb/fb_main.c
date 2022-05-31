/****************************************************************************
 * apps/examples/fb/fb_main.c
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define NCOLORS 6

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fb_state_s
{
  int fd;
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
#ifdef CONFIG_FB_OVERLAY
  struct fb_overlayinfo_s oinfo;
#endif
  FAR void *fbmem;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define SSD1306_DEF_FONT_SIZE 5
static const uint8_t SSD1306_font[][SSD1306_DEF_FONT_SIZE]=
{
    {0x00, 0x00, 0x00, 0x00, 0x00},   // space
    {0x00, 0x00, 0x2f, 0x00, 0x00},   // !
    {0x00, 0x07, 0x00, 0x07, 0x00},   // "
    {0x14, 0x7f, 0x14, 0x7f, 0x14},   // #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12},   // $
    {0x23, 0x13, 0x08, 0x64, 0x62},   // %
    {0x36, 0x49, 0x55, 0x22, 0x50},   // &
    {0x00, 0x05, 0x03, 0x00, 0x00},   // '
    {0x00, 0x1c, 0x22, 0x41, 0x00},   // (
    {0x00, 0x41, 0x22, 0x1c, 0x00},   // )
    {0x14, 0x08, 0x3E, 0x08, 0x14},   // *
    {0x08, 0x08, 0x3E, 0x08, 0x08},   // +
    {0x00, 0x00, 0xA0, 0x60, 0x00},   // ,
    {0x08, 0x08, 0x08, 0x08, 0x08},   // -
    {0x00, 0x60, 0x60, 0x00, 0x00},   // .
    {0x20, 0x10, 0x08, 0x04, 0x02},   // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E},   // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00},   // 1
    {0x42, 0x61, 0x51, 0x49, 0x46},   // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31},   // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10},   // 4
    {0x27, 0x45, 0x45, 0x45, 0x39},   // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30},   // 6
    {0x01, 0x71, 0x09, 0x05, 0x03},   // 7
    {0x36, 0x49, 0x49, 0x49, 0x36},   // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E},   // 9
    {0x00, 0x36, 0x36, 0x00, 0x00},   // :
    {0x00, 0x56, 0x36, 0x00, 0x00},   // ;
    {0x08, 0x14, 0x22, 0x41, 0x00},   // <
    {0x14, 0x14, 0x14, 0x14, 0x14},   // =
    {0x00, 0x41, 0x22, 0x14, 0x08},   // >
    {0x02, 0x01, 0x51, 0x09, 0x06},   // ?
    {0x32, 0x49, 0x59, 0x51, 0x3E},   // @
    {0x7C, 0x12, 0x11, 0x12, 0x7C},   // A
    {0x7F, 0x49, 0x49, 0x49, 0x36},   // B
    {0x3E, 0x41, 0x41, 0x41, 0x22},   // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C},   // D
    {0x7F, 0x49, 0x49, 0x49, 0x41},   // E
    {0x7F, 0x09, 0x09, 0x09, 0x01},   // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A},   // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F},   // H
    {0x00, 0x41, 0x7F, 0x41, 0x00},   // I
    {0x20, 0x40, 0x41, 0x3F, 0x01},   // J
    {0x7F, 0x08, 0x14, 0x22, 0x41},   // K
    {0x7F, 0x40, 0x40, 0x40, 0x40},   // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},   // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F},   // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E},   // O
    {0x7F, 0x09, 0x09, 0x09, 0x06},   // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E},   // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46},   // R
    {0x46, 0x49, 0x49, 0x49, 0x31},   // S
    {0x01, 0x01, 0x7F, 0x01, 0x01},   // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F},   // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F},   // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F},   // W
    {0x63, 0x14, 0x08, 0x14, 0x63},   // X
    {0x07, 0x08, 0x70, 0x08, 0x07},   // Y
    {0x61, 0x51, 0x49, 0x45, 0x43},   // Z
    {0x00, 0x7F, 0x41, 0x41, 0x00},   // [
    {0x55, 0xAA, 0x55, 0xAA, 0x55},   // Backslash (Checker pattern)
    {0x00, 0x41, 0x41, 0x7F, 0x00},   // ]
    {0x04, 0x02, 0x01, 0x02, 0x04},   // ^
    {0x40, 0x40, 0x40, 0x40, 0x40},   // _
    {0x00, 0x03, 0x05, 0x00, 0x00},   // `
    {0x20, 0x54, 0x54, 0x54, 0x78},   // a
    {0x7F, 0x48, 0x44, 0x44, 0x38},   // b
    {0x38, 0x44, 0x44, 0x44, 0x20},   // c
    {0x38, 0x44, 0x44, 0x48, 0x7F},   // d
    {0x38, 0x54, 0x54, 0x54, 0x18},   // e
    {0x08, 0x7E, 0x09, 0x01, 0x02},   // f
    {0x18, 0xA4, 0xA4, 0xA4, 0x7C},   // g
    {0x7F, 0x08, 0x04, 0x04, 0x78},   // h
    {0x00, 0x44, 0x7D, 0x40, 0x00},   // i
    {0x40, 0x80, 0x84, 0x7D, 0x00},   // j
    {0x7F, 0x10, 0x28, 0x44, 0x00},   // k
    {0x00, 0x41, 0x7F, 0x40, 0x00},   // l
    {0x7C, 0x04, 0x18, 0x04, 0x78},   // m
    {0x7C, 0x08, 0x04, 0x04, 0x78},   // n
    {0x38, 0x44, 0x44, 0x44, 0x38},   // o
    {0xFC, 0x24, 0x24, 0x24, 0x18},   // p
    {0x18, 0x24, 0x24, 0x18, 0xFC},   // q
    {0x7C, 0x08, 0x04, 0x04, 0x08},   // r
    {0x48, 0x54, 0x54, 0x54, 0x20},   // s
    {0x04, 0x3F, 0x44, 0x40, 0x20},   // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C},   // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C},   // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C},   // w
    {0x44, 0x28, 0x10, 0x28, 0x44},   // x
    {0x1C, 0xA0, 0xA0, 0xA0, 0x7C},   // y
    {0x44, 0x64, 0x54, 0x4C, 0x44},   // z
    {0x00, 0x10, 0x7C, 0x82, 0x00},   // {
    {0x00, 0x00, 0xFF, 0x00, 0x00},   // |
    {0x00, 0x82, 0x7C, 0x10, 0x00},   // }
    {0x00, 0x06, 0x09, 0x09, 0x06}    // ~ (Degrees)
};


static const char g_default_fbdev[] = CONFIG_EXAMPLES_FB_DEFAULTFB;

/* Violet-Blue-Green-Yellow-Orange-Red */

static const uint32_t g_rgb24[NCOLORS] =
{
  RGB24_VIOLET, RGB24_BLUE, RGB24_GREEN,
  RGB24_YELLOW, RGB24_ORANGE, RGB24_RED
};

static const uint16_t g_rgb16[NCOLORS] =
{
  RGB16_VIOLET, RGB16_BLUE, RGB16_GREEN,
  RGB16_YELLOW, RGB16_ORANGE, RGB16_RED
};

static const uint8_t g_rgb8[NCOLORS] =
{
  RGB8_VIOLET, RGB8_BLUE, RGB8_GREEN,
  RGB8_YELLOW, RGB8_ORANGE, RGB8_RED
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * draw_rect
 ****************************************************************************/


static void draw_bit_column1(FAR struct fb_state_s *state,
		uint8_t col_val, int x, int y, int flags)
{
  int bit_shift = x % 8;
  uint8_t mask = 0x80 >> bit_shift;

  int line;

  uint8_t *pixels;

  for (line = 7; line >= 0; line--)
  {
	  pixels = state->fbmem +
			  (y + line) * state->pinfo.stride + (x >> 3);
	  *pixels = (*pixels & ~mask);
	  if (col_val & 0x80) //TODO add flag reverse
	  {
		  *pixels |= mask;
	  }
	  col_val = (col_val << 1);
  }
}

static void draw_bit_row1(FAR struct fb_state_s *state,
		uint8_t col_val, int x, int y, int flags)
{
  int bit_shift;
  uint8_t mask;

  int line;

  uint8_t *pixels;

  for (line = 7; line >= 0; line--)
  {
	  int bit_shift = (x + line) % 8;
	  uint8_t mask = 0x80 >> bit_shift;
	  pixels = state->fbmem +
			  y * state->pinfo.stride + ((x + line)>> 3);
	  *pixels = (*pixels & ~mask);
	  if (col_val & 0x80) //TODO add flag reverse color
	  {
		  *pixels |= mask;
	  }
	  col_val = (col_val << 1);
  }
}

static void write_text(FAR struct fb_state_s *state, int x_start, int x_end,
		int y_start, int y_end, int flags, FAR const char *text)
{
  FAR const char *letter = text;

  if (x_start + 5 > state->vinfo.xres)
  {
	  printf("Wrong argument x_start = %d, schould be less then %d\n",
			  x_start, state->vinfo.xres-5);
	  return;
  }

  if (y_start + 8 > state->vinfo.yres)
  {
	  printf("Wrong argument y_start = %d, schould be less then %d\n",
			  y_start, state->vinfo.yres-8);
	  return;
  }

  if (x_end >= state->vinfo.xres-1)
    {
	  x_end = state->vinfo.xres-1;
    }

  if (y_end >= state->vinfo.yres-1)
    {
	  y_end = state->vinfo.yres-1;
    }

  if (x_end - x_start < 5)
    {
	  printf("too short row\n");
	  return;
    }

  if (y_end - y_start < 8)
    {
	  printf("too short column\n");
	  return;
    }


  int x = flags & 0x01 ? x_end-6 : x_start;
  int y = y_start;
  int col;
  uint8_t val;
  while (*letter != '\0')
  {
	  if (flags & 0x01)
        {
          if (y + 5 > y_end)
            {
              y = y_start;
              x-= 9;
            }
		  if (x < x_start)
			  break;
        }
	  else
        {
          if (x + 5 > x_end)
            {
              x = x_start;
              y+= 9;
            }
          if (y + 8 > y_end)
            break;
        }

	  int font_idx = (uint8_t)((*letter - 32) & 0xff);
	  printf("%c (idx %3d): Bits: 0x", *letter, font_idx);
	  for (col = 0; col < 5; col++)
        {
          val = SSD1306_font[font_idx][col];
          printf("%02x", val);
          if (flags & 0x01)
            {
              draw_bit_row1(state, val, x, y+col, flags);
            }
          else
            {
              draw_bit_column1(state, val, x+col, y, flags);
            }
	  }
	  printf("\n");

	  //Write letter separator
	  if (flags & 0x01)
        {
		  if (y+5 < y_end)
		  {
			  draw_bit_column1(state, 0, x+5, y, 0);
		  }
          y += 6;
        }
	  else
        {
		  if (x+5 < x_end)
		  {
			  draw_bit_column1(state, 0, x+5, y, 0);
		  }
	      x+= 6;
        }
	  letter++;
  }
}


static void draw_rect32(FAR struct fb_state_s *state,
                        FAR struct fb_area_s *area, int color)
{
  FAR uint32_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint32_t *)row) + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb24[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect16(FAR struct fb_state_s *state,
                        FAR struct fb_area_s *area, int color)
{
  FAR uint16_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint16_t *)row) + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb16[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect8(FAR struct fb_state_s *state,
                       FAR struct fb_area_s *area, int color)
{
  FAR uint8_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = row + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb8[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect1(FAR struct fb_state_s *state,
                       FAR struct fb_area_s *area, int color)
{
  FAR uint8_t *pixel;
  FAR uint8_t *row;
  uint8_t color8 = (color & 1) == 0 ? 0 : 0xff;

  int start_full_x;
  int end_full_x;
  int start_bit_shift;
  int last_bits;
  uint8_t lmask;
  uint8_t rmask;
  int y;

  /* Calculate the framebuffer address of the first row to draw on */

  row    = (FAR uint8_t *)state->fbmem + state->pinfo.stride * area->y;

  /* Calculate the position of the first complete (with all bits) byte.
   * Then calculate the last byte with all the bits.
   */

  start_full_x = ((area->x + 7) >> 3);
  end_full_x   = ((area->x + area->w) >> 3);

  /* Calculate the number of bits in byte before start that need to remain
   * unchanged. Later calculate the mask.
   */

  start_bit_shift = 8 + area->x - (start_full_x << 3);
  lmask = 0xff >> start_bit_shift;

  /* Calculate the number of bits that needs to be changed after last byte
   * with all the bits. Later calculate the mask.
   */

  last_bits = area->x + area->w - (end_full_x << 3);
  rmask = 0xff << (8 - last_bits);

  /* Calculate a mask on the first and last bytes of the sequence that may
   * not be completely filled with pixel.
   */

  /* Now draw each row, one-at-a-time */

  for (y = 0; y < area->h; y++)
    {
      /* 'pixel' points to the 1st pixel the next row */

      /* Special case: The row starts and ends within the same byte */

      if (start_full_x > end_full_x)
        {
          pixel = row + start_full_x - 1;
          *pixel = (*pixel & (~lmask | ~rmask)) | (lmask & rmask & color8);
          continue;
        }

      if (start_bit_shift != 0)
        {
          pixel = row + start_full_x - 1;
          *pixel = (*pixel & ~lmask) | (lmask & color8);
        }

      if (end_full_x > start_full_x)
        {
          pixel = row + start_full_x;
          memset(pixel, color8, end_full_x - start_full_x);
        }

      if (last_bits != 0)
        {
          pixel = row + end_full_x;
          *pixel = (*pixel & ~rmask) | (rmask & color8);
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect(FAR struct fb_state_s *state,
                      FAR struct fb_area_s *area, int color)
{
#ifdef CONFIG_FB_UPDATE
  int ret;
#endif

  switch (state->pinfo.bpp)
    {
      case 32:
        draw_rect32(state, area, color);
        break;

      case 16:
        draw_rect16(state, area, color);
        break;

      case 8:
      default:
        draw_rect8(state, area, color);
        break;

      case 1:
        draw_rect1(state, area, color);
        break;
    }

#ifdef CONFIG_FB_UPDATE
  area->h = state->vinfo.yres - area->y;
  ret = ioctl(state->fd, FBIO_UPDATE,
              (unsigned long)((uintptr_t)area));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_UPDATE) failed: %d\n",
              errcode);
    }
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * fb_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *fbdev = g_default_fbdev;
  struct fb_state_s state;
  struct fb_area_s area;
  int nsteps;
  int xstep;
  int ystep;
  int width;
  int height;
  int color;
  int x;
  int y;
  int ret;

  /* There is a single required argument:  The path to the framebuffer
   * driver.
   */

  if (argc == 2)
    {
      //fbdev = argv[1];
    }
  else if (argc != 1)
    {
      fprintf(stderr, "ERROR: Single argument required\n");
      fprintf(stderr, "USAGE: %s [<fb-driver-path>]\n", argv[0]);
      return EXIT_FAILURE;
    }

  /* Open the framebuffer driver */

  state.fd = open(fbdev, O_RDWR);
  if (state.fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", fbdev, errcode);
      return EXIT_FAILURE;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(state.fd, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&state.vinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_VIDEOINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("VideoInfo:\n");
  printf("      fmt: %u\n", state.vinfo.fmt);
  printf("     xres: %u\n", state.vinfo.xres);
  printf("     yres: %u\n", state.vinfo.yres);
  printf("  nplanes: %u\n", state.vinfo.nplanes);

#ifdef CONFIG_FB_OVERLAY
  printf("noverlays: %u\n", state.vinfo.noverlays);

  /* Select the first overlay, which should be the composed framebuffer */

  ret = ioctl(state.fd, FBIO_SELECT_OVERLAY, 0);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_SELECT_OVERLAY) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  /* Get the first overlay information */

  state.oinfo.overlay = 0;
  ret = ioctl(state.fd, FBIOGET_OVERLAYINFO,
                        (unsigned long)((uintptr_t)&state.oinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_OVERLAYINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("OverlayInfo (overlay 0):\n");
  printf("    fbmem: %p\n", state.oinfo.fbmem);
  printf("    fblen: %lu\n", (unsigned long)state.oinfo.fblen);
  printf("   stride: %u\n", state.oinfo.stride);
  printf("  overlay: %u\n", state.oinfo.overlay);
  printf("      bpp: %u\n", state.oinfo.bpp);
  printf("    blank: %u\n", state.oinfo.blank);
  printf("chromakey: 0x%08" PRIx32 "\n", state.oinfo.chromakey);
  printf("    color: 0x%08" PRIx32 "\n", state.oinfo.color);
  printf("   transp: 0x%02x\n", state.oinfo.transp.transp);
  printf("     mode: %u\n", state.oinfo.transp.transp_mode);
  printf("     area: (%u,%u) => (%u,%u)\n",
                      state.oinfo.sarea.x, state.oinfo.sarea.y,
                      state.oinfo.sarea.w, state.oinfo.sarea.h);
  printf("     accl: %" PRIu32 "\n", state.oinfo.accl);

#endif

  ret = ioctl(state.fd, FBIOGET_PLANEINFO,
              (unsigned long)((uintptr_t)&state.pinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("PlaneInfo (plane 0):\n");
  printf("    fbmem: %p\n", state.pinfo.fbmem);
  printf("    fblen: %lu\n", (unsigned long)state.pinfo.fblen);
  printf("   stride: %u\n", state.pinfo.stride);
  printf("  display: %u\n", state.pinfo.display);
  printf("      bpp: %u\n", state.pinfo.bpp);

  /* Only these pixel depths are supported.  viinfo.fmt is ignored, only
   * certain color formats are supported.
   */

  if (state.pinfo.bpp != 32 && state.pinfo.bpp != 16 &&
      state.pinfo.bpp != 8  && state.pinfo.bpp != 1)
    {
      fprintf(stderr, "ERROR: bpp=%u not supported\n", state.pinfo.bpp);
      close(state.fd);
      return EXIT_FAILURE;
    }

  /* mmap() the framebuffer.
   *
   * NOTE: In the FLAT build the frame buffer address returned by the
   * FBIOGET_PLANEINFO IOCTL command will be the same as the framebuffer
   * address.  mmap(), however, is the preferred way to get the framebuffer
   * address because in the KERNEL build, it will perform the necessary
   * address mapping to make the memory accessible to the application.
   */

  state.fbmem = mmap(NULL, state.pinfo.fblen, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_FILE, state.fd, 0);
  if (state.fbmem == MAP_FAILED)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("Mapped FB: %p\n", state.fbmem);

  /* Draw some rectangles */

  nsteps = 2 * (NCOLORS - 1) + 1;
  xstep  = state.vinfo.xres / nsteps;
  ystep  = state.vinfo.yres / nsteps;
  width  = state.vinfo.xres;
  height = state.vinfo.yres;

  for (x = 0, y = 0, color = 0;
       color < NCOLORS;
       x += xstep, y += ystep, color++)
    {
      area.x = x;
      area.y = y;
      area.w = width;
      area.h = height;

      printf("%2d: (%3d,%3d) (%3d,%3d)\n",
             color, area.x, area.y, area.w, area.h);

      draw_rect(&state, &area, color);
      usleep(500 * 1000);

      width  -= (2 * xstep);
      height -= (2 * ystep);

      uint8_t *row = state.fbmem + 148 * state.pinfo.stride;
      printf("Memory line 148: "
    		  "0x%02x%02x%02x%02x %02x%02x%02x%02x "
    		  "%02x%02x%02x%02x %02x%02x%02x%02x\n",
    		  row[0],  row[1],  row[2],  row[3],
			  row[4],  row[5],  row[6],  row[7],
    		  row[8],  row[9],  row[10], row[11],
			  row[12], row[13], row[14], row[15]
      );
    }

  write_text(&state, 0, 127, 0, 296, 0, "Hello world !!!");

  write_text(&state, 0, 127, 10, 296, 0, "Hello world !!!");

  if (argc == 2)
    {
	  write_text(&state, 0, 127, 30, 296, 0, argv[1]);
    }

  area.h = state.vinfo.yres;
  area.y = 0;
  area.w = 128;
  area.x = 0;
  ioctl(state.fd, FBIO_UPDATE,
              (unsigned long)((uintptr_t)&area));



/*
  area.w = 20;
  for (x = 0; x<= 20; x += 6)
  {
	  area.h = 10;
	  area.x = x;
	  area.y = x;
	  draw_rect(&state, &area, 1);
  }

  for (x = 3; x<= 20; x += 6)
  {
	  area.h = 10;
	  area.x = x;
	  area.y = x;
	  draw_rect(&state, &area, 1);
  }
*/

  munmap(state.fbmem, state.pinfo.fblen);
  close(state.fd);
  return EXIT_SUCCESS;
}
