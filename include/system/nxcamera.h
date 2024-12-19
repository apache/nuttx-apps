/****************************************************************************
 * apps/include/system/nxcamera.h
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

#ifndef __APPS_INCLUDE_SYSTEM_NXCAMERA_H
#define __APPS_INCLUDE_SYSTEM_NXCAMERA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/video/video.h>
#include <nuttx/video/fb.h>
#include <mqueue.h>
#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Standard Video Message Queue message IDs */

#define VIDEO_MSG_NONE              0
#define VIDEO_MSG_STOP              1

/****************************************************************************
 * Public Type Declarations
 ****************************************************************************/

/* This structure describes the internal state of the nxcamera */

struct nxcamera_s
{
  int                   loopstate;                   /* Current looper test state */
  int                   capture_fd;                  /* File descriptor of active
                                                      * capture device */
  char                  capturedev[CONFIG_NAME_MAX]; /* Preferred capture device */
  int                   display_fd;                  /* File descriptor of active
                                                      * display device */
  char                  displaydev[CONFIG_NAME_MAX]; /* Display framebuffer device */
  struct fb_planeinfo_s display_pinfo;               /* Display plane info */
  struct fb_videoinfo_s display_vinfo;               /* Display video controller info */
  char                  videopath[CONFIG_NAME_MAX];  /* Output video file path */
  char                  imagepath[CONFIG_NAME_MAX];  /* Output image file path */
  int                   crefs;                       /* Number of references */
  pthread_mutex_t       mutex;                       /* Thread sync mutex */
  char                  mqname[14];                  /* Name of display message queue */
  mqd_t                 mq;                          /* Message queue for the
                                                      * loopthread */
  pthread_t             loop_id;                     /* Thread ID of the loopthread */
  v4l2_format_t         fmt;                         /* Buffer format */
  size_t                nbuffers;                    /* Number of buffers */
  FAR size_t            *buf_sizes;                  /* Buffer lengths */
  FAR uint8_t           **bufs;                      /* Buffer pointers */
};

struct video_msg_s
{
  uint16_t      msg_id;       /* Message ID */
  union
  {
    FAR void    *ptr;         /* Buffer being dequeued */
    uint32_t    data;         /* Message data */
  } u;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: nxcamera_create
 *
 *   Allocates and Initializes a nxcamera context that is passed to all
 *   nxcamera routines.  The looper MUST be destroyed using the
 *   nxcamera_destroy() routine since the context is reference counted.
 *   The context can be used in a mode where the caller creates the
 *   context, starts a looping, and then forgets about the context
 *   and it will self free.  This is because the nxcamera_stream
 *   will also create a reference to the context, so the client calling
 *   nxcamera_destroy() won't actually de-allocate anything. The freeing
 *   will occur after the loopthread has completed.
 *
 *   Alternately, the caller can create the object and hold on to it, then
 *   the context will persist until the original creator destroys it.
 *
 * Input Parameters:    None
 *
 * Returned Value:
 *   Pointer to created nxcamera context or NULL if error.
 ****************************************************************************/

FAR struct nxcamera_s *nxcamera_create(void);

/****************************************************************************
 * Name: nxcamera_release
 *
 *   Reduces the reference count to the looper and if it reaches zero,
 *   frees all memory used by the context.
 *
 * Input Parameters:
 *   pcam    Pointer to the nxcamera context
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxcamera_release(FAR struct nxcamera_s *pcam);

/****************************************************************************
 * Name: nxcamera_reference
 *
 *   Increments the reference count to the looper.
 *
 * Input Parameters:
 *   pcam    Pointer to the nxcamera context
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxcamera_reference(FAR struct nxcamera_s *pcam);

/****************************************************************************
 * Name: nxcamera_stream
 *
 *   nxcamera_stream() tries to capture and then display the raw data using
 *   the Video system. If a capture device is specified, it will try to use
 *   that device.
 *
 * Input:
 *   pcam       Pointer to the initialized Looper context
 *   width      Capture frame width
 *   height     Capture frame height
 *   framerate  Capture frame rate
 *   format     Capture frame pixel format
 *
 * Returns:
 *   OK         Video is being streamed
 *   -EBUSY     Capture device is busy
 *   -ENOSYS    No supported video format found
 *   -ENODEV    No video capture or framebuffer device suitable
 *
 ****************************************************************************/

int nxcamera_stream(FAR struct nxcamera_s *pcam,
                    uint16_t width, uint16_t height,
                    uint32_t framerate, uint32_t format);

/****************************************************************************
 * Name: nxcamera_stop
 *
 *   Stops current loopback.
 *
 * Input Parameters:
 *   pcam   - Pointer to the context to initialize
 *
 * Returned Value:
 *   OK if file found, device found, and loopback started.
 *
 ****************************************************************************/

int nxcamera_stop(FAR struct nxcamera_s *pcam);

/****************************************************************************
 * Name: nxcamera_setdevice
 *
 *   Sets the preferred video device to use with the instance of the
 *   nxcamera.
 *
 * Input Parameters:
 *   pcam   - Pointer to the context to initialize
 *   device - Pointer to pathname of the preferred device
 *
 * Returned Value:
 *   OK if context initialized successfully, error code otherwise.
 *
 ****************************************************************************/

int nxcamera_setdevice(FAR struct nxcamera_s *pcam,
                       FAR const char *device);

/****************************************************************************
 * Name: nxcamera_setfb
 *
 *   Sets the output framebuffer device to use with the
 *   provided nxcamera context.
 *
 * Input Parameters:
 *   pcam   - Pointer to the context to initialize
 *   device - Pointer to pathname of the preferred framebuffer device
 *
 * Returned Value:
 *   OK if context initialized successfully, error code otherwise.
 *
 ****************************************************************************/

int nxcamera_setfb(FAR struct nxcamera_s *pcam, FAR const char *device);

/****************************************************************************
 * Name: nxcamera_setfile
 *
 *   Sets the output file path to use with the provided nxcamera context.
 *
 * Input Parameters:
 *   pcam      - Pointer to the context to initialize
 *   file      - Pointer to pathname of the preferred output file
 *   isimage   - The path is for image file or not
 *
 * Returned Value:
 *   OK if file found, device found, and loopback started.
 *
 ****************************************************************************/

int nxcamera_setfile(FAR struct nxcamera_s *pcam, FAR const char *pfile,
                     bool isimage);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_SYSTEM_NXCAMERA_H */
