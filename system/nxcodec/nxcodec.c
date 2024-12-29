/****************************************************************************
 * apps/system/nxcodec/nxcodec.c
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

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "nxcodec.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline bool nxcodec_splane_video(FAR struct v4l2_capability *cap)
{
  return (cap->capabilities & V4L2_CAP_VIDEO_M2M) ||
         ((cap->capabilities & V4L2_CAP_STREAMING) &&
         (cap->capabilities & (V4L2_CAP_VIDEO_OUTPUT |
                               V4L2_CAP_VIDEO_CAPTURE)));
}

static inline bool nxcodec_mplane_video(FAR struct v4l2_capability *cap)
{
  return (cap->capabilities & V4L2_CAP_VIDEO_M2M_MPLANE) ||
         ((cap->capabilities & V4L2_CAP_STREAMING) &&
         (cap->capabilities & (V4L2_CAP_VIDEO_OUTPUT_MPLANE |
                               V4L2_CAP_VIDEO_CAPTURE_MPLANE)));
}

static int nxcodec_prepare_contexts(FAR nxcodec_t *codec)
{
  struct v4l2_capability cap;
  int ret;

  memset(&cap, 0, sizeof(cap));
  ret = ioctl(codec->fd, VIDIOC_QUERYCAP, &cap);
  if (ret < 0)
    {
      printf("nxcodec VIDIOC_QUERYCAP error: %d\n", errno);
      return -errno;
    }

  if (nxcodec_mplane_video(&cap))
    {
      codec->capture.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
      codec->output.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

      printf("nxcodec is multi-planar\n");

      return 0;
    }

  if (nxcodec_splane_video(&cap))
    {
      codec->capture.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      codec->output.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

      printf("nxcodec is single-planar\n");

      return 0;
    }

  return -EINVAL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int nxcodec_init(FAR nxcodec_t *codec)
{
  int ret;

  codec->fd = open(codec->devname, O_RDWR | O_NONBLOCK);
  if (codec->fd < 0)
    {
      printf("nxcodec open device error: %d\n", errno);
      return -errno;
    }

  ret = nxcodec_prepare_contexts(codec);
  if (ret < 0)
    {
      printf("nxcodec prepare context error: %d\n", errno);
      goto err0;
    }

  ret = nxcodec_context_get_format(&codec->output);
  if (ret < 0)
    {
      printf("nxcodec v4l2 output format not supported\n");
      goto err0;
    }

  ret = nxcodec_context_get_format(&codec->capture);
  if (ret < 0)
    {
      printf("nxcodec v4l2 capture format not supported\n");
      goto err0;
    }

  codec->output.format.type = codec->output.type;

  ret = nxcodec_context_set_format(&codec->output);
  if (ret < 0)
    {
      printf("nxcodec can't set v4l2 output format\n");
      goto err0;
    }

  printf("nxcodec set output format DONE\n");

  codec->output.fd = open(codec->output.filename, O_RDONLY);
  if (codec->output.fd < 0)
    {
      printf("nxcodec failed to open output file: %s \n",
             codec->output.filename);
      ret = -errno;
      goto err0;
    }

  codec->capture.format.type = codec->capture.type;

  ret = nxcodec_context_set_format(&codec->capture);
  if (ret < 0)
    {
      printf("nxcodec can't to set v4l2 capture format\n");
      goto err1;
    }

  printf("nxcodec set capture format DONE\n");

  codec->capture.fd = open(codec->capture.filename,
                           O_WRONLY | O_CREAT, 0644);
  if (codec->capture.fd < 0)
    {
      printf("nxcodec failed to open input file %s \n",
             codec->capture.filename);
      ret = -errno;
      goto err1;
    }

  return 0;

err1:
  close(codec->output.fd);
err0:
  close(codec->fd);
  return ret;
}

int nxcodec_start(FAR nxcodec_t *codec)
{
  int ret;

  ret = nxcodec_context_init(&codec->output);
  if (ret < 0)
    {
      printf("nxcodec can't request output buffers\n");
      return ret;
    }

  ret = nxcodec_context_set_status(&codec->output, VIDIOC_STREAMON);
  if (ret < 0)
    {
      printf("nxcodec set output VIDIOC_STREAMON failed\n");
      goto err0;
    }

  ret = nxcodec_context_init(&codec->capture);
  if (ret < 0)
    {
      printf("nxcodec can't request capture buffers\n");
      goto err0;
    }

  ret = nxcodec_context_set_status(&codec->capture, VIDIOC_STREAMON);
  if (ret < 0)
    {
      printf("nxcodec set capture VIDIOC_STREAMON failed\n");
      goto err1;
    }

  ret = nxcodec_context_enqueue_frame(&codec->output);
  if (ret < 0 && ret != -EAGAIN)
    {
      printf("nxcodec enqueue frame failed %d\n", errno);
      goto err1;
    }

  return 0;

err1:
  nxcodec_context_uninit(&codec->capture);
err0:
  nxcodec_context_uninit(&codec->output);
  return ret;
}

int nxcodec_stop(FAR nxcodec_t *codec)
{
  int ret;

  if (!codec)
    {
      return 0;
    }

  nxcodec_context_uninit(&codec->output);

  ret = nxcodec_context_set_status(&codec->output, VIDIOC_STREAMOFF);
  if (ret < 0)
    {
      printf("nxcodec set output VIDIOC_STREAMOFF failed\n");
      return ret;
    }

  nxcodec_context_uninit(&codec->capture);

  ret = nxcodec_context_set_status(&codec->capture, VIDIOC_STREAMOFF);
  if (ret < 0)
    {
      printf("nxcodec set capture VIDIOC_STREAMOFF failed\n");
      return ret;
    }

  return 0;
}

int nxcodec_uninit(FAR nxcodec_t *codec)
{
  close(codec->capture.fd);
  close(codec->output.fd);
  close(codec->fd);

  return 0;
}
