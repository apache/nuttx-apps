/****************************************************************************
 * apps/system/nxlooper/nxlooper.c
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

#include <sys/types.h>
#include <sys/ioctl.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <debug.h>

#include <nuttx/audio/audio.h>

#include "system/nxlooper.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXLOOPER_STATE_IDLE      0
#define NXLOOPER_STATE_RECORDING 1
#define NXLOOPER_STATE_LOOPING   2
#define NXLOOPER_STATE_PAUSED    3

#define AUDIO_APB_RECORD         (1 << 4)
#define AUDIO_APB_PLAY           (1 << 5)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************

 * Name: nxlooper_opendevice
 *
 *   nxlooper_opendevice() either searches the Audio system for record and
 *   playback devices and opens them, or tries to open the preferred devices
 *   if specified.
 *
 * Return:
 *    OK        if compatible device opened (searched or preferred)
 *    -ENODEV   if no compatible device opened.
 *    -ENOENT   if preferred device couldn't be opened.
 *
 ****************************************************************************/

static int nxlooper_opendevice(FAR struct nxlooper_s *plooper)
{
#ifdef CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE
  /* If we have a preferred device, then open it */

  if (plooper->playdev[0] != '\0' && plooper->recorddev[0] != '\0')
    {
      plooper->playdev_fd = open(plooper->playdev, O_RDWR | O_CLOEXEC);
      if (plooper->playdev_fd == -1)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          auderr("ERROR: Failed to open plooper->playdev %d\n", -errcode);
          return -errcode;
        }

      plooper->recorddev_fd = open(plooper->recorddev, O_RDWR | O_CLOEXEC);
      if (plooper->recorddev_fd == -1)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          auderr("ERROR: Failed to open plooper->recorddev %d\n", -errcode);

          close(plooper->playdev_fd);
          plooper->playdev_fd = -1;

          return -errcode;
        }

      return OK;
    }
#endif /* CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE */

#if defined(CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE) && \
    defined(CONFIG_NXLOOPER_INCLUDE_DEVICE_SEARCH)

  else

#endif

#ifdef CONFIG_NXLOOPER_INCLUDE_DEVICE_SEARCH
    {
      struct audio_caps_s caps;
      FAR struct dirent *pdevice;
      FAR DIR *dirp;
      char path[64];

      /* Search for a device in the audio device directory */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
      dirp = opendir("/dev");
#else
      dirp = opendir(CONFIG_AUDIO_DEV_PATH);
#endif /* CONFIG_AUDIO_DEV_ROOT */
#else
      dirp = opendir("/dev/audio");
#endif /* CONFIG_AUDIO_CUSTOM_DEV_PATH */
      if (dirp == NULL)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          auderr("ERROR: Failed to open /dev/audio: %d\n", -errcode);
          return -errcode;
        }

      while ((pdevice = readdir(dirp)) != NULL)
        {
          int dev_fd;
          /* We found the next device.  Try to open it and
           * get its audio capabilities.
           */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
          snprintf(path, sizeof(path), "/dev/%s", pdevice->d_name);
#else
          snprintf(path, sizeof(path), CONFIG_AUDIO_DEV_PATH "/%s",
                   pdevice->d_name);
#endif /* CONFIG_AUDIO_DEV_ROOT */
#else
          snprintf(path, sizeof(path), "/dev/audio/%s", pdevice->d_name);
#endif /* CONFIG_AUDIO_CUSTOM_DEV_PATH */

          if ((dev_fd = open(path, O_RDWR | O_CLOEXEC)) != -1)
            {
              /* We have the device file open.  Now issue an AUDIO ioctls to
               * get the capabilities
               */

              caps.ac_len = sizeof(caps);
              caps.ac_type = AUDIO_TYPE_QUERY;
              caps.ac_subtype = AUDIO_TYPE_QUERY;

              if (ioctl(dev_fd, AUDIOIOC_GETCAPS,
                        (unsigned long)&caps) == caps.ac_len)
                {
                  if ((plooper->playdev_fd == -1) &&
                       caps.ac_controls.b[0] & AUDIO_TYPE_OUTPUT)
                    {
                      plooper->playdev_fd = dev_fd;
                    }
                  else if ((plooper->recorddev_fd == -1) &&
                            caps.ac_controls.b[0] & AUDIO_TYPE_INPUT)
                    {
                      plooper->recorddev_fd = dev_fd;
                    }
                  else
                    {
                      close(dev_fd);
                    }

                  if (plooper->recorddev_fd != -1 &&
                      plooper->playdev_fd != -1)
                    {
                      closedir(dirp);
                      return OK;
                    }
                }
              else
                {
                  close(dev_fd);
                }
            }
        }

      closedir(dirp);
    }

  /* Device not found */

  auderr("ERROR: Device not found\n");
  if (plooper->playdev_fd != -1)
    {
      close(plooper->playdev_fd);
      plooper->playdev_fd = -1;
    }

  if (plooper->recorddev_fd != -1)
    {
      close(plooper->recorddev_fd);
      plooper->recorddev_fd = -1;
    }

#endif /* CONFIG_NXLOOPER_INCLUDE_DEVICE_SEARCH */
  return -ENODEV;
}

/****************************************************************************
 * Name: nxlooper_enqueuerecordbuffer
 *
 * Description:
 *   Enqueue the audio buffer in the downstream device.  Normally we are
 *   called with a buffer of data to be enqueued in the audio stream.
 *
 *   Be we may also receive an empty length buffer (with only the
 *   AUDIO_APB_FINAL set) in the event of certain read error occurs or in the
 *   event that the file was an exact multiple of the nmaxbytes size of the
 *   audio buffer.  In that latter case, we have an end of file with no bytes
 *   read.
 *
 *   These infrequent zero length buffers have to be passed through because
 *   the include the AUDIO_APB_FINAL flag that is needed to terminate the
 *   audio stream.
 *
 ****************************************************************************/

static int nxlooper_enqueuerecordbuffer(FAR struct nxlooper_s *plooper,
                                        FAR struct ap_buffer_s *apb)
{
  struct audio_buf_desc_s bufdesc;
  int ret;

  /* Now enqueue the buffer with the audio device. */

  apb->nbytes = apb->nmaxbytes;
#ifdef CONFIG_AUDIO_MULTI_SESSION
  bufdesc.session  = plooper->precordses;
#endif
  bufdesc.numbytes = apb->nbytes;
  bufdesc.u.buffer = apb;

  apb->flags = AUDIO_APB_RECORD;

  ret = ioctl(plooper->recorddev_fd, AUDIOIOC_ENQUEUEBUFFER,
              (unsigned long)&bufdesc);
  if (ret < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      auderr("ERROR: AUDIOIOC_ENQUEUEBUFFER ioctl failed: %d\n", errcode);
      return -errcode;
    }

  return OK;
}

static int nxlooper_enqueueplaybuffer(FAR struct nxlooper_s *plooper,
                                      FAR struct ap_buffer_s *apb)
{
  struct audio_buf_desc_s bufdesc;
  int ret;

  /* Now enqueue the buffer with the audio device. */
#ifdef CONFIG_AUDIO_MULTI_SESSION
  bufdesc.session  = plooper->pplayses;
#endif
  bufdesc.numbytes = apb->nbytes;
  bufdesc.u.buffer = apb;

  apb->flags = AUDIO_APB_PLAY;

  ret = ioctl(plooper->playdev_fd, AUDIOIOC_ENQUEUEBUFFER,
              (unsigned long)&bufdesc);
  if (ret < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      auderr("ERROR: AUDIOIOC_ENQUEUEBUFFER ioctl failed: %d\n", errcode);
      return -errcode;
    }

  return OK;
}

/****************************************************************************
 * Name: nxlooper_thread_loopthread
 *
 *  This is the thread that record the raw audio data and enqueues /
 *  dequeues buffers from the selected and opened audio device.
 *
 ****************************************************************************/

static void *nxlooper_loopthread(pthread_addr_t pvarg)
{
  FAR struct nxlooper_s   *plooper = (FAR struct nxlooper_s *)pvarg;
  FAR struct ap_buffer_s  *apb;
  FAR struct ap_buffer_s  *apbtemp;
  struct dq_queue_s       playdq;
  struct dq_queue_s       recorddq;
  struct audio_msg_s      msg;
  struct audio_buf_desc_s buf_desc;
  struct ap_buffer_info_s recordbuf_info;
  struct ap_buffer_info_s playbuf_info;
  FAR struct ap_buffer_s  **playbufs;
  FAR struct ap_buffer_s  **recordbufs;
  unsigned int            prio;
  ssize_t                 size;
  bool                    running = true;
  int                     x;
  int                     ret;

  audinfo("Entry\n");
  dq_init(&playdq);
  dq_init(&recorddq);

  /* Query the audio device for it's preferred buffer size / qty */

  if ((ret = ioctl(plooper->recorddev_fd, AUDIOIOC_GETBUFFERINFO,
                   (unsigned long)&recordbuf_info)) != OK)
    {
      /* Driver doesn't report it's buffer size.  Use our default. */

      recordbuf_info.buffer_size = CONFIG_AUDIO_BUFFER_NUMBYTES;
      recordbuf_info.nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  /* Create array of pointers to buffers */

  recordbufs = (FAR struct ap_buffer_s **)
    calloc(recordbuf_info.nbuffers, sizeof(FAR void *));
  if (recordbufs == NULL)
    {
      /* Error allocating memory for buffer storage! */

      ret = -ENOMEM;
      goto err_out;
    }

  /* Create our audio pipeline buffers to use for queueing up data */

  for (x = 0; x < recordbuf_info.nbuffers; x++)
    {
      /* Fill in the buffer descriptor struct to issue an alloc request */
#ifdef CONFIG_AUDIO_MULTI_SESSION
      buf_desc.session = plooper->precordses;
#endif
      buf_desc.numbytes = recordbuf_info.buffer_size;
      buf_desc.u.pbuffer = &recordbufs[x];

      ret = ioctl(plooper->recorddev_fd, AUDIOIOC_ALLOCBUFFER,
                  (unsigned long)&buf_desc);

      if (ret != sizeof(buf_desc))
        {
          /* Buffer alloc Operation not supported or error allocating! */

          auderr("ERROR: Could not allocate buffer %d\n", x);
          goto err_out;
        }
    }

  /* Fill up the pipeline with enqueued buffers */

  for (x = 0; x < recordbuf_info.nbuffers; x++)
    {
      /* Write the next buffer of data */

      ret = nxlooper_enqueuerecordbuffer(plooper, recordbufs[x]);
      if (ret != OK)
        {
          goto err_out;
        }
    }

  if ((ret = ioctl(plooper->playdev_fd, AUDIOIOC_GETBUFFERINFO,
                   (unsigned long)&playbuf_info)) != OK)
    {
      /* Driver doesn't report it's buffer size.  Use our default. */

      playbuf_info.buffer_size = CONFIG_AUDIO_BUFFER_NUMBYTES;
      playbuf_info.nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  playbufs = (FAR struct ap_buffer_s **)
    calloc(playbuf_info.nbuffers, sizeof(FAR void *));
  if (playbufs == NULL)
    {
      /* Error allocating memory for buffer storage! */

      ret = -ENOMEM;
      goto err_out;
    }

  /* Create our audio pipeline buffers to use for queueing up data */

  for (x = 0; x < playbuf_info.nbuffers; x++)
    {
      /* Fill in the buffer descriptor struct to issue an alloc request */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      buf_desc.session = plooper->pplayses;
#endif
      buf_desc.numbytes = playbuf_info.buffer_size;
      buf_desc.u.pbuffer = &playbufs[x];

      ret = ioctl(plooper->playdev_fd, AUDIOIOC_ALLOCBUFFER,
                  (unsigned long)&buf_desc);

      if (ret != sizeof(buf_desc))
        {
          /* Buffer alloc Operation not supported or error allocating! */

          auderr("ERROR: Could not allocate buffer %d\n", x);
          goto err_out;
        }

      dq_addlast(&playbufs[x]->dq_entry, &playdq);
    }

  /* Start the audio device */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(plooper->recorddev_fd, AUDIOIOC_START,
              (unsigned long)plooper->precordses);
#else
  ret = ioctl(plooper->recorddev_fd, AUDIOIOC_START, 0);
#endif
  if (ret < 0)
    {
      goto err_out;
    }
  else
    {
      plooper->loopstate = NXLOOPER_STATE_RECORDING;
    }

  /* Loop until we specifically break.  running == true means that we are
   * still looping waiting for the loopback to complete.  All of the data
   * may have been sent, but the loopback is not complete until we get
   * AUDIO_MSG_STOP message
   *
   * The normal protocol for looping errors detected by the audio driver
   * is as follows:
   *
   * (1) The audio driver will indicated the error by returning a negated
   *     error value when the next buffer is enqueued.  The upper level
   *     then knows that this buffer was not queue.
   * (2) The audio driver must return all queued buffers using the
   *     AUDIO_MSG_DEQUEUE message.
   */

  while (running)
    {
      size = mq_receive(plooper->mq, (FAR char *)&msg, sizeof(msg), &prio);

      /* Validate a message was received */

      audinfo("message received size %zd id%d\n", size, msg.msg_id);
      if (size != sizeof(msg))
        {
          /* Interrupted by a signal? What to do? */

          continue;
        }

      /* Perform operation based on message id */

      switch (msg.msg_id)
        {
          /* An audio buffer is being dequeued by the driver */

          case AUDIO_MSG_DEQUEUE:
            apb = msg.u.ptr;
            if (apb->flags & AUDIO_APB_PLAY)
              {
                dq_addlast(&apb->dq_entry, &playdq);
              }
            else if (apb->flags & AUDIO_APB_RECORD)
              {
                dq_addlast(&apb->dq_entry, &recorddq);
              }

              if (dq_count(&playdq) != 0 && dq_count(&recorddq) != 0)
              {
                 apbtemp = (struct ap_buffer_s *)dq_remfirst(&recorddq);
                 apb = (struct ap_buffer_s *)dq_remfirst(&playdq);

                 apb->nbytes = apbtemp->nbytes;
                 memcpy(apb->samp, apbtemp->samp, apbtemp->nbytes);

                 ret = nxlooper_enqueuerecordbuffer(plooper, apbtemp);
                 if (ret == OK)
                 {
                   ret = nxlooper_enqueueplaybuffer(plooper, apb);
                   if (ret == OK &&
                       plooper->loopstate == NXLOOPER_STATE_RECORDING)
                   {
#ifdef CONFIG_AUDIO_MULTI_SESSION
                     ret = ioctl(plooper->playdev_fd, AUDIOIOC_START,
                                    (unsigned long)plooper->pplayses);
#else
                     ret = ioctl(plooper->playdev_fd, AUDIOIOC_START, 0);
#endif
                     if (ret == OK)
                       {
                         plooper->loopstate = NXLOOPER_STATE_LOOPING;
                       }
                   }
                 }
              }

            if (ret == OK)
              {
                /* Go through to AUDIO_MSG_STOP */

                break;
              }

          /* Someone wants to stop the loopback. */

          case AUDIO_MSG_STOP:

            /* Send a stop message to the device */

            audinfo("Stopping looping\n");
#ifdef CONFIG_AUDIO_MULTI_SESSION
            ioctl(plooper->playdev_fd, AUDIOIOC_STOP,
                 (unsigned long)plooper->pplayses);
            ioctl(plooper->recorddev_fd, AUDIOIOC_STOP,
                 (unsigned long)plooper->precordses);
#else
            ioctl(plooper->playdev_fd, AUDIOIOC_STOP, 0);
            ioctl(plooper->recorddev_fd, AUDIOIOC_STOP, 0);
#endif

            running = false;
            break;

          /* Unknown / unsupported message ID */

          default:
            break;
        }
    }

  /* Release our audio buffers and unregister / release the device */

err_out:
  audinfo("Clean-up and exit\n");

  if (playbufs != NULL)
    {
      audinfo("Freeing play buffers\n");
      for (x = 0; x < playbuf_info.nbuffers; x++)
        {
          /* Fill in the buffer descriptor struct to issue a free request */

          if (playbufs[x] != NULL)
            {
#ifdef CONFIG_AUDIO_MULTI_SESSION
              buf_desc.session = plooper->pplayses;
#endif
              buf_desc.u.buffer = playbufs[x];
              ioctl(plooper->playdev_fd,
                    AUDIOIOC_FREEBUFFER,
                    (unsigned long)&buf_desc);
            }
        }

      /* Free the pointers to the buffers */

      free(playbufs);
    }

  if (recordbufs != NULL)
    {
      audinfo("Freeing record buffers\n");
      for (x = 0; x < recordbuf_info.nbuffers; x++)
        {
          /* Fill in the buffer descriptor struct to issue a free request */

          if (recordbufs[x] != NULL)
            {
#ifdef CONFIG_AUDIO_MULTI_SESSION
              buf_desc.session = plooper->precordses;
#endif
              buf_desc.u.buffer = recordbufs[x];
              ioctl(plooper->recorddev_fd,
                    AUDIOIOC_FREEBUFFER,
                    (unsigned long)&buf_desc);
            }
        }

      /* Free the pointers to the buffers */

      free(recordbufs);
    }

  /* Unregister the message queue and release the session */

  ioctl(plooper->playdev_fd,
        AUDIOIOC_UNREGISTERMQ,
        (unsigned long)plooper->mq);
  ioctl(plooper->recorddev_fd,
        AUDIOIOC_UNREGISTERMQ,
        (unsigned long)plooper->mq);
#ifdef CONFIG_AUDIO_MULTI_SESSION
  ioctl(plooper->playdev_fd, AUDIOIOC_RELEASE,
        (unsigned long)plooper->pplayses);
  ioctl(plooper->recorddev_fd, AUDIOIOC_RELEASE,
        (unsigned long)plooper->precordses);
#else
  ioctl(plooper->playdev_fd, AUDIOIOC_RELEASE, 0);
  ioctl(plooper->recorddev_fd, AUDIOIOC_RELEASE, 0);
#endif

  /* Cleanup */

  while (sem_wait(&plooper->sem) < 0)
    {
    }

  close(plooper->playdev_fd);             /* Close the play device */
  close(plooper->recorddev_fd);           /* Close the record device */
  plooper->playdev_fd = -1;               /* Mark play device as closed */
  plooper->recorddev_fd = -1;             /* Mark record device as closed */
  mq_close(plooper->mq);                  /* Close the message queue */
  mq_unlink(plooper->mqname);             /* Unlink the message queue */
  plooper->loopstate = NXLOOPER_STATE_IDLE;

  sem_post(&plooper->sem);                /* Release the semaphore */

  audinfo("Exit\n");

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxlooper_setvolume
 *
 *   nxlooper_setvolume() sets the volume.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
int nxlooper_setvolume(FAR struct nxlooper_s *plooper, uint16_t volume)
{
  struct audio_caps_desc_s  cap_desc;
  int ret;

  /* Thread sync using the semaphore */

  while (sem_wait(&plooper->sem) < 0)
    ;

  /* If we are currently looping, then we need to post a message to
   * the loopthread to perform the volume change operation.  If we
   * are not looping, then just store the volume setting and it will
   * be applied before the next loopback begins.
   */

  if (plooper->loopstate == NXLOOPER_STATE_LOOPING)
    {
      /* Send a CONFIGURE ioctl to the device to set the volume */
#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session                = plooper->pplayses;
#endif
      cap_desc.caps.ac_len            = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type           = AUDIO_TYPE_FEATURE;
      cap_desc.caps.ac_format.hw      = AUDIO_FU_VOLUME;
      cap_desc.caps.ac_controls.hw[0] = volume;
      ret = ioctl(plooper->playdev_fd, AUDIOIOC_CONFIGURE,
                  (unsigned long)&cap_desc);
      if (ret < 0)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          auderr("ERROR: AUDIOIOC_CONFIGURE ioctl failed: %d\n", errcode);
          sem_post(&plooper->sem);
          return -errcode;
        }
    }

  /* Store the volume setting */

  plooper->volume = volume;
  sem_post(&plooper->sem);

  return OK;
}
#endif /* CONFIG_AUDIO_EXCLUDE_VOLUME */

/****************************************************************************
 * Name: nxlooper_pause
 *
 *   nxlooper_pause() pauses loopback without cancelling it.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxlooper_pause(FAR struct nxlooper_s *plooper)
{
  int   ret = OK;

  if (plooper->loopstate == NXLOOPER_STATE_LOOPING)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(plooper->playdev_fd, AUDIOIOC_PAUSE,
          (unsigned long)plooper->pplayses);

      if (ret == OK)
        {
          ret = ioctl(plooper->recorddev_fd, AUDIOIOC_PAUSE,
                      (unsigned long)plooper->precordses);
        }

#else
      ret = ioctl(plooper->playdev_fd, AUDIOIOC_PAUSE, 0);

      if (ret == OK)
        {
          ret = ioctl(plooper->recorddev_fd, AUDIOIOC_PAUSE, 0);
        }

#endif
      if (ret == OK)
        {
          plooper->loopstate = NXLOOPER_STATE_PAUSED;
        }
    }

  return ret;
}
#endif /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxlooper_resume
 *
 *   nxlooper_resume() resumes loopback after a pause operation.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxlooper_resume(FAR struct nxlooper_s *plooper)
{
  int ret = OK;

  if (plooper->loopstate == NXLOOPER_STATE_PAUSED)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(plooper->recorddev_fd, AUDIOIOC_RESUME,
          (unsigned long)plooper->pplayses);

      if (ret == OK)
        {
          ret = ioctl(plooper->playdev_fd, AUDIOIOC_RESUME,
            (unsigned long)plooper->precordses);
        }

#else
      ret = ioctl(plooper->recorddev_fd, AUDIOIOC_RESUME, 0);
      if (ret == OK)
        {
          ret = ioctl(plooper->playdev_fd, AUDIOIOC_RESUME, 0);
        }

#endif
      if (ret == OK)
        {
          plooper->loopstate = NXLOOPER_STATE_LOOPING;
        }
    }

  return ret;
}
#endif /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxlooper_setdevice
 *
 *   nxlooper_setdevice() sets the preferred audio device to use with the
 *   provided nxlooper context.
 *
 ****************************************************************************/

#ifdef CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE
int nxlooper_setdevice(FAR struct nxlooper_s *plooper,
                       FAR const char *pdevice)
{
  int                 temp_fd;
  struct audio_caps_s caps;

  DEBUGASSERT(plooper != NULL);
  DEBUGASSERT(pdevice != NULL);

  /* Try to open the device */

  temp_fd = open(pdevice, O_RDWR);
  if (temp_fd == -1)
    {
      /* Error opening the device */

      return -ENOENT;
    }

  /* Validate it's an Audio device by issuing an AUDIOIOC_GETCAPS ioctl */

  caps.ac_len     = sizeof(caps);
  caps.ac_type    = AUDIO_TYPE_QUERY;
  caps.ac_subtype = AUDIO_TYPE_QUERY;
  if (ioctl(temp_fd, AUDIOIOC_GETCAPS, (unsigned long)&caps) != caps.ac_len)
    {
      /* Not an Audio device! */

      close(temp_fd);
      return -ENODEV;
    }

  /* Close the file */

  close(temp_fd);

  /* Save the path of the preferred device */

  if (caps.ac_controls.b[0] & AUDIO_TYPE_OUTPUT)
    {
      strncpy(plooper->playdev, pdevice, sizeof(plooper->playdev));
    }
  else if (caps.ac_controls.b[0] & AUDIO_TYPE_INPUT)
    {
      strncpy(plooper->recorddev, pdevice, sizeof(plooper->playdev));
    }

  return OK;
}
#endif /* CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE */

/****************************************************************************
 * Name: nxlooper_stop
 *
 *   nxlooper_stop() stops the current playback to loop and closes the
 *   file and the associated device.
 *
 * Input:
 *   plooper    Pointer to the initialized looper context
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
int nxlooper_stop(FAR struct nxlooper_s *plooper)
{
  struct audio_msg_s term_msg;
  FAR void           *value;

  DEBUGASSERT(plooper != NULL);

  /* Validate we are not in IDLE state */

  sem_wait(&plooper->sem);                      /* Get the semaphore */
  if (plooper->loopstate == NXLOOPER_STATE_IDLE)
    {
      sem_post(&plooper->sem);                  /* Release the semaphore */
      return OK;
    }

  sem_post(&plooper->sem);

  /* Notify the loopback thread that it needs to cancel the loopback */

  term_msg.msg_id = AUDIO_MSG_STOP;
  term_msg.u.data = 0;
  mq_send(plooper->mq, (FAR const char *)&term_msg, sizeof(term_msg),
          CONFIG_NXLOOPER_MSG_PRIO);

  /* Join the thread.  The thread will do all the cleanup. */

  pthread_join(plooper->loop_id, &value);
  plooper->loop_id = 0;

  return OK;
}
#endif /* CONFIG_AUDIO_EXCLUDE_STOP */

/****************************************************************************
 * Name: nxlooper_loopraw
 *
 *   nxlooper_loopraw() tries to record and then play the raw data using the
 *   Audio system.  If a device is specified, it will try to use that
 *   device.
 * Input:
 *   plooper    Pointer to the initialized Looper context
 *   nchannels  channel num
 *   bpsampe    bit width
 *   samprate   sample rate
 *   chmap      channel map
 *
 * Returns:
 *   OK         File is being looped
 *   -EBUSY     The media device is busy
 *   -ENOSYS    The media file is an unsupported type
 *   -ENODEV    No audio device suitable
 *   -ENOENT    The media file was not found
 *
 ****************************************************************************/

int nxlooper_loopraw(FAR struct nxlooper_s *plooper,
                     uint8_t nchannels, uint8_t bpsamp,
                     uint32_t samprate, uint8_t chmap)
{
  struct mq_attr           attr;
  struct sched_param       sparam;
  pthread_attr_t           tattr;
  struct audio_caps_desc_s cap_desc;
  struct ap_buffer_info_s  buf_info;
  FAR void                 *value;
  int                      ret;

  DEBUGASSERT(plooper != NULL);

  if (plooper->loopstate != NXLOOPER_STATE_IDLE)
    {
      return -EBUSY;
    }

  audinfo("==============================\n");
  audinfo("loopback raw data\n");
  audinfo("==============================\n");

  /* Try to open the device */

  ret = nxlooper_opendevice(plooper);
  if (ret < 0)
    {
      /* Error opening the device */

      auderr("ERROR: nxlooper_opendevice failed: %d\n", ret);
      return ret;
    }

  /* Try to reserve record device */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(plooper->recorddev_fd, AUDIOIOC_RESERVE,
              (unsigned long)&plooper->precordses);
#else
  ret = ioctl(plooper->recorddev_fd, AUDIOIOC_RESERVE, 0);
#endif

  if (ret < 0)
    {
      /* Device is busy or error */

      auderr("ERROR: Failed to reserve record device: %d\n", ret);
      ret = -errno;
      goto err_out_dev;
    }

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(plooper->playdev_fd, AUDIOIOC_RESERVE,
              (unsigned long)&plooper->pplayses);
#else
  ret = ioctl(plooper->playdev_fd, AUDIOIOC_RESERVE, 0);
#endif

  if (ret < 0)
    {
      /* Device is busy or error */

      auderr("ERROR: Failed to reserve play device: %d\n", ret);
      ret = -errno;
      goto err_out_record;
    }

#ifdef CONFIG_AUDIO_MULTI_SESSION
  cap_desc.session = plooper->precordses;
#endif
  cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
  cap_desc.caps.ac_type = AUDIO_TYPE_INPUT;
  cap_desc.caps.ac_channels = nchannels ? nchannels : 2;
  cap_desc.caps.ac_chmap = chmap ? chmap : 3;
  cap_desc.caps.ac_controls.hw[0] = samprate ? samprate : 48000;
  cap_desc.caps.ac_controls.b[3] = samprate >> 16;
  cap_desc.caps.ac_controls.b[2] = bpsamp ? bpsamp : 16;
  ret = ioctl(plooper->recorddev_fd, AUDIOIOC_CONFIGURE,
              (unsigned long)&cap_desc);
  if (ret < 0)
    {
      ret = -errno;
      goto err_out;
    }

#ifdef CONFIG_AUDIO_MULTI_SESSION
  cap_desc.session = plooper->pplayses;
#endif
  cap_desc.caps.ac_type = AUDIO_TYPE_OUTPUT;

  ret = ioctl(plooper->playdev_fd, AUDIOIOC_CONFIGURE,
              (unsigned long)&cap_desc);
  if (ret < 0)
    {
      ret = -errno;
      goto err_out;
    }

  /* Query the audio device for its preferred buffer size / qty */

  if ((ioctl(plooper->playdev_fd, AUDIOIOC_GETBUFFERINFO,
             (unsigned long)&buf_info)) != OK)
    {
      /* Driver doesn't report its buffer size.  Use our default. */

      buf_info.nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  /* Create a message queue for the loopthread */

  attr.mq_maxmsg  = buf_info.nbuffers + 8;
  attr.mq_msgsize = sizeof(struct audio_msg_s);
  attr.mq_curmsgs = 0;
  attr.mq_flags   = 0;

  snprintf(plooper->mqname, sizeof(plooper->mqname), "/tmp/%0lx",
           (unsigned long)((uintptr_t)plooper));

  plooper->mq = mq_open(plooper->mqname, O_RDWR | O_CREAT, 0644, &attr);
  if (plooper->mq == (mqd_t) -1)
    {
      /* Unable to open message queue! */

      ret = -errno;
      auderr("ERROR: mq_open failed: %d\n", ret);
      goto err_out;
    }

  /* Register our message queue with the audio device */

  ioctl(plooper->recorddev_fd, AUDIOIOC_REGISTERMQ,
        (unsigned long)plooper->mq);

  ioctl(plooper->playdev_fd, AUDIOIOC_REGISTERMQ,
        (unsigned long)plooper->mq);

  /* Check if there was a previous thread and join it if there was
   * to perform clean-up.
   */

  if (plooper->loop_id != 0)
    {
      pthread_join(plooper->loop_id, &value);
    }

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 9;
  pthread_attr_setschedparam(&tattr, &sparam);
  pthread_attr_setstacksize(&tattr, CONFIG_NXLOOPER_LOOPTHREAD_STACKSIZE);

  /* Add a reference count to the looper for the thread and start the
   * thread.  We increment for the thread to avoid thread start-up
   * race conditions.
   */

  nxlooper_reference(plooper);
  ret = pthread_create(&plooper->loop_id, &tattr, nxlooper_loopthread,
                       (pthread_addr_t)plooper);
  pthread_attr_destroy(&tattr);
  if (ret != OK)
    {
      ret = -ret;
      auderr("ERROR: Failed to create loopthread: %d\n", ret);
      goto err_out;
    }

  /* Name the thread */

  pthread_setname_np(plooper->loop_id, "loopthread");
  return OK;

err_out:

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ioctl(plooper->playdev_fd, AUDIOIOC_RELEASE,
        (unsigned long)plooper->pplayses);
#else
  ioctl(plooper->playdev_fd, AUDIOIOC_RELEASE, 0);
#endif

err_out_record:

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ioctl(plooper->recorddev_fd, AUDIOIOC_RELEASE,
        (unsigned long)plooper->precordses);
#else
  ioctl(plooper->recorddev_fd, AUDIOIOC_RELEASE, 0);
#endif

err_out_dev:

  close(plooper->playdev_fd);
  plooper->playdev_fd = -1;
  close(plooper->recorddev_fd);
  plooper->recorddev_fd = -1;

  return ret;
}

/****************************************************************************
 * Name: nxlooper_create
 *
 *   nxlooper_create() allocates and initializes a nxlooper context for
 *   use by further nxlooper operations.  This routine must be called before
 *   to perform the create for proper reference counting.
 *
 * Input Parameters:  None
 *
 * Returned values:
 *   Pointer to the created context or NULL if there was an error.
 *
 ****************************************************************************/

FAR struct nxlooper_s *nxlooper_create(void)
{
  FAR struct nxlooper_s *plooper;

  /* Allocate the memory */

  plooper = (FAR struct nxlooper_s *)malloc(sizeof(struct nxlooper_s));
  if (plooper == NULL)
    {
      return NULL;
    }

  /* Initialize the context data */

  plooper->loopstate = NXLOOPER_STATE_IDLE;
  plooper->playdev_fd = -1;
  plooper->recorddev_fd = -1;
#ifdef CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE
  plooper->playdev[0] = '\0';
  plooper->recorddev[0] = '\0';
#endif
  plooper->mq = 0;
  plooper->loop_id = 0;
  plooper->crefs = 1;

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  plooper->volume = 400;
#endif

#ifdef CONFIG_AUDIO_MULTI_SESSION
  plooper->pplayses = NULL;
  plooper->precordses = NULL;
#endif

  sem_init(&plooper->sem, 0, 1);

  return plooper;
}

/****************************************************************************
 * Name: nxlooper_release
 *
 *   nxlooper_release() reduces the reference count by one and if it
 *   reaches zero, frees the context.
 *
 * Input Parameters:
 *   plooper    Pointer to the NxLooper context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxlooper_release(FAR struct nxlooper_s *plooper)
{
  int      refcount;
  FAR void *value;

  /* Grab the semaphore */

  while (sem_wait(&plooper->sem) < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      if (errcode != EINTR)
        {
          auderr("ERROR: sem_wait failed: %d\n", errcode);
          return;
        }
    }

  /* Check if there was a previous thread and join it if there was */

  if (plooper->loop_id != 0)
    {
      sem_post(&plooper->sem);
      pthread_join(plooper->loop_id, &value);
      plooper->loop_id = 0;

      while (sem_wait(&plooper->sem) < 0)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          if (errcode != EINTR)
            {
              auderr("ERROR: sem_wait failed: %d\n", errcode);
              return;
            }
        }
    }

  /* Reduce the reference count */

  refcount = plooper->crefs--;
  sem_post(&plooper->sem);

  /* If the ref count *was* one, then free the context */

  if (refcount == 1)
    {
      free(plooper);
    }
}

/****************************************************************************
 * Name: nxlooper_reference
 *
 *   nxlooper_reference() increments the reference count by one.
 *
 * Input Parameters:
 *   plooper    Pointer to the NxLooper context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxlooper_reference(FAR struct nxlooper_s *plooper)
{
  /* Grab the semaphore */

  while (sem_wait(&plooper->sem) < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      if (errcode != EINTR)
        {
          auderr("ERROR: sem_wait failed: %d\n", errcode);
          return;
        }
    }

  /* Increment the reference count */

  plooper->crefs++;
  sem_post(&plooper->sem);
}

/****************************************************************************
 * Name: nxlooper_systemreset
 *
 *   nxlooper_systemreset() performs a HW reset on all registered
 *   audio devices.
 *
 ****************************************************************************/

#ifdef CONFIG_NXLOOPER_INCLUDE_SYSTEM_RESET
int nxlooper_systemreset(FAR struct nxlooper_s *plooper)
{
  struct audio_caps_s caps;
  FAR struct dirent   *pdevice;
  FAR DIR             *dirp;
  char                path[64];
  int                 temp_fd;

  /* Search for a device in the audio device directory */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
  dirp = opendir("/dev");
#else
  dirp = opendir(CONFIG_AUDIO_DEV_PATH);
#endif
#else
  dirp = opendir("/dev/audio");
#endif
  if (dirp == NULL)
    {
      return -ENODEV;
    }

  caps.ac_len     = sizeof(caps);
  caps.ac_type    = AUDIO_TYPE_QUERY;
  caps.ac_subtype = AUDIO_TYPE_QUERY;

  while ((pdevice = readdir(dirp)) != NULL)
    {
      /* We found the next device.  Try to open it and
       * get its audio capabilities.
       */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
      snprintf(path, sizeof(path), "/dev/%s", pdevice->d_name);
#else
      snprintf(path, sizeof(path), CONFIG_AUDIO_DEV_PATH "/%s",
               pdevice->d_name);
#endif
#else
      snprintf(path, sizeof(path), "/dev/audio/%s", pdevice->d_name);
#endif
      if ((temp_fd = open(path, O_RDWR)) != -1)
        {
          /* Validate it's an Audio device by issuing an
           * AUDIOIOC_GETCAPS ioctl
           */

          if (ioctl(temp_fd, AUDIOIOC_GETCAPS, (unsigned long)&caps) ==
              caps.ac_len)
            {
              /* We have the device file open.  Now issue an
               * AUDIO ioctls to perform a HW reset
               */

              ioctl(temp_fd, AUDIOIOC_HWRESET, 0);
            }

          /* Now close the device */

          close(temp_fd);
        }
    }

  return OK;
}
#endif /* CONFIG_NXLOOPER_INCLUDE_SYSTEM_RESET */
