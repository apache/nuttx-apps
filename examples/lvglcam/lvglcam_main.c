/****************************************************************************
 * apps/examples/lvglcam/lvglcam_main.c
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

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/utsname.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <malloc.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nuttx/video/video.h>

#ifdef CONFIG_EXAMPLES_LVGLCAM_ACCEL
#  include <nuttx/sensors/ioctl.h>
#  include <nuttx/uorb.h>
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_NET
#  include <arpa/inet.h>
#  include <net/if.h>
#  include <netinet/in.h>
#  include "netutils/netlib.h"
#endif

#include <lvgl/lvgl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CAM_NBUFFERS       CONFIG_EXAMPLES_LVGLCAM_NBUFFERS
#define CAM_ALIGN          16               /* Fitted frame size alignment */

/* Snapshots are top-down BMP files named IMG_0001.BMP to IMG_9999.BMP */

#define SNAP_MAX           9999
#define SNAP_PATHLEN       64
#define BMP_FILEHDR        14
#define BMP_INFOHDR        40
#define BMP_MASKS          12
#define BMP_RGB            0
#define BMP_BITFIELDS      3

/* Layout: the view card and the data column, side by side on landscape
 * displays and stacked on portrait ones.
 */

#define UI_PERIOD_MS       250              /* Live data refresh */
#define UI_PAD             6
#define UI_RADIUS          8
#define UI_ROW             4
#define UI_SIDE_MIN        160              /* Data column width */
#define UI_SIDE_DIV        3                /* Of the display */

#define CHART_POINTS       40
#define CHART_H            40
#define CHART_DIVS         3
#define CHART_SCALE        10               /* Points per m/s2 */
#define CHART_RANGE        120

#define ACCEL_INTERVAL_US  100000           /* 10 Hz */
#define RAD_TO_DEG         57.29578f

/* Depth card: a V4L2 Z16 depth camera, one distance in mm per pixel, 0 for
 * no target.
 */

#define DEPTH_MAX          16               /* Zones per side */
#define DEPTH_NBUFFERS     2
#define DEPTH_VIEW         64               /* Heatmap size */
#define DEPTH_HUE_FAR      240              /* Blue at the far end */
#define DEPTH_STOP_MS      10               /* DQBUF cancel period */

#define COLOR_BG           lv_color_hex(0x0d1117)
#define COLOR_CARD         lv_color_hex(0x161b22)
#define COLOR_ACCENT       lv_color_hex(0x2f81f7)
#define COLOR_TEXT         lv_color_hex(0xe6edf3)
#define COLOR_DIM          lv_color_hex(0x8b949e)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Camera formats that LVGL draws directly */

struct cam_format_s
{
  uint32_t fourcc;
  lv_color_format_t cf;
  uint8_t bpp;                          /* Bits per pixel */
};

struct lvglcam_s
{
  /* Camera, used by the LVGL thread only */

  int vfd;
  FAR const struct cam_format_s *fmt;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  FAR uint8_t *buf[CAM_NBUFFERS];
  size_t buflen[CAM_NBUFFERS];
  lv_draw_buf_t dbuf[CAM_NBUFFERS];
  int shown;                            /* Buffer on the canvas */
  bool streaming;
  uint32_t frames;
  uint32_t lastframes;
  uint32_t lastms;

  /* UI */

  FAR lv_obj_t *view;
  FAR lv_obj_t *canvas;
  FAR lv_obj_t *info;
  FAR lv_obj_t *side;
  FAR lv_obj_t *sys;

#ifdef CONFIG_EXAMPLES_LVGLCAM_ACCEL
  int accfd;
  FAR lv_obj_t *acc;
  FAR lv_obj_t *chart;
  FAR lv_chart_series_t *series[3];
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_NET
  FAR lv_obj_t *net;
#endif

  /* Shared with the snapshot and depth threads */

  pthread_mutex_t lock;
  pthread_cond_t cond;
  bool running;

#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
  FAR lv_obj_t *snap;
  pthread_t snapthread;
  bool snapstarted;
  int hold;                             /* Buffer held for a snapshot */
  bool snapdone;                        /* Buffer 'hold' can be released */
  int snapret;
  bool snapnew;                         /* Result to show */
  char snappath[SNAP_PATHLEN];
  int snaps;                            /* Last snapshot number */
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_DEPTH
  pthread_t depththread;
  bool depthstarted;
  bool depthdone;
  int depthfd;
  int deptherr;
  uint32_t depthw;
  uint32_t depthh;
  uint16_t depthmm[DEPTH_MAX * DEPTH_MAX];
  uint32_t depthframes;
  uint32_t depthshown;
  uint16_t depthpix[DEPTH_MAX * DEPTH_MAX];
  FAR lv_obj_t *depthcard;
  FAR lv_obj_t *depthmap;
  FAR lv_obj_t *depthinfo;
#endif
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* In order of preference */

static const struct cam_format_s g_formats[] =
{
  {
    V4L2_PIX_FMT_RGB565, LV_COLOR_FORMAT_RGB565, 16
  },
  {
    V4L2_PIX_FMT_XRGB32, LV_COLOR_FORMAT_XRGB8888, 32
  },
  {
    V4L2_PIX_FMT_BGR24, LV_COLOR_FORMAT_RGB888, 24
  },
};

static struct lvglcam_s g_cam;
static volatile sig_atomic_t g_stop;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lvglcam_signal
 ****************************************************************************/

static void lvglcam_signal(int signo)
{
  g_stop = 1;
}

/****************************************************************************
 * Name: cam_qbuf
 ****************************************************************************/

static int cam_qbuf(FAR struct lvglcam_s *cam, int index)
{
  struct v4l2_buffer buf;
  int ret;

  memset(&buf, 0, sizeof(buf));
  buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index  = index;

  if (ioctl(cam->vfd, VIDIOC_QBUF, (uintptr_t)&buf) < 0)
    {
      ret = -errno;
      printf("lvglcam: VIDIOC_QBUF failed: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: cam_open
 *
 * Description:
 *   Open the camera and pick the first format that LVGL draws directly.
 *
 ****************************************************************************/

static int cam_open(FAR struct lvglcam_s *cam)
{
  struct v4l2_fmtdesc desc;
  size_t i;
  int ret;

  cam->vfd = open(CONFIG_EXAMPLES_LVGLCAM_VIDEO_DEVPATH, O_RDONLY);
  if (cam->vfd < 0)
    {
      ret = -errno;
      printf("lvglcam: open %s failed: %d\n",
             CONFIG_EXAMPLES_LVGLCAM_VIDEO_DEVPATH, ret);
      return ret;
    }

  for (i = 0; i < nitems(g_formats) && cam->fmt == NULL; i++)
    {
      memset(&desc, 0, sizeof(desc));
      desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

      while (ioctl(cam->vfd, VIDIOC_ENUM_FMT, (uintptr_t)&desc) >= 0)
        {
          if (desc.pixelformat == g_formats[i].fourcc)
            {
              cam->fmt = &g_formats[i];
              break;
            }

          desc.index++;
        }
    }

  if (cam->fmt == NULL)
    {
      printf("lvglcam: no RGB565, XRGB32 or BGR24 camera format\n");
      return -ENOTSUP;
    }

  return OK;
}

/****************************************************************************
 * Name: cam_setfmt
 ****************************************************************************/

static int cam_setfmt(FAR struct lvglcam_s *cam, uint32_t width,
                      uint32_t height)
{
  struct v4l2_format fmt;

  if (width == 0 || height == 0)
    {
      return -EINVAL;
    }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = width;
  fmt.fmt.pix.height      = height;
  fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = cam->fmt->fourcc;

  if (ioctl(cam->vfd, VIDIOC_S_FMT, (uintptr_t)&fmt) < 0)
    {
      return -errno;
    }

  cam->width  = fmt.fmt.pix.width;
  cam->height = fmt.fmt.pix.height;
  cam->stride = fmt.fmt.pix.bytesperline;
  if (cam->stride == 0)
    {
      cam->stride = cam->width * cam->fmt->bpp / 8;
    }

  return OK;
}

/****************************************************************************
 * Name: cam_size
 *
 * Description:
 *   Set the largest frame size that fits the view: the full frame scaled
 *   to the view first, then the largest listed size that fits, then the
 *   smallest listed size.
 *
 ****************************************************************************/

static int cam_size(FAR struct lvglcam_s *cam, uint32_t vw, uint32_t vh)
{
  struct v4l2_frmsizeenum size;
  uint32_t maxw = 0;
  uint32_t maxh = 0;
  uint32_t bestw = 0;
  uint32_t besth = 0;
  uint32_t minw = UINT32_MAX;
  uint32_t minh = 0;
  uint32_t w;
  uint32_t h;
  int ret;

  if (CONFIG_EXAMPLES_LVGLCAM_WIDTH > 0 &&
      CONFIG_EXAMPLES_LVGLCAM_HEIGHT > 0)
    {
      return cam_setfmt(cam, CONFIG_EXAMPLES_LVGLCAM_WIDTH,
                        CONFIG_EXAMPLES_LVGLCAM_HEIGHT);
    }

  memset(&size, 0, sizeof(size));
  size.pixel_format = cam->fmt->fourcc;

  while (ioctl(cam->vfd, VIDIOC_ENUM_FRAMESIZES, (uintptr_t)&size) >= 0)
    {
      if (size.type == V4L2_FRMSIZE_TYPE_DISCRETE)
        {
          w = size.discrete.width;
          h = size.discrete.height;
        }
      else
        {
          w = size.stepwise.max_width;
          h = size.stepwise.max_height;
        }

      if (w * h > maxw * maxh)
        {
          maxw = w;
          maxh = h;
        }

      if (w <= vw && h <= vh && w * h > bestw * besth)
        {
          bestw = w;
          besth = h;
        }

      if (w * h < minw * minh || minw == UINT32_MAX)
        {
          minw = w;
          minh = h;
        }

      if (size.type != V4L2_FRMSIZE_TYPE_DISCRETE)
        {
          break;
        }

      size.index++;
    }

  if (maxw == 0)
    {
      maxw = vw;
      maxh = vh;
    }

  /* The full frame with its aspect ratio, scaled down to the view */

  if ((uint64_t)vw * maxh <= (uint64_t)vh * maxw)
    {
      w = MIN(vw, maxw);
      h = (uint64_t)w * maxh / maxw;
    }
  else
    {
      h = MIN(vh, maxh);
      w = (uint64_t)h * maxw / maxh;
    }

  ret = cam_setfmt(cam, w / CAM_ALIGN * CAM_ALIGN,
                   h / CAM_ALIGN * CAM_ALIGN);
  if (ret < 0)
    {
      ret = cam_setfmt(cam, bestw, besth);
    }

  if (ret < 0 && minw != UINT32_MAX)
    {
      ret = cam_setfmt(cam, minw, minh);
    }

  if (ret < 0)
    {
      printf("lvglcam: VIDIOC_S_FMT failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: cam_start
 *
 * Description:
 *   Map the capture buffers, queue them and start streaming.
 *
 ****************************************************************************/

static int cam_start(FAR struct lvglcam_s *cam)
{
  struct v4l2_requestbuffers req;
  struct v4l2_buffer buf;
  enum v4l2_buf_type type;
  int ret;
  int i;

  memset(&req, 0, sizeof(req));
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  req.count  = CAM_NBUFFERS;
  req.mode   = V4L2_BUF_MODE_FIFO;

  if (ioctl(cam->vfd, VIDIOC_REQBUFS, (uintptr_t)&req) < 0)
    {
      ret = -errno;
      printf("lvglcam: VIDIOC_REQBUFS failed: %d\n", ret);
      return ret;
    }

  if (req.count < CAM_NBUFFERS)
    {
      printf("lvglcam: only %" PRIu32 " capture buffers\n", req.count);
      return -ENOMEM;
    }

  for (i = 0; i < CAM_NBUFFERS; i++)
    {
      memset(&buf, 0, sizeof(buf));
      buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index  = i;

      if (ioctl(cam->vfd, VIDIOC_QUERYBUF, (uintptr_t)&buf) < 0)
        {
          ret = -errno;
          printf("lvglcam: VIDIOC_QUERYBUF failed: %d\n", ret);
          return ret;
        }

      cam->buf[i] = mmap(NULL, buf.length, PROT_READ, MAP_SHARED,
                         cam->vfd, buf.m.offset);
      if (cam->buf[i] == MAP_FAILED)
        {
          cam->buf[i] = NULL;
          ret = -errno;
          printf("lvglcam: mmap failed: %d\n", ret);
          return ret;
        }

      cam->buflen[i] = buf.length;
      if (buf.length < cam->stride * cam->height ||
          lv_draw_buf_init(&cam->dbuf[i], cam->width, cam->height,
                           cam->fmt->cf, cam->stride, cam->buf[i],
                           buf.length) != LV_RESULT_OK)
        {
          printf("lvglcam: capture buffer unusable\n");
          return -EINVAL;
        }

      ret = cam_qbuf(cam, i);
      if (ret < 0)
        {
          return ret;
        }
    }

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(cam->vfd, VIDIOC_STREAMON, (uintptr_t)&type) < 0)
    {
      ret = -errno;
      printf("lvglcam: VIDIOC_STREAMON failed: %d\n", ret);
      return ret;
    }

  cam->streaming = true;
  return OK;
}

/****************************************************************************
 * Name: cam_close
 ****************************************************************************/

static void cam_close(FAR struct lvglcam_s *cam)
{
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int i;

  if (cam->streaming)
    {
      ioctl(cam->vfd, VIDIOC_STREAMOFF, (uintptr_t)&type);
      cam->streaming = false;
    }

  for (i = 0; i < CAM_NBUFFERS; i++)
    {
      if (cam->buf[i] != NULL)
        {
          munmap(cam->buf[i], cam->buflen[i]);
          cam->buf[i] = NULL;
        }
    }

  if (cam->vfd >= 0)
    {
      close(cam->vfd);
      cam->vfd = -1;
    }
}

/****************************************************************************
 * Name: cam_frame
 *
 * Description:
 *   Show the next captured frame and give the previous one back.
 *
 ****************************************************************************/

static int cam_frame(FAR struct lvglcam_s *cam)
{
  struct v4l2_buffer buf;
  int hold = -1;
  int ret;

  memset(&buf, 0, sizeof(buf));
  buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (ioctl(cam->vfd, VIDIOC_DQBUF, (uintptr_t)&buf) < 0)
    {
      ret = -errno;
      printf("lvglcam: VIDIOC_DQBUF failed: %d\n", ret);
      return ret;
    }

  if (buf.index >= CAM_NBUFFERS)
    {
      return -EIO;
    }

  if ((buf.flags & V4L2_BUF_FLAG_ERROR) != 0)
    {
      return cam_qbuf(cam, buf.index);
    }

  lv_canvas_set_draw_buf(cam->canvas, &cam->dbuf[buf.index]);
  lv_obj_invalidate(cam->canvas);
  cam->frames++;

#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
  /* A buffer held for a snapshot is given back by cam_release() */

  pthread_mutex_lock(&cam->lock);
  hold = cam->hold;
  pthread_mutex_unlock(&cam->lock);
#endif

  ret = OK;
  if (cam->shown >= 0 && cam->shown != hold)
    {
      ret = cam_qbuf(cam, cam->shown);
    }

  cam->shown = buf.index;
  return ret;
}

#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
/****************************************************************************
 * Name: cam_release
 *
 * Description:
 *   Give the buffer of a saved snapshot back to the camera, unless it is
 *   still shown.
 *
 ****************************************************************************/

static int cam_release(FAR struct lvglcam_s *cam)
{
  int hold = -1;

  pthread_mutex_lock(&cam->lock);
  if (cam->snapdone)
    {
      hold          = cam->hold;
      cam->hold     = -1;
      cam->snapdone = false;
    }

  pthread_mutex_unlock(&cam->lock);

  if (hold >= 0 && hold != cam->shown)
    {
      return cam_qbuf(cam, hold);
    }

  return OK;
}

/****************************************************************************
 * Name: snap_open
 *
 * Description:
 *   Create the next free snapshot file, never overwrite one.
 *
 ****************************************************************************/

static int snap_open(FAR struct lvglcam_s *cam, FAR char *path, size_t size)
{
  int fd;
  int n;

  for (n = cam->snaps + 1; n <= SNAP_MAX; n++)
    {
      snprintf(path, size, "%s/IMG_%04d.BMP",
               CONFIG_EXAMPLES_LVGLCAM_SNAPDIR, n);

      fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
      if (fd >= 0)
        {
          cam->snaps = n;
          return fd;
        }

      if (errno != EEXIST)
        {
          return -errno;
        }
    }

  path[0] = '\0';
  return -ENOSPC;
}

/****************************************************************************
 * Name: snap_put32
 ****************************************************************************/

static void snap_put32(FAR uint8_t *p, uint32_t value)
{
  p[0] = value;
  p[1] = value >> 8;
  p[2] = value >> 16;
  p[3] = value >> 24;
}

/****************************************************************************
 * Name: snap_save
 *
 * Description:
 *   Save a frame as a top-down BMP.
 *
 ****************************************************************************/

static int snap_save(FAR struct lvglcam_s *cam, FAR const uint8_t *frame,
                     FAR char *path, size_t size)
{
  uint32_t bpp = cam->fmt->bpp;
  uint32_t row = (cam->width * bpp / 8 + 3) & ~3;
  uint32_t hdr = BMP_FILEHDR + BMP_INFOHDR + (bpp == 16 ? BMP_MASKS : 0);
  uint8_t header[BMP_FILEHDR + BMP_INFOHDR + BMP_MASKS];
  FAR uint8_t *line;
  ssize_t nwritten;
  uint32_t y;
  int ret = OK;
  int fd;

  memset(header, 0, sizeof(header));
  header[0] = 'B';
  header[1] = 'M';
  snap_put32(&header[2], hdr + row * cam->height);
  snap_put32(&header[10], hdr);
  snap_put32(&header[14], BMP_INFOHDR);
  snap_put32(&header[18], cam->width);
  snap_put32(&header[22], -(int32_t)cam->height);
  header[26] = 1;                       /* Planes */
  header[28] = bpp;
  snap_put32(&header[30], bpp == 16 ? BMP_BITFIELDS : BMP_RGB);
  snap_put32(&header[34], row * cam->height);
  if (bpp == 16)
    {
      snap_put32(&header[54], 0xf800);
      snap_put32(&header[58], 0x07e0);
      snap_put32(&header[62], 0x001f);
    }

  line = calloc(1, row);
  if (line == NULL)
    {
      return -ENOMEM;
    }

  fd = snap_open(cam, path, size);
  if (fd < 0)
    {
      free(line);
      return fd;
    }

  if (write(fd, header, hdr) != (ssize_t)hdr)
    {
      ret = -EIO;
    }

  /* One write for unpadded frames, else one per line */

  if (row == cam->stride && ret >= 0)
    {
      nwritten = write(fd, frame, row * cam->height);
      if (nwritten != (ssize_t)(row * cam->height))
        {
          ret = nwritten < 0 ? -errno : -EIO;
        }
    }

  for (y = 0; row != cam->stride && y < cam->height && ret >= 0; y++)
    {
      memcpy(line, frame + y * cam->stride, cam->width * bpp / 8);
      nwritten = write(fd, line, row);
      if (nwritten != (ssize_t)row)
        {
          ret = nwritten < 0 ? -errno : -EIO;
        }
    }

  if (close(fd) < 0 && ret >= 0)
    {
      ret = -errno;
    }

  if (ret < 0)
    {
      unlink(path);
    }

  free(line);
  return ret;
}

/****************************************************************************
 * Name: snap_thread
 *
 * Description:
 *   Save the frames held by the UI, off the LVGL thread.
 *
 ****************************************************************************/

static FAR void *snap_thread(FAR void *arg)
{
  FAR struct lvglcam_s *cam = (FAR struct lvglcam_s *)arg;
  char path[SNAP_PATHLEN];
  int hold;
  int ret;

  for (; ; )
    {
      pthread_mutex_lock(&cam->lock);
      while (cam->running && (cam->hold < 0 || cam->snapdone))
        {
          pthread_cond_wait(&cam->cond, &cam->lock);
        }

      hold = cam->running ? cam->hold : -1;
      pthread_mutex_unlock(&cam->lock);

      if (hold < 0)
        {
          break;
        }

      path[0] = '\0';
      ret = snap_save(cam, cam->buf[hold], path, sizeof(path));

      pthread_mutex_lock(&cam->lock);
      cam->snapdone = true;
      strlcpy(cam->snappath, path, sizeof(cam->snappath));
      cam->snapret = ret;
      cam->snapnew = true;
      pthread_mutex_unlock(&cam->lock);
    }

  return NULL;
}

/****************************************************************************
 * Name: snap_cb
 *
 * Description:
 *   Hold the shown frame for the snapshot thread.  The camera keeps the
 *   other buffers.
 *
 ****************************************************************************/

static void snap_cb(FAR lv_event_t *e)
{
  FAR struct lvglcam_s *cam = lv_event_get_user_data(e);
  bool busy;

  pthread_mutex_lock(&cam->lock);
  busy = cam->hold >= 0 || cam->shown < 0;
  if (!busy)
    {
      cam->hold = cam->shown;
      pthread_cond_broadcast(&cam->cond);
    }

  pthread_mutex_unlock(&cam->lock);

  if (!busy)
    {
      lv_label_set_text(cam->snap, LV_SYMBOL_SAVE " saving...");
    }
}
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_DEPTH
/****************************************************************************
 * Name: depth_setfmt
 *
 * Description:
 *   Set the largest Z16 frame of up to DEPTH_MAX x DEPTH_MAX zones.
 *
 ****************************************************************************/

static int depth_setfmt(FAR struct lvglcam_s *cam, int fd)
{
  struct v4l2_frmsizeenum size;
  struct v4l2_format fmt;
  uint32_t w = 0;
  uint32_t h = 0;

  memset(&size, 0, sizeof(size));
  size.pixel_format = V4L2_PIX_FMT_Z16;

  while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, (uintptr_t)&size) >= 0 &&
         size.type == V4L2_FRMSIZE_TYPE_DISCRETE)
    {
      if (size.discrete.width <= DEPTH_MAX &&
          size.discrete.height <= DEPTH_MAX &&
          size.discrete.width * size.discrete.height > w * h)
        {
          w = size.discrete.width;
          h = size.discrete.height;
        }

      size.index++;
    }

  if (w == 0)
    {
      return -ENOTSUP;
    }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = w;
  fmt.fmt.pix.height      = h;
  fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_Z16;

  if (ioctl(fd, VIDIOC_S_FMT, (uintptr_t)&fmt) < 0)
    {
      return -errno;
    }

  pthread_mutex_lock(&cam->lock);
  cam->depthw = w;
  cam->depthh = h;
  pthread_mutex_unlock(&cam->lock);

  return OK;
}

/****************************************************************************
 * Name: depth_thread
 *
 * Description:
 *   Stream the depth camera and keep its latest frame for the UI.  It runs
 *   in its own thread because a depth camera can take a while to start.
 *
 ****************************************************************************/

static FAR void *depth_thread(FAR void *arg)
{
  FAR struct lvglcam_s *cam = (FAR struct lvglcam_s *)arg;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  FAR uint8_t *mem[DEPTH_NBUFFERS];
  size_t memlen[DEPTH_NBUFFERS];
  struct v4l2_requestbuffers req;
  struct v4l2_buffer buf;
  bool streaming = false;
  size_t len;
  int ret = OK;
  int fd;
  int i;

  memset(mem, 0, sizeof(mem));

  fd = open(CONFIG_EXAMPLES_LVGLCAM_DEPTH_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      ret = -errno;
      goto errout;
    }

  pthread_mutex_lock(&cam->lock);
  cam->depthfd = fd;
  pthread_mutex_unlock(&cam->lock);

  ret = depth_setfmt(cam, fd);
  if (ret < 0)
    {
      goto errout;
    }

  memset(&req, 0, sizeof(req));
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  req.count  = DEPTH_NBUFFERS;
  if (ioctl(fd, VIDIOC_REQBUFS, (uintptr_t)&req) < 0)
    {
      ret = -errno;
      goto errout;
    }

  for (i = 0; i < DEPTH_NBUFFERS; i++)
    {
      memset(&buf, 0, sizeof(buf));
      buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index  = i;
      if (ioctl(fd, VIDIOC_QUERYBUF, (uintptr_t)&buf) < 0)
        {
          ret = -errno;
          goto errout;
        }

      mem[i] = mmap(NULL, buf.length, PROT_READ, MAP_SHARED, fd,
                    buf.m.offset);
      if (mem[i] == MAP_FAILED)
        {
          mem[i] = NULL;
          ret = -errno;
          goto errout;
        }

      memlen[i] = buf.length;
      if (ioctl(fd, VIDIOC_QBUF, (uintptr_t)&buf) < 0)
        {
          ret = -errno;
          goto errout;
        }
    }

  if (ioctl(fd, VIDIOC_STREAMON, (uintptr_t)&type) < 0)
    {
      ret = -errno;
      goto errout;
    }

  streaming = true;
  len = cam->depthw * cam->depthh * sizeof(uint16_t);

  for (; ; )
    {
      pthread_mutex_lock(&cam->lock);
      if (!cam->running)
        {
          pthread_mutex_unlock(&cam->lock);
          break;
        }

      pthread_mutex_unlock(&cam->lock);

      memset(&buf, 0, sizeof(buf));
      buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      if (ioctl(fd, VIDIOC_DQBUF, (uintptr_t)&buf) < 0)
        {
          if (errno != ECANCELED && errno != EINTR)
            {
              ret = -errno;
            }

          break;
        }

      if (buf.index < DEPTH_NBUFFERS &&
          (buf.flags & V4L2_BUF_FLAG_ERROR) == 0 && buf.bytesused >= len)
        {
          pthread_mutex_lock(&cam->lock);
          memcpy(cam->depthmm, mem[buf.index], len);
          cam->depthframes++;
          pthread_mutex_unlock(&cam->lock);
        }

      if (ioctl(fd, VIDIOC_QBUF, (uintptr_t)&buf) < 0)
        {
          ret = -errno;
          break;
        }
    }

errout:
  if (streaming)
    {
      ioctl(fd, VIDIOC_STREAMOFF, (uintptr_t)&type);
    }

  for (i = 0; i < DEPTH_NBUFFERS; i++)
    {
      if (mem[i] != NULL)
        {
          munmap(mem[i], memlen[i]);
        }
    }

  pthread_mutex_lock(&cam->lock);
  if (cam->depthfd >= 0)
    {
      close(cam->depthfd);
      cam->depthfd = -1;
    }

  cam->deptherr  = ret;
  cam->depthdone = true;
  pthread_cond_broadcast(&cam->cond);
  pthread_mutex_unlock(&cam->lock);

  return NULL;
}

/****************************************************************************
 * Name: depth_update
 *
 * Description:
 *   Show the latest depth frame: near is red, far is blue, no target is
 *   left dark.
 *
 ****************************************************************************/

static void depth_update(FAR struct lvglcam_s *cam)
{
  uint16_t mm[DEPTH_MAX * DEPTH_MAX];
  uint32_t frames;
  uint32_t zones;
  uint32_t w;
  uint32_t h;
  uint16_t min = UINT16_MAX;
  uint16_t max = 0;
  bool done;
  uint32_t i;
  int err;
  int d;

  pthread_mutex_lock(&cam->lock);
  w      = cam->depthw;
  h      = cam->depthh;
  zones  = w * h;
  memcpy(mm, cam->depthmm, zones * sizeof(uint16_t));
  frames = cam->depthframes;
  done   = cam->depthdone;
  err    = cam->deptherr;
  pthread_mutex_unlock(&cam->lock);

  if (done && err == -ENOENT)
    {
      lv_obj_add_flag(cam->depthcard, LV_OBJ_FLAG_HIDDEN);
      return;
    }

  if (done && err < 0)
    {
      lv_label_set_text_fmt(cam->depthinfo, "error %d", err);
      return;
    }

  if (frames == cam->depthshown || zones == 0)
    {
      return;
    }

  cam->depthshown = frames;

  if (cam->depthmap == NULL)
    {
      cam->depthmap = lv_canvas_create(cam->depthcard);
      lv_canvas_set_buffer(cam->depthmap, cam->depthpix, w, h,
                           LV_COLOR_FORMAT_RGB565);
      lv_image_set_antialias(cam->depthmap, false);
      lv_obj_set_size(cam->depthmap, DEPTH_VIEW * w / MAX(w, h),
                      DEPTH_VIEW * h / MAX(w, h));
      lv_image_set_inner_align(cam->depthmap, LV_IMAGE_ALIGN_STRETCH);
    }

  for (i = 0; i < zones; i++)
    {
      lv_color_t color = COLOR_BG;

      if (mm[i] != 0)
        {
          d = MIN(mm[i], CONFIG_EXAMPLES_LVGLCAM_DEPTH_FAR);
          color = lv_color_hsv_to_rgb(DEPTH_HUE_FAR * d /
                                      CONFIG_EXAMPLES_LVGLCAM_DEPTH_FAR,
                                      100, 100);
          min = MIN(mm[i], min);
          max = MAX(mm[i], max);
        }

      cam->depthpix[i] = lv_color_to_u16(color);
    }

  lv_obj_invalidate(cam->depthmap);

  if (max == 0)
    {
      lv_label_set_text_fmt(cam->depthinfo, "%" PRIu32 "x%" PRIu32
                            "  no target", w, h);
    }
  else
    {
      lv_label_set_text_fmt(cam->depthinfo, "%" PRIu32 "x%" PRIu32
                            "  %u-%u mm", w, h, min, max);
    }
}

/****************************************************************************
 * Name: depth_stop
 ****************************************************************************/

static void depth_stop(FAR struct lvglcam_s *cam)
{
  struct timespec ts;

  if (!cam->depthstarted)
    {
      return;
    }

  pthread_mutex_lock(&cam->lock);
  while (!cam->depthdone)
    {
      if (cam->depthfd >= 0)
        {
          ioctl(cam->depthfd, VIDIOC_CANCEL_DQBUF,
                V4L2_BUF_TYPE_VIDEO_CAPTURE);
        }

      clock_gettime(CLOCK_MONOTONIC, &ts);
      ts.tv_nsec += DEPTH_STOP_MS * 1000000;
      if (ts.tv_nsec >= 1000000000)
        {
          ts.tv_sec++;
          ts.tv_nsec -= 1000000000;
        }

      pthread_cond_timedwait(&cam->cond, &cam->lock, &ts);
    }

  pthread_mutex_unlock(&cam->lock);
  pthread_join(cam->depththread, NULL);
  cam->depthstarted = false;
}
#endif

/****************************************************************************
 * Name: cpuload
 ****************************************************************************/

static int cpuload(void)
{
  char line[64];
  int load = -1;
  FAR FILE *fp;

  fp = fopen("/proc/cpuload", "r");
  if (fp != NULL)
    {
      if (fgets(line, sizeof(line), fp) != NULL)
        {
          load = (int)(atof(line) + 0.5);
        }

      fclose(fp);
    }

  return load;
}

/****************************************************************************
 * Name: ui_card
 ****************************************************************************/

static FAR lv_obj_t *ui_card(FAR lv_obj_t *parent, FAR const char *title,
                             FAR lv_obj_t **body)
{
  FAR lv_obj_t *card;
  FAR lv_obj_t *label;

  card = lv_obj_create(parent);
  lv_obj_set_width(card, LV_PCT(100));
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
  lv_obj_set_style_text_color(card, COLOR_TEXT, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_radius(card, UI_RADIUS, 0);
  lv_obj_set_style_pad_all(card, UI_PAD, 0);
  lv_obj_set_style_pad_row(card, UI_ROW, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  label = lv_label_create(card);
  lv_label_set_text(label, title);
  lv_obj_set_style_text_color(label, COLOR_ACCENT, 0);

  if (body != NULL)
    {
      *body = lv_label_create(card);
      lv_label_set_text(*body, "-");
    }

  return card;
}

/****************************************************************************
 * Name: ui_timer
 *
 * Description:
 *   Refresh the live data and the snapshot result.
 *
 ****************************************************************************/

static void ui_timer(FAR lv_timer_t *timer)
{
  FAR struct lvglcam_s *cam = lv_timer_get_user_data(timer);
  struct mallinfo mem = mallinfo();
  struct timespec ts;
  uint32_t now = lv_tick_get();
#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
  char path[SNAP_PATHLEN];
  bool snapnew;
  int snapret;
#endif
  int load;

  if (now != cam->lastms)
    {
      lv_label_set_text_fmt(cam->info, LV_SYMBOL_VIDEO " %" PRIu32 "x%"
                            PRIu32 "  %" PRIu32 " fps", cam->width,
                            cam->height, (cam->frames - cam->lastframes) *
                            1000 / (now - cam->lastms));
    }

  cam->lastframes = cam->frames;
  cam->lastms     = now;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  load = cpuload();
  if (load >= 0)
    {
      lv_label_set_text_fmt(cam->sys, "Uptime  %02u:%02u:%02u\n"
                            "CPU     %d%%\nFree    %u KiB",
                            (unsigned)(ts.tv_sec / 3600),
                            (unsigned)(ts.tv_sec / 60 % 60),
                            (unsigned)(ts.tv_sec % 60), load,
                            (unsigned)(mem.fordblks / 1024));
    }
  else
    {
      lv_label_set_text_fmt(cam->sys, "Uptime  %02u:%02u:%02u\n"
                            "Free    %u KiB",
                            (unsigned)(ts.tv_sec / 3600),
                            (unsigned)(ts.tv_sec / 60 % 60),
                            (unsigned)(ts.tv_sec % 60),
                            (unsigned)(mem.fordblks / 1024));
    }

#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
  if (cam->snap != NULL)
    {
      pthread_mutex_lock(&cam->lock);
      snapnew = cam->snapnew;
      snapret = cam->snapret;
      strlcpy(path, cam->snappath, sizeof(path));
      cam->snapnew = false;
      pthread_mutex_unlock(&cam->lock);

      if (snapnew && snapret >= 0)
        {
          lv_label_set_text_fmt(cam->snap, LV_SYMBOL_OK " %s", path);
        }
      else if (snapnew)
        {
          lv_label_set_text_fmt(cam->snap, LV_SYMBOL_WARNING
                                " save failed: %d", snapret);
        }
    }
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_ACCEL
  if (cam->accfd >= 0)
    {
      struct sensor_accel acc;

      if (read(cam->accfd, &acc, sizeof(acc)) == sizeof(acc))
        {
          float norm  = sqrtf(acc.x * acc.x + acc.y * acc.y +
                              acc.z * acc.z);
          float roll  = atan2f(acc.y, acc.z) * RAD_TO_DEG;
          float pitch = atan2f(-acc.x, sqrtf(acc.y * acc.y +
                                             acc.z * acc.z)) * RAD_TO_DEG;

          lv_label_set_text_fmt(cam->acc, "Roll   %4d deg\n"
                                "Pitch  %4d deg\n|a|    %d.%02d m/s2",
                                (int)roll, (int)pitch, (int)norm,
                                (int)(fmodf(norm, 1.0f) * 100));
          lv_chart_set_next_value(cam->chart, cam->series[0],
                                  (int32_t)(acc.x * CHART_SCALE));
          lv_chart_set_next_value(cam->chart, cam->series[1],
                                  (int32_t)(acc.y * CHART_SCALE));
          lv_chart_set_next_value(cam->chart, cam->series[2],
                                  (int32_t)(acc.z * CHART_SCALE));
        }
    }
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_NET
  if (cam->net != NULL)
    {
      FAR const char *addrstr = "no address";
      uint8_t flags = 0;
#  ifdef CONFIG_NET_IPv4
      struct in_addr addr;

      if (netlib_get_ipv4addr(CONFIG_EXAMPLES_LVGLCAM_NETDEV, &addr) >= 0 &&
          addr.s_addr != INADDR_ANY)
        {
          addrstr = inet_ntoa(addr);
        }
#  endif

      netlib_getifstatus(CONFIG_EXAMPLES_LVGLCAM_NETDEV, &flags);
      lv_label_set_text_fmt(cam->net, "%s  %s\n%s",
                            CONFIG_EXAMPLES_LVGLCAM_NETDEV,
                            (flags & IFF_RUNNING) ? "link up" : "link down",
                            addrstr);
    }
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_DEPTH
  depth_update(cam);
#endif
}

/****************************************************************************
 * Name: ui_create
 *
 * Description:
 *   Create the title, the view card and the data column.  Cards of
 *   devices that do not open are left out.
 *
 ****************************************************************************/

static void ui_create(FAR struct lvglcam_s *cam)
{
  FAR lv_obj_t *scr = lv_screen_active();
  int32_t xres = lv_display_get_horizontal_resolution(NULL);
  int32_t yres = lv_display_get_vertical_resolution(NULL);
  bool wide = xres >= yres;
  FAR lv_obj_t *body;
  FAR lv_obj_t *card;
  FAR lv_obj_t *row;
#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
  FAR lv_obj_t *btn;
#endif
  FAR lv_obj_t *label;
  struct utsname name;

  lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
  lv_obj_set_style_text_color(scr, COLOR_TEXT, 0);
  lv_obj_set_style_pad_all(scr, UI_ROW, 0);
  lv_obj_set_style_pad_row(scr, UI_ROW, 0);
  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  label = lv_label_create(scr);
  if (uname(&name) >= 0)
    {
      lv_label_set_text_fmt(label, "NuttX %s  |  %s  |  LVGL %d.%d",
                            name.release, name.machine, LVGL_VERSION_MAJOR,
                            LVGL_VERSION_MINOR);
    }
  else
    {
      lv_label_set_text_fmt(label, "NuttX  |  LVGL %d.%d",
                            LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR);
    }

  lv_obj_set_style_text_color(label, COLOR_DIM, 0);

  body = lv_obj_create(scr);
  lv_obj_remove_style_all(body);
  lv_obj_set_width(body, LV_PCT(100));
  lv_obj_set_flex_grow(body, 1);
  lv_obj_set_style_pad_column(body, UI_ROW, 0);
  lv_obj_set_style_pad_row(body, UI_ROW, 0);
  lv_obj_set_flex_flow(body, wide ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);

  /* View card: the frames, clipped to the card, and their info */

  card = lv_obj_create(body);
  lv_obj_set_flex_grow(card, 1);
  lv_obj_set_size(card, wide ? 0 : LV_PCT(100), wide ? LV_PCT(100) : 0);
  lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
  lv_obj_set_style_text_color(card, COLOR_TEXT, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_radius(card, UI_RADIUS, 0);
  lv_obj_set_style_pad_all(card, UI_PAD, 0);
  lv_obj_set_style_pad_row(card, UI_ROW, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  cam->view = lv_obj_create(card);
  lv_obj_remove_style_all(cam->view);
  lv_obj_set_width(cam->view, LV_PCT(100));
  lv_obj_set_flex_grow(cam->view, 1);
  lv_obj_remove_flag(cam->view, LV_OBJ_FLAG_SCROLLABLE);

  /* Bottom row: frame info, snapshot result and the snapshot button */

  row = lv_obj_create(card);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_column(row, UI_PAD, 0);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  cam->info = lv_label_create(row);
  lv_label_set_text(cam->info, LV_SYMBOL_VIDEO " starting...");

#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
  cam->snap = lv_label_create(row);
  lv_obj_set_flex_grow(cam->snap, 1);
  lv_label_set_long_mode(cam->snap, LV_LABEL_LONG_DOT);
  lv_label_set_text(cam->snap, "");
  lv_obj_set_style_text_color(cam->snap, COLOR_DIM, 0);
  lv_obj_set_style_text_align(cam->snap, LV_TEXT_ALIGN_RIGHT, 0);

  btn = lv_button_create(row);
  lv_obj_set_style_bg_color(btn, COLOR_ACCENT, 0);
  lv_obj_add_event_cb(btn, snap_cb, LV_EVENT_CLICKED, cam);
  label = lv_label_create(btn);
  lv_label_set_text(label, LV_SYMBOL_IMAGE " Save");
#endif

  /* Data column */

  cam->side = lv_obj_create(body);
  lv_obj_remove_style_all(cam->side);
  if (wide)
    {
      lv_obj_set_size(cam->side, MAX(UI_SIDE_MIN, xres / UI_SIDE_DIV),
                      LV_PCT(100));
    }
  else
    {
      lv_obj_set_size(cam->side, LV_PCT(100), yres / UI_SIDE_DIV);
    }

  lv_obj_set_style_pad_row(cam->side, UI_ROW, 0);
  lv_obj_set_flex_flow(cam->side, LV_FLEX_FLOW_COLUMN);

  ui_card(cam->side, LV_SYMBOL_SETTINGS " System", &cam->sys);

#ifdef CONFIG_EXAMPLES_LVGLCAM_NET
  {
    uint8_t flags;

    if (netlib_getifstatus(CONFIG_EXAMPLES_LVGLCAM_NETDEV, &flags) >= 0)
      {
        ui_card(cam->side, LV_SYMBOL_WIFI " Network", &cam->net);
      }
  }
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_ACCEL
  if (cam->accfd >= 0)
    {
      card = ui_card(cam->side, LV_SYMBOL_REFRESH " Accelerometer",
                     &cam->acc);
      cam->chart = lv_chart_create(card);
      lv_obj_set_size(cam->chart, LV_PCT(100), CHART_H);
      lv_chart_set_point_count(cam->chart, CHART_POINTS);
      lv_chart_set_range(cam->chart, LV_CHART_AXIS_PRIMARY_Y, -CHART_RANGE,
                         CHART_RANGE);
      lv_chart_set_div_line_count(cam->chart, CHART_DIVS, 0);
      lv_obj_set_style_bg_opa(cam->chart, LV_OPA_TRANSP, 0);
      lv_obj_set_style_border_width(cam->chart, 0, 0);
      lv_obj_set_style_size(cam->chart, 0, 0, LV_PART_INDICATOR);
      cam->series[0] = lv_chart_add_series(cam->chart,
                                        lv_palette_main(LV_PALETTE_RED),
                                        LV_CHART_AXIS_PRIMARY_Y);
      cam->series[1] = lv_chart_add_series(cam->chart,
                                        lv_palette_main(LV_PALETTE_GREEN),
                                        LV_CHART_AXIS_PRIMARY_Y);
      cam->series[2] = lv_chart_add_series(cam->chart,
                                        lv_palette_main(LV_PALETTE_BLUE),
                                        LV_CHART_AXIS_PRIMARY_Y);
    }
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_DEPTH
  cam->depthcard = ui_card(cam->side, LV_SYMBOL_EYE_OPEN " Depth",
                           &cam->depthinfo);
  lv_label_set_text(cam->depthinfo, "starting...");
#endif

  lv_obj_update_layout(scr);
}

/****************************************************************************
 * Name: ui_view
 *
 * Description:
 *   Create the canvas that shows the frames, centred in the view.
 *
 ****************************************************************************/

static void ui_view(FAR struct lvglcam_s *cam)
{
  cam->canvas = lv_canvas_create(cam->view);
  lv_obj_set_size(cam->canvas, cam->width, cam->height);
  lv_obj_center(cam->canvas);
  lv_timer_create(ui_timer, UI_PERIOD_MS, cam);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct lvglcam_s *cam = &g_cam;
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  pthread_condattr_t cattr;
  struct sigaction act;
  struct pollfd pfd;
  uint32_t idle;
  int ret;

  if (lv_is_initialized())
    {
      printf("lvglcam: LVGL already initialized\n");
      return EXIT_FAILURE;
    }

  memset(cam, 0, sizeof(*cam));
  memset(&result, 0, sizeof(result));
  cam->vfd     = -1;
  cam->shown   = -1;
#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
  cam->hold    = -1;
#endif
  cam->running = true;
#ifdef CONFIG_EXAMPLES_LVGLCAM_ACCEL
  cam->accfd   = -1;
#endif
#ifdef CONFIG_EXAMPLES_LVGLCAM_DEPTH
  cam->depthfd = -1;
#endif

  pthread_mutex_init(&cam->lock, NULL);
  pthread_condattr_init(&cattr);
  pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
  pthread_cond_init(&cam->cond, &cattr);
  pthread_condattr_destroy(&cattr);

  /* Stop with SIGINT or SIGTERM */

  g_stop = 0;
  memset(&act, 0, sizeof(act));
  act.sa_handler = lvglcam_signal;
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);

  lv_init();
  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif

#ifdef CONFIG_INPUT_TOUCHSCREEN
  info.input_path = CONFIG_EXAMPLES_LVGLCAM_INPUT_DEVPATH;
#endif

  lv_nuttx_init(&info, &result);
  if (result.disp == NULL)
    {
      printf("lvglcam: LVGL display initialization failed\n");
      ret = -ENODEV;
      goto errout;
    }

  ret = cam_open(cam);
  if (ret < 0)
    {
      goto errout;
    }

#ifdef CONFIG_EXAMPLES_LVGLCAM_ACCEL
  cam->accfd = open(CONFIG_EXAMPLES_LVGLCAM_ACCEL_DEVPATH,
                    O_RDONLY | O_NONBLOCK);
  if (cam->accfd >= 0)
    {
      ioctl(cam->accfd, SNIOC_SET_INTERVAL, ACCEL_INTERVAL_US);
    }
#endif

  ui_create(cam);

  ret = cam_size(cam, lv_obj_get_content_width(cam->view),
                 lv_obj_get_content_height(cam->view));
  if (ret < 0)
    {
      goto errout;
    }

  ret = cam_start(cam);
  if (ret < 0)
    {
      goto errout;
    }

  ui_view(cam);

#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
  if (cam->snap != NULL)
    {
      ret = pthread_create(&cam->snapthread, NULL, snap_thread, cam);
      if (ret != 0)
        {
          printf("lvglcam: pthread_create() failed: %d\n", ret);
          ret = -ret;
          goto errout;
        }

      cam->snapstarted = true;
    }
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_DEPTH
  ret = pthread_create(&cam->depththread, NULL, depth_thread, cam);
  if (ret != 0)
    {
      printf("lvglcam: pthread_create() failed: %d\n", ret);
      ret = -ret;
      goto errout;
    }

  cam->depthstarted = true;
#endif

  /* Render, then wait for a frame until the next LVGL timer is due */

  pfd.fd     = cam->vfd;
  pfd.events = POLLIN;

  while (!g_stop)
    {
      idle = lv_timer_handler();

      pfd.revents = 0;
      ret = poll(&pfd, 1, LV_CLAMP(1, idle, UI_PERIOD_MS));
      if (ret < 0 && errno != EINTR)
        {
          ret = -errno;
          printf("lvglcam: poll failed: %d\n", ret);
          break;
        }

      if (ret > 0 && (pfd.revents & POLLIN) != 0)
        {
          ret = cam_frame(cam);
          if (ret < 0)
            {
              break;
            }
        }

#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
      ret = cam_release(cam);
      if (ret < 0)
        {
          break;
        }
#endif
    }

  ret = g_stop ? OK : ret;

errout:
  pthread_mutex_lock(&cam->lock);
  cam->running = false;
  pthread_cond_broadcast(&cam->cond);
  pthread_mutex_unlock(&cam->lock);

#ifdef CONFIG_EXAMPLES_LVGLCAM_DEPTH
  depth_stop(cam);
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_SNAPSHOT
  if (cam->snapstarted)
    {
      pthread_join(cam->snapthread, NULL);
    }
#endif

  cam_close(cam);

#ifdef CONFIG_EXAMPLES_LVGLCAM_ACCEL
  if (cam->accfd >= 0)
    {
      close(cam->accfd);
    }
#endif

  lv_nuttx_deinit(&result);
  lv_deinit();

  pthread_cond_destroy(&cam->cond);
  pthread_mutex_destroy(&cam->lock);

  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
