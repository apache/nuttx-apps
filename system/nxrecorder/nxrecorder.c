/****************************************************************************
 * apps/system/nxrecorder/nxrecorder.c
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
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

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
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_NXRECORDER_FMT_FROM_EXT
struct nxrecorder_ext_fmt_s
{
  FAR const char *ext;
  uint16_t       format;
  CODE int       (*getsubformat)(int fd);
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_AUDIO_FORMAT_MP3
int nxrecorder_getmp3subformat(int fd);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NXRECORDER_FMT_FROM_EXT
static const struct nxrecorder_ext_fmt_s g_known_ext[] =
{
#ifdef CONFIG_AUDIO_FORMAT_AC3
  { "ac3",      AUDIO_FMT_AC3, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_MP3
  { "mp3",      AUDIO_FMT_MP3, nxrecorder_getmp3subformat },
#endif
#ifdef CONFIG_AUDIO_FORMAT_DTS
  { "dts",      AUDIO_FMT_DTS, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_WMA
  { "wma",      AUDIO_FMT_WMA, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_PCM
  { "wav",      AUDIO_FMT_PCM, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_MIDI
  { "mid",      AUDIO_FMT_MIDI, NULL },
  { "midi",     AUDIO_FMT_MIDI, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_OGG_VORBIS
  { "ogg",      AUDIO_FMT_OGG_VORBIS, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_AMR
  { "amr",      AUDIO_FMT_AMR, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_OPUS
  { "opus",     AUDIO_FMT_OPUS, NULL }
#endif
};

static const int g_known_ext_count = sizeof(g_known_ext) /
                    sizeof(struct nxrecorder_ext_fmt_s);
#endif

static const struct nxrecorder_enc_ops_s g_enc_ops[] =
{
  {
    AUDIO_FMT_AMR,
    nxrecorder_write_amr,
    nxrecorder_write_common,
  },
  {
    AUDIO_FMT_PCM,
    NULL,
    nxrecorder_write_common,
  },
  {
    AUDIO_FMT_MP3,
    NULL,
    nxrecorder_write_common,
  },
  {
    AUDIO_FMT_OPUS,
    NULL,
    nxrecorder_write_common,
  }
};

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

static int nxrecorder_opendevice(FAR struct nxrecorder_s *precorder,
                                 int format, int subfmt)
{
  struct audio_caps_s cap;
  bool supported = true;
  int x;

  /* If we have a device, then open it */

  if (precorder->device[0] != '\0')
    {
      /* Use the saved prefformat to test if the requested
       * format is specified by the device
       */

      /* Device supports the format.  Open the device file. */

      precorder->dev_fd = open(precorder->device, O_RDWR | O_CLOEXEC);
      if (precorder->dev_fd == -1)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          auderr("ERROR: Failed to open %s: %d\n",
                 precorder->device, -errcode);
          UNUSED(errcode);
          return -ENOENT;
        }

      cap.ac_len     = sizeof(cap);
      cap.ac_type    = AUDIO_TYPE_QUERY;
      cap.ac_subtype = AUDIO_TYPE_QUERY;

      if (ioctl(precorder->dev_fd, AUDIOIOC_GETCAPS,
                (uintptr_t)&cap) == cap.ac_len)
        {
          if (((cap.ac_format.hw & (1 << (format - 1))) ||
               (cap.ac_format.hw & (1 << (AUDIO_FMT_OTHER - 1)))) &&
              (cap.ac_controls.b[0] & AUDIO_TYPE_INPUT))
            {
              if (!(cap.ac_format.hw & (1 << (format - 1))))
                {
                  /* Get the format supported by the driver
                   * through cap.ac_controls.w
                   */

                  cap.ac_len     = sizeof(cap);
                  cap.ac_type    = AUDIO_TYPE_QUERY;
                  cap.ac_subtype = AUDIO_FMT_OTHER;
                  if (ioctl(precorder->dev_fd, AUDIOIOC_GETCAPS,
                            (uintptr_t)&cap) == cap.ac_len)
                    {
                      if (!(cap.ac_controls.w & (1 << (format - 1))))
                        {
                          supported = false;
                        }
                    }
                }

              /* Test if subformat needed and detected */

              if (subfmt != AUDIO_FMT_UNDEF && supported)
                {
                  /* Prepare to get sub-formats for
                   * this main format
                   */

                  cap.ac_subtype = format;
                  cap.ac_format.b[0] = 0;

                  while (ioctl(precorder->dev_fd, AUDIOIOC_GETCAPS,
                               (uintptr_t)&cap) == cap.ac_len)
                    {
                      /* Check the next set of 4 controls
                       * to find the subformat
                       */

                      for (x = 0; x < sizeof(cap.ac_controls.b); x++)
                        {
                          if (cap.ac_controls.b[x] == subfmt)
                            {
                              /* Sub format supported! */

                              break;
                            }
                          else if (cap.ac_controls.b[x] ==
                                   AUDIO_SUBFMT_END)
                            {
                              /* Sub format not supported */

                              supported = false;
                              break;
                            }
                        }

                      /* If we reached the end of the subformat list,
                       * then break out of the loop.
                       */

                      if (x != sizeof(cap.ac_controls))
                        {
                          break;
                        }

                      /* Increment ac_format.b[0] to get next
                       * set of subformats
                       */

                      cap.ac_format.b[0]++;
                    }
                }

              if (supported)
                {
                  /* Yes, it supports this format.  Use this device */

                  return OK;
                }
            }

            close(precorder->dev_fd);
        }
    }

  /* Device not found */

  auderr("ERROR: Device not found\n");
  precorder->dev_fd = -1;
  return -ENODEV;
}

/****************************************************************************
 * Name: nxrecorder_getmp3subformat
 *
 *   nxrecorder_getmp3subformat() just ruturn AUDIO_SUBFMT_PCM_MP3.
 *
 ****************************************************************************/

#ifdef CONFIG_AUDIO_FORMAT_MP3
int nxrecorder_getmp3subformat(int fd)
{
  return AUDIO_SUBFMT_PCM_MP3;
}
#endif

#ifdef CONFIG_NXRECORDER_FMT_FROM_EXT

/****************************************************************************
 * Name: nxprecorder_fmtfromext
 *
 *   nxrecorder_fmtfromext() tries to determine the file format based
 *   on the extension of the supplied filename.
 *
 ****************************************************************************/

static inline int nxrecorder_fmtfromext(FAR struct nxrecorder_s *precorder,
                                        FAR const char *pfilename,
                                        FAR int *subfmt)
{
  FAR const char *pext;
  uint8_t         x;
  uint8_t         c;

  /* Find the file extension, if any */

  x = strlen(pfilename) - 1;
  while (x > 0)
    {
      /* Search backward for the first '.' */

      if (pfilename[x] == '.')
        {
          /* First '.' found.  Now compare with known extensions */

          pext = &pfilename[x + 1];
          for (c = 0; c < g_known_ext_count; c++)
            {
              /* Test for extension match */

              if (strcasecmp(pext, g_known_ext[c].ext) == 0)
                {
                  /* Test if we have a sub-format detection routine */

                  if (subfmt && g_known_ext[c].getsubformat)
                    {
                      *subfmt = g_known_ext[c].getsubformat(precorder->fd);
                    }

                  /* Return the format for this extension */

                  return g_known_ext[c].format;
                }
            }
        }

      /* Stop if we find a '/' */

      if (pfilename[x] == '/')
        {
          break;
        }

      x--;
    }

  return AUDIO_FMT_UNDEF;
}
#endif

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

  ret = precorder->ops->write_data(precorder->fd, apb);
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
              (uintptr_t)&bufdesc);
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
 * Name: nxrecorder_jointhread
 ****************************************************************************/

static void nxrecorder_jointhread(FAR struct nxrecorder_s *precorder)
{
  FAR void *value;
  int id = 0;

  if (gettid() == precorder->record_id)
    {
      return;
    }

  pthread_mutex_lock(&precorder->mutex);

  if (precorder->record_id > 0)
    {
      id = precorder->record_id;
      precorder->record_id = 0;
    }

  pthread_mutex_unlock(&precorder->mutex);

  if (id > 0)
    {
      pthread_join(id, &value);
    }
}

/****************************************************************************
 * Name: nxrecorder_thread_recordthread
 *
 *  This is the thread that write the audio file file and enqueues /
 *  dequeues buffers from the selected and opened audio device.
 *
 ****************************************************************************/

static FAR void *nxrecorder_recordthread(pthread_addr_t pvarg)
{
  FAR struct nxrecorder_s *precorder = (FAR struct nxrecorder_s *)pvarg;
  struct audio_msg_s      msg;
  struct audio_buf_desc_s buf_desc;
  ssize_t                 size;
  bool                    running = true;
  bool                    streaming = true;
  bool                    failed = false;
  struct ap_buffer_info_s buf_info;
  unsigned int            prio;
#ifdef CONFIG_DEBUG_FEATURES
  int                     outstanding = 0;
#endif
  int                     x;
  int                     ret;

  audinfo("Entry\n");

  /* Query the audio device for its preferred buffer size / qty */

  if ((ret = ioctl(precorder->dev_fd, AUDIOIOC_GETBUFFERINFO,
                   (uintptr_t)&buf_info)) != OK)
    {
      /* Driver doesn't report its buffer size.  Use our default. */

      buf_info.buffer_size = CONFIG_AUDIO_BUFFER_NUMBYTES;
      buf_info.nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  /* Create array of pointers to buffers */

  FAR struct ap_buffer_s *pbuffers[buf_info.nbuffers];

  /* Create our audio pipeline buffers to use for queueing up data */

  memset(pbuffers, 0, sizeof(pbuffers));

  for (x = 0; x < buf_info.nbuffers; x++)
    {
      /* Fill in the buffer descriptor struct to issue an alloc request */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      buf_desc.session = precorder->session;
#endif

      buf_desc.numbytes = buf_info.buffer_size;
      buf_desc.u.pbuffer = &pbuffers[x];

      ret = ioctl(precorder->dev_fd, AUDIOIOC_ALLOCBUFFER,
                  (uintptr_t)&buf_desc);
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
                  (uintptr_t)precorder->session);
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

#ifdef CONFIG_DEBUG_FEATURES
            audinfo("Stopping! outstanding=%d\n", outstanding);
#endif

#ifdef CONFIG_AUDIO_MULTI_SESSION
            ioctl(precorder->dev_fd, AUDIOIOC_STOP,
                  (uintptr_t)precorder->session);
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
#ifdef CONFIG_DEBUG_FEATURES
            audinfo("Record complete.  outstanding=%d\n", outstanding);
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

  audinfo("Freeing buffers\n");
  for (x = 0; x < buf_info.nbuffers; x++)
    {
      /* Fill in the buffer descriptor struct to issue a free request */

      if (pbuffers[x] != NULL)
        {
#ifdef CONFIG_AUDIO_MULTI_SESSION
         buf_desc.session = precorder->session;
#endif
          buf_desc.u.buffer = pbuffers[x];
          ioctl(precorder->dev_fd, AUDIOIOC_FREEBUFFER,
                (uintptr_t)&buf_desc);
        }
    }

  /* Unregister the message queue and release the session */

  ioctl(precorder->dev_fd,
        AUDIOIOC_UNREGISTERMQ,
        (uintptr_t)precorder->mq);
#ifdef CONFIG_AUDIO_MULTI_SESSION
  ioctl(precorder->dev_fd,
        AUDIOIOC_RELEASE,
        (uintptr_t)precorder->session);
#else
  ioctl(precorder->dev_fd,
        AUDIOIOC_RELEASE,
        0);
#endif

  /* Cleanup */

  pthread_mutex_lock(&precorder->mutex);

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

  pthread_mutex_unlock(&precorder->mutex);

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
                  (uintptr_t)precorder->session);
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
                  (uintptr_t)precorder->session);
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

  strlcpy(precorder->device, pdevice, sizeof(precorder->device));

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
  struct audio_msg_s term_msg;

  DEBUGASSERT(precorder != NULL);

  /* Validate we are not in IDLE state */

  pthread_mutex_lock(&precorder->mutex);
  if (precorder->state == NXRECORDER_STATE_IDLE)
    {
      pthread_mutex_unlock(&precorder->mutex);
      return OK;
    }

  pthread_mutex_unlock(&precorder->mutex);

  /* Notify the recordback thread that it needs to cancel the recordback */

  term_msg.msg_id = AUDIO_MSG_STOP;
  term_msg.u.data = 0;
  mq_send(precorder->mq, (FAR const char *)&term_msg, sizeof(term_msg),
          CONFIG_NXRECORDER_MSG_PRIO);

  /* Join the thread.  The thread will do all the cleanup. */

  nxrecorder_jointhread(precorder);

  return OK;
}
#endif /* CONFIG_AUDIO_EXCLUDE_STOP */

/****************************************************************************
 * Name: nxrecorder_recordinteral
 *
 *   nxrecorder_recordinternal() tries to record audio file using the Audio
 *   system. If a device is specified, it will try to use that
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

int nxrecorder_recordinternal(FAR struct nxrecorder_s *precorder,
                              FAR const char *pfilename, int filefmt,
                              uint8_t nchannels, uint8_t bpsamp,
                              uint32_t samprate, uint8_t chmap)
{
  struct mq_attr           attr;
  struct sched_param       sparam;
  pthread_attr_t           tattr;
  struct audio_caps_desc_s cap_desc;
  struct ap_buffer_info_s  buf_info;
  struct audio_caps_s      caps;
  int                      min_channels;
  int                      ret;
  int                      subfmt = AUDIO_FMT_UNDEF;
  int                      index;

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

  if ((precorder->fd = open(pfilename, O_WRONLY | O_CREAT | O_TRUNC,
                            0666)) == -1)
    {
      /* File not found.  Test if its in the mediadir */

      auderr("ERROR: Could not open %s\n", pfilename);
      return -ENOENT;
    }

  if (filefmt == AUDIO_FMT_UNDEF)
    {
      filefmt = nxrecorder_fmtfromext(precorder, pfilename, &subfmt);
    }

  /* Test if we determined the file format */

  if (filefmt == AUDIO_FMT_UNDEF)
    {
      /* Hmmm, it's some unknown / unsupported type */

      auderr("ERROR: Unsupported format: %d\n", filefmt);
      ret = -ENOSYS;
      goto err_out_nodev;
    }

  /* Try to open the device */

  ret = nxrecorder_opendevice(precorder, filefmt, subfmt);
  if (ret < 0)
    {
      /* Error opening the device */

      auderr("ERROR: nxrecorder_opendevice failed: %d\n", ret);
      goto err_out_nodev;
    }

  for (index = 0; index < sizeof(g_enc_ops) / sizeof(g_enc_ops[0]); index++)
    {
      if (g_enc_ops[index].format == filefmt)
        {
          precorder->ops = &g_enc_ops[index];
          break;
        }
    }

  if (!precorder->ops)
    {
      ret = -ENOSYS;
      goto err_out;
    }

  if (precorder->ops->pre_write)
    {
      ret = precorder->ops->pre_write(precorder->fd,
                                      samprate, nchannels, bpsamp);
      if (ret < 0)
        {
          goto err_out;
        }
    }

  /* Try to reserve the device */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(precorder->dev_fd, AUDIOIOC_RESERVE,
              (uintptr_t)&precorder->session);
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

  caps.ac_len = sizeof(caps);
  caps.ac_type = AUDIO_TYPE_INPUT;
  caps.ac_subtype = AUDIO_TYPE_QUERY;

  if (ioctl(precorder->dev_fd, AUDIOIOC_GETCAPS,
      (unsigned long)&caps) == caps.ac_len)
    {
      min_channels = caps.ac_channels >> 4;

      if (min_channels != 0 && nchannels < min_channels)
        {
          ret = -EINVAL;
          goto err_out;
        }
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
  cap_desc.caps.ac_subtype        = filefmt;
  ret = ioctl(precorder->dev_fd, AUDIOIOC_CONFIGURE,
              (uintptr_t)&cap_desc);
  if (ret < 0)
    {
      ret = -errno;
      goto err_out;
    }

  /* Query the audio device for its preferred buffer count */

  if (ioctl(precorder->dev_fd, AUDIOIOC_GETBUFFERINFO,
            (uintptr_t)&buf_info) != OK)
    {
      /* Driver doesn't report its buffer size.  Use our default. */

      buf_info.nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  /* Create a message queue for the recordthread */

  attr.mq_maxmsg  = buf_info.nbuffers + 8;
  attr.mq_msgsize = sizeof(struct audio_msg_s);
  attr.mq_curmsgs = 0;
  attr.mq_flags   = 0;

  snprintf(precorder->mqname, sizeof(precorder->mqname), "/tmp/%0lx",
           (unsigned long)((uintptr_t)precorder));

  precorder->mq = mq_open(precorder->mqname, O_RDWR | O_CREAT, 0644, &attr);
  if (precorder->mq == (mqd_t) -1)
    {
      /* Unable to open message queue! */

      ret = -errno;
      auderr("ERROR: mq_open failed: %d\n", ret);
      goto err_out;
    }

  /* Register our message queue with the audio device */

  ioctl(precorder->dev_fd,
        AUDIOIOC_REGISTERMQ,
        (uintptr_t)precorder->mq);

  /* Check if there was a previous thread and join it if there was
   * to perform clean-up.
   */

  nxrecorder_jointhread(precorder);

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

  precorder = (FAR struct nxrecorder_s *)malloc(sizeof(struct nxrecorder_s));
  if (precorder == NULL)
    {
      return NULL;
    }

  /* Initialize the context data */

  precorder->state = NXRECORDER_STATE_IDLE;
  precorder->dev_fd = -1;
  precorder->fd = -1;
  precorder->device[0] = '\0';
  precorder->mq = 0;
  precorder->record_id = 0;
  precorder->crefs = 1;
  precorder->ops = NULL;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  precorder->session = NULL;
#endif

  pthread_mutex_init(&precorder->mutex, NULL);

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

  /* Check if there was a previous thread and join it if there was */

  nxrecorder_jointhread(precorder);

  pthread_mutex_lock(&precorder->mutex);

  /* Reduce the reference count */

  refcount = precorder->crefs--;
  pthread_mutex_unlock(&precorder->mutex);

  /* If the ref count *was* one, then free the context */

  if (refcount == 1)
    {
      pthread_mutex_destroy(&precorder->mutex);
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
  pthread_mutex_lock(&precorder->mutex);

  /* Increment the reference count */

  precorder->crefs++;
  pthread_mutex_unlock(&precorder->mutex);
}
