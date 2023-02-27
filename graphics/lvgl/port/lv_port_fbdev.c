/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_fbdev.c
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
#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "lv_port_fbdev.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_FB_UPDATE)
#  define FBDEV_UPDATE_AREA(obj, area) fbdev_update_area(obj, area)
#else
#  define FBDEV_UPDATE_AREA(obj, area)
#endif

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct fbdev_obj_s
{
  lv_disp_draw_buf_t disp_draw_buf;
  lv_disp_drv_t disp_drv;
  FAR lv_disp_t *disp;
  FAR void *last_buffer;
  FAR void *act_buffer;
  lv_area_t inv_areas[LV_INV_BUF_SIZE];
  uint16_t inv_areas_len;
  lv_area_t final_area;

  int fd;
  FAR void *fbmem;
  uint32_t fbmem2_yoffset;
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;

  bool double_buffer;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_FB_UPDATE)

/****************************************************************************
 * Name: fbdev_update_area
 ****************************************************************************/

static void fbdev_update_area(FAR struct fbdev_obj_s *fbdev_obj,
                              FAR const lv_area_t *area_p)
{
  struct fb_area_s fb_area;

  fb_area.x = area_p->x1;
  fb_area.y = area_p->y1;
  fb_area.w = area_p->x2 - area_p->x1 + 1;
  fb_area.h = area_p->y2 - area_p->y1 + 1;

  LV_LOG_TRACE("area: (%d, %d) %d x %d",
               fb_area.x, fb_area.y, fb_area.w, fb_area.h);

  ioctl(fbdev_obj->fd, FBIO_UPDATE,
        (unsigned long)((uintptr_t)&fb_area));

  LV_LOG_TRACE("finished");
}
#endif

/****************************************************************************
 * Name: fbdev_switch_buffer
 ****************************************************************************/

static void fbdev_switch_buffer(FAR struct fbdev_obj_s *fbdev_obj)
{
  FAR lv_disp_t *disp_refr = fbdev_obj->disp;
  uint16_t inv_index;

  /* check inv_areas_len, it must == 0 */

  if (fbdev_obj->inv_areas_len != 0)
    {
      LV_LOG_ERROR("Repeated flush action detected! "
                    "inv_areas_len(%d) != 0",
                    fbdev_obj->inv_areas_len);
      fbdev_obj->inv_areas_len = 0;
    }

  /* Save dirty area table for next synchronizationn */

  for (inv_index = 0; inv_index < disp_refr->inv_p; inv_index++)
    {
      if (disp_refr->inv_area_joined[inv_index] == 0)
        {
          fbdev_obj->inv_areas[fbdev_obj->inv_areas_len] =
              disp_refr->inv_areas[inv_index];
          fbdev_obj->inv_areas_len++;
        }
    }

  /* Save the buffer address for the next synchronization */

  fbdev_obj->last_buffer = fbdev_obj->act_buffer;

  LV_LOG_TRACE("Commit buffer = %p, yoffset = %" PRIu32,
               fbdev_obj->act_buffer,
               fbdev_obj->pinfo.yoffset);

  if (fbdev_obj->act_buffer == fbdev_obj->fbmem)
    {
      fbdev_obj->pinfo.yoffset = 0;
      fbdev_obj->act_buffer = fbdev_obj->fbmem
        + fbdev_obj->fbmem2_yoffset * fbdev_obj->pinfo.stride;
    }
  else
    {
      fbdev_obj->pinfo.yoffset = fbdev_obj->fbmem2_yoffset;
      fbdev_obj->act_buffer = fbdev_obj->fbmem;
    }

  /* Commit buffer to fb driver */

  ioctl(fbdev_obj->fd, FBIOPAN_DISPLAY,
        (unsigned long)((uintptr_t)&(fbdev_obj->pinfo)));

  LV_LOG_TRACE("finished");
}

#if defined(CONFIG_FB_SYNC)

/****************************************************************************
 * Name: fbdev_disp_vsync_refr
 ****************************************************************************/

static void fbdev_disp_vsync_refr(FAR lv_timer_t *timer)
{
  int ret;
  FAR struct fbdev_obj_s *fbdev_obj = timer->user_data;

  LV_LOG_TRACE("Check vsync...");

  ret = ioctl(fbdev_obj->fd, FBIO_WAITFORVSYNC, NULL);
  if (ret != OK)
    {
      LV_LOG_TRACE("No vsync signal detect");
      return;
    }

  LV_LOG_TRACE("Refresh start");

  _lv_disp_refr_timer(NULL);
}

#endif /* CONFIG_FB_SYNC */

/****************************************************************************
 * Name: fbdev_check_inv_area_covered
 ****************************************************************************/

static bool fbdev_check_inv_area_covered(FAR lv_disp_t *disp_refr,
                                         FAR const lv_area_t *area_p)
{
  int i;

  for (i = 0; i < disp_refr->inv_p; i++)
    {
      FAR const lv_area_t *cur_area;

      /* Skip joined area */

      if (disp_refr->inv_area_joined[i])
        {
          continue;
        }

      cur_area = &disp_refr->inv_areas[i];

      /* Check cur_area is coverd area_p  */

      if (_lv_area_is_in(area_p, cur_area, 0))
        {
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: fbdev_render_start
 ****************************************************************************/

static void fbdev_render_start(FAR lv_disp_drv_t *disp_drv)
{
  FAR struct fbdev_obj_s *fbdev_obj = disp_drv->user_data;
  FAR lv_disp_t *disp_refr;
  FAR lv_draw_ctx_t *draw_ctx;
  lv_coord_t hor_res;
  int i;

  /* No need sync buffer when inv_areas_len == 0 */

  if (fbdev_obj->inv_areas_len == 0)
    {
      LV_LOG_TRACE("No sync area");
      return;
    }

  LV_LOG_TRACE("Start sync %d areas...", fbdev_obj->inv_areas_len);

  disp_refr = _lv_refr_get_disp_refreshing();
  draw_ctx = disp_drv->draw_ctx;
  hor_res = disp_drv->hor_res;

  for (i = 0; i < fbdev_obj->inv_areas_len; i++)
    {
      FAR const lv_area_t *last_area = &fbdev_obj->inv_areas[i];

      LV_LOG_TRACE("Check area[%d]: (%d, %d) %d x %d",
        i,
        (int)last_area->x1, (int)last_area->y1,
        (int)lv_area_get_width(last_area),
        (int)lv_area_get_height(last_area));

      if (fbdev_check_inv_area_covered(disp_refr, last_area))
        {
          LV_LOG_TRACE("Skipped");
          continue;
        }

      /* Sync the inv area of ​​the previous frame */

      draw_ctx->buffer_copy(
        draw_ctx,
        fbdev_obj->act_buffer, hor_res, last_area,
        fbdev_obj->last_buffer, hor_res, last_area);

      LV_LOG_TRACE("Copied");
    }

  fbdev_obj->inv_areas_len = 0;
}

/****************************************************************************
 * Name: fbdev_flush_direct
 ****************************************************************************/

static void fbdev_flush_direct(FAR lv_disp_drv_t *disp_drv,
                               FAR const lv_area_t *area_p,
                               FAR lv_color_t *color_p)
{
  FAR struct fbdev_obj_s *fbdev_obj = disp_drv->user_data;

  /* Commit the buffer after the last flush */

  if (!lv_disp_flush_is_last(disp_drv))
    {
      lv_disp_flush_ready(disp_drv);
      return;
    }

  fbdev_switch_buffer(fbdev_obj);

  FBDEV_UPDATE_AREA(fbdev_obj, area_p);

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

/****************************************************************************
 * Name: fbdev_update_part
 ****************************************************************************/

static void fbdev_update_part(FAR struct fbdev_obj_s *fbdev_obj,
                              FAR lv_disp_drv_t *disp_drv,
                              FAR const lv_area_t *area_p)
{
  FAR lv_area_t *final_area = &fbdev_obj->final_area;

  if (final_area->x1 < 0)
    {
      *final_area = *area_p;
    }
  else
    {
      _lv_area_join(final_area, final_area, area_p);
    }

  if (!lv_disp_flush_is_last(disp_drv))
    {
      lv_disp_flush_ready(disp_drv);
      return;
    }

  if (fbdev_obj->double_buffer)
    {
      fbdev_switch_buffer(fbdev_obj);
    }

  FBDEV_UPDATE_AREA(fbdev_obj, final_area);

  /* Mark it is invalid */

  final_area->x1 = -1;

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

/****************************************************************************
 * Name: fbdev_flush_normal
 ****************************************************************************/

static void fbdev_flush_normal(FAR lv_disp_drv_t *disp_drv,
                               FAR const lv_area_t *area_p,
                               FAR lv_color_t *color_p)
{
  FAR struct fbdev_obj_s *fbdev_obj = disp_drv->user_data;

  int x1 = area_p->x1;
  int y1 = area_p->y1;
  int y2 = area_p->y2;
  int y;
  int w = lv_area_get_width(area_p);

  FAR lv_color_t *fbp = fbdev_obj->act_buffer;
  fb_coord_t fb_xres = fbdev_obj->vinfo.xres;
  int hor_size = w * sizeof(lv_color_t);
  FAR lv_color_t *cur_pos = fbp + y1 * fb_xres + x1;

  LV_LOG_TRACE("start copy");

  for (y = y1; y <= y2; y++)
    {
      lv_memcpy(cur_pos, color_p, hor_size);
      cur_pos += fb_xres;
      color_p += w;
    }

  LV_LOG_TRACE("end copy");

  fbdev_update_part(fbdev_obj, disp_drv, area_p);
}

/****************************************************************************
 * Name: fbdev_get_pinfo
 ****************************************************************************/

static int fbdev_get_pinfo(int fd, FAR struct fb_planeinfo_s *pinfo)
{
  int ret = ioctl(fd, FBIOGET_PLANEINFO,
                  (unsigned long)((uintptr_t)pinfo));
  if (ret < 0)
    {
      LV_LOG_ERROR("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d", errno);
      return ret;
    }

  LV_LOG_INFO("PlaneInfo (plane %d):", pinfo->display);
  LV_LOG_INFO("    fbmem: %p", pinfo->fbmem);
  LV_LOG_INFO("    fblen: %lu", (unsigned long)pinfo->fblen);
  LV_LOG_INFO("   stride: %u", pinfo->stride);
  LV_LOG_INFO("  display: %u", pinfo->display);
  LV_LOG_INFO("      bpp: %u", pinfo->bpp);

  /* Only these pixel depths are supported.  viinfo.fmt is ignored, only
   * certain color formats are supported.
   */

  if (pinfo->bpp != 32 && pinfo->bpp != 16 &&
      pinfo->bpp != 8  && pinfo->bpp != 1)
    {
      LV_LOG_ERROR("bpp = %u not supported", pinfo->bpp);
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: fbdev_try_init_fbmem2
 ****************************************************************************/

static int fbdev_try_init_fbmem2(FAR struct fbdev_obj_s *state)
{
  uintptr_t buf_offset;
  struct fb_planeinfo_s pinfo;

  memset(&pinfo, 0, sizeof(pinfo));

  /* Get display[1] planeinfo */

  pinfo.display = state->pinfo.display + 1;

  if (fbdev_get_pinfo(state->fd, &pinfo) < 0)
    {
      return -1;
    }

  /* check display and match bpp */

  if (!(pinfo.display != state->pinfo.display
     && pinfo.bpp == state->pinfo.bpp))
    {
      LV_LOG_INFO("fbmem2 is incorrect");
      return -1;
    }

  /* Check the buffer address offset,
   * It needs to be divisible by pinfo.stride
   */

  buf_offset = pinfo.fbmem - state->fbmem;

  if ((buf_offset % state->pinfo.stride) != 0)
    {
      LV_LOG_ERROR("The buf_offset(%" PRIuPTR ") is incorrect,"
                   " it needs to be divisible"
                   " by pinfo.stride(%d)",
                   buf_offset, state->pinfo.stride);
      return -1;
    }

  /* Enable double buffer mode */

  state->double_buffer = true;
  state->fbmem2_yoffset = buf_offset / state->pinfo.stride;

  LV_LOG_INFO("Use non-consecutive fbmem2 = %p, yoffset = %" PRIu32,
              pinfo.fbmem, state->fbmem2_yoffset);

  return 0;
}

/****************************************************************************
 * Name: fbdev_init
 ****************************************************************************/

static FAR lv_disp_t *fbdev_init(FAR struct fbdev_obj_s *state)
{
  FAR struct fbdev_obj_s *fbdev_obj = malloc(sizeof(struct fbdev_obj_s));
  FAR lv_disp_drv_t *disp_drv;
  int fb_xres = state->vinfo.xres;
  int fb_yres = state->vinfo.yres;
  size_t fb_size = fb_xres * fb_yres;
  FAR lv_color_t *buf1 = NULL;
  FAR lv_color_t *buf2 = NULL;

  if (fbdev_obj == NULL)
    {
      LV_LOG_ERROR("fbdev_obj_s malloc failed");
      return NULL;
    }

  *fbdev_obj = *state;
  disp_drv = &(fbdev_obj->disp_drv);

  lv_disp_drv_init(disp_drv);
  disp_drv->draw_buf = &(fbdev_obj->disp_draw_buf);
  disp_drv->screen_transp = false;
  disp_drv->user_data = fbdev_obj;
  disp_drv->hor_res = fb_xres;
  disp_drv->ver_res = fb_yres;

  if (fbdev_obj->double_buffer)
    {
      LV_LOG_INFO("Double buffer mode");

      buf1 = fbdev_obj->fbmem;
      buf2 = fbdev_obj->fbmem
        + fbdev_obj->fbmem2_yoffset * fbdev_obj->pinfo.stride;

      disp_drv->direct_mode = true;
      disp_drv->flush_cb = fbdev_flush_direct;
      disp_drv->render_start_cb = fbdev_render_start;
    }
  else
    {
      LV_LOG_INFO("Single buffer mode");

      buf1 = malloc(fb_size * sizeof(lv_color_t));
      LV_ASSERT_MALLOC(buf1);

      if (!buf1)
        {
          LV_LOG_ERROR("failed to malloc draw buffer");
          goto failed;
        }

      disp_drv->flush_cb = fbdev_flush_normal;
    }

  lv_disp_draw_buf_init(&(fbdev_obj->disp_draw_buf), buf1, buf2, fb_size);
  fbdev_obj->act_buffer = fbdev_obj->fbmem;
  fbdev_obj->disp = lv_disp_drv_register(&(fbdev_obj->disp_drv));

#if defined(CONFIG_FB_SYNC)
  /* If double buffer and vsync is supported, use active refresh method */

  if (fbdev_obj->disp_drv.direct_mode)
    {
      FAR lv_timer_t *refr_timer = _lv_disp_get_refr_timer(fbdev_obj->disp);
      lv_timer_del(refr_timer);
      fbdev_obj->disp->refr_timer = NULL;
      lv_timer_create(fbdev_disp_vsync_refr, 1, fbdev_obj);
    }
#endif

  return fbdev_obj->disp;

failed:
  free(fbdev_obj);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_fbdev_init
 *
 * Description:
 *   Framebuffer device interface initialization.
 *
 * Input Parameters:
 *   dev_path - Framebuffer device path, set to NULL to use the default path.
 *
 * Returned Value:
 *   lv_disp object address on success; NULL on failure.
 *
 ****************************************************************************/

FAR lv_disp_t *lv_port_fbdev_init(FAR const char *dev_path)
{
  FAR const char *device_path = dev_path;
  struct fbdev_obj_s state;
  int ret;
  FAR lv_disp_t *disp;

  memset(&state, 0, sizeof(state));

  if (device_path == NULL)
    {
      device_path = CONFIG_LV_PORT_FBDEV_DEFAULT_DEVICEPATH;
    }

  LV_LOG_INFO("fbdev %s opening", device_path);

  state.fd = open(device_path, O_RDWR);
  if (state.fd < 0)
    {
      LV_LOG_ERROR("fbdev %s open failed: %d", device_path, errno);
      return NULL;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(state.fd, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&state.vinfo));
  if (ret < 0)
    {
      LV_LOG_ERROR("ioctl(FBIOGET_VIDEOINFO) failed: %d", errno);
      close(state.fd);
      return NULL;
    }

  LV_LOG_INFO("VideoInfo:");
  LV_LOG_INFO("      fmt: %u", state.vinfo.fmt);
  LV_LOG_INFO("     xres: %u", state.vinfo.xres);
  LV_LOG_INFO("     yres: %u", state.vinfo.yres);
  LV_LOG_INFO("  nplanes: %u", state.vinfo.nplanes);

  ret = fbdev_get_pinfo(state.fd, &state.pinfo);

  if (ret < 0)
    {
      close(state.fd);
      return NULL;
    }

  /* Check color depth */

  if (!(state.pinfo.bpp == LV_COLOR_DEPTH ||
      (state.pinfo.bpp == 24 && LV_COLOR_DEPTH == 32)))
    {
      LV_LOG_ERROR("fbdev bpp = %d, LV_COLOR_DEPTH = %d, "
                   "color depth does not match.",
                   state.pinfo.bpp, LV_COLOR_DEPTH);
      close(state.fd);
      return NULL;
    }

  state.double_buffer = (state.pinfo.yres_virtual == (state.vinfo.yres * 2));

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
      LV_LOG_ERROR("ioctl(FBIOGET_PLANEINFO) failed: %d", errno);
      close(state.fd);
      return NULL;
    }

  LV_LOG_INFO("Mapped FB: %p", state.fbmem);

  if (state.double_buffer)
    {
      state.fbmem2_yoffset = state.vinfo.yres;
    }
  else
    {
      fbdev_try_init_fbmem2(&state);
    }

  disp = fbdev_init(&state);

  if (!disp)
    {
      munmap(state.fbmem, state.pinfo.fblen);
      close(state.fd);
      return NULL;
    }

  return disp;
}
