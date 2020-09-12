/****************************************************************************
 * apps/system/nxrecorder/nxrecorder.c
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
#include <errno.h>
#include <dirent.h>
#include <debug.h>

#include <nuttx/audio/audio.h>
#include "system/nxrecorder.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXRECORDER_STATE_IDLE      0
#define NXRECORDER_STATE_RECORDING 1
#define NXRECORDER_STATE_PAUSED    2

#ifndef CONFIG_NXRECORDER_MSG_PRIO
#  define CONFIG_NXRECORDER_MSG_PRIO  1
#endif

#ifndef CONFIG_NXRECORDER_RECORDTHREAD_STACKSIZE
#  define CONFIG_NXRECORDER_RECORDTHREAD_STACKSIZE    1500
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxrecorder_opendevice
 *
 *   nxrecorder_opendevice() either searches the Audio system for a device
 *   that is compatible with the specified audio format and opens it, or
 *   tries to open the device if specified and validates that it supports
 *   the requested format.
 *
 * Return:
 *    OK        if compatible device opened
 *    -ENODEV   if no compatible device opened.
 *    -ENOENT   if device couldn't be opened.
 *
 ****************************************************************************/

static int nxrecorder_opendevice(FAR struct nxrecorder_s *precorder)
{
  /* If we have a device, then open it */

  if (precorder->device[0] != '\0')
    {
      /* Use the saved prefformat to test if the requested
       * format is specified by the device
       */

      /* Device supports the format.  Open the device file. */

      precorder->dev_fd = open(precorder->device, O_RDWR);
      if (precorder->dev_fd == -1)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          auderr("ERROR: Failed to open %s: %d\n", -errcode);
          UNUSED(errcode);
          return -ENOENT;
        }

      return OK;
    }

  /* Device not found */

  auderr("ERROR: Device not found\n");
  precorder->dev_fd = -1;
  return -ENODEV;
}

/****************************************************************************
 * Name: nxrecorder_writebuffer
 *
 *  Write the next block of data to the pcm raw data file into the specified
 *  buffer.
 *
 ****************************************************************************/

static int nxrecorder_writebuffer(FAR struct nxrecorder_s *precorder,
                                  FAR struct ap_buffer_s *apb)
{
  int ret;

  /* Validate the file is still open.  It will be closed automatically when
   * we encounter the end of file (or, perhaps, a write error that we cannot
   * handle.
   */

  if (precorder->fd == -1)
    {
      /* Return -ENODATA to indicate that there is nothing more to write to
       * the file.
       */

      return -ENODATA;
    }

  /* Write data to the file. */

  ret = write(precorder->fd, apb->samp, apb->nbytes);
  if (ret < 0)
    {
      return ret;
    }

  apb->curbyte = 0;
  apb->flags   = 0;

  /* Return OK to indicate that the buffer should be passed through to the
   * audio device.  This does not necessarily indicate that data was written
   * correctly.
   */

  return OK;
}

/****************************************************************************
 * Name: nxrecorder_enqueuebuffer
 *
 * Description:
 *   Enqueue the audio buffer in the downstream device.  Normally we are
 *   called with a buffer of data to be enqueued in the audio stream.
 *
 *   Be we may also receive an empty length buffer (with only the
 *   AUDIO_APB_FINAL set) in the event of certain write error occurs or in
 *   the event that the file was an exact multiple of the nmaxbytes size of
 *   the audio buffer.
 *   In that latter case, we have an end of file with no bytes written.
 *
 *   These infrequent zero length buffers have to be passed through because
 *   the include the AUDIO_APB_FINAL flag that is needed to terminate the
 *   audio stream.
 *
 ****************************************************************************/

static int nxrecorder_enqueuebuffer(FAR struct nxrecorder_s *precorder,
                                    FAR struct ap_buffer_s *apb)
{
  struct audio_buf_desc_s bufdesc;
  int ret;

  /* Now enqueue the buffer with the audio device.  If the number of
   * bytes in the file happens to be an exact multiple of the audio
   * buffer size, then we will receive the last buffer size = 0.  We
   * encode this buffer also so the audio system knows its the end of
   * the file and can do proper clean-up.
   */

  apb->nbytes = apb->nmaxbytes;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  bufdesc.session   = precorder->session;
#endif
  bufdesc.numbytes  = apb->nbytes;
  bufdesc.u.buffer = apb;

  ret = ioctl(precorder->dev_fd, AUDIOIOC_ENQUEUEBUFFER,
              (unsigned long)&bufdesc);
  if (ret < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      auderr("ERROR: AUDIOIOC_ENQUEUEBUFFER ioctl failed: %d\n", errcode);
      return -errcode;
    }

  /* Return OK to indicate that we successfully write data to the file
   * (and we are not yet at the end of file)
   */

  return OK;
}

/****************************************************************************
 * Name: nxrecorder_thread_recordthread
 *
 *  This is the thread that write the audio file file and enqueues /
 *  dequeues buffers from the selected and opened audio device.
 *
 ****************************************************************************/

static void *nxrecorder_recordthread(pthread_addr_t pvarg)
{
  struct nxrecorder_s         *precorder = (struct nxrecorder_s *) pvarg;
  struct audio_msg_s          msg;
  struct audio_buf_desc_s     buf_desc;
  ssize_t                     size;
  bool                        running = true;
  bool                        streaming = true;
  bool                        failed = false;
  struct ap_buffer_info_s     buf_info;
  FAR struct ap_buffer_s      **pbuffers;
  unsigned int                prio;
#ifdef CONFIG_DEBUG_FEATURES
  int                         outstanding = 0;
#endif
  int                         x;
  int                         ret;

  audinfo("Entry\n");

  /* Query the audio device for it's preferred buffer size / qty */

  if ((ret = ioctl(precorder->dev_fd, AUDIOIOC_GETBUFFERINFO,
          (unsigned long) &buf_info)) != OK)
    {
      /* Driver doesn't report it's buffer size.  Use our default. */

      buf_info.buffer_size = CONFIG_AUDIO_BUFFER_NUMBYTES;
      buf_info.nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  /* Create array of pointers to buffers */

  pbuffers = (FAR struct ap_buffer_s **) malloc(buf_info.nbuffers *
                                                sizeof(FAR void *));
  if (pbuffers == NULL)
    {
      /* Error allocating memory for buffer storage! */

      ret = -ENOMEM;
      running = false;
      goto err_out;
    }

  /* Create our audio pipeline buffers to use for queueing up data */

  for (x = 0; x < buf_info.nbuffers; x++)
    {
      pbuffers[x] = NULL;
    }

  for (x = 0; x < buf_info.nbuffers; x++)
    {
      /* Fill in the buffer descriptor struct to issue an alloc request */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      buf_desc.session = precorder->session;
#endif

      buf_desc.numbytes = buf_info.buffer_size;
      buf_desc.u.pbuffer = &pbuffers[x];

      ret = ioctl(precorder->dev_fd, AUDIOIOC_ALLOCBUFFER,
                  (unsigned long) &buf_desc);
      if (ret != sizeof(buf_desc))
        {
          /* Buffer alloc Operation not supported or error allocating! */

          auderr("ERROR: Could not allocate buffer %d\n", x);
          running = false;
          goto err_out;
        }
    }

  /* Fill up the pipeline with enqueued buffers */

  for (x = 0; x < buf_info.nbuffers; x++)
    {
      /* Write the next buffer of data */

      ret = nxrecorder_enqueuebuffer(precorder, pbuffers[x]);
      if (ret != OK)
        {
          /* Failed to enqueue the buffer.  The driver is not happy with
           * the buffer.  Perhaps a encoder has detected something that it
           * does not like in the stream and has stopped streaming.  This
           * would happen normally if we send a file in the incorrect format
           * to an audio encoder.
           *
           * We must stop streaming as gracefully as possible.  Close the
           * file so that no further data is written.
           */

          close(precorder->fd);
          precorder->fd = -1;

          /* We are no longer streaming data to the file.  Be we will
           * need to wait for any outstanding buffers to be recovered.  We
           * also still expect the audio driver to send a AUDIO_MSG_COMPLETE
           * message after all queued buffers have been returned.
           */

           streaming = false;
           failed = true;
           break;
        }
#ifdef CONFIG_DEBUG_FEATURES
      else
        {
          /* The audio driver has one more buffer */

          outstanding++;
        }
#endif
    }

  audinfo("%d buffers queued, running=%d streaming=%d\n",
          x, running, streaming);

  /* Start the audio device */

  if (running && !failed)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(precorder->dev_fd, AUDIOIOC_START,
                  (unsigned long) precorder->session);
#else
      ret = ioctl(precorder->dev_fd, AUDIOIOC_START, 0);
#endif

      if (ret < 0)
        {
          /* Error starting the audio stream!  We need to continue running
           * in order to recover the audio buffers that have already been
           * queued.
           */

          failed = true;
        }
    }

  if (running && !failed)
    {
      /* Indicate we are recording a file */

      precorder->state = NXRECORDER_STATE_RECORDING;
    }

  /* Loop until we specifically break.  running == true means that we are
   * still looping waiting for the recordback to complete.  All of the file
   * data may have been sent (if streaming == false), but the recordback is
   * not complete until we get the AUDIO_MSG_COMPLETE (or AUDIO_MSG_STOP)
   * message
   *
   * The normal protocol for streaming errors detected by the audio driver
   * is as follows:
   *
   * (1) The audio driver will indicated the error by returning a negated
   *     error value when the next buffer is enqueued.  The upper level
   *     then knows that this buffer was not queue.
   * (2) The audio driver must return all queued buffers using the
   *     AUDIO_MSG_DEQUEUE message, and
   * (3) Terminate recording by sending the AUDIO_MSG_COMPLETE message.
   */

  audinfo("%s\n", running ? "Recording..." : "Not running");
  while (running)
    {
      /* Wait for a signal either from the Audio driver that it needs
       * additional buffer data, or from a user-space signal to pause,
       * stop, etc.
       */

      size = mq_receive(precorder->mq, (FAR char *)&msg, sizeof(msg), &prio);

      /* Validate a message was received */

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
#ifdef CONFIG_DEBUG_FEATURES
            /* Make sure that we believe that the audio driver has at
             * least one buffer.
             */

            DEBUGASSERT(msg.u.ptr && outstanding > 0);
            outstanding--;
#endif

            /* Write data to the file directly into this buffer and
             * re-enqueue it.  streaming == true means that we have
             * not yet hit the end-of-file.
             */

            if (streaming)
              {
                /* Write the next buffer of data */

                ret = nxrecorder_writebuffer(precorder, msg.u.ptr);
                if (ret != OK)
                  {
                    /* Out of data.  Stay in the loop until the device sends
                     * us a COMPLETE message, but stop trying to record more
                     * data.
                     */

                    streaming = false;
                  }

                /* Enqueue buffer by sending it to the audio driver */

                else
                  {
                    ret = nxrecorder_enqueuebuffer(precorder, msg.u.ptr);
                    if (ret != OK)
                      {
                        /* There is some issue from the audio driver.
                         * Perhaps a problem in the file format?
                         *
                         * We must stop streaming as gracefully as possible.
                         * Close the file so that no further data is written.
                         */

                        close(precorder->fd);
                        precorder->fd = -1;

                        /* Stop streaming and wait for buffers to be
                         * returned and to receive the AUDIO_MSG_COMPLETE
                         * indication.
                         */

                        streaming = false;
                        failed = true;
                      }
#ifdef CONFIG_DEBUG_FEATURES
                    else
                      {
                        /* The audio driver has one more buffer */

                        outstanding++;
                      }
#endif
                  }
              }
            break;

          /* Someone wants to stop the recordback. */

          case AUDIO_MSG_STOP:

            /* Send a stop message to the device */

            audinfo("Stopping! outstanding=%d\n", outstanding);

#ifdef CONFIG_AUDIO_MULTI_SESSION
            ioctl(precorder->dev_fd, AUDIOIOC_STOP,
                 (unsigned long) precorder->session);
#else
            ioctl(precorder->dev_fd, AUDIOIOC_STOP, 0);
#endif
            /* Stay in the running loop (without sending more data).
             * we will need to recover our audio buffers.  We will
             * loop until AUDIO_MSG_COMPLETE is received.
             */

            streaming = false;
            break;

          /* Message indicating the recordback is complete */

          case AUDIO_MSG_COMPLETE:
            audinfo("Record complete.  outstanding=%d\n", outstanding);
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

  if (pbuffers != NULL)
    {
      audinfo("Freeing buffers\n");
      for (x = 0; x < buf_info.nbuffers; x++)
        {
          /* Fill in the buffer descriptor struct to issue a free request */

          if (pbuffers[x] != NULL)
            {
#ifdef CONFIG_AUDIO_MULTI_SESSION
              buf_desc.session = pplayer->session;
#endif
              buf_desc.u.buffer = pbuffers[x];
              ioctl(precorder->dev_fd,
                    AUDIOIOC_FREEBUFFER,
                    (unsigned long) &buf_desc);
            }
        }

      /* Free the pointers to the buffers */

      free(pbuffers);
    }

  /* Unregister the message queue and release the session */

  ioctl(precorder->dev_fd,
        AUDIOIOC_UNREGISTERMQ,
        (unsigned long) precorder->mq);
#ifdef CONFIG_AUDIO_MULTI_SESSION
  ioctl(precorder->dev_fd,
        AUDIOIOC_RELEASE,
        (unsigned long) precorder->session);
#else
  ioctl(precorder->dev_fd,
        AUDIOIOC_RELEASE,
        0);
#endif

  /* Cleanup */

  while (sem_wait(&precorder->sem) < 0)
    {
    }

  /* Close the files */

  if (0 < precorder->fd)
    {
      close(precorder->fd);                 /* Close the file */
      precorder->fd = -1;                   /* Clear out the FD */
    }

  close(precorder->dev_fd);                 /* Close the device */
  precorder->dev_fd = -1;                   /* Mark device as closed */
  mq_close(precorder->mq);                  /* Close the message queue */
  mq_unlink(precorder->mqname);             /* Unlink the message queue */
  precorder->state = NXRECORDER_STATE_IDLE; /* Go to IDLE */

  sem_post(&precorder->sem);                /* Release the semaphore */

  /* The record thread is done with the context.  Release it, which may
   * actually cause the context to be freed if the creator has already
   * abandoned (released) the context too.
   */

  nxrecorder_release(precorder);

  audinfo("Exit\n");

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxrecorder_pause
 *
 *   nxrecorder_pause() pauses recordback without cancelling it.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxrecorder_pause(FAR struct nxrecorder_s *precorder)
{
  int ret = OK;

  if (precorder->state == NXRECORDER_STATE_RECORDING)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(precorder->dev_fd, AUDIOIOC_PAUSE,
          (unsigned long) precorder->session);
#else
      ret = ioctl(precorder->dev_fd, AUDIOIOC_PAUSE, 0);
#endif
      if (ret == OK)
        {
          precorder->state = NXRECORDER_STATE_PAUSED;
        }
    }

  return ret;
}
#endif /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxrecorder_resume
 *
 *   nxrecorder_resume() resumes recordback after a pause operation.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxrecorder_resume(FAR struct nxrecorder_s *precorder)
{
  int ret = OK;

  if (precorder->state == NXRECORDER_STATE_PAUSED)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(precorder->dev_fd, AUDIOIOC_RESUME,
          (unsigned long) precorder->session);
#else
      ret = ioctl(precorder->dev_fd, AUDIOIOC_RESUME, 0);
#endif
      if (ret == OK)
        {
          precorder->state = NXRECORDER_STATE_RECORDING;
        }
    }

  return ret;
}
#endif /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxrecorder_setdevice
 *
 *   nxrecorder_setdevice() sets the audio device to use with the provided
 *   nxrecorder context.
 *
 ****************************************************************************/

int nxrecorder_setdevice(FAR struct nxrecorder_s *precorder,
                         FAR const char *pdevice)
{
  int temp_fd;

  DEBUGASSERT(precorder != NULL);
  DEBUGASSERT(pdevice != NULL);

  /* Try to open the device */

  temp_fd = open(pdevice, O_RDWR);
  if (temp_fd == -1)
    {
      /* Error opening the device */

      return -ENOENT;
    }

  close(temp_fd);

  /* Save the path and format capabilities of the device */

  strncpy(precorder->device, pdevice, sizeof(precorder->device));

  return OK;
}

/****************************************************************************
 * Name: nxrecorder_stop
 *
 *   nxrecorder_stop() stops the current recordback and closes the file and
 *   the associated device.
 *
 * Input:
 *   precorder    Pointer to the initialized MRecorder context
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
int nxrecorder_stop(FAR struct nxrecorder_s *precorder)
{
  struct audio_msg_s  term_msg;
  FAR void            *value;

  DEBUGASSERT(precorder != NULL);

  /* Validate we are not in IDLE state */

  sem_wait(&precorder->sem);                      /* Get the semaphore */
  if (precorder->state == NXRECORDER_STATE_IDLE)
    {
      sem_post(&precorder->sem);                  /* Release the semaphore */
      return OK;
    }

  sem_post(&precorder->sem);

  /* Notify the recordback thread that it needs to cancel the recordback */

  term_msg.msg_id = AUDIO_MSG_STOP;
  term_msg.u.data = 0;
  mq_send(precorder->mq, (FAR const char *)&term_msg, sizeof(term_msg),
          CONFIG_NXRECORDER_MSG_PRIO);

  /* Join the thread.  The thread will do all the cleanup. */

  pthread_join(precorder->record_id, &value);
  precorder->record_id = 0;

  return OK;
}
#endif /* CONFIG_AUDIO_EXCLUDE_STOP */

/****************************************************************************
 * Name: nxrecorder_recordraw
 *
 *   nxrecorder_recordraw() tries to record the raw data file using the Audio
 *   system.  If a device is specified, it will try to use that
 *   device.
 * Input:
 *   precorder  Pointer to the initialized MRecorder context
 *   pfilename  Pointer to the filename to record
 *   nchannels  channel num
 *   bpsampe    bit width
 *   samprate   sample rate
 *   chmap      channel map
 *
 * Returns:
 *   OK         File is being recorded
 *   -EBUSY     The media device is busy
 *   -ENOSYS    The media file is an unsupported type
 *   -ENODEV    No audio device suitable to record the media type
 *   -ENOENT    The media file was not found
 *
 ****************************************************************************/

int nxrecorder_recordraw(FAR struct nxrecorder_s *precorder,
                         FAR const char *pfilename, uint8_t nchannels,
                         uint8_t bpsamp, uint32_t samprate, uint8_t chmap)
{
  struct mq_attr           attr;
  struct sched_param       sparam;
  pthread_attr_t           tattr;
  struct audio_caps_desc_s cap_desc;
  FAR void                 *value;
  int                      ret;

  DEBUGASSERT(precorder != NULL);
  DEBUGASSERT(pfilename != NULL);

  if (precorder->state != NXRECORDER_STATE_IDLE)
    {
      return -EBUSY;
    }

  audinfo("==============================\n");
  audinfo("Recording file %s\n", pfilename);
  audinfo("==============================\n");

  /* Test that the specified file exists */

  if ((precorder->fd = open(pfilename, O_WRONLY | O_CREAT | O_TRUNC)) == -1)
    {
      /* File not found.  Test if its in the mediadir */

      auderr("ERROR: Could not open %s\n", pfilename);
      return -ENOENT;
    }

  /* Try to open the device */

  ret = nxrecorder_opendevice(precorder);
  if (ret < 0)
    {
      /* Error opening the device */

      auderr("ERROR: nxrecorder_opendevice failed: %d\n", ret);
      goto err_out_nodev;
    }

  /* Try to reserve the device */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(precorder->dev_fd, AUDIOIOC_RESERVE,
              (unsigned long)&precorder->session);
#else
  ret = ioctl(precorder->dev_fd, AUDIOIOC_RESERVE, 0);
#endif
  if (ret < 0)
    {
      /* Device is busy or error */

      auderr("ERROR: Failed to reserve device: %d\n", ret);
      ret = -errno;
      goto err_out;
    }

#ifdef CONFIG_AUDIO_MULTI_SESSION
  cap_desc.session = precorder->session;
#endif
  cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
  cap_desc.caps.ac_type = AUDIO_TYPE_INPUT;
  cap_desc.caps.ac_channels = nchannels ? nchannels : 2;
  cap_desc.caps.ac_chmap    = chmap;
  cap_desc.caps.ac_controls.hw[0] = samprate ? samprate : 48000;
  cap_desc.caps.ac_controls.b[3] = samprate >> 16;
  cap_desc.caps.ac_controls.b[2]  = bpsamp ? bpsamp : 16;
  ret = ioctl(precorder->dev_fd, AUDIOIOC_CONFIGURE,
              (unsigned long)&cap_desc);
  if (ret < 0)
    {
      ret = -errno;
      goto err_out;
    }

  /* Create a message queue for the recordthread */

  attr.mq_maxmsg  = 16;
  attr.mq_msgsize = sizeof(struct audio_msg_s);
  attr.mq_curmsgs = 0;
  attr.mq_flags   = 0;

  snprintf(precorder->mqname, sizeof(precorder->mqname), "/tmp/%0lx",
           (unsigned long)((uintptr_t)precorder));

  precorder->mq = mq_open(precorder->mqname, O_RDWR | O_CREAT, 0644, &attr);
  if (precorder->mq == NULL)
    {
      /* Unable to open message queue! */

      ret = -errno;
      auderr("ERROR: mq_open failed: %d\n", ret);
      goto err_out;
    }

  /* Register our message queue with the audio device */

  ioctl(precorder->dev_fd,
        AUDIOIOC_REGISTERMQ,
        (unsigned long)precorder->mq);

  /* Check if there was a previous thread and join it if there was
   * to perform clean-up.
   */

  if (precorder->record_id != 0)
    {
      pthread_join(precorder->record_id, &value);
    }

  /* Start the recordfile thread to stream the media file to the
   * audio device.
   */

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 9;
  pthread_attr_setschedparam(&tattr, &sparam);
  pthread_attr_setstacksize(&tattr,
                            CONFIG_NXRECORDER_RECORDTHREAD_STACKSIZE);

  /* Add a reference count to the recorder for the thread and start the
   * thread.  We increment for the thread to avoid thread start-up
   * race conditions.
   */

  nxrecorder_reference(precorder);
  ret = pthread_create(&precorder->record_id,
                       &tattr,
                       nxrecorder_recordthread,
                       (pthread_addr_t) precorder);
  if (ret != OK)
    {
      auderr("ERROR: Failed to create recordthread: %d\n", ret);
      goto err_out;
    }

  /* Name the thread */

  pthread_setname_np(precorder->record_id, "recordthread");
  return OK;

err_out:
  close(precorder->dev_fd);
  precorder->dev_fd = -1;

err_out_nodev:
  if (0 < precorder->fd)
    {
      close(precorder->fd);
      precorder->fd = -1;
    }

  return ret;
}

/****************************************************************************
 * Name: nxrecorder_create
 *
 *   nxrecorder_create() allocates and initializes a nxrecorder context for
 *   use by further nxrecorder operations.  This routine must be called
 *   before to perform the create for proper reference counting.
 *
 * Input Parameters:  None
 *
 * Returned values:
 *   Pointer to the created context or NULL if there was an error.
 *
 ****************************************************************************/

FAR struct nxrecorder_s *nxrecorder_create(void)
{
  FAR struct nxrecorder_s *precorder;

  /* Allocate the memory */

  precorder = (FAR struct nxrecorder_s *) malloc(
                                           sizeof(struct nxrecorder_s));
  if (precorder == NULL)
    {
      return NULL;
    }

  /* Initialize the context data */

  precorder->state = NXRECORDER_STATE_IDLE;
  precorder->dev_fd = -1;
  precorder->fd = -1;
  precorder->device[0] = '\0';
  precorder->mq = NULL;
  precorder->record_id = 0;
  precorder->crefs = 1;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  precorder->session = NULL;
#endif

  sem_init(&precorder->sem, 0, 1);

  return precorder;
}

/****************************************************************************
 * Name: nxrecorder_release
 *
 *   nxrecorder_release() reduces the reference count by one and if it
 *   reaches zero, frees the context.
 *
 * Input Parameters:
 *   precorder    Pointer to the NxRecorder context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxrecorder_release(FAR struct nxrecorder_s *precorder)
{
  int         refcount;
  FAR void    *value;

  /* Grab the semaphore */

  while (sem_wait(&precorder->sem) < 0)
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

  if (precorder->record_id != 0)
    {
      sem_post(&precorder->sem);
      pthread_join(precorder->record_id, &value);
      precorder->record_id = 0;

      while (sem_wait(&precorder->sem) < 0)
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

  refcount = precorder->crefs--;
  sem_post(&precorder->sem);

  /* If the ref count *was* one, then free the context */

  if (refcount == 1)
    {
      free(precorder);
    }
}

/****************************************************************************
 * Name: nxrecorder_reference
 *
 *   nxrecorder_reference() increments the reference count by one.
 *
 * Input Parameters:
 *   precorder    Pointer to the NxRecorder context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxrecorder_reference(FAR struct nxrecorder_s *precorder)
{
  /* Grab the semaphore */

  while (sem_wait(&precorder->sem) < 0)
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

  precorder->crefs++;
  sem_post(&precorder->sem);
}
