/****************************************************************************
 * apps/system/nxplayer/nxplayer.c
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
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
#include <nuttx/audio/audio.h>
#include <debug.h>

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <dirent.h>

#include <apps/nxplayer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXPLAYER_STATE_IDLE      0
#define NXPLAYER_STATE_PLAYING   1
#define NXPLAYER_STATE_PAUSED    2

#ifndef CONFIG_AUDIO_NUM_BUFFERS
#  define CONFIG_AUDIO_NUM_BUFFERS  2
#endif

#ifndef CONFIG_AUDIO_BUFFER_NUMBYTES
#  define CONFIG_AUDIO_BUFFER_NUMBYTES  8192
#endif

#ifndef CONFIG_NXPLAYER_MSG_PRIO
#  define CONFIG_NXPLAYER_MSG_PRIO  1
#endif

#ifndef CONFIG_NXPLAYER_PLAYTHREAD_STACKSIZE
#  define CONFIG_NXPLAYER_PLAYTHREAD_STACKSIZE    1500
#endif

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_FMT_FROM_EXT
struct nxplayer_ext_fmt_s
{
  const char  *ext;
  uint16_t    format;
  CODE int    (*getsubformat)(FAR FILE *fd);
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_AUDIO_FORMAT_MIDI
int nxplayer_getmidisubformat(FAR FILE *fd);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_FMT_FROM_EXT
static const struct nxplayer_ext_fmt_s g_known_ext[] = {
#ifdef CONFIG_AUDIO_FORMAT_AC3
  { "ac3",      AUDIO_FMT_AC3, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_MP3
  { "mp3",      AUDIO_FMT_MP3, NULL },
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
  { "mid",      AUDIO_FMT_MIDI, nxplayer_getmidisubformat },
  { "midi",     AUDIO_FMT_MIDI, nxplayer_getmidisubformat },
#endif
#ifdef CONFIG_AUDIO_FORMAT_OGG_VORBIS
  { "ogg",      AUDIO_FMT_OGG_VORBIS, NULL }
#endif
};
static const int g_known_ext_count = sizeof(g_known_ext) /
                    sizeof(struct nxplayer_ext_fmt_s);
#endif /* CONFIG_NXPLAYER_FMT_FROM_EXT */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxplayer_opendevice
 *
 *   nxplayer_opendevice() either searches the Audio system for a device
 *   that is compatible with the specified audio format and opens it, or
 *   tries to open the prefered device if specified and validates that
 *   it supports the requested format.
 *
 * Return:
 *    OK        if compatible device opened (searched or preferred)
 *    -ENODEV   if no compatible device opened.
 *    -ENOENT   if preferred device couldn't be opened.
 *
 ****************************************************************************/

static int nxplayer_opendevice(FAR struct nxplayer_s *pPlayer, int format,
    int subfmt)
{
  struct dirent*        pDevice;
  DIR*                  dirp;
  char                  path[64];
  struct audio_caps_s   caps;
  uint8_t               supported = TRUE;
  uint8_t               x;

  /* If we have a preferred device, then open it */

#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
  if (pPlayer->prefdevice[0] != '\0')
    {
      /* Use the saved prefformat to test if the requested
       * format is specified by the device
       */

      if (((pPlayer->prefformat & (1 << (format - 1)) == 0) ||
          ((pPlayer->preftype & AUDIO_TYPE_OUTPUT) == 0)))
        {
          /* Format not supported by the device */

          return -ENODEV;
        }

      /* Device supports the format.  Open the device file. */

      pPlayer->devFd = open(pPlayer->prefdevice, O_RDWR);
      if (pPlayer->devFd == -1)
        return -ENOENT;

      return OK;
    }
#endif

#if defined(CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE) && \
    defined(CONFIG_NXPLAYER_INCLUDE_DEVICE_SEARCH)

  else

#endif

#ifdef CONFIG_NXPLAYER_INCLUDE_DEVICE_SEARCH
    {
      /* Search for a device in the audio device directory */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
      dirp = opendir("/dev");
#else
      dirp = opendir(CONFIG_AUDIO_DEV_PATH);
#endif  /* CONFIG_AUDIO_DEV_ROOT */
#else
      dirp = opendir("/dev/audio");
#endif  /* CONFIG_AUDIO_CUSTOM_DEV_PATH */
      if (dirp == NULL)
        {
          return -ENODEV;
        }

      while ((pDevice = readdir(dirp)) != NULL)
        {
          /* We found the next device.  Try to open it and
           * get its audio capabilities.
           */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
          snprintf(path,  sizeof(path), "/dev/%s", pDevice->d_name);
#else
          snprintf(path,  sizeof(path), CONFIG_AUDIO_DEV_PATH "/%s", pDevice->d_name);
#endif  /* CONFIG_AUDIO_DEV_ROOT */
#else
          snprintf(path,  sizeof(path), "/dev/audio/%s", pDevice->d_name);
#endif  /* CONFIG_AUDIO_CUSTOM_DEV_PATH */
          if ((pPlayer->devFd = open(path, O_RDWR)) != -1)
            {
              /* We have the device file open.  Now issue an
               * AUDIO ioctls to get the capabilities
               */

              caps.ac_len = sizeof(caps);
              caps.ac_type = AUDIO_TYPE_QUERY;
              caps.ac_subtype = AUDIO_TYPE_QUERY;

              if (ioctl(pPlayer->devFd, AUDIOIOC_GETCAPS, (unsigned long) &caps)
                  == caps.ac_len)
                {
                  /* Test if this device supports the format we want */

                  int ac_format = caps.ac_format[0] | (caps.ac_format[1] << 8);
                  if (((ac_format & (1 << (format - 1))) != 0) &&
                      (caps.ac_controls[0] & AUDIO_TYPE_OUTPUT))
                    {
                      /* Do subformat detection */

                      if (subfmt != AUDIO_FMT_UNDEF)
                        {
                          /* Prepare to get sub-formats for this main format */

                          caps.ac_subtype = format;
                          caps.ac_format[0] = 0;
                          while (ioctl(pPlayer->devFd, AUDIOIOC_GETCAPS,
                              (unsigned long) &caps) == caps.ac_len)
                            {
                              /* Check the next set of 4 controls to find the subformat */
                              for (x = 0; x < sizeof(caps.ac_controls); x++)
                                {
                                  if (caps.ac_controls[x] == subfmt)
                                    {
                                      /* Sub format supported! */

                                      break;
                                    }
                                  else if (caps.ac_controls[x] == AUDIO_SUBFMT_END)
                                    {
                                      /* Sub format not supported */

                                      supported = FALSE;
                                      break;
                                    }
                                }

                              /* If we reached the end of the subformat list, then
                               * break out of the loop.
                               */

                              if (x != sizeof(caps.ac_controls))
                                {
                                  break;
                                }

                              /* Increment ac_format[0] to get next set of subformats */

                              caps.ac_format[0]++;
                            }
                        }

                      /* Test if subformat needed and detected */

                      if (supported)
                        {
                          /* Yes, it supports this format.  Use this device */

                          closedir(dirp);
                          return OK;
                        }
                    }
                }

              /* Not this device! */

              close(pPlayer->devFd);
            }
        }

      /* Close the directory */

      closedir(dirp);
    }
#endif  /* CONFIG_NXPLAYER_INCLUDE_DEVICE_SEARCH */

  /* Device not found */

  pPlayer->devFd = -1;
  return -ENODEV;
}

/****************************************************************************
 * Name: nxplayer_getmidisubformat
 *
 *   nxplayer_getmidisubformat() reads the MIDI header and determins the
 *   MIDI format of the file.
 *
 ****************************************************************************/

#ifdef CONFIG_AUDIO_FORMAT_MIDI
int nxplayer_getmidisubformat(FAR FILE *fd)
{
  char    type[2];
  int     ret;

  /* Seek to location 8 in the file (the format type) */

  fseek(fd, 8, SEEK_SET);
  fread(type, 1, 2, fd);

  /* Set return value based on type */

  switch (type[1])
    {
      case 0:
        ret = AUDIO_SUBFMT_MIDI_0;
        break;

      case 1:
        ret = AUDIO_SUBFMT_MIDI_1;
        break;

      case 2:
        ret = AUDIO_SUBFMT_MIDI_2;
        break;
    }
  fseek(fd, 0, SEEK_SET);

  return ret;
}
#endif

/****************************************************************************
 * Name: nxplayer_fmtfromextension
 *
 *   nxplayer_fmtfromextension() tries to determine the file format based
 *   on the extension of the supplied filename.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_FMT_FROM_EXT
static inline int nxplayer_fmtfromextension(FAR struct nxplayer_s *pPlayer,
    char* pFilename, int *subfmt)
{
  const char  *pExt;
  uint8_t      x;
  uint8_t      c;

  /* Find the file extension, if any */

  x = strlen(pFilename) - 1;
  while (x > 0)
    {
      /* Seach backward for the first '.' */

      if (pFilename[x] == '.')
        {
          /* First '.' found.  Now compare with known extensions */

          pExt = &pFilename[x+1];
          for (c = 0; c < g_known_ext_count; c++)
            {
              /* Test for extension match */

              if (strcasecmp(pExt, g_known_ext[c].ext) == 0)
                {
                  /* Test if we have a sub-format detection routine */

                  if (subfmt && g_known_ext[c].getsubformat)
                    {

                      *subfmt = g_known_ext[c].getsubformat(pPlayer->fileFd);
                    }

                  /* Return the format for this extension */

                  return g_known_ext[c].format;
                }
            }
        }

      /* Stop if we find a '/' */

      if (pFilename[x] == '/')
        break;

      x--;
    }

  return AUDIO_FMT_UNDEF;
}
#endif  /* CONFIG_NXPLAYER_FMT_FROM_EXT */

/****************************************************************************
 * Name: nxplayer_fmtfromheader
 *
 *   nxplayer_fmtfromheader() tries to determine the file format by checking
 *   the file header for known file types.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_FMT_FROM_HEADER
static int nxplayer_fmtfromheader(FAR struct nxplayer_s *pPlayer)
{
  return AUDIO_FMT_UNDEF;
}
#endif /* CONFIG_NXPLAYER_FMT_FROM_HEADER */

/****************************************************************************
 * Name: nxplayer_mediasearch
 *
 *   nxplayer_mediasearch() searches the subdirectories in the mediadir
 *   for the specified media file.  We borrow the caller's path stack
 *   variable (playfile) to conserve stack space.
 *
 ****************************************************************************/

#if defined(CONFIG_NXPLAYER_MEDIA_SEARCH) && defined(CONFIG_NXPLAYER_INCLUDE_MEDIADIR)
static int nxplayer_mediasearch(FAR struct nxplayer_s *pPlayer, char *pFilename,
    char *path, int pathmax)
{
  return -ENOENT;
}
#endif

/****************************************************************************
 * Name: nxplayer_enqueuebuffer
 *
 *  Reads the next block of data from the media file into the specified
 *  buffer and enqueues it to the audio device.
 *
 ****************************************************************************/

static int nxplayer_enqueuebuffer(struct nxplayer_s *pPlayer,
    struct ap_buffer_s* pBuf)
{
  struct audio_buf_desc_s bufdesc;
  int     ret;

  //auddbg("Entry: %p\n", pBuf);

  /* Validate the file is still open */

  if (pPlayer->fileFd == NULL)
    return OK;

  /* Read data into the buffer. */

  pBuf->nbytes = fread(&pBuf->samp, 1, pBuf->nmaxbytes, pPlayer->fileFd);
  if (pBuf->nbytes < pBuf->nmaxbytes)
    {
      fclose(pPlayer->fileFd);
      pPlayer->fileFd = NULL;
    }

  /* Now enqueue the buffer with the audio device.  If the number of bytes
   * in the file happens to be an exact multiple of the audio buffer size,
   * then we will receive the last buffer size = 0.  We encode this buffer
   * also so the audio system knows its the end of the file and can do
   * proper cleanup.
   */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  bufdesc.session = pPlayer->session;
#endif
  bufdesc.numbytes = pBuf->nbytes;
  bufdesc.u.pBuffer = pBuf;
  ret = ioctl(pPlayer->devFd, AUDIOIOC_ENQUEUEBUFFER, (unsigned long)
      &bufdesc);
  if (ret >= 0)
    ret = OK;
  else
    ret = errno;

  return ret;
}

/****************************************************************************
 * Name: nxplayer_thread_playthread
 *
 *  This is the thread that reads the audio file file and enqueue's /
 *  dequeues buffers to the selected and opened audio device.
 *
 ****************************************************************************/

static void *nxplayer_playthread(pthread_addr_t pvarg)
{
  struct nxplayer_s           *pPlayer = (struct nxplayer_s *) pvarg;
  struct audio_msg_s          msg;
  struct audio_buf_desc_s     buf_desc;
  int                         prio;
  ssize_t                     size;
  uint8_t                     running = TRUE;
  uint8_t                     playing = TRUE;
  int                         x, ret;
#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
  struct ap_buffer_info_s     buf_info;
  FAR struct ap_buffer_s**    pBuffers;
#else
  FAR struct ap_buffer_s*     pBuffers[CONFIG_AUDIO_NUM_BUFFERS];
#endif

  auddbg("Entry\n");

  /* Query the audio device for it's preferred buffer size / qty */

#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
  if ((ret = ioctl(pPlayer->devFd, AUDIOIOC_GETBUFFERINFO,
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
      running = FALSE;
      goto err_out;
    }

  /* Create our audio pipeline buffers to use for queueing up data */

  for (x = 0; x < buf_info.nbuffers; x++)
      pBuffers[x] = NULL;

  for (x = 0; x < buf_info.nbuffers; x++)
#else /* CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFER */

  for (x = 0; x < CONFIG_AUDIO_NUM_BUFFERS; x++)
      pBuffers[x] = NULL;

  for (x = 0; x < CONFIG_AUDIO_NUM_BUFFERS; x++)
#endif /* CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFER */
    {
      /* Fill in the buffer descriptor struct to issue an alloc request */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      buf_desc.session = pPlayer->session;
#endif
#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
      buf_desc.numbytes = buf_info.buffer_size;
#else
      buf_desc.numbytes = CONFIG_AUDIO_BUFFER_NUMBYTES;
#endif
      buf_desc.u.ppBuffer = &pBuffers[x];
      ret = ioctl(pPlayer->devFd, AUDIOIOC_ALLOCBUFFER,
          (unsigned long) &buf_desc);
      if (ret != sizeof(buf_desc))
        {
          /* Buffer alloc Operation not supported or error allocating! */

          auddbg("nxplayer_playthread: can't alloc buffer %d\n", x);
          running = FALSE;
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
      /* Enqueue next buffer */

      ret = nxplayer_enqueuebuffer(pPlayer, pBuffers[x]);
      if (ret != OK)
        {
          /* Error encoding initial buffers or file is small */

          if (x == 0)
            running = FALSE;
          else
            playing = FALSE;

          break;
        }
    }

  /* Start the audio device */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(pPlayer->devFd, AUDIOIOC_START,
      (unsigned long) pPlayer->session);
#else
  ret = ioctl(pPlayer->devFd, AUDIOIOC_START, 0);
#endif
  if (ret < 0)
    {
      /* Error starting the audio stream!  */

      running = FALSE;
    }

  /* Indicate we are playing a file */

  pPlayer->state = NXPLAYER_STATE_PLAYING;

  /* Set parameters such as volume, bass, etc. */

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  nxplayer_setvolume(pPlayer, pPlayer->volume);
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
  nxplayer_setbalance(pPlayer, pPlayer->balance);
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_TONE
  nxplayer_setbass(pPlayer, pPlayer->bass);
  nxplayer_settreble(pPlayer, pPlayer->treble);
#endif

  /* Loop until we specifically break */

  while (running)
    {
      /* Wait for a signal either from the Audio driver that it needs
       * additional buffer data, or from a user-space signal to pause,
       * stop, etc.
       */

      size = mq_receive(pPlayer->mq, &msg, sizeof(msg), &prio);

      /* Validate a message was received */

      if (size != sizeof(msg))
        {
          /* Interrupted by a signal? What to do? */

        }

      /* Perform operation based on message id */

      switch (msg.msgId)
        {
          /* An audio buffer is being dequeued by the driver */

          case AUDIO_MSG_DEQUEUE:

            /* Read data from the file directly into this buffer
             * and re-enqueue it.
             */

            if (playing)
              {
                ret = nxplayer_enqueuebuffer(pPlayer, msg.u.pPtr);
                if (ret != OK)
                  {
                    /* Out of data.  Stay in the loop until the
                     * device sends us a COMPLETE message, but stop
                     * trying to play more data.
                     */

                    playing = FALSE;
                  }
              }
            break;

          /* Someone wants to stop the playback. */

          case AUDIO_MSG_STOP:

            /* Send a stop message to the device */

#ifdef CONFIG_AUDIO_MULTI_SESSION
            ioctl(pPlayer->devFd, AUDIOIOC_STOP,
                (unsigned long) pPlayer->session);
#else
            ioctl(pPlayer->devFd, AUDIOIOC_STOP, 0);
#endif
            playing = FALSE;
            running = FALSE;
            break;

          /* Message indicating the playback is complete */

          case AUDIO_MSG_COMPLETE:

            running = FALSE;
            break;

          /* Unknown / unsupported message ID */

          default:
            break;
        }
    }

  /* Release our audio buffers and unregister / release the device */

err_out:
  /* Unregister the message queue and release the session */

  ioctl(pPlayer->devFd, AUDIOIOC_UNREGISTERMQ, (unsigned long) pPlayer->mq);
#ifdef CONFIG_AUDIO_MULTI_SESSION
  ioctl(pPlayer->devFd, AUDIOIOC_RELEASE, (unsigned long) pPlayer->session);
#else
  ioctl(pPlayer->devFd, AUDIOIOC_RELEASE, 0);
#endif

  /* Cleanup */

  while (sem_wait(&pPlayer->sem) != OK)
    ;

#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
  if (pBuffers != NULL)
    {
      auddbg("Freeing buffers\n");
      for (x = 0; x < buf_info.nbuffers; x++)
        {
          /* Fill in the buffer descriptor struct to issue a free request */

          if (pBuffers[x] != NULL)
            {
              buf_desc.u.pBuffer = pBuffers[x];
              ioctl(pPlayer->devFd, AUDIOIOC_FREEBUFFER, (unsigned long) &buf_desc);
            }
        }

      /* Free the pointers to the buffers */

      free(pBuffers);
    }
#else
    auddbg("Freeing buffers\n");
    for (x = 0; x < CONFIG_AUDIO_NUM_BUFFERS; x++)
      {
        /* Fill in the buffer descriptor struct to issue a free request */

        if (pBuffers[x] != NULL)
          {
            buf_desc.u.pBuffer = pBuffers[x];
            ioctl(pPlayer->devFd, AUDIOIOC_FREEBUFFER, (unsigned long) &buf_desc);
          }
      }
#endif

  /* Close the files */

  if (pPlayer->fileFd != NULL)
    {
      fclose(pPlayer->fileFd);            /* Close the file */
      pPlayer->fileFd = NULL;             /* Clear out the FD */
    }
  close(pPlayer->devFd);                  /* Close the device */
  pPlayer->devFd = -1;                    /* Mark device as closed */
  mq_close(pPlayer->mq);                  /* Close the message queue */
  mq_unlink(pPlayer->mqname);             /* Unlink the message queue */
  pPlayer->state = NXPLAYER_STATE_IDLE;   /* Go to IDLE */

  sem_post(&pPlayer->sem);                /* Release the semaphore */

  /* The playthread is done with the context.  Release it, which may
   * actually cause the context to be freed if the creator has already
   * abandoned (released) the context too.
   */

  nxplayer_release(pPlayer);

  auddbg("Exit\n");

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxplayer_setvolume
 *
 *   nxplayer_setvolume() sets the volume.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
int nxplayer_setvolume(FAR struct nxplayer_s *pPlayer, uint16_t volume)
{
  struct audio_caps_desc_s  cap_desc;

  /* Thread sync using the semaphore */

  while (sem_wait(&pPlayer->sem) != OK)
    ;

  /* If we are currently playing, then we need to post a message to
   * the playthread to perform the volume change operation.  If we
   * are not playing, then just store the volume setting and it will
   * be applied before the next playback begins.
   */

  if (pPlayer->state == NXPLAYER_STATE_PLAYING)
    {
      /* Send a CONFIGURE ioctl to the device to set the volume */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session= pPlayer->session;
#endif
      cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type = AUDIO_TYPE_FEATURE;
      *((uint16_t *) cap_desc.caps.ac_format) = AUDIO_FU_VOLUME;
      *((uint16_t *) cap_desc.caps.ac_controls) = volume;
      ioctl(pPlayer->devFd, AUDIOIOC_CONFIGURE, (unsigned long) &cap_desc);
    }

  /* Store the volume setting */

  pPlayer->volume = volume;

  sem_post(&pPlayer->sem);

  return -ENOENT;
}
#endif  /* CONFIG_AUDIO_EXCLUDE_VOLUME */

/****************************************************************************
 * Name: nxplayer_setbass
 *
 *   nxplayer_setbass() sets the bass level and range.
 *
 * Input:
 *   pPlayer  - Pointer to the nxplayer context
 *   level    - Bass level in percentage (0-100)
 *   range    - Bass range in percentage (0-100)
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
int nxplayer_setbass(FAR struct nxplayer_s *pPlayer, uint8_t level)
{
  struct audio_caps_desc_s  cap_desc;

  /* Thread sync using the semaphore */

  while (sem_wait(&pPlayer->sem) != OK)
    ;

  /* If we are currently playing, then we need to post a message to
   * the playthread to perform the volume change operation.  If we
   * are not playing, then just store the bass setting and it will
   * be applied before the next playback begins.
   */

  if (pPlayer->state == NXPLAYER_STATE_PLAYING)
    {
      /* Send a CONFIGURE ioctl to the device to set the volume */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session= pPlayer->session;
#endif
      cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type = AUDIO_TYPE_FEATURE;
      *((uint16_t *) cap_desc.caps.ac_format) = AUDIO_FU_BASS;
      cap_desc.caps.ac_controls[0] = level;
      ioctl(pPlayer->devFd, AUDIOIOC_CONFIGURE, (unsigned long) &cap_desc);
    }

  /* Store the volume setting */

  pPlayer->bass = level;

  sem_post(&pPlayer->sem);

  return -ENOENT;
}
#endif /* CONFIG_AUDIO_EXCLUDE_TONE */

/****************************************************************************
 * Name: nxplayer_settreble
 *
 *   nxplayer_settreble() sets the treble level and range.
 *
 * Input:
 *   pPlayer  - Pointer to the nxplayer context
 *   level    - Treble level in percentage (0-100)
 *   range    - Treble range in percentage (0-100)
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
int nxplayer_settreble(FAR struct nxplayer_s *pPlayer, uint8_t level)
{
  struct audio_caps_desc_s  cap_desc;

  /* Thread sync using the semaphore */

  while (sem_wait(&pPlayer->sem) != OK)
    ;

  /* If we are currently playing, then we need to post a message to
   * the playthread to perform the volume change operation.  If we
   * are not playing, then just store the treble setting and it will
   * be applied before the next playback begins.
   */

  if (pPlayer->state == NXPLAYER_STATE_PLAYING)
    {
      /* Send a CONFIGURE ioctl to the device to set the volume */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session= pPlayer->session;
#endif
      cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type = AUDIO_TYPE_FEATURE;
      *((uint16_t *) cap_desc.caps.ac_format) = AUDIO_FU_TREBLE;
      cap_desc.caps.ac_controls[0] = level;
      ioctl(pPlayer->devFd, AUDIOIOC_CONFIGURE, (unsigned long) &cap_desc);
    }

  /* Store the volume setting */

  pPlayer->treble = level;

  sem_post(&pPlayer->sem);

  return -ENOENT;
}
#endif /* CONFIG_AUDIO_EXCLUDE_TONE */

/****************************************************************************
 * Name: nxplayer_setbalance
 *
 *   nxplayer_setbalance() sets the volume.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
int nxplayer_setbalance(FAR struct nxplayer_s *pPlayer, uint16_t balance)
{
  struct audio_caps_desc_s  cap_desc;

  /* Thread sync using the semaphore */

  while (sem_wait(&pPlayer->sem) != OK)
    ;

  /* If we are currently playing, then we need to post a message to
   * the playthread to perform the volume change operation.  If we
   * are not playing, then just store the volume setting and it will
   * be applied before the next playback begins.
   */

  if (pPlayer->state == NXPLAYER_STATE_PLAYING)
    {
      /* Send a CONFIGURE ioctl to the device to set the volume */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session= pPlayer->session;
#endif
      cap_desc.caps.ac_len = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type = AUDIO_TYPE_FEATURE;
      *((uint16_t *) cap_desc.caps.ac_format) = AUDIO_FU_BALANCE;
      *((uint16_t *) cap_desc.caps.ac_controls) = balance;
      ioctl(pPlayer->devFd, AUDIOIOC_CONFIGURE, (unsigned long) &cap_desc);
    }

  /* Store the volume setting */

  pPlayer->balance = balance;

  sem_post(&pPlayer->sem);

  return -ENOENT;
}
#endif

/****************************************************************************
 * Name: nxplayer_pause
 *
 *   nxplayer_pause() pauses playback without cancelling it.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxplayer_pause(FAR struct nxplayer_s *pPlayer)
{
  int   ret = OK;

  if (pPlayer->state == NXPLAYER_STATE_PLAYING)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(pPlayer->devFd, AUDIOIOC_PAUSE,
          (unsigned long) pPlayer->session);
#else
      ret = ioctl(pPlayer->devFd, AUDIOIOC_PAUSE, 0);
#endif
      if (ret == OK)
        pPlayer->state = NXPLAYER_STATE_PAUSED;
    }

  return ret;
}
#endif  /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxplayer_resume
 *
 *   nxplayer_resume() resumes playback after a pause operation.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxplayer_resume(FAR struct nxplayer_s *pPlayer)
{
  int   ret = OK;

  if (pPlayer->state == NXPLAYER_STATE_PAUSED)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(pPlayer->devFd, AUDIOIOC_RESUME,
          (unsigned long) pPlayer->session);
#else
      ret = ioctl(pPlayer->devFd, AUDIOIOC_RESUME, 0);
#endif
      if (ret == OK)
        pPlayer->state = NXPLAYER_STATE_PLAYING;
    }

  return ret;
}
#endif  /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxplayer_setdevice
 *
 *   nxplayer_setdevice() sets the perferred audio device to use with the
 *   provided nxplayer context.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
int nxplayer_setdevice(FAR struct nxplayer_s *pPlayer, char* pDevice)
{
  int                   tempFd;
  struct audio_caps_s   caps;

  DEBUGASSERT(pPlayer != NULL);
  DEBUGASSERT(pDevice != NULL);

  /* Try to open the device */

  tempFd = open(pDevice, O_RDWR);
  if (tempFd == -1)
    {
      /* Error opening the device */

      return -ENOENT;
    }

  /* Validate it's an Audio device by issuing an AUDIOIOC_GETCAPS ioctl */

  caps.ac_len = sizeof(caps);
  caps.ac_type = AUDIO_TYPE_QUERY;
  caps.ac_subtype = AUDIO_TYPE_QUERY;
  if (ioctl(tempFd, AUDIOIOC_GETCAPS, (unsigned long) &caps) != caps.ac_len)
    {
      /* Not an Audio device! */

      close(tempFd);
      return -ENODEV;
    }

  /* Close the file */

  close(tempFd);

  /* Save the path and format capabilities of the preferred device */

  strncpy(pPlayer->prefdevice, pDevice, sizeof(pPlayer->prefdevice));
  pPlayer->prefformat = caps.ac_format[0] | (caps.ac_format[1] << 8);
  pPlayer->preftype = caps.ac_controls[0];

  return OK;
}
#endif  /* CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE */

/****************************************************************************
 * Name: nxplayer_stop
 *
 *   nxplayer_stop() stops the current playback and closes the file and
 *   the associated device.
 *
 * Input:
 *   pPlayer    Pointer to the initialized MPlayer context
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
int nxplayer_stop(FAR struct nxplayer_s *pPlayer)
{
  struct audio_msg_s  term_msg;
  FAR void*           value;

  DEBUGASSERT(pPlayer != NULL);

  /* Validate we are not in IDLE state */

  sem_wait(&pPlayer->sem);                      /* Get the semaphore */
  if (pPlayer->state == NXPLAYER_STATE_IDLE)
    {
      sem_post(&pPlayer->sem);                  /* Release the semaphore */
      return OK;
    }

  sem_post(&pPlayer->sem);

  /* Notify the playback thread that it needs to cancel the playback */

  term_msg.msgId = AUDIO_MSG_STOP;
  term_msg.u.data = 0;
  mq_send(pPlayer->mq, &term_msg, sizeof(term_msg), CONFIG_NXPLAYER_MSG_PRIO);

  /* Join the thread.  The thread will do all the cleanup. */

  pthread_join(pPlayer->playId, &value);
  pPlayer->playId = 0;

  return OK;
}
#endif  /* CONFIG_AUDIO_EXCLUDE_STOP */

/****************************************************************************
 * Name: nxplayer_playfile
 *
 *   nxplayer_playfile() tries to play the specified file using the Audio
 *   system.  If a preferred device is specified, it will try to use that
 *   device otherwise it will perform a search of the Audio device files
 *   to find a suitable device.
 *
 * Input:
 *   pPlayer    Pointer to the initialized MPlayer context
 *   pFilename  Pointer to the filename to play
 *   filefmt    Format of the file or AUD_FMT_UNDEF if unknown / to be
 *              determined by nxplayer_playfile()
 *
 * Returns:
 *   OK         File is being played
 *   -EBUSY     The media device is busy
 *   -ENOSYS    The media file is an unsupported type
 *   -ENODEV    No audio device suitable to play the media type
 *   -ENOENT    The media file was not found
 *
 ****************************************************************************/

int nxplayer_playfile(FAR struct nxplayer_s *pPlayer, char* pFilename, int filefmt,
    int subfmt)
{
  int                 ret, tmpsubfmt = AUDIO_FMT_UNDEF;
  struct mq_attr      attr;
  struct sched_param  sparam;
  pthread_attr_t      tattr;
  void*               value;
#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
  char                path[128];
#endif

  DEBUGASSERT(pPlayer != NULL);
  DEBUGASSERT(pFilename != NULL);

  if (pPlayer->state != NXPLAYER_STATE_IDLE)
    {
      return -EBUSY;
    }

  auddbg("==============================\n");
  auddbg("Playing file %s\n", pFilename);
  auddbg("==============================\n");

  /* Test that the specified file exists */

  if ((pPlayer->fileFd = fopen(pFilename, "r")) == NULL)
    {
      /* File not found.  Test if its in the mediadir */

#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
      snprintf(path, sizeof(path), "%s/%s", pPlayer->mediadir,
          pFilename);

      if ((pPlayer->fileFd = fopen(path, "r")) == NULL)
        {
#ifdef CONFIG_NXPLAYER_MEDIA_SEARCH
          /* File not found in the media dir.  Do a search */

          if (nxplayer_mediasearch(pPlayer, pFilename, path, sizeof(path)) != OK)
            {
              return -ENOENT;
            }
#else
          return -ENOENT;
#endif  /* CONFIG_NXPLAYER_MEDIA_SEARCH */
        }

#else   /* CONFIG_NXPLAYER_INCLUDE_MEDIADIR */
        return -ENOENT;
#endif  /* CONFIG_NXPLAYER_INCLUDE_MEDIADIR */
    }

  /* Try to determine the format of audio file based on the extension */

#ifdef CONFIG_NXPLAYER_FMT_FROM_EXT
  if (filefmt == AUDIO_FMT_UNDEF)
    filefmt = nxplayer_fmtfromextension(pPlayer, pFilename, &tmpsubfmt);
#endif

  /* If type not identified, then test for known header types */

#ifdef CONFIG_NXPLAYER_FMT_FROM_HEADER
  if (filefmt == AUDIO_FMT_UNDEF)
    filefmt = nxplayer_fmtfromheader(pPlayer, &subfmt, &tmpsubfmt);
#endif

  /* Test if we determined the file format */

  if (filefmt == AUDIO_FMT_UNDEF)
    {
      /* Hmmm, it's some unknown / unsupported type */

      ret = -ENOSYS;
      goto err_out_nodev;
    }

  /* Test if we have a sub format assignment from above */

  if (subfmt == AUDIO_FMT_UNDEF)
    {
      subfmt = tmpsubfmt;
    }

  /* Try to open the device */

  ret = nxplayer_opendevice(pPlayer, filefmt, subfmt);
  if (ret < 0)
    {
      /* Error opening the device */

      goto err_out_nodev;
    }

  /* Try to reserve the device */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(pPlayer->devFd, AUDIOIOC_RESERVE, (unsigned long)
      &pPlayer->session);
#else
  ret = ioctl(pPlayer->devFd, AUDIOIOC_RESERVE, 0);
#endif
  if (ret < 0)
    {
      /* Device is busy or error */

      ret = -errno;
      goto err_out;
    }

  /* Create a message queue for the playthread */

  attr.mq_maxmsg = 16;
  attr.mq_msgsize = sizeof(struct audio_msg_s);
  attr.mq_curmsgs = 0;
  attr.mq_flags = 0;

  snprintf(pPlayer->mqname, sizeof(pPlayer->mqname), "/tmp/%0X", pPlayer);
  pPlayer->mq = mq_open(pPlayer->mqname, O_RDWR | O_CREAT, 0644, &attr);
  if (pPlayer->mq == NULL)
    {
      /* Unable to open message queue! */

      ret = -errno;
      goto err_out;
    }

  /* Register our message queue with the audio device */

  ioctl(pPlayer->devFd, AUDIOIOC_REGISTERMQ, (unsigned long) pPlayer->mq);

  /* Check if there was a previous thread and join it if there was
   * to perform cleanup.
   */

  if (pPlayer->playId != 0)
    {
      pthread_join(pPlayer->playId, &value);
    }

  /* Start the playfile thread to stream the media file to the
   * audio device.
   */

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 9;
  (void)pthread_attr_setschedparam(&tattr, &sparam);
  (void)pthread_attr_setstacksize(&tattr, CONFIG_NXPLAYER_PLAYTHREAD_STACKSIZE);

  /* Add a reference count to the player for the thread and start the
   * thread.  We increment for the thread to avoid thread start-up
   * race conditions.
   */

  nxplayer_reference(pPlayer);
  ret = pthread_create(&pPlayer->playId, &tattr, nxplayer_playthread,
                       (pthread_addr_t) pPlayer);
  if (ret != OK)
    {
      auddbg("Error %d creating playthread\n", ret);
      goto err_out;
    }

  /* Name the thread */

  pthread_setname_np(pPlayer->playId, "playthread");

  return OK;

err_out:
  close(pPlayer->devFd);
  pPlayer->devFd = -1;

err_out_nodev:
  if (pPlayer->fileFd != NULL)
    {
      fclose(pPlayer->fileFd);
      pPlayer->fileFd = NULL;
    }

  return ret;
}

/****************************************************************************
 * Name: nxplayer_setmediadir
 *
 *   nxplayer_setmediadir() sets the root path for media searches.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
void nxplayer_setmediadir(FAR struct nxplayer_s *pPlayer, char *mediadir)
{
  strncpy(pPlayer->mediadir, mediadir, sizeof(pPlayer->mediadir));
}
#endif

/****************************************************************************
 * Name: nxplayer_create
 *
 *   nxplayer_create() allocates and initializes a nxplayer context for
 *   use by further nxplayer operations.  This routine must be called before
 *   to perform the create for proper reference counting.
 *
 * Input Parameters:  None
 *
 * Returned values:
 *   Pointer to the created context or NULL if there was an error.
 *
 **************************************************************************/

FAR struct nxplayer_s *nxplayer_create(void)
{
  FAR struct nxplayer_s *pPlayer;

  /* Allocate the memory */

  pPlayer = (FAR struct nxplayer_s *) malloc(sizeof(struct nxplayer_s));
  if (pPlayer == NULL)
    {
      return NULL;
    }

  /* Initialize the context data */

  pPlayer->state = NXPLAYER_STATE_IDLE;
  pPlayer->devFd = -1;
  pPlayer->fileFd = NULL;
#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
  pPlayer->prefdevice[0] = '\0';
  pPlayer->prefformat = 0;
  pPlayer->preftype = 0;
#endif
  pPlayer->mq = NULL;
  pPlayer->playId = 0;
  pPlayer->crefs = 1;

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
  pPlayer->bass = 50;
  pPlayer->treble = 50;
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
  pPlayer->balance = 500;
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  pPlayer->volume = 400;
#endif

#ifdef CONFIG_AUDIO_MULTI_SESSION
  pPlayer->session = NULL;
#endif

#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
  strncpy(pPlayer->mediadir, CONFIG_NXPLAYER_DEFAULT_MEDIADIR,
      sizeof(pPlayer->mediadir));
#endif
  sem_init(&pPlayer->sem, 0, 1);

  return pPlayer;
}

/****************************************************************************
 * Name: nxplayer_release
 *
 *   nxplayer_release() reduces the reference count by one and if it
 *   reaches zero, frees the context.
 *
 * Input Parameters:
 *   pPlayer    Pointer to the NxPlayer context
 *
 * Returned values:    None
 *
 **************************************************************************/

void nxplayer_release(FAR struct nxplayer_s* pPlayer)
{
  int         refcount, ret;
  FAR void*   value;

  /* Grab the semaphore */

  while ((ret = sem_wait(&pPlayer->sem)) != OK)
    {
      if (ret != -EINTR)
        {
          auddbg("Error getting semaphore\n");
          return;
        }
    }

  /* Check if there was a previous thread and join it if there was */

  if (pPlayer->playId != 0)
    {
      sem_post(&pPlayer->sem);
      pthread_join(pPlayer->playId, &value);
      pPlayer->playId = 0;
      while ((ret = sem_wait(&pPlayer->sem)) != OK)
        {
          if (ret != -EINTR)
            {
              auddbg("Error getting semaphore\n");
              return;
            }
        }
    }

  /* Reduce the reference count */

  refcount = pPlayer->crefs--;
  sem_post(&pPlayer->sem);

  /* If the ref count *was* one, then free the context */

  if (refcount == 1)
    free(pPlayer);
}

/****************************************************************************
 * Name: nxplayer_reference
 *
 *   nxplayer_reference() increments the reference count by one.
 *
 * Input Parameters:
 *   pPlayer    Pointer to the NxPlayer context
 *
 * Returned values:    None
 *
 **************************************************************************/

void nxplayer_reference(FAR struct nxplayer_s* pPlayer)
{
  int     ret;

  /* Grab the semaphore */

  while ((ret = sem_wait(&pPlayer->sem)) != OK)
    {
      if (ret != -EINTR)
        {
          auddbg("Error getting semaphore\n");
          return;
        }
    }

  /* Increment the reference count */

  pPlayer->crefs++;
  sem_post(&pPlayer->sem);
}

/****************************************************************************
 * Name: nxplayer_detach
 *
 *   nxplayer_detach() detaches from the playthread to make it independant
 *     so the caller can abandon the context while the file is still
 *     being played.
 *
 * Input Parameters:
 *   pPlayer    Pointer to the NxPlayer context
 *
 * Returned values:    None
 *
 **************************************************************************/

void nxplayer_detach(FAR struct nxplayer_s* pPlayer)
{
#if 0
  int     ret;

  /* Grab the semaphore */

  while ((ret = sem_wait(&pPlayer->sem)) != OK)
    {
      if (ret != -EINTR)
        {
          auddbg("Error getting semaphore\n");
          return;
        }
    }

  if (pPlayer->playId != NULL)
    {
      /* Do a pthread detach */

      pthread_detach(pPlayer->playId);
      pPlayer->playId = NULL;
    }

  sem_post(&pPlayer->sem);
#endif
}

/****************************************************************************
 * Name: nxplayer_systemreset
 *
 *   nxplayer_systemreset() performs a HW reset on all registered
 *   audio devices.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_SYSTEM_RESET
int nxplayer_systemreset(FAR struct nxplayer_s *pPlayer)
{
  struct dirent*        pDevice;
  DIR*     		          dirp;
  char                  path[64];

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

  while ((pDevice = readdir(dirp)) != NULL)
    {
      /* We found the next device.  Try to open it and
       * get its audio capabilities.
       */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
      snprintf(path,  sizeof(path), "/dev/%s", pDevice->d_name);
#else
      snprintf(path,  sizeof(path), CONFIG_AUDIO_DEV_PATH "/%s", pDevice->d_name);
#endif
#else
      snprintf(path,  sizeof(path), "/dev/audio/%s", pDevice->d_name);
#endif
      if ((pPlayer->devFd = open(path, O_RDWR)) != -1)
        {
          /* We have the device file open.  Now issue an
           * AUDIO ioctls to perform a HW reset
           */

          ioctl(pPlayer->devFd, AUDIOIOC_HWRESET, 0);

          /* Now close the device */

          close(pPlayer->devFd);
        }

    }

  pPlayer->devFd = -1;
  return OK;
}
#endif  /* CONFIG_NXPLAYER_INCLUDE_SYSTEM_RESET */

