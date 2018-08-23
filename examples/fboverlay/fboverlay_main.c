/****************************************************************************
 * examples/fboverlay/fboverlay_main.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Marco Krahl <ocram.lhark@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <nuttx/video/fb.h>

#include <nuttx/config.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rgb8888
 *
 * Description:
 *   Color conversion from argb8888 to rgb color channel
 *
 * Parameters:
 *   argb - argb8888 color
 *   a    - Reference ot 8-bit alpha channel
 *   r    - Reference ot 8-bit red channel
 *   g    - Reference ot 8-bit green channel
 *   b    - Reference ot 8-bit blue channel
 *
 ****************************************************************************/

#ifdef CONFIG_FB_CMAP
static void rgb8888(uint32_t argb, uint8_t * a, uint8_t * r, uint8_t * g,
                    uint8_t * b)
{
  *a = (uint8_t)(argb >> 24);
  *r = (uint8_t)(argb >> 16);
  *g = (uint8_t)(argb >> 8);
  *b = (uint8_t)argb;
}
#endif

/****************************************************************************
 * Name: rgb565
 *
 * Description:
 *   Color conversion from argb8888 to rgb565
 *
 * Parameters:
 *   argb - argb8888 color
 *
 ****************************************************************************/

#ifdef CONFIG_FB_FMT_RGB16_565
static uint16_t rgb565(uint32_t argb)
{
  uint16_t r = (argb >> 8) & 0xf800;
  uint16_t g = (argb >> 5) & 0x7e0;
  uint16_t b = (argb >> 3) & 0x1f;

  return (r | g | b);
}
#endif

/****************************************************************************
 * Name: draw_rect8
 *
 * Description:
 *   Draw a rectangle for 8 bit color mode
 *
 * Parameters:
 *   fbmem - Points to framebuffer start address
 *   oinfo - Refence to overlay information
 *   area  - Area draw
 *   color - cmap color entry
 *
 ****************************************************************************/

#ifdef CONFIG_FB_CMAP
static void draw_rect8(FAR void *fbmem, FAR struct fb_overlayinfo_s * oinfo,
                       FAR const struct fb_area_s * area, uint8_t color)
{
  FAR uint8_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  printf("Fill area (%d,%d,%d,%d) with color: %08x\n" , area->x, area->y,
          area->w, area->h, color);

  row = (FAR uint8_t *)fbmem + oinfo->stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = row + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = color;
        }

      row += oinfo->stride;
    }
}
#endif

/****************************************************************************
 * Name: draw_rect16
 *
 * Description:
 *   Draw a rectangle for 16 bit color mode
 *
 * Parameters:
 *   fbmem - Points to framebuffer start address
 *   oinfo - Refence to overlay information
 *   area  - Area draw
 *   rgb   - rgb565 color
 *
 ****************************************************************************/

static void draw_rect16(FAR void *fbmem, FAR struct fb_overlayinfo_s * oinfo,
                        FAR const struct fb_area_s * area, uint16_t rgb)
{
  FAR uint16_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  printf("Fill area (%d,%d,%d,%d) with color: %04x\n", area->x, area->y,
          area->w, area->h, rgb);

  row = (FAR uint8_t *)fbmem + oinfo->stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint16_t *)row) + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = rgb;
        }

      row += oinfo->stride;
    }
}

/****************************************************************************
 * Name: draw_rect24
 *
 * Description:
 *   Draw a rectangle for 24 bit color mode
 *
 * Parameters:
 *   fbmem - Points to framebuffer start address
 *   oinfo - Refence to overlay information
 *   area  - Area draw
 *   rgb   - rgb888 color
 *
 ****************************************************************************/

static void draw_rect24(FAR void *fbmem, FAR struct fb_overlayinfo_s * oinfo,
                        FAR const struct fb_area_s * area, uint32_t rgb)
{
  int         x;
  int         y;
  uint8_t     r;
  uint8_t     g;
  uint8_t     b;
  FAR uint8_t *dest;
  FAR uint8_t *row;

  r = rgb;
  g = (rgb >> 8);
  b = (rgb >> 16);

  printf("Fill area (%d,%d,%d,%d) with color: %08x -> (r,g,b) = "
         "(%02x,%02x,%02x)\n", area->x, area->y, area->w, area->h,
         rgb, b, g, r);

  row = (FAR uint8_t *)fbmem + oinfo->stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = row + area->x * 3;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = r;
          *dest++ = g;
          *dest++ = b;
        }

      row += oinfo->stride;
    }
}

/****************************************************************************
 * Name: draw_rect32
 *
 * Description:
 *   Draw a rectangle for 32 bit color mode
 *
 * Parameters:
 *   fbmem - Points to framebuffer start address
 *   oinfo - Refence to overlay information
 *   area  - Area draw
 *   argb  - argb8888 color
 *
 ****************************************************************************/

static void draw_rect32(FAR void *fbmem, FAR struct fb_overlayinfo_s * oinfo,
                        FAR const struct fb_area_s * area, uint32_t argb)
{
  int          x;
  int          y;
  FAR uint32_t *dest;
  FAR uint8_t  *row;

  printf("Fill area (%d,%d,%d,%d) with color: %08x -> (a,r,g,b) = "
         "(%02x,%02x,%02x,%02x)\n", area->x, area->y, area->w, area->h,
         argb, argb >> 24, argb >> 16, argb >> 8, argb);

  row = (FAR uint8_t *)fbmem + oinfo->stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint32_t *)row) + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = argb;
        }

      row += oinfo->stride;
    }
}

/****************************************************************************
 * Name: video_cmap
 *
 * Description:
 *   Sets cmap
 *
 * Parameters:
 *   fb        - Open framebuffer filehandle
 *   cmap      - Readonly reference to color map
 *
 ****************************************************************************/

#ifdef CONFIG_FB_CMAP
static int video_putcmap(int fb, FAR const struct fb_cmap_s *cmap)
{
    int n;
    int ret;

    printf("Set cmap\n");
    for (n = cmap->first; n < cmap->len; n++)
      {
          printf("  Color %d: (a,r,g,b) = (%02x,%02x,%02x,%02x)\n", n,
#  ifdef CONFIG_CMAP_TRANSPARENCY
                 cmap->transp[n],
#  else
                 (uint8_t)0xff,
#  endif
                 cmap->red[n],
                 cmap->green[n],
                 cmap->blue[n]);
      }

    ret = ioctl(fb, FBIOPUT_CMAP, (unsigned long)(uintptr_t)cmap);
    if (ret != OK)
      {
          fprintf(stderr, "Unable to set camp\n");
      }

    return ret;
}

/****************************************************************************
 * Name: video_getcmap
 *
 * Description:
 *   Sets cmap
 *
 * Parameters:
 *   fb        - Open framebuffer filehandle
 *   cmap      - Reference to color map
 *
 ****************************************************************************/

static int video_getcmap(int fb, FAR struct fb_cmap_s *cmap)
{
    int n;
    int ret;

    ret = ioctl(fb, FBIOGET_CMAP, (unsigned long)(uintptr_t)cmap);
    if (ret != OK)
      {
          fprintf(stderr, "Unable to set camp\n");
      }

    printf("Get cmap\n");
    for (n = cmap->first; n < cmap->len; n++)
      {
          printf("  Color %d: (a,r,g,b) = (%02x,%02x,%02x,%02x)\n", n,
#  ifdef CONFIG_CMAP_TRANSPARENCY
                 cmap->transp[n],
#  else
                 (uint8_t)0xff,
#  endif
                 cmap->red[n],
                 cmap->green[n],
                 cmap->blue[n]);
      }

    return ret;
}
#endif /* CONFIG_FB_CMAP */

/****************************************************************************
 * Name: print_video_info
 *
 * Description:
 *   Prints video information
 *
 * Parameters:
 *   fb        - Open framebuffer filehandle
 *
 ****************************************************************************/

static void print_video_info(int fb)
{
  int ret;
  struct fb_videoinfo_s vinfo;

  ret = ioctl(fb, FBIOGET_VIDEOINFO, (unsigned long)(uintptr_t)&vinfo);
  if (ret != OK)
    {
      fprintf(stderr, "Unable to get video information\n");
      return;
    }

  printf("VideoInfo:\n"
         "      fmt: %u\n"
         "     xres: %u\n"
         "     yres: %u\n"
         "  nplanes: %u\n"
         "noverlays: %u\n",
         vinfo.fmt, vinfo.xres, vinfo.yres, vinfo.nplanes, vinfo.noverlays);
}

/****************************************************************************
 * Name: print_plane_info
 *
 * Description:
 *   Prints plane information
 *
 * Parameters:
 *   fb - Open framebuffer filehandle
 *
 ****************************************************************************/

static void print_plane_info(int fb)
{
  int ret;
  FAR void *fbmem;
  struct fb_planeinfo_s pinfo;

  ret = ioctl(fb, FBIOGET_PLANEINFO, (unsigned long)(uintptr_t)&pinfo);
  if (ret != OK)
    {
      fprintf(stderr, "Unable to get plane information\n");
      return;
    }

  fbmem = mmap(NULL, pinfo.fblen, PROT_READ|PROT_WRITE,
      MAP_SHARED|MAP_FILE, fb, 0);

  if (fbmem == MAP_FAILED)
    {
      fprintf(stderr, "ERROR: ioctl(FBIO_MMAP) failed: %d\n", errno);
      return;
    }

  printf("PlaneInfo:\n"
         "    fbmem: %p\n"
         "    fblen: %lu\n"
         "   stride: %u\n"
         "  display: %u\n"
         "      bpp: %u\n",
         pinfo.fbmem, (unsigned long)pinfo.fblen, pinfo.stride, pinfo.display,
         pinfo.bpp, fbmem);
}

/****************************************************************************
 * Name: print_overlay_info
 *
 * Description:
 *   Prints overlay information
 *
 * Parameters:
 *   fb        - Open framebuffer filehandle
 *   overlayno - Overlay number
 *
 ****************************************************************************/

static void print_overlay_info(int fb, uint8_t overlayno)
{
  int ret;
  FAR void *fbmem;
  struct fb_overlayinfo_s oinfo;

  /* Select overlay to compare fbmem from overlayinfo and the one by mmap */

  ret = ioctl(fb, FBIO_SELECT_OVERLAY, overlayno);
  if (ret != OK)
    {
      fprintf(stderr, "Unable to select overlay: %d\n", overlayno);
      return;
    }

  oinfo.overlay = overlayno;

  ret = ioctl(fb, FBIOGET_OVERLAYINFO, (unsigned long)(uintptr_t)&oinfo);
  if (ret != OK)
    {
      fprintf(stderr, "Unable to get overlay information\n");
      return;
    }

  fbmem = mmap(NULL, oinfo.fblen, PROT_READ|PROT_WRITE,
               MAP_SHARED|MAP_FILE, fb, 0);

  if (fbmem == MAP_FAILED)
    {
      fprintf(stderr, "ERROR: ioctl(FBIO_MMAP) failed: %d\n", errno);
      return;
    }

  printf("OverlayInfo:\n"
         "    fbmem: %p\n"
         "    fblen: %lu\n"
         "   stride: %u\n"
         "  overlay: %u\n"
         "      bpp: %u\n"
         "    blank: %08x\n"
         "chromakey: %08x\n"
         "    color: %08x\n"
         "   transp: %02x\n"
         "     mode: %08x\n"
         "     accl: %08x\n"
         "     mmap: %p\n",
         oinfo.fbmem, (unsigned long)oinfo.fblen, oinfo.stride, oinfo.overlay,
         oinfo.bpp, oinfo.blank, oinfo.chromakey, oinfo.color,
         oinfo.transp.transp, oinfo.transp.transp_mode, oinfo.accl, fbmem);
}

/****************************************************************************
 * Name: overlay_fill
 *
 * Description:
 *   Fill the overlay area with a user defined color
 *
 * Parameters:
 *   fb        - Open framebuffer filehandle
 *   overlayno - Overlay number
 *   color      - color
 *
 ****************************************************************************/

static int overlay_fill(int fb, uint8_t overlayno, uint32_t color,
                        FAR const struct fb_area_s * area)
{
  int ret;
  FAR void *fbmem;
  struct fb_videoinfo_s vinfo;
  struct fb_overlayinfo_s oinfo;

  ret = ioctl(fb, FBIOGET_VIDEOINFO, (unsigned long)((uintptr_t)&vinfo));
  if (ret < 0)
    {
      fprintf(stderr, "Unable to get video info\n");
      return -1;
    }

  oinfo.overlay = overlayno;

  ret = ioctl(fb, FBIOGET_OVERLAYINFO, (unsigned long)((uintptr_t)&oinfo));
  if (ret < 0)
    {
      fprintf(stderr, "Unable to get overlay info\n");
      return -1;
    }

  ret = ioctl(fb, FBIO_SELECT_OVERLAY, overlayno);
  if (ret != OK)
    {
      fprintf(stderr, "Unable to select overlay: %d\n", overlayno);
      return -1;
    }

  fbmem = mmap(NULL, oinfo.fblen, PROT_READ|PROT_WRITE,
               MAP_SHARED|MAP_FILE, fb, 0);

  if (fbmem == MAP_FAILED)
    {
      fprintf(stderr, "ERROR: ioctl(FBIO_MMAP) failed\n");
      ret = -1;
    }
  else
    {
      uint32_t offset = (area->y + area->h - 1) * oinfo.stride +
                            (area->x + area->w) * oinfo.bpp / 8;

      if (offset > oinfo.fblen)
        {
          fprintf(stderr, "Area is out of range: %d >= %d\n", offset, oinfo.fblen);
          return -1;
        }

#ifdef CONFIG_FB_SYNC
      ret = ioctl(fb, FBIO_WAITFORVSYNC, 0);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to sync upon vertical line\n");
        }
#endif

#ifdef CONFIG_FB_CMAP
      if (vinfo.fmt == FB_FMT_RGB8)
        {
          draw_rect8(fbmem, &oinfo, area, color);
        }
#endif
      else if (vinfo.fmt == FB_FMT_RGB16_565)
        {
          draw_rect16(fbmem, &oinfo, area, color);
        }
      else if (vinfo.fmt == FB_FMT_RGB24)
        {
          draw_rect24(fbmem, &oinfo, area, color);
        }
      else if (vinfo.fmt == FB_FMT_RGB32)
        {
          draw_rect32(fbmem, &oinfo, area, color);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: overlay_accl
 *
 * Description:
 *   Determines overlay acceleration support
 *
 * Parameters:
 *   fb        - Open framebuffer filehandle
 *   overlayno - Overlay number
 *   accl      - Acceleration to detect
 *
 * Return:
 *   OK    - Success
 *   ERROR - Failed
 ****************************************************************************/

static int overlay_accl(int fb, uint8_t overlayno, uint32_t accl)
{
  int ret;
  struct fb_overlayinfo_s oinfo;

  oinfo.overlay = overlayno;

  ret = ioctl(fb, FBIOGET_OVERLAYINFO, (unsigned long)((uintptr_t)&oinfo));
  if (ret != OK)
    {
      fprintf(stderr, "Unable to get overlay information\n");
      return -1;
    }

  printf("%s: %08x %08x\n", __func__, oinfo.accl, accl);
  return (oinfo.accl & accl) ? OK : -1;
}

/****************************************************************************
 * Name: overlay_color
 *
 * Description:
 *   Set overlay color
 *
 * Parameters:
 *   fb    - Open framebuffer filehandle
 *   oinfo - Overlay information
 *
 ****************************************************************************/

static int overlay_color(int fb, FAR struct fb_overlayinfo_s *oinfo)
{
  int ret;

  printf("Overlay: %d, set color: 0x%08x\n", oinfo->overlay, oinfo->color);

  ret = overlay_accl(fb, oinfo->overlay, FB_ACCL_COLOR);
  if (ret != OK)
    {
      fprintf(stderr, "No hardware acceleration to set a color within the "
              "selected overlay area\n");
    }
  else
    {
#ifdef CONFIG_FB_SYNC
      ret = ioctl(fb, FBIO_WAITFORVSYNC, 0);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to sync upon vertical line\n");
        }
#endif

      ret = ioctl(fb, FBIOSET_COLOR, (unsigned long)(uintptr_t)oinfo);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to set overlay color\n");
        }
    }

  return ret;
}

/****************************************************************************
 * Name: overlay_chromakey
 *
 * Description:
 *   Set overlay chromakey
 *
 * Parameters:
 *   fb        - Open framebuffer filehandle
 *   oinfo - Overlay information
 *
 ****************************************************************************/

static int overlay_chromakey(int fb, FAR struct fb_overlayinfo_s *oinfo)
{
  int ret;

  printf("Overlay: %d, set chromakey: 0x%08x\n", oinfo->overlay,
         oinfo->chromakey);

  ret = overlay_accl(fb, oinfo->overlay, FB_ACCL_CHROMA);
  if (ret != OK)
    {
      fprintf(stderr, "No hardware acceleration to set the chromakey within "
              "the selected overlay area\n");
    }
  else
    {
#ifdef CONFIG_FB_SYNC
      ret = ioctl(fb, FBIO_WAITFORVSYNC, 0);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to sync upon vertical line\n");
        }
#endif

      ret = ioctl(fb, FBIOSET_CHROMAKEY, (unsigned long)(uintptr_t)oinfo);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to set overlay chroma key\n");
        }
    }

  return ret;
}

/****************************************************************************
 * Name: overlay_transp
 *
 * Description:
 *   Set overlay transparency
 *
 * Parameters:
 *   fb    - Open framebuffer filehandle
 *   oinfo - Overlay information
 *
 ****************************************************************************/

static int overlay_transp(int fb, FAR struct fb_overlayinfo_s *oinfo)
{
  int ret;

  printf("Overlay: %d, set transp: %d, transp_mode: %08x\n", oinfo->overlay,
         oinfo->transp.transp, oinfo->transp.transp_mode);

  ret = overlay_accl(fb, oinfo->overlay, FB_ACCL_TRANSP);
  if (ret != OK)
    {
      fprintf(stderr, "No hardware acceleration to set the transparency within "
              "the selected overlay area\n");
    }
  else
    {
#ifdef CONFIG_FB_SYNC
      ret = ioctl(fb, FBIO_WAITFORVSYNC, 0);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to sync upon vertical line\n");
        }
#endif

      ret = ioctl(fb, FBIOSET_TRANSP, (unsigned long)(uintptr_t)oinfo);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to set transparency\n");
        }
    }

  return ret;
}

/****************************************************************************
 * Name: overlay_blank
 *
 * Description:
 *   Enable or disable overlay
 *
 * Parameters:
 *   fb    - Open framebuffer filehandle
 *   oinfo - Overlay information
 *
 ****************************************************************************/

static int overlay_blank(int fb, FAR struct fb_overlayinfo_s *oinfo)
{
  int ret;

  printf("Overlay: %d, set blank: %08x\n", oinfo->overlay, oinfo->blank);

  ret = ioctl(fb, FBIOSET_BLANK, (unsigned long)(uintptr_t)oinfo);
  if (ret != OK)
    {
      fprintf(stderr, "Unable to blank overlay\n");
    }

  return ret;
}

/****************************************************************************
 * Name: overlay_area
 *
 * Description:
 *   Select overlay area
 *
 * Parameters:
 *   fb        - Open framebuffer filehandle
 *   oinfo     - Overlay information
 *
 ****************************************************************************/

static int overlay_area(int fb, FAR struct fb_overlayinfo_s *oinfo)
{
  int ret;

  printf("Overlay: %d, set xpos: %d, ypos: %d, xres: %d, yres: %d\n",
         oinfo->overlay, oinfo->sarea.x, oinfo->sarea.y, oinfo->sarea.w,
         oinfo->sarea.h);

  ret = overlay_accl(fb, oinfo->overlay, FB_ACCL_AREA);
  if (ret != OK)
    {
      fprintf(stderr, "No hardware acceleration to select area within "
              "the selected overlay area\n");
    }
  else
    {
      ret = ioctl(fb, FBIOSET_AREA, (unsigned long)(uintptr_t)oinfo);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to select area\n");
        }
    }

  return ret;
}

/****************************************************************************
 * Name: overlay_blit
 *
 * Description:
 *   Blit content from source to destination overlay
 *
 * Parameters:
 *   fb         - Open framebuffer filehandle
 *   blit       - Blit control information
 *
 ****************************************************************************/

#ifdef CONFIG_FB_OVERLAY_BLIT
static int overlay_blit(int fb, FAR struct fb_overlayblit_s *blit)
{
  int ret;

  ret = overlay_accl(fb, blit->src.overlay, FB_ACCL_BLIT);
  if (ret == OK)
    {
      ret = overlay_accl(fb, blit->dest.overlay, FB_ACCL_BLIT);
    }

  if (ret != OK)
    {
      fprintf(stderr, "Hardware blit not supported\n");
    }
  else
    {
#  ifdef CONFIG_FB_SYNC
      ret = ioctl(fb, FBIO_WAITFORVSYNC, 0);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to sync upon vertical line\n");
        }
#  endif

      ret = ioctl(fb, FBIOSET_BLIT, (unsigned long)(uintptr_t)blit);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to blit overlay content\n");
        }
    }

  return ret;
}

/****************************************************************************
 * Name: overlay_blend
 *
 * Description:
 *   Blend content from foreground and background overlay to a destination
 *   overlay
 *
 * Parameters:
 *   fb         - Open framebuffer filehandle
 *   blend      - Blend operation information
 *
 ****************************************************************************/

static int overlay_blend(int fb, FAR struct fb_overlayblend_s *blend)
{
  int ret;

  ret = overlay_accl(fb, blend->foreground.overlay, FB_ACCL_BLEND);
  if (ret == OK)
    {
      ret = overlay_accl(fb, blend->background.overlay, FB_ACCL_BLEND);
    }
  if (ret == OK)
    {
      ret = overlay_accl(fb, blend->dest.overlay, FB_ACCL_BLEND);
    }

  if (ret != OK)
    {
      fprintf(stderr, "Hardware blending not supported\n");
    }
  else
    {
#  ifdef CONFIG_FB_SYNC
      ret = ioctl(fb, FBIO_WAITFORVSYNC, 0);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to sync upon vertical line\n");
        }
#  endif

      ret = ioctl(fb, FBIOSET_BLEND, (unsigned long)(uintptr_t)blend);
      if (ret != OK)
        {
          fprintf(stderr, "Unable to blend overlay content\n");
        }
    }

  return ret;
}
#endif /* CONFIG_FB_OVERLAY_BLIT */

/****************************************************************************
 * Name: fbopen
 *
 * Description:
 *   Open framebuffer device
 *
 * Parameters:
 *   device - Path to framebuffer device
 *
 * Return:
 *   Open filehandle to framebuffer device or ERROR when failed
 ****************************************************************************/

static int fbopen(const char * device)
{
  int fb = open(device, O_RDWR);

  if (fb < 0)
    {
      fprintf(stderr, "Unable to open framebuffer device: %s\n", device);
      return EXIT_FAILURE;
    }

  return fb;
}

/****************************************************************************
 * Name: usage
 *
 * Description:
 *   Commandline info
 *
 * Parameters:
 *   progname - Name of the programm
 *
 ****************************************************************************/

static void usage(const char * progname)
{
  fprintf(stderr,
          "usage: %s <option> -d <fbdev>\n"
          "\n"
          "    -vinfo\n"
          "    -pinfo\n"
          "    -oinfo overlayno\n"
          "    -fill overlayno <color> <xpos> <ypos> <xres> <yres>\n"
          "      color: pixel format color\n"
          "      xpos: x-offset\n"
          "      ypos: y-offset\n"
          "      xres: x-resolution or area width\n"
          "      yres: y-resolution or area height\n"
#ifdef CONFIG_FB_CMAP
          "    -cmap <color1> <color2> <color3> <color4> <color5>\n"
          "      colors: 0xAARRGGBB\n"
          "      one color must be set at least\n"
#endif
          "    -color overlayno <value>\n"
          "      value: pixel format color\n"
          "    -chroma overlayno <value>\n"
          "      value: pixel format color\n"
          "    -transp overlayno <value> <mode>\n"
          "      value: 0-255\n"
          "      mode : %d = const alpha, %d = pixel alpha\n"
          "    -blank : <value>\n"
          "      0 : On\n"
          "      1 : Off\n"
          "    -area overlayno <xpos> <ypos> <xres> <yres>\n"
#ifdef CONFIG_FB_OVERLAY_BLIT
          "    -blit doverlayno <destxpos> <destypos> <destxres> <destyres>\n"
          "          soverlayno <srcxpos> <srcypos> <srcxres> <srcyres>\n"
          "    -blend doverlayno <dxpos> <dypos> <dxres> <dyres>\n"
          "           foverlayno <fxpos> <fypos> <fxres> <fyres>\n"
          "           boverlayno <bxpos> <bypos> <bxres> <byres>\n"
#endif
          "\n"
          "    -d <fbdev> optional, default: \"/dev/fb0\"\n",
          progname, FB_CONST_ALPHA, FB_PIXEL_ALPHA);
}

/****************************************************************************
 * Name: fboverlay_main
 *
 * Description:
 *   Main entry point for the fboverlay test application.
 *
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int fboverlay_main(int argc, char *argv[])
#endif
{

  char *fbdevice;
  int  fb = -1;

  if (argc < 2)
    {
      usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (argc >= 2 && !strcmp(argv[argc - 2], "-d"))
    {
      fbdevice = argv[argc - 1];
    }
  else
    {
      fbdevice = "/dev/fb0";
    }

  if (!strcmp(argv[1], "-vinfo"))
    {
      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          print_video_info(fb);
        }
    }
  else if (!strcmp(argv[1], "-pinfo"))
    {
      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          print_plane_info(fb);
        }
    }
  else if (!strcmp(argv[1], "-oinfo") && argc >= 3)
    {
      int overlayno = atoi(argv[2]);

      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          print_overlay_info(fb, overlayno);
        }
    }
  else if (!strcmp(argv[1], "-fill") && argc >= 8)
    {
      struct fb_area_s area;
      int overlayno = atoi(argv[2]);
      uint32_t argb = strtoul(argv[3], NULL, 16);

      area.x = strtoul(argv[4], NULL, 10);
      area.y = strtoul(argv[5], NULL, 10);
      area.w = strtoul(argv[6], NULL, 10);
      area.h = strtoul(argv[7], NULL, 10);

      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          overlay_fill(fb, overlayno, argb, &area);
        }
    }
#ifdef CONFIG_FB_CMAP
  else if (!strcmp(argv[1], "-cmap") && argc >= 2)
    {
      struct fb_cmap_s cmap;
      uint8_t a[5];
      uint8_t r[5];
      uint8_t g[5];
      uint8_t b[5];

      cmap.first = 0;
      cmap.len = 1;
      rgb8888(strtoul(argv[2], NULL, 16), &a[0], &r[0], &g[0], &b[0]);

      if (argc >= 3)
        {
          rgb8888(strtoul(argv[3], NULL, 16), &a[1], &r[1], &g[1], &b[1]);
          cmap.len = 2;
        }
      if (argc >= 4)
        {
          rgb8888(strtoul(argv[4], NULL, 16), &a[2], &r[2], &g[2], &b[2]);
          cmap.len = 3;
        }
      if (argc >= 5)
        {
          rgb8888(strtoul(argv[5], NULL, 16), &a[3], &r[3], &g[3], &b[3]);
          cmap.len = 4;
        }
      if (argc >= 6)
        {
          rgb8888(strtoul(argv[6], NULL, 16), &a[4], &r[4], &g[4], &b[4]);
          cmap.len = 5;
        }
#  ifdef CONFIG_FB_TRANSPARENCY
      cmap.transp = a;
#  endif
      cmap.red = r;
      cmap.green = g;
      cmap.blue = b;
      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          /* Set new cmap */

          video_putcmap(fb, &cmap);

          /* Fetch cmap form driver to compare */

          video_getcmap(fb, &cmap);
        }
    }
#endif /* CONFIG_FB_CMAP */
  else if (!strcmp(argv[1], "-color") && argc >= 4)
    {
      struct fb_overlayinfo_s oinfo;

      oinfo.overlay   = atoi(argv[2]);
      oinfo.color     = strtoul(argv[3], NULL, 16);

      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          overlay_color(fb, &oinfo);
        }
    }
  else if (!strcmp(argv[1], "-chroma") && argc >= 4)
    {
      struct fb_overlayinfo_s oinfo;

      oinfo.overlay   = atoi(argv[2]);
      oinfo.chromakey = strtoul(argv[3], NULL, 16);

      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          overlay_chromakey(fb, &oinfo);
        }
    }
  else if (!strcmp(argv[1], "-transp") && argc >= 5)
    {
      struct fb_overlayinfo_s oinfo;

      oinfo.overlay            = atoi(argv[2]);
      oinfo.transp.transp      = strtoul(argv[3], NULL, 10);
      oinfo.transp.transp_mode = strtoul(argv[4], NULL, 10);

      if (oinfo.transp.transp_mode != FB_CONST_ALPHA &&
          oinfo.transp.transp_mode != FB_PIXEL_ALPHA)
        {
          fprintf(stderr, "Invalid transparency mode: %d\n",
                  oinfo.transp.transp_mode);
        }
      else
        {
          fb = fbopen(fbdevice);
          if (fb >= 0)
            {
              overlay_transp(fb, &oinfo);
            }
        }
    }
  else if (!strcmp(argv[1], "-area") && argc >= 7)
    {
      struct fb_overlayinfo_s oinfo;

      oinfo.overlay = atoi(argv[2]);
      oinfo.sarea.x = strtoul(argv[3], NULL, 10);
      oinfo.sarea.y = strtoul(argv[4], NULL, 10);
      oinfo.sarea.w = strtoul(argv[5], NULL, 10);
      oinfo.sarea.h = strtoul(argv[6], NULL, 10);

      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          overlay_area(fb, &oinfo);
        }
    }
  else if (!strcmp(argv[1], "-blank") && argc >= 3)
    {
      struct fb_overlayinfo_s oinfo;

      oinfo.overlay  = atoi(argv[2]);
      oinfo.blank    = strtoul(argv[3], NULL, 10);

      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          overlay_blank(fb, &oinfo);
        }
    }
#ifdef CONFIG_FB_OVERLAY_BLIT
  else if (!strcmp(argv[1], "-blit") && argc >= 11)
    {
      struct fb_overlayblit_s blit;

      blit.dest.overlay = atoi(argv[2]);
      blit.dest.area.x  = strtoul(argv[3], NULL, 10);
      blit.dest.area.y  = strtoul(argv[4], NULL, 10);
      blit.dest.area.w  = strtoul(argv[5], NULL, 10);
      blit.dest.area.h  = strtoul(argv[6], NULL, 10);

      blit.src.overlay = atoi(argv[7]);
      blit.src.area.x  = strtoul(argv[8], NULL, 10);
      blit.src.area.y  = strtoul(argv[9], NULL, 10);
      blit.src.area.w  = strtoul(argv[10], NULL, 10);
      blit.src.area.h  = strtoul(argv[11], NULL, 10);

      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          overlay_blit(fb, &blit);
        }
    }
  else if (!strcmp(argv[1], "-blend") && argc >= 16)
    {
      struct fb_overlayblend_s blend;

      blend.dest.overlay       = atoi(argv[2]);
      blend.dest.area.x        = strtoul(argv[3], NULL, 10);
      blend.dest.area.y        = strtoul(argv[4], NULL, 10);
      blend.dest.area.w        = strtoul(argv[5], NULL, 10);
      blend.dest.area.h        = strtoul(argv[6], NULL, 10);

      blend.foreground.overlay = atoi(argv[7]);
      blend.foreground.area.x  = strtoul(argv[8], NULL, 10);
      blend.foreground.area.y  = strtoul(argv[9], NULL, 10);
      blend.foreground.area.w  = strtoul(argv[10], NULL, 10);
      blend.foreground.area.h  = strtoul(argv[11], NULL, 10);

      blend.background.overlay = atoi(argv[12]);
      blend.background.area.x  = strtoul(argv[13], NULL, 10);
      blend.background.area.y  = strtoul(argv[14], NULL, 10);
      blend.background.area.w  = strtoul(argv[15], NULL, 10);
      blend.background.area.h  = strtoul(argv[16], NULL, 10);

      fb = fbopen(fbdevice);
      if (fb >= 0)
        {
          overlay_blend(fb, &blend);
        }
    }
#endif
  else
    {
      usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (fb >= 0)
    {
      close(fb);
    }

  return EXIT_SUCCESS;
}
