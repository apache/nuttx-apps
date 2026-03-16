/****************************************************************************
 * apps/examples/uvc_cam/uvc_cam_main.c
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
 * USB Video Class camera streaming application.
 * Captures frames from V4L2 camera and streams to /dev/uvc0.
 *
 * Queries the sensor's native pixel format, resolution and frame rate
 * via V4L2, then streams frames to the UVC gadget.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/boardctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <nuttx/video/video.h>
#include <nuttx/video/v4l2_cap.h>
#include <nuttx/usb/uvc.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define UVC_DEV_DEFAULT  "/dev/uvc0"
#define VIDEO_DEV_DEFAULT "/dev/video0"

#ifndef CONFIG_EXAMPLES_UVC_CAM_NFRAMES
#  define CONFIG_EXAMPLES_UVC_CAM_NFRAMES 0  /* 0 = infinite */
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pixfmt_to_bpp
 *
 * Description:
 *   Return bytes-per-pixel for common V4L2 pixel formats.
 *
 ****************************************************************************/

static int pixfmt_to_bpp(uint32_t pixfmt)
{
  switch (pixfmt)
    {
      case V4L2_PIX_FMT_YUYV:
      case V4L2_PIX_FMT_RGB565:
        return 2;
      case V4L2_PIX_FMT_RGB24:
        return 3;
      default:
        return 0;
    }
}

/****************************************************************************
 * Name: camera_init
 *
 * Description:
 *   Open and configure the V4L2 camera device.
 *   Queries the sensor's native pixel format, resolution and frame rate
 *   and returns them via output parameters.
 *
 ****************************************************************************/

static int camera_init(int fd, FAR uint32_t *pixfmt, FAR uint16_t *width,
                       FAR uint16_t *height, FAR uint16_t *fps)
{
  struct v4l2_fmtdesc fmtdesc;
  struct v4l2_frmivalenum frmival;
  struct v4l2_frmsizeenum frmsize;
  struct v4l2_format fmt;
  struct v4l2_requestbuffers req;
  int ret;

  /* Query native pixel format from sensor */

  memset(&fmtdesc, 0, sizeof(fmtdesc));
  fmtdesc.index = 0;
  fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc);
  if (ret < 0)
    {
      printf("VIDIOC_ENUM_FMT failed: %d\n", errno);
      return -errno;
    }

  printf("Sensor format: %.4s\n", (FAR const char *)&fmtdesc.pixelformat);

  /* Query native resolution from sensor */

  memset(&frmsize, 0, sizeof(frmsize));
  frmsize.index        = 0;
  frmsize.buf_type     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  frmsize.pixel_format = fmtdesc.pixelformat;

  ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize);
  if (ret < 0)
    {
      printf("VIDIOC_ENUM_FRAMESIZES failed: %d\n", errno);
      return -errno;
    }

  printf("Sensor native: %dx%d\n",
         frmsize.discrete.width, frmsize.discrete.height);

  /* Set format — capture at native resolution */

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = frmsize.discrete.width;
  fmt.fmt.pix.height      = frmsize.discrete.height;
  fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = fmtdesc.pixelformat;

  ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
  if (ret < 0)
    {
      printf("VIDIOC_S_FMT failed: %d\n", errno);
      return -errno;
    }

  /* Request buffers — match camcap: USERPTR + FIFO */

  memset(&req, 0, sizeof(req));
  req.count  = 1;
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;
  req.mode   = V4L2_BUF_MODE_FIFO;

  ret = ioctl(fd, VIDIOC_REQBUFS, &req);
  if (ret < 0)
    {
      printf("VIDIOC_REQBUFS failed: %d\n", errno);
      return -errno;
    }

  /* NOTE: Do NOT call VIDIOC_STREAMON here.
   * With USERPTR + FIFO mode, STREAMON would start DMA capture
   * immediately using the container's uninitialized userptr (0),
   * causing DMA to write 153KB of video data to address 0 and
   * corrupt memory.  STREAMON must be called AFTER the first QBUF
   * so that userptr points to a valid buffer.
   */

  /* Query frame interval from sensor */

  memset(&frmival, 0, sizeof(frmival));
  frmival.index        = 0;
  frmival.buf_type     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  frmival.pixel_format = fmtdesc.pixelformat;
  frmival.width        = frmsize.discrete.width;
  frmival.height       = frmsize.discrete.height;

  ret = ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival);
  if (ret < 0)
    {
      printf("VIDIOC_ENUM_FRAMEINTERVALS failed: %d\n", errno);
      return -errno;
    }

  *pixfmt = fmtdesc.pixelformat;
  *width  = frmsize.discrete.width;
  *height = frmsize.discrete.height;
  *fps    = frmival.discrete.numerator > 0
          ? frmival.discrete.denominator / frmival.discrete.numerator
          : 0;
  return OK;
}

/****************************************************************************
 * Name: camera_capture_frame
 *
 * Description:
 *   Capture one frame from V4L2 into the provided buffer.
 *
 ****************************************************************************/

static int camera_capture_frame(int camfd, FAR uint8_t *buf, size_t buflen,
                                bool do_streamon)
{
  struct v4l2_buffer vbuf;
  int ret;

  memset(&vbuf, 0, sizeof(vbuf));
  vbuf.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vbuf.memory    = V4L2_MEMORY_USERPTR;
  vbuf.index     = 0;
  vbuf.m.userptr = (uintptr_t)buf;
  vbuf.length    = buflen;

  ret = ioctl(camfd, VIDIOC_QBUF, &vbuf);
  if (ret < 0)
    {
      printf("VIDIOC_QBUF failed: %d\n", errno);
      return -errno;
    }

  /* Start streaming after the first QBUF so DMA has a valid buffer */

  if (do_streamon)
    {
      enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      ret = ioctl(camfd, VIDIOC_STREAMON, &type);
      if (ret < 0)
        {
          printf("VIDIOC_STREAMON failed: %d\n", errno);
          return -errno;
        }
    }

  ret = ioctl(camfd, VIDIOC_DQBUF, &vbuf);
  if (ret < 0)
    {
      printf("VIDIOC_DQBUF failed: %d\n", errno);
      return -errno;
    }

  return vbuf.bytesused > 0 ? (int)vbuf.bytesused : (int)buflen;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct boardioc_usbdev_ctrl_s ctrl;
  struct uvc_params_s uvc_params;
  FAR void *uvc_handle = NULL;
  FAR const char *video_dev = VIDEO_DEV_DEFAULT;
  FAR const char *uvc_dev = UVC_DEV_DEFAULT;
  FAR uint8_t *framebuf;
  ssize_t written;
  enum v4l2_buf_type cap_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  uint32_t pixfmt = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t fps = 0;
  uint32_t frame_size;
  int bpp;
  int camfd;
  int uvcfd;
  int nframes;
  int count = 0;
  int ret;

  nframes = CONFIG_EXAMPLES_UVC_CAM_NFRAMES;
  if (argc > 1)
    {
      nframes = atoi(argv[1]);
    }

  if (argc > 2)
    {
      video_dev = argv[2];
    }

  if (argc > 3)
    {
      uvc_dev = argv[3];
    }

  /* Create capture device */

  ret = capture_initialize(video_dev);
  if (ret < 0 && ret != -EEXIST)
    {
      printf("Failed to initialize capture: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* Open camera and query resolution */

  camfd = open(video_dev, O_RDWR);
  if (camfd < 0)
    {
      printf("Failed to open %s: %d\n", video_dev, errno);
      return EXIT_FAILURE;
    }

  ret = camera_init(camfd, &pixfmt, &width, &height, &fps);
  if (ret < 0)
    {
      printf("Camera init failed: %d\n", ret);
      close(camfd);
      return EXIT_FAILURE;
    }

  bpp = pixfmt_to_bpp(pixfmt);
  if (bpp == 0)
    {
      printf("Unsupported pixel format: 0x%08lx\n",
             (unsigned long)pixfmt);
      close(camfd);
      return EXIT_FAILURE;
    }

  frame_size = (uint32_t)width * height * bpp;
  printf("UVC Camera: %dx%d %.4s @%dfps, frames=%d (0=infinite)\n",
         width, height, (FAR const char *)&pixfmt, fps, nframes);

  /* Allocate frame buffer — 32-byte aligned for DMA (same as camcap) */

  framebuf = memalign(32, frame_size);
  if (!framebuf)
    {
      printf("Failed to allocate frame buffer (%lu bytes)\n",
             (unsigned long)frame_size);
      close(camfd);
      return EXIT_FAILURE;
    }

  /* Initialize and connect UVC gadget via boardctl.
   * Pass video params queried from camera so USB descriptors
   * match the actual sensor resolution.
   */

  uvc_params.width  = width;
  uvc_params.height = height;
  uvc_params.fps    = fps;

  uvc_handle = (FAR void *)&uvc_params;
#ifdef CONFIG_USBUVC_COMPOSITE
  ctrl.usbdev   = BOARDIOC_USBDEV_COMPOSITE;
#else
  ctrl.usbdev   = BOARDIOC_USBDEV_UVC;
#endif
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = 0;
  ctrl.config   = 0;
  ctrl.handle   = &uvc_handle;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);

  if (ret < 0)
    {
      printf("Failed to initialize UVC device: %d\n", ret);
      close(camfd);
      free(framebuf);
      return EXIT_FAILURE;
    }

  /* Open UVC device */

  uvcfd = open(uvc_dev, O_WRONLY);
  if (uvcfd < 0)
    {
      printf("Failed to open %s: %d\n", uvc_dev, errno);
      ctrl.action = BOARDIOC_USBDEV_DISCONNECT;
      boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
      close(camfd);
      free(framebuf);
      return EXIT_FAILURE;
    }

  /* Wait for USB host to enumerate and start streaming */

  struct pollfd pfd;
  pfd.fd     = uvcfd;
  pfd.events = POLLOUT;

  printf("Waiting for USB host to connect...\n");
  while (poll(&pfd, 1, 500) <= 0 || !(pfd.revents & POLLOUT))
    {
    }

  printf("Streaming started...\n");

  /* Main capture-stream loop */

  while (nframes == 0 || count < nframes)
    {
      ret = camera_capture_frame(camfd, framebuf, frame_size,
                                 count == 0);
      if (ret < 0)
        {
          printf("Capture failed: %d\n", ret);
          break;
        }

      /* Write frame to UVC directly from framebuf.
       * DMA won't touch framebuf until next QBUF.
       */

      written = 0;
      while (written < (ssize_t)frame_size)
        {
          ret = write(uvcfd, framebuf + written,
                      frame_size - written);
          if (ret < 0)
            {
              if (errno == EAGAIN)
                {
                  /* Host stopped streaming - wait for reconnect.
                   *
                   * NOTE: The printf inside the loop is required
                   * on boards where the USB-UART bridge (CH341)
                   * and the ESP32-S3 OTG share an on-board USB
                   * hub (e.g. lckfb-szpi).  Continuous UART TX
                   * keeps the hub active and prevents the host
                   * from power-cycling the hub port, which would
                   * cause a full board POWERON reset.
                   */

                  printf("Host disconnected, waiting...\n");
                  pfd.revents = 0;
                  while (poll(&pfd, 1, 500) <= 0
                         || !(pfd.revents & POLLOUT))
                    {
                      printf(".\n");
                    }

                  printf("Streaming restarted.\n");
                  written = 0;
                  break;
                }

              printf("UVC write failed: %d\n", errno);
              goto out;
            }

          written += ret;
        }

      count++;
      if ((count % 10) == 0)
        {
          printf("Streamed %d frames\n", count);
        }
    }

  printf("Done. Streamed %d frames total.\n", count);

out:
  ioctl(camfd, VIDIOC_STREAMOFF, &cap_type);

  close(uvcfd);
  close(camfd);

  /* Disconnect UVC gadget */

  ctrl.action = BOARDIOC_USBDEV_DISCONNECT;
  boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);

  free(framebuf);
  return EXIT_SUCCESS;
}
