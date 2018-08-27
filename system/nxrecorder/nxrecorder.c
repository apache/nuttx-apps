/****************************************************************************
 * apps/system/nxrecorder/nxrecorder.c
 *
 *   Copyright (C) 2017 Pinecone Inc. All rights reserved.
 *   Author: Zhong An <zhongan@pinecone.net>
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

#ifndef CONFIG_AUDIO_NUM_BUFFERS
#  define CONFIG_AUDIO_NUM_BUFFERS  2
#endif

#ifndef CONFIG_AUDIO_BUFFER_NUMBYTES
#  define CONFIG_AUDIO_BUFFER_NUMBYTES  8192
#endif

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

static int nxrecorder_opendevice(FAR struct nxrecorder_s *pRecorder)
{
  /* If we have a device, then open it */

  if (pRecorder->device[0] != '\0')
    {
      /* Use the saved prefformat to test if the requested
       * format is specified by the device
       */

      /* Device supports the format.  Open the device file. */

      pRecorder->devFd = open(pRecorder->device, O_RDWR);
      if (pRecorder->devFd == -1)
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
  pRecorder->devFd = -1;
  return -ENODEV;
}

/****************************************************************************
 * Name: nxrecorder_writebuffer
 *
 *  Write the next block of data to the pcm raw data file into the specified
 *  buffer.
 *
 ****************************************************************************/

static int nxrecorder_writebuffer(FAR struct nxrecorder_s *pRecorder,
                                  FAR struct ap_buffer_s *apb)
{
  int ret;

  /* Validate the file is still open.  It will be closed automatically when
   * we encounter the end of file (or, perhaps, a write error that we cannot
   * handle.
   */

  if (pRecorder->fd == -1)
    {
      /* Return -ENODATA to indicate that there is nothing more to write to
       * the file.
       */

      return -ENODATA;
    }

  /* Write data to the file. */

  ret = write(pRecorder->fd, apb->samp, apb->nbytes);
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
 *   AUDIO_APB_FINAL set) in the event of certain write error occurs or in the
 *   event that the file was an exact multiple of the nmaxbytes size of the
 *   audio buffer.  In that latter case, we have an end of file with no bytes
 *   written.
 *
 *   These infrequent zero length buffers have to be passed through because
 *   the include the AUDIO_APB_FINAL flag that is needed to terminate the
 *   audio stream.
 *
 ****************************************************************************/

static int nxrecorder_enqueuebuffer(FAR struct nxrecorder_s *pRecorder,
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
  bufdesc.session   = pRecorder->session;
#endif
  bufdesc.numbytes  = apb->nbytes;
  bufdesc.u.pBuffer = apb;

  ret = ioctl(pRecorder->devFd, AUDIOIOC_ENQUEUEBUFFER,
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
  struct nxrecorder_s         *pRecorder = (struct nxrecorder_s *) pvarg;
  struct audio_msg_s          msg;
  struct audio_buf_desc_s     buf_desc;
  ssize_t                     size;
  bool                        running = true;
  bool                        streaming = true;
  bool                        failed = false;
#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
  struct ap_buffer_info_s     buf_info;
  FAR struct ap_buffer_s      **pBuffers;
#else
  FAR struct ap_buffer_s      *pBuffers[CONFIG_AUDIO_NUM_BUFFERS];
#endif
#ifdef CONFIG_DEBUG_FEATURES
  int                         outstanding = 0;
#endif
  int                         prio;
  int                         x;
  int                         ret;

  audinfo("Entry\n");

  /* Query the audio device for it's preferred buffer size / qty */

#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
  if ((ret = ioctl(pRecorder->devFd, AUDIOIOC_GETBUFFERINFO,
          (unsigned long) &buf_info)) != OK)
    {
      /* Driver doesn't report it's buffer size.  Use our default. */

      buf_info.buffer_size = CONFIG_AUDIO_BUFFER_NUMBYTES;
      buf_info.nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  /* Create array of pointers to buffers */

  pBuffers = (FAR struct ap_buffer_s **) malloc(buf_info.nbuffers * sizeof(FAR void *));
  if (pBuffers == NULL)
    {
      /* Error allocating memory for buffer storage! */

      ret = -ENOMEM;
      running = false;
      goto err_out;
    }

  /* Create our audio pipeline buffers to use for queueing up data */

  for (x = 0; x < buf_info.nbuffers; x++)
    {
      pBuffers[x] = NULL;
    }

  for (x = 0; x < buf_info.nbuffers; x++)
#else /* CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFER */

  for (x = 0; x < CONFIG_AUDIO_NUM_BUFFERS; x++)
    {
      pBuffers[x] = NULL;
    }

  for (x = 0; x < CONFIG_AUDIO_NUM_BUFFERS; x++)
#endif /* CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFER */
    {
      /* Fill in the buffer descriptor struct to issue an alloc request */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      buf_desc.session = pRecorder->session;
#endif
#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
      buf_desc.numbytes = buf_info.buffer_size;
#else
      buf_desc.numbytes = CONFIG_AUDIO_BUFFER_NUMBYTES;
#endif
      buf_desc.u.ppBuffer = &pBuffers[x];

      ret = ioctl(pRecorder->devFd, AUDIOIOC_ALLOCBUFFER,
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

#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
  for (x = 0; x < buf_info.nbuffers; x++)
#else
  for (x = 0; x < CONFIG_AUDIO_NUM_BUFFERS; x++)
#endif
    {
      /* Write the next buffer of data */

      ret = nxrecorder_enqueuebuffer(pRecorder, pBuffers[x]);
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

          close(pRecorder->fd);
          pRecorder->fd = -1;

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
      ret = ioctl(pRecorder->devFd, AUDIOIOC_START,
                  (unsigned long) pRecorder->session);
#else
      ret = ioctl(pRecorder->devFd, AUDIOIOC_START, 0);
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

      pRecorder->state = NXRECORDER_STATE_RECORDING;

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

  audinfo("%s\n", running ? "Recording..." : "Not runnning");
  while (running)
    {
      /* Wait for a signal either from the Audio driver that it needs
       * additional buffer data, or from a user-space signal to pause,
       * stop, etc.
       */

      size = mq_receive(pRecorder->mq, (FAR char *)&msg, sizeof(msg), &prio);

      /* Validate a message was received */

      if (size != sizeof(msg))
        {
          /* Interrupted by a signal? What to do? */

          continue;
        }

      /* Perform operation based on message id */

      switch (msg.msgId)
        {
          /* An audio buffer is being dequeued by the driver */

          case AUDIO_MSG_DEQUEUE:
#ifdef CONFIG_DEBUG_FEATURES
            /* Make sure that we believe that the audio driver has at
             * least one buffer.
             */

            DEBUGASSERT(msg.u.pPtr && outstanding > 0);
            outstanding--;
#endif

            /* Write data to the file directly into this buffer and
             * re-enqueue it.  streaming == true means that we have
             * not yet hit the end-of-file.
             */

            if (streaming)
              {
                /* Write the next buffer of data */

                ret = nxrecorder_writebuffer(pRecorder, msg.u.pPtr);
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
                    ret = nxrecorder_enqueuebuffer(pRecorder, msg.u.pPtr);
                    if (ret != OK)
                      {
                        /* There is some issue from the audio driver.
                         * Perhaps a problem in the file format?
                         *
                         * We must stop streaming as gracefully as possible.
                         * Close the file so that no further data is written.
                         */

                        close(pRecorder->fd);
                        pRecorder->fd = -1;

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
            ioctl(pRecorder->devFd, AUDIOIOC_STOP,
                 (unsigned long) pRecorder->session);
#else
            ioctl(pRecorder->devFd, AUDIOIOC_STOP, 0);
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

#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
  if (pBuffers != NULL)
    {
      audinfo("Freeing buffers\n");
      for (x = 0; x < buf_info.nbuffers; x++)
        {
          /* Fill in the buffer descriptor struct to issue a free request */

          if (pBuffers[x] != NULL)
            {
#ifdef CONFIG_AUDIO_MULTI_SESSION
              buf_desc.session = pPlayer->session;
#endif
              buf_desc.u.pBuffer = pBuffers[x];
              ioctl(pRecorder->devFd, AUDIOIOC_FREEBUFFER, (unsigned long) &buf_desc);
            }
        }

      /* Free the pointers to the buffers */

      free(pBuffers);
    }
#else
    audinfo("Freeing buffers\n");
    for (x = 0; x < CONFIG_AUDIO_NUM_BUFFERS; x++)
      {
        /* Fill in the buffer descriptor struct to issue a free request */

        if (pBuffers[x] != NULL)
          {
#ifdef CONFIG_AUDIO_MULTI_SESSION
            buf_desc.session = pPlayer->session;
#endif
            buf_desc.u.pBuffer = pBuffers[x];
            ioctl(pRecorder->devFd, AUDIOIOC_FREEBUFFER, (unsigned long) &buf_desc);
          }
      }
#endif

  /* Unregister the message queue and release the session */

  ioctl(pRecorder->devFd, AUDIOIOC_UNREGISTERMQ, (unsigned long) pRecorder->mq);
#ifdef CONFIG_AUDIO_MULTI_SESSION
  ioctl(pRecorder->devFd, AUDIOIOC_RELEASE, (unsigned long) pRecorder->session);
#else
  ioctl(pRecorder->devFd, AUDIOIOC_RELEASE, 0);
#endif

  /* Cleanup */

  while (sem_wait(&pRecorder->sem) < 0)
    {
    }

  /* Close the files */

  if (0 < pRecorder->fd)
    {
      close(pRecorder->fd);                 /* Close the file */
      pRecorder->fd = -1;                   /* Clear out the FD */
    }

  close(pRecorder->devFd);                  /* Close the device */
  pRecorder->devFd = -1;                    /* Mark device as closed */
  mq_close(pRecorder->mq);                  /* Close the message queue */
  mq_unlink(pRecorder->mqname);             /* Unlink the message queue */
  pRecorder->state = NXRECORDER_STATE_IDLE; /* Go to IDLE */

  sem_post(&pRecorder->sem);                /* Release the semaphore */

  /* The record thread is done with the context.  Release it, which may
   * actually cause the context to be freed if the creator has already
   * abandoned (released) the context too.
   */

  nxrecorder_release(pRecorder);

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
int nxrecorder_pause(FAR struct nxrecorder_s *pRecorder)
{
  int ret = OK;

  if (pRecorder->state == NXRECORDER_STATE_RECORDING)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(pRecorder->devFd, AUDIOIOC_PAUSE,
          (unsigned long) pRecorder->session);
#else
      ret = ioctl(pRecorder->devFd, AUDIOIOC_PAUSE, 0);
#endif
      if (ret == OK)
        {
          pRecorder->state = NXRECORDER_STATE_PAUSED;
        }
    }

  return ret;
}
#endif  /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxrecorder_resume
 *
 *   nxrecorder_resume() resumes recordback after a pause operation.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxrecorder_resume(FAR struct nxrecorder_s *pRecorder)
{
  int ret = OK;

  if (pRecorder->state == NXRECORDER_STATE_PAUSED)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(pRecorder->devFd, AUDIOIOC_RESUME,
          (unsigned long) pRecorder->session);
#else
      ret = ioctl(pRecorder->devFd, AUDIOIOC_RESUME, 0);
#endif
      if (ret == OK)
        {
          pRecorder->state = NXRECORDER_STATE_RECORDING;
        }
    }

  return ret;
}
#endif  /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxrecorder_setdevice
 *
 *   nxrecorder_setdevice() sets the audio device to use with the provided
 *   nxrecorder context.
 *
 ****************************************************************************/

int nxrecorder_setdevice(FAR struct nxrecorder_s *pRecorder,
                         FAR const char *pDevice)
{
  int tempFd;

  DEBUGASSERT(pRecorder != NULL);
  DEBUGASSERT(pDevice != NULL);

  /* Try to open the device */

  tempFd = open(pDevice, O_RDWR);
  if (tempFd == -1)
    {
      /* Error opening the device */

      return -ENOENT;
    }

  close(tempFd);

  /* Save the path and format capabilities of the device */

  strncpy(pRecorder->device, pDevice, sizeof(pRecorder->device));

  return OK;
}

/****************************************************************************
 * Name: nxrecorder_stop
 *
 *   nxrecorder_stop() stops the current recordback and closes the file and
 *   the associated device.
 *
 * Input:
 *   pRecorder    Pointer to the initialized MRecorder context
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
int nxrecorder_stop(FAR struct nxrecorder_s *pRecorder)
{
  struct audio_msg_s  term_msg;
  FAR void            *value;

  DEBUGASSERT(pRecorder != NULL);

  /* Validate we are not in IDLE state */

  sem_wait(&pRecorder->sem);                      /* Get the semaphore */
  if (pRecorder->state == NXRECORDER_STATE_IDLE)
    {
      sem_post(&pRecorder->sem);                  /* Release the semaphore */
      return OK;
    }

  sem_post(&pRecorder->sem);

  /* Notify the recordback thread that it needs to cancel the recordback */

  term_msg.msgId = AUDIO_MSG_STOP;
  term_msg.u.data = 0;
  mq_send(pRecorder->mq, (FAR const char *)&term_msg, sizeof(term_msg),
          CONFIG_NXRECORDER_MSG_PRIO);

  /* Join the thread.  The thread will do all the cleanup. */

  pthread_join(pRecorder->recordId, &value);
  pRecorder->recordId = 0;

  return OK;
}
#endif  /* CONFIG_AUDIO_EXCLUDE_STOP */

/****************************************************************************
 * Name: nxrecorder_recordraw
 *
 *   nxrecorder_recordraw() tries to record the raw data file using the Audio
 *   system.  If a device is specified, it will try to use that
 *   device.
 * Input:
 *   pRecorder  Pointer to the initialized MRecorder context
 *   pFilename  Pointer to the filename to record
 *   nchannels  channel num
 *   bpsampe    bit width
 *   samprate   sample rate
 *
 * Returns:
 *   OK         File is being recorded
 *   -EBUSY     The media device is busy
 *   -ENOSYS    The media file is an unsupported type
 *   -ENODEV    No audio device suitable to record the media type
 *   -ENOENT    The media file was not found
 *
 ****************************************************************************/

int nxrecorder_recordraw(FAR struct nxrecorder_s *pRecorder,
                         FAR const char *pFilename, uint8_t nchannels,
                         uint8_t bpsamp, uint32_t samprate)
{
  struct mq_attr           attr;
  struct sched_param       sparam;
  pthread_attr_t           tattr;
  struct audio_caps_desc_s cap_desc;
  FAR void                 *value;
  int                      ret;

  DEBUGASSERT(pRecorder != NULL);
  DEBUGASSERT(pFilename != NULL);

  if (pRecorder->state != NXRECORDER_STATE_IDLE)
    {
      return -EBUSY;
    }

  audinfo("==============================\n");
  audinfo("Recording file %s\n", pFilename);
  audinfo("==============================\n");

  /* Test that the specified file exists */

  if ((pRecorder->fd = open(pFilename, O_WRONLY | O_CREAT)) == -1)
    {
      /* File not found.  Test if its in the mediadir */

        auderr("ERROR: Could not open %s\n", pFilename);
        return -ENOENT;
    }

  /* Try to open the device */

  ret = nxrecorder_opendevice(pRecorder);
  if (ret < 0)
    {
      /* Error opening the device */

      auderr("ERROR: nxrecorder_opendevice failed: %d\n", ret);
      goto err_out_nodev;
    }

  /* Try to reserve the device */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(pRecorder->devFd, AUDIOIOC_RESERVE,
              (unsigned long)&pRecorder->session);
#else
  ret = ioctl(pRecorder->devFd, AUDIOIOC_RESERVE, 0);
#endif
  if (ret < 0)
    {
      /* Device is busy or error */

      auderr("ERROR: Failed to reserve device: %d\n", ret);
      ret = -errno;
      goto err_out;
    }

#ifdef CONFIG_AUDIO_MULTI_SESSION
  cap_desc.session = pRecorder->session;
#endif
  cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
  cap_desc.caps.ac_type = AUDIO_TYPE_INPUT;
  cap_desc.caps.ac_channels = nchannels ? nchannels : 2;
  cap_desc.caps.ac_controls.hw[0] = samprate ? samprate : 48000;
  cap_desc.caps.ac_controls.b[3] = samprate >> 16;
  cap_desc.caps.ac_controls.b[2]  = bpsamp ? bpsamp : 16;
  ret = ioctl(pRecorder->devFd, AUDIOIOC_CONFIGURE,
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

  snprintf(pRecorder->mqname, sizeof(pRecorder->mqname), "/tmp/%0lx",
           (unsigned long)((uintptr_t)pRecorder));

  pRecorder->mq = mq_open(pRecorder->mqname, O_RDWR | O_CREAT, 0644, &attr);
  if (pRecorder->mq == NULL)
    {
      /* Unable to open message queue! */

      ret = -errno;
      auderr("ERROR: mq_open failed: %d\n", ret);
      goto err_out;
    }

  /* Register our message queue with the audio device */

  ioctl(pRecorder->devFd, AUDIOIOC_REGISTERMQ, (unsigned long)pRecorder->mq);

  /* Check if there was a previous thread and join it if there was
   * to perform clean-up.
   */

  if (pRecorder->recordId != 0)
    {
      pthread_join(pRecorder->recordId, &value);
    }

  /* Start the recordfile thread to stream the media file to the
   * audio device.
   */

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 9;
  (void)pthread_attr_setschedparam(&tattr, &sparam);
  (void)pthread_attr_setstacksize(&tattr,
                                  CONFIG_NXRECORDER_RECORDTHREAD_STACKSIZE);

  /* Add a reference count to the recorder for the thread and start the
   * thread.  We increment for the thread to avoid thread start-up
   * race conditions.
   */

  nxrecorder_reference(pRecorder);
  ret = pthread_create(&pRecorder->recordId, &tattr, nxrecorder_recordthread,
                       (pthread_addr_t) pRecorder);
  if (ret != OK)
    {
      auderr("ERROR: Failed to create recordthread: %d\n", ret);
      goto err_out;
    }

  /* Name the thread */

  pthread_setname_np(pRecorder->recordId, "recordthread");
  return OK;

err_out:
  close(pRecorder->devFd);
  pRecorder->devFd = -1;

err_out_nodev:
  if (0 < pRecorder->fd)
    {
      close(pRecorder->fd);
      pRecorder->fd = -1;
    }

  return ret;
}

/****************************************************************************
 * Name: nxrecorder_create
 *
 *   nxrecorder_create() allocates and initializes a nxrecorder context for
 *   use by further nxrecorder operations.  This routine must be called before
 *   to perform the create for proper reference counting.
 *
 * Input Parameters:  None
 *
 * Returned values:
 *   Pointer to the created context or NULL if there was an error.
 *
 ****************************************************************************/

FAR struct nxrecorder_s *nxrecorder_create(void)
{
  FAR struct nxrecorder_s *pRecorder;

  /* Allocate the memory */

  pRecorder = (FAR struct nxrecorder_s *) malloc(sizeof(struct nxrecorder_s));
  if (pRecorder == NULL)
    {
      return NULL;
    }

  /* Initialize the context data */

  pRecorder->state = NXRECORDER_STATE_IDLE;
  pRecorder->devFd = -1;
  pRecorder->fd = -1;
  pRecorder->device[0] = '\0';
  pRecorder->mq = NULL;
  pRecorder->recordId = 0;
  pRecorder->crefs = 1;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  pRecorder->session = NULL;
#endif

  sem_init(&pRecorder->sem, 0, 1);

  return pRecorder;
}

/****************************************************************************
 * Name: nxrecorder_release
 *
 *   nxrecorder_release() reduces the reference count by one and if it
 *   reaches zero, frees the context.
 *
 * Input Parameters:
 *   pRecorder    Pointer to the NxRecorder context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxrecorder_release(FAR struct nxrecorder_s *pRecorder)
{
  int         refcount;
  FAR void    *value;

  /* Grab the semaphore */

  while (sem_wait(&pRecorder->sem) < 0)
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

  if (pRecorder->recordId != 0)
    {
      sem_post(&pRecorder->sem);
      pthread_join(pRecorder->recordId, &value);
      pRecorder->recordId = 0;

      while (sem_wait(&pRecorder->sem) < 0)
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

  refcount = pRecorder->crefs--;
  sem_post(&pRecorder->sem);

  /* If the ref count *was* one, then free the context */

  if (refcount == 1)
    {
      free(pRecorder);
    }
}

/****************************************************************************
 * Name: nxrecorder_reference
 *
 *   nxrecorder_reference() increments the reference count by one.
 *
 * Input Parameters:
 *   pRecorder    Pointer to the NxRecorder context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxrecorder_reference(FAR struct nxrecorder_s *pRecorder)
{
  /* Grab the semaphore */

  while (sem_wait(&pRecorder->sem) < 0)
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

  pRecorder->crefs++;
  sem_post(&pRecorder->sem);
}
