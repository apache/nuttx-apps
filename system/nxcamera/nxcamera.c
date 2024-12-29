/****************************************************************************
 * apps/system/nxcamera/nxcamera.c
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

#include <assert.h>
#include <debug.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include <nuttx/queue.h>
#include <nuttx/video/video.h>
#include <nuttx/video/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>

#include <system/nxcamera.h>

#ifdef CONFIG_LIBYUV
#  include <libyuv.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXCAMERA_STATE_IDLE      0
#define NXCAMERA_STATE_STREAMING 1
#define NXCAMERA_STATE_LOOPING   2
#define NXCAMERA_STATE_PAUSED    3

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * pan_display
 ****************************************************************************/

static void pan_display(int fb_device, FAR struct fb_planeinfo_s *plane_info)
{
  struct pollfd pfd;
  int ret;
  pfd.fd = fb_device;
  pfd.events = POLLOUT;

  ret = poll(&pfd, 1, 0);

  if (ret > 0)
    {
      ioctl(fb_device, FBIOPAN_DISPLAY, plane_info);
    }
}

static int show_image(FAR struct nxcamera_s *pcam, FAR v4l2_buffer_t *buf)
{
#ifdef CONFIG_LIBYUV
  if (pcam->display_vinfo.fmt == FB_FMT_RGB32)
    {
      return ConvertToARGB(pcam->bufs[buf->index],
                           pcam->buf_sizes[buf->index],
                           pcam->display_pinfo.fbmem,
                           pcam->display_pinfo.stride,
                           0,
                           0,
                           pcam->fmt.fmt.pix.width,
                           pcam->fmt.fmt.pix.height,
                           pcam->fmt.fmt.pix.width,
                           pcam->fmt.fmt.pix.height,
                           0,
                           pcam->fmt.fmt.pix.pixelformat);
    }
  else if (pcam->display_vinfo.fmt == FB_FMT_RGB16_565)
    {
      if (pcam->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420)
        {
          return ConvertFromI420(pcam->bufs[buf->index],
                                 pcam->fmt.fmt.pix.width,
                                 &pcam->bufs[buf->index][
                                            pcam->fmt.fmt.pix.width *
                                            pcam->fmt.fmt.pix.height],
                                 pcam->fmt.fmt.pix.width / 2,
                                 &pcam->bufs[buf->index][
                                      pcam->fmt.fmt.pix.width *
                                      pcam->fmt.fmt.pix.height * 5 / 4],
                                 pcam->fmt.fmt.pix.width / 2,
                                 pcam->display_pinfo.fbmem,
                                 pcam->display_pinfo.stride,
                                 pcam->fmt.fmt.pix.width,
                                 pcam->fmt.fmt.pix.height,
                                 V4L2_PIX_FMT_RGB565);
        }
      else
        {
          int ret;
          FAR uint8_t *dst = malloc(pcam->fmt.fmt.pix.width *
                                    pcam->fmt.fmt.pix.height * 3 / 2);
          if (!dst)
            {
              return -ENOMEM;
            }

          ret = ConvertToI420(pcam->bufs[buf->index],
                              pcam->buf_sizes[buf->index],
                              dst,
                              pcam->fmt.fmt.pix.width,
                              &dst[pcam->fmt.fmt.pix.width *
                                        pcam->fmt.fmt.pix.height],
                              pcam->fmt.fmt.pix.width / 2,
                              &dst[pcam->fmt.fmt.pix.width *
                                        pcam->fmt.fmt.pix.height * 5 / 4],
                              pcam->fmt.fmt.pix.width / 2,
                              0,
                              0,
                              pcam->fmt.fmt.pix.width,
                              pcam->fmt.fmt.pix.height,
                              pcam->fmt.fmt.pix.width,
                              pcam->fmt.fmt.pix.height,
                              0,
                              pcam->fmt.fmt.pix.pixelformat);
          if (ret < 0)
            {
              goto err;
            }

          ret = ConvertFromI420(dst,
                                pcam->fmt.fmt.pix.width,
                                &dst[pcam->fmt.fmt.pix.width *
                                            pcam->fmt.fmt.pix.height],
                                pcam->fmt.fmt.pix.width / 2,
                                &dst[pcam->fmt.fmt.pix.width *
                                     pcam->fmt.fmt.pix.height * 5 / 4],
                                pcam->fmt.fmt.pix.width / 2,
                                pcam->display_pinfo.fbmem,
                                pcam->display_pinfo.stride,
                                pcam->fmt.fmt.pix.width,
                                pcam->fmt.fmt.pix.height,
                                V4L2_PIX_FMT_RGB565);
          if (ret < 0)
            {
              goto err;
            }

err:
          free(dst);
          return ret;
        }
    }

  return 0;
#else
  FAR uint32_t *pbuf = (FAR uint32_t *)pcam->bufs[buf->index];
  vinfo("show image from %p: %" PRIx32 " %" PRIx32, pbuf, pbuf[0], pbuf[1]);
  return 0;
#endif
}

/****************************************************************************
 * Name: nxcamera_opendevice
 *
 *   nxcamera_opendevice() tries to open the preferred devices as specified.
 *
 * Return:
 *    OK        if compatible device opened (searched or preferred)
 *    -ENODEV   if no compatible device opened.
 *    -ENOENT   if preferred device couldn't be opened.
 *
 ****************************************************************************/

static int nxcamera_opendevice(FAR struct nxcamera_s *pcam)
{
  int errcode;

  if (pcam->capturedev[0] != '\0')
    {
      pcam->capture_fd = open(pcam->capturedev, O_RDWR);
      if (pcam->capture_fd == -1)
        {
          errcode = errno;
          DEBUGASSERT(errcode > 0);

          verr("ERROR: Failed to open pcam->capturedev %d\n", -errcode);
          return -errcode;
        }

      if (pcam->displaydev[0] != '\0')
        {
          pcam->display_fd = open(pcam->displaydev, O_RDWR);
          if (pcam->display_fd == -1)
            {
              errcode = errno;
              DEBUGASSERT(errcode > 0);

              close(pcam->capture_fd);
              pcam->capture_fd = -1;
              verr("ERROR: Failed to open pcam->displaydev %d\n", -errcode);
              return -errcode;
            }

          errcode = ioctl(pcam->display_fd, FBIOGET_PLANEINFO,
                          ((uintptr_t)&pcam->display_pinfo));

          if (errcode == OK)
            {
              pcam->display_pinfo.fbmem = mmap(NULL,
                                               pcam->display_pinfo.fblen,
                                               PROT_READ | PROT_WRITE,
                                               MAP_SHARED | MAP_FILE,
                                               pcam->display_fd,
                                               0);
            }

          if (errcode < 0 || pcam->display_pinfo.fbmem == MAP_FAILED)
            {
              errcode = errno;
              close(pcam->capture_fd);
              close(pcam->display_fd);
              verr("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n", -errcode);
              return -errcode;
            }

          return OK;
        }
      else
        {
          /* TODO: Add file output */

          return -ENOTSUP;
        }
    }

  return -ENODEV;
}

/****************************************************************************
 * Name: nxcameraer_jointhread
 ****************************************************************************/

static void nxcamera_jointhread(FAR struct nxcamera_s *pcam)
{
  FAR void *value;
  int id = 0;

  if (gettid() == pcam->loop_id)
    {
      return;
    }

  pthread_mutex_lock(&pcam->mutex);

  if (pcam->loop_id > 0)
    {
      id = pcam->loop_id;
      pcam->loop_id = 0;
    }

  pthread_mutex_unlock(&pcam->mutex);

  if (id > 0)
    {
      pthread_join(id, &value);
    }
}

/****************************************************************************
 * Name: nxcamera_loopthread
 *
 *  This is the thread that streams the video and handles video controls.
 *
 ****************************************************************************/

static void *nxcamera_loopthread(pthread_addr_t pvarg)
{
  FAR struct nxcamera_s   *pcam = (FAR struct nxcamera_s *)pvarg;
  unsigned int            prio;
  ssize_t                 size;
  struct video_msg_s      msg;
  bool                    streaming = true;
  int                     i;
  int                     ret;
  struct v4l2_buffer      buf;
  uint32_t                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  vinfo("Entry\n");
  memset(&buf, 0, sizeof(buf));
  for (i = 0; i < pcam->nbuffers; i++)
    {
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = i;
      ret = ioctl(pcam->capture_fd, VIDIOC_QBUF, (uintptr_t)&buf);
      if (ret < 0)
        {
          verr("VIDIOC_QBUF failed: %d\n", ret);
          goto err_out;
        }
    }

  /* VIDIOC_STREAMON start stream */

  ret = ioctl(pcam->capture_fd, VIDIOC_STREAMON, (uintptr_t)&type);
  if (ret < 0)
    {
      verr("VIDIOC_STREAMON failed: %d\n", ret);
      goto err_out;
    }
  else
    {
      pcam->loopstate = NXCAMERA_STATE_STREAMING;
    }

  /* Loop until we specifically break. streaming == true means that we are
   * still looping waiting for the stream to complete.  All of the data
   * may have been sent, but the stream is not complete until we get
   * VIDEO_MSG_STOP message
   *
   * The normal protocol for looping errors detected by the video driver
   * is as follows:
   *
   * (1) The video driver will indicated the error by returning a negated
   *     error value when the next buffer is enqueued.  The upper level
   *     then knows that this buffer was not queued.
   * (2) The video driver must return all queued buffers using the
   *     VIDEO_MSG_DEQUEUE message.
   */

  while (streaming)
    {
      size = mq_receive(pcam->mq, (FAR char *)&msg, sizeof(msg), &prio);

      /* Validate a message was received */

      if (size == sizeof(msg))
        {
          /* Perform operation based on message id */

          vinfo("message received size %zd id%d\n", size, msg.msg_id);
          switch (msg.msg_id)
            {
              /* Someone wants to stop the stream. */

              case VIDEO_MSG_STOP:

                /* Send a stop message to the device */

                vinfo("Stopping looping\n");
                ioctl(pcam->capture_fd, VIDIOC_STREAMOFF, (uintptr_t)&type);
                streaming = false;
                goto err_out;

              /* Unknown / unsupported message ID */

              default:
                break;
            }
        }

      ret = ioctl(pcam->capture_fd, VIDIOC_DQBUF, (uintptr_t)&buf);
      if (ret < 0)
        {
          verr("Fail DQBUF %d\n", errno);
          goto err_out;
        }

      ret = show_image(pcam, &buf);
      if (ret < 0)
        {
          verr("Fail to show image %d\n", -ret);
          goto err_out;
        }

      if (pcam->display_pinfo.yres_virtual > pcam->display_vinfo.yres)
        {
          pan_display(pcam->display_fd, &pcam->display_pinfo);
        }

      ret = ioctl(pcam->capture_fd, VIDIOC_QBUF, (uintptr_t)&buf);
      if (ret < 0)
        {
          verr("Fail QBUF %d\n", errno);
          goto err_out;
        }
    }

  /* Release our video buffers and unregister / release the device */

err_out:
  vinfo("Clean-up and exit\n");

  /* Cleanup */

  pthread_mutex_lock(&pcam->mutex);  /* Lock the mutex */

  close(pcam->display_fd);           /* Close the display device */
  close(pcam->capture_fd);           /* Close the capture device */
  pcam->display_fd = -1;             /* Mark display device as closed */
  pcam->capture_fd = -1;             /* Mark capture device as closed */
  mq_close(pcam->mq);                /* Close the message queue */
  mq_unlink(pcam->mqname);           /* Unlink the message queue */
  pcam->loopstate = NXCAMERA_STATE_IDLE;
  for (i = 0; i < pcam->nbuffers; i++)
    {
      munmap(pcam->bufs[i], pcam->buf_sizes[i]);
    }

  free(pcam->bufs);
  free(pcam->buf_sizes);
  pthread_mutex_unlock(&pcam->mutex);     /* Unlock the mutex */

  vinfo("Exit\n");

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxcamera_setdevice
 *
 *   nxcamera_setdevice() sets the preferred video device to use with the
 *   provided nxcamera context.
 *
 ****************************************************************************/

int nxcamera_setdevice(FAR struct nxcamera_s *pcam,
                       FAR const char *device)
{
  int                    temp_fd;
  struct v4l2_capability caps;

  DEBUGASSERT(pcam != NULL);
  DEBUGASSERT(device != NULL);

  /* Try to open the device */

  temp_fd = open(device, O_RDWR);
  if (temp_fd == -1)
    {
      /* Error opening the device */

      return -errno;
    }

  /* Validate it's a video device by issuing an VIDIOC_QUERYCAP ioctl */

  if (ioctl(temp_fd, VIDIOC_QUERYCAP, (uintptr_t)&caps) != OK)
    {
      /* Not a video device! */

      close(temp_fd);
      return -errno;
    }

  /* Close the file */

  close(temp_fd);

  /* Save the path of the preferred device */

  if ((caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
    {
      return -ENODEV;
    }

  strlcpy(pcam->capturedev, device, sizeof(pcam->capturedev));
  return OK;
}

/****************************************************************************
 * Name: nxcamera_setfb
 *
 *   nxcamera_setfb() sets the output framebuffer device to use with the
 *   provided nxcamera context.
 *
 ****************************************************************************/

int nxcamera_setfb(FAR struct nxcamera_s *pcam, FAR const char *device)
{
  int temp_fd;

  DEBUGASSERT(pcam != NULL);
  DEBUGASSERT(device != NULL);

  /* Try to open the device */

  temp_fd = open(device, O_RDWR);
  if (temp_fd == -1)
    {
      /* Error opening the device */

      return -errno;
    }

  /* Validate it's a fb device by issuing an FBIOGET_VIDEOINFO ioctl */

  if (ioctl(temp_fd, FBIOGET_VIDEOINFO,
            (uintptr_t)&pcam->display_vinfo) != OK)
    {
      /* Not an Video device! */

      close(temp_fd);
      return -errno;
    }

  /* Close the file */

  close(temp_fd);

  if (pcam->display_vinfo.nplanes == 0)
    {
      return -ENODEV;
    }

  /* Save the path of the framebuffer device */

  strlcpy(pcam->displaydev, device, sizeof(pcam->displaydev));
  return OK;
}

/****************************************************************************
 * Name: nxcamera_setfile
 *
 *   nxcamera_setfile() sets the output file path to use with the provided
 *   nxcamera context.
 *
 ****************************************************************************/

int nxcamera_setfile(FAR struct nxcamera_s *pcam, FAR const char *pfile,
                     bool isimage)
{
  int temp_fd;

  DEBUGASSERT(pcam != NULL);
  DEBUGASSERT(pfile != NULL);

  /* Try to open the file */

  temp_fd = open(pfile, O_CREAT | O_RDWR);
  if (temp_fd == -1)
    {
      /* Error opening the file */

      return -errno;
    }

  /* Close the file */

  close(temp_fd);

  /* Save the path of the output file */

  if (isimage)
    {
      strlcpy(pcam->imagepath, pfile, sizeof(pcam->imagepath));
    }
  else
    {
      strlcpy(pcam->videopath, pfile, sizeof(pcam->videopath));
    }

  return OK;
}

/****************************************************************************
 * Name: nxcamera_stop
 *
 *   nxcamera_stop() stops the current playback to loop and closes the
 *   file and the associated device.
 *
 * Input:
 *   pcam    Pointer to the initialized looper context
 *
 ****************************************************************************/

int nxcamera_stop(FAR struct nxcamera_s *pcam)
{
  struct video_msg_s term_msg;

  DEBUGASSERT(pcam != NULL);

  /* Validate we are not in IDLE state */

  pthread_mutex_lock(&pcam->mutex);          /* Lock the mutex */
  if (pcam->loopstate == NXCAMERA_STATE_IDLE)
    {
      pthread_mutex_unlock(&pcam->mutex);    /* Unlock the mutex */
      return OK;
    }

  /* Notify the stream thread that it needs to cancel the stream */

  term_msg.msg_id = VIDEO_MSG_STOP;
  term_msg.u.data = 0;
  mq_send(pcam->mq, (FAR const char *)&term_msg, sizeof(term_msg),
          CONFIG_NXCAMERA_MSG_PRIO);

  pthread_mutex_unlock(&pcam->mutex);

  /* Join the thread.  The thread will do all the cleanup. */

  nxcamera_jointhread(pcam);

  return OK;
}

/****************************************************************************
 * Name: nxcamera_stream
 *
 *   nxcamera_stream() tries to capture and then display the raw data using
 *   the Video system. If a capture device is specified, it will try to use
 *   that device.
 *
 * Input:
 *   pcam    Pointer to the initialized Looper context
 *   width      Capture frame width
 *   height     Capture frame height
 *   framerate  Capture frame rate
 *   format     Capture frame pixel format
 *
 * Returns:
 *   OK         Video is being looped
 *   -EBUSY     Capture device is busy
 *   -ENOSYS    No supported video format found
 *   -ENODEV    No video capture or framebuffer device suitable
 *
 ****************************************************************************/

int nxcamera_stream(FAR struct nxcamera_s *pcam,
                    uint16_t width, uint16_t height,
                    uint32_t framerate, uint32_t format)
{
  struct mq_attr             attr;
  struct sched_param         sparam;
  pthread_attr_t             tattr;
  int                        ret;
  int                        i;
  struct v4l2_buffer         buf;
  struct v4l2_requestbuffers req;
  struct v4l2_streamparm     parm;

  DEBUGASSERT(pcam != NULL);

  pthread_mutex_lock(&pcam->mutex);          /* Lock the mutex */
  if (pcam->loopstate != NXCAMERA_STATE_IDLE)
    {
      pthread_mutex_unlock(&pcam->mutex);    /* Unlock the mutex */
      return -EBUSY;
    }

  pthread_mutex_unlock(&pcam->mutex);    /* Unlock the mutex */

  vinfo("==============================\n");
  vinfo("streaming video\n");
  vinfo("==============================\n");

  /* Try to open the device */

  ret = nxcamera_opendevice(pcam);
  if (ret < 0)
    {
      /* Error opening the device */

      verr("ERROR: nxcamera_opendevice failed: %d\n", ret);
      return ret;
    }

  /* VIDIOC_S_FMT set format */

  pcam->fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  pcam->fmt.fmt.pix.width       = width;
  pcam->fmt.fmt.pix.height      = height;
  pcam->fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  pcam->fmt.fmt.pix.pixelformat = format;

  ret = ioctl(pcam->capture_fd, VIDIOC_S_FMT, (uintptr_t)&pcam->fmt);
  if (ret < 0)
    {
      ret = -errno;
      verr("VIDIOC_S_FMT failed: %d\n", ret);
      return ret;
    }

  memset(&parm, 0, sizeof(parm));
  parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  parm.parm.capture.timeperframe.denominator = framerate;
  parm.parm.capture.timeperframe.numerator = 1;
  ret = ioctl(pcam->capture_fd, VIDIOC_S_PARM, (uintptr_t)&parm);
  if (ret < 0)
    {
      ret = -errno;
      verr("VIDIOC_S_PARM failed: %d\n", ret);
      return ret;
    }

  /* VIDIOC_REQBUFS initiate user pointer I/O */

  memset(&req, 0, sizeof(req));
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  req.count  = CONFIG_VIDEO_REQBUFS_COUNT_MAX;

  ret = ioctl(pcam->capture_fd, VIDIOC_REQBUFS, (uintptr_t)&req);
  if (ret < 0)
    {
      ret = -errno;
      verr("VIDIOC_REQBUFS failed: %d\n", ret);
      return ret;
    }

  if (req.count < 2)
    {
      verr("VIDIOC_REQBUFS failed: not enough buffers\n");
      return -ENOMEM;
    }

  pcam->nbuffers  = req.count;
  pcam->bufs      = calloc(req.count, sizeof(*pcam->bufs));
  pcam->buf_sizes = calloc(req.count, sizeof(*pcam->buf_sizes));
  if (pcam->bufs == NULL || pcam->buf_sizes == NULL)
    {
      verr("Cannot allocate buffer pointers\n");
      ret = -ENOMEM;
      goto err_out;
    }

  /* VIDIOC_QBUF enqueue buffer */

  for (i = 0; i < req.count; i++)
    {
      memset(&buf, 0, sizeof(buf));
      buf.type   = req.type;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index  = i;
      ret = ioctl(pcam->capture_fd, VIDIOC_QUERYBUF, (uintptr_t)&buf);
      if (ret < 0)
        {
          ret = -errno;
          verr("VIDIOC_QUERYBUF failed: %d\n", ret);
          goto err_out;
        }

      pcam->bufs[i] = mmap(NULL, buf.length,
                           PROT_READ | PROT_WRITE, MAP_SHARED,
                           pcam->capture_fd, buf.m.offset);
      if (pcam->bufs[i] == MAP_FAILED)
        {
          ret = -errno;
          verr("MMAP failed\n");
          goto err_out;
        }

      pcam->buf_sizes[i] = buf.length;
    }

  /* Create a message queue for the loopthread */

  memset(&attr, 0, sizeof(attr));
  attr.mq_maxmsg  = 8;
  attr.mq_msgsize = sizeof(struct video_msg_s);

  snprintf(pcam->mqname, sizeof(pcam->mqname), "/tmp/%lx",
           (unsigned long)((uintptr_t)pcam) & 0xffffffff);

  pcam->mq = mq_open(pcam->mqname, O_RDWR | O_CREAT | O_NONBLOCK, 0644,
                     &attr);
  if (pcam->mq == (mqd_t)-1)
    {
      /* Unable to open message queue! */

      ret = -errno;
      verr("ERROR: mq_open failed: %d\n", ret);
      goto err_out;
    }

  /* Check if there was a previous thread and join it if there was
   * to perform clean-up.
   */

  nxcamera_jointhread(pcam);

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 9;
  pthread_attr_setschedparam(&tattr, &sparam);
  pthread_attr_setstacksize(&tattr, CONFIG_NXCAMERA_LOOPTHREAD_STACKSIZE);

  /* Add a reference count to the looper for the thread and start the
   * thread.  We increment for the thread to avoid thread start-up
   * race conditions.
   */

  nxcamera_reference(pcam);
  ret = pthread_create(&pcam->loop_id, &tattr, nxcamera_loopthread,
                       (pthread_addr_t)pcam);
  pthread_attr_destroy(&tattr);
  if (ret != OK)
    {
      ret = -ret;
      verr("ERROR: Failed to create loopthread: %d\n", ret);
      goto err_out;
    }

  /* Name the thread */

  pthread_setname_np(pcam->loop_id, "nxcameraloop");
  return OK;

err_out:
  if (pcam->bufs)
    {
      for (i = 0; i < pcam->nbuffers; i++)
        {
          if (pcam->bufs[i] != NULL && pcam->bufs[i] != MAP_FAILED)
            {
              munmap(pcam->bufs[i], pcam->buf_sizes[i]);
            }
        }

      free(pcam->bufs);
    }

  if (pcam->buf_sizes)
    {
      free(pcam->buf_sizes);
    }

  return ret;
}

/****************************************************************************
 * Name: nxcamera_create
 *
 *   nxcamera_create() allocates and initializes a nxcamera context for
 *   use by further nxcamera operations.  This routine must be called before
 *   to perform the create for proper reference counting.
 *
 * Input Parameters:  None
 *
 * Returned values:
 *   Pointer to the created context or NULL if there was an error.
 *
 ****************************************************************************/

FAR struct nxcamera_s *nxcamera_create(void)
{
  FAR struct nxcamera_s *pcam;
  int err;

  /* Allocate the memory */

  pcam = (FAR struct nxcamera_s *)calloc(1, sizeof(struct nxcamera_s));
  if (pcam == NULL)
    {
      return NULL;
    }

  /* Initialize the context data */

  pcam->loopstate = NXCAMERA_STATE_IDLE;
  pcam->display_fd = -1;
  pcam->capture_fd = -1;
  err = pthread_mutex_init(&pcam->mutex, NULL);
  if (err)
    {
      verr("ERROR: pthread_mutex_init failed: %d\n", err);
      free(pcam);
      pcam = NULL;
    }

  return pcam;
}

/****************************************************************************
 * Name: nxcamera_release
 *
 *   nxcamera_release() reduces the reference count by one and if it
 *   reaches zero, frees the context.
 *
 * Input Parameters:
 *   pcam    Pointer to the NxCamera context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxcamera_release(FAR struct nxcamera_s *pcam)
{
  int      refcount;

  /* Check if there was a previous thread and join it if there was */

  nxcamera_jointhread(pcam);

  /* Lock the mutex */

  pthread_mutex_lock(&pcam->mutex);

  /* Reduce the reference count */

  refcount = pcam->crefs--;
  pthread_mutex_unlock(&pcam->mutex);

  /* If the ref count *was* one, then free the context */

  if (refcount == 1)
    {
      pthread_mutex_destroy(&pcam->mutex);
      free(pcam);
    }
}

/****************************************************************************
 * Name: nxcamera_reference
 *
 *   nxcamera_reference() increments the reference count by one.
 *
 * Input Parameters:
 *   pcam    Pointer to the NxCamera context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxcamera_reference(FAR struct nxcamera_s *pcam)
{
  /* Lock the mutex */

  pthread_mutex_lock(&pcam->mutex);

  /* Increment the reference count */

  pcam->crefs++;
  pthread_mutex_unlock(&pcam->mutex);
}
