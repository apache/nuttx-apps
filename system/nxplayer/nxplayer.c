/****************************************************************************
 * apps/system/nxplayer/nxplayer.c
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
#include <sys/param.h>
#include <unistd.h>
#ifdef CONFIG_NXPLAYER_HTTP_STREAMING_SUPPORT
#  include <sys/time.h>
#  include <sys/socket.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#endif

#include <netutils/netlib.h>
#include <nuttx/audio/audio.h>

#include "system/nxplayer.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXPLAYER_STATE_IDLE      0
#define NXPLAYER_STATE_PLAYING   1
#define NXPLAYER_STATE_PAUSED    2

#ifndef CONFIG_NXPLAYER_MSG_PRIO
#  define CONFIG_NXPLAYER_MSG_PRIO  1
#endif

#ifndef CONFIG_NXPLAYER_PLAYTHREAD_STACKSIZE
#  define CONFIG_NXPLAYER_PLAYTHREAD_STACKSIZE    1500
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_FMT_FROM_EXT
struct nxplayer_ext_fmt_s
{
  FAR const char *ext;
  uint16_t       format;
  CODE int       (*getsubformat)(int fd);
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_AUDIO_FORMAT_MIDI
int nxplayer_getmidisubformat(int fd);
#endif

#ifdef CONFIG_AUDIO_FORMAT_MP3
int nxplayer_getmp3subformat(int fd);
#endif

#ifdef CONFIG_AUDIO_FORMAT_SBC
int nxplayer_getsbcsubformat(int fd);
#endif
/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_FMT_FROM_EXT
static const struct nxplayer_ext_fmt_s g_known_ext[] =
{
#ifdef CONFIG_AUDIO_FORMAT_AC3
  { "ac3",      AUDIO_FMT_AC3, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_MP3
  { "mp3",      AUDIO_FMT_MP3, nxplayer_getmp3subformat },
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
  { "ogg",      AUDIO_FMT_OGG_VORBIS, NULL },
#endif
#ifdef CONFIG_AUDIO_FORMAT_SBC
  { "sbc",      AUDIO_FMT_SBC, nxplayer_getsbcsubformat }
#endif
};

static const int g_known_ext_count = sizeof(g_known_ext) /
                    sizeof(struct nxplayer_ext_fmt_s);

static const struct nxplayer_dec_ops_s g_dec_ops[] =
{
  {
    AUDIO_FMT_MP3,
    nxplayer_parse_mp3,
    nxplayer_fill_common
  },
  {
    AUDIO_FMT_SBC,
    nxplayer_parse_sbc,
    nxplayer_fill_common
  },
  {
    AUDIO_FMT_PCM,
    NULL,
    nxplayer_fill_common
  }
};

#endif /* CONFIG_NXPLAYER_FMT_FROM_EXT */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_HTTP_STREAMING_SUPPORT

/****************************************************************************
 * Name: _open_with_http
 *
 *   _open_with_http() opens specified fullurl which is http:// or local file
 *   path and returns a file descriptor.
 *
 ****************************************************************************/

static int _open_with_http(const char *fullurl)
{
  char relurl[CONFIG_NXPLAYER_HTTP_MAXFILENAME];
  char hostname[CONFIG_NXPLAYER_HTTP_MAXHOSTNAME];
  int  resp_chk = 0;
  char resp_msg[] = "\r\n\r\n";
  struct timeval tv;
  uint16_t port = 80;
  char buf[PATH_MAX];
  int  s;
  int  n;
  char c;

  if (NULL == strstr(fullurl, "http://"))
    {
      /* assumes local file specified */

      s = open(fullurl, O_RDONLY);
      return s;
    }

  memset(relurl, 0, sizeof(relurl));

  n = netlib_parsehttpurl(fullurl, &port,
                          hostname, sizeof(hostname) - 1,
                          relurl, sizeof(relurl) - 1);

  if (OK != n)
    {
      printf("netlib_parsehttpurl() returned %d\n", n);
      return n;
    }

  s = socket(AF_INET, SOCK_STREAM, 0);
  DEBUGASSERT(s != -1);

  tv.tv_sec  = 10; /* TODO */
  tv.tv_usec = 0;

  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
  setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port   = htons(port);

  FAR struct hostent *he;
  he = gethostbyname(hostname);

  memcpy(&server.sin_addr.s_addr,
         he->h_addr, sizeof(in_addr_t));

  n = connect(s,
              (struct sockaddr *)&server,
              sizeof(struct sockaddr_in));

  if (-1 == n)
    {
      close(s);
      return -1;
    }

  /* Send GET request */

  snprintf(buf, sizeof(buf), "GET /%s HTTP/1.0\r\n\r\n", relurl);
  n = write(s, buf, strlen(buf));

  usleep(100 * 1000); /* TODO */

  /* Check status line : e.g. "HTTP/1.x XXX" */

  memset(buf, 0, sizeof(buf));
  read(s, buf, 12);
  n = atoi(buf + 9);

  if (200 != n)
    {
      close(s);
      return -1;
    }

  /* Skip response header */

  while (1)
    {
      n = read(s, &c, 1);

      if (1 == n)
        {
          if (resp_msg[resp_chk] == c)
            {
              resp_chk++;
            }
          else
            {
              resp_chk = 0;
            }
        }

      if (resp_chk == 4)
        {
          break;
        }
    }

  return s;
}
#endif

/****************************************************************************
 * Name: nxplayer_opendevice
 *
 *   nxplayer_opendevice() either searches the Audio system for a device
 *   that is compatible with the specified audio format and opens it, or
 *   tries to open the preferred device if specified and validates that
 *   it supports the requested format.
 *
 * Return:
 *    OK        if compatible device opened (searched or preferred)
 *    -ENODEV   if no compatible device opened.
 *    -ENOENT   if preferred device couldn't be opened.
 *
 ****************************************************************************/

static int nxplayer_opendevice(FAR struct nxplayer_s *pplayer, int format,
                               int subfmt)
{
  /* If we have a preferred device, then open it */

#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
  if (pplayer->prefdevice[0] != '\0')
    {
      /* Use the saved prefformat to test if the requested
       * format is specified by the device
       */

      if ((pplayer->prefformat & (1 << (format - 1))) == 0 ||
          (pplayer->preftype & AUDIO_TYPE_OUTPUT) == 0)
        {
          /* Format not supported by the device */

          auderr("ERROR: Format not supported by device: %d\n", format);
          return -ENODEV;
        }

      /* Device supports the format.  Open the device file. */

      pplayer->dev_fd = open(pplayer->prefdevice, O_RDWR | O_CLOEXEC);
      if (pplayer->dev_fd == -1)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          auderr("ERROR: Failed to open %s: %d\n",
                 pplayer->prefdevice, -errcode);
          UNUSED(errcode);
          return -ENOENT;
        }

      return OK;
    }
#endif

#if defined(CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE) && \
    defined(CONFIG_NXPLAYER_INCLUDE_DEVICE_SEARCH)

  else

#endif

#ifdef CONFIG_NXPLAYER_INCLUDE_DEVICE_SEARCH
    {
      struct audio_caps_s cap;
      FAR struct dirent *pdevice;
      FAR DIR *dirp;
      char path[PATH_MAX];
      uint8_t supported = true;
      uint8_t x;

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
          UNUSED(errcode);
          return -ENODEV;
        }

      while ((pdevice = readdir(dirp)) != NULL)
        {
          /* We found the next device.  Try to open it and
           * get its audio capabilities.
           */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
          snprintf(path,  sizeof(path), "/dev/%s", pdevice->d_name);
#else
          snprintf(path,  sizeof(path), CONFIG_AUDIO_DEV_PATH "/%s",
                   pdevice->d_name);
#endif /* CONFIG_AUDIO_DEV_ROOT */
#else
          snprintf(path,  sizeof(path), "/dev/audio/%s", pdevice->d_name);
#endif /* CONFIG_AUDIO_CUSTOM_DEV_PATH */

          if ((pplayer->dev_fd = open(path, O_RDWR | O_CLOEXEC)) != -1)
            {
              /* We have the device file open.  Now issue an AUDIO ioctls to
               * get the capabilities
               */

              cap.ac_len = sizeof(cap);
              cap.ac_type = AUDIO_TYPE_QUERY;
              cap.ac_subtype = AUDIO_TYPE_QUERY;

              if (ioctl(pplayer->dev_fd, AUDIOIOC_GETCAPS,
                        (unsigned long)&cap) == cap.ac_len)
                {
                  /* Test if this device supports the format we want */

                  if (((cap.ac_format.hw & (1 << (format - 1))) != 0) &&
                      (cap.ac_controls.b[0] & AUDIO_TYPE_OUTPUT))
                    {
                      /* Do subformat detection */

                      if (subfmt != AUDIO_FMT_UNDEF)
                        {
                          /* Prepare to get sub-formats for
                           * this main format
                           */

                          cap.ac_subtype = format;
                          cap.ac_format.b[0] = 0;

                          while (ioctl(pplayer->dev_fd, AUDIOIOC_GETCAPS,
                                      (unsigned long)&cap) == cap.ac_len)
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

              close(pplayer->dev_fd);
            }
        }

      /* Close the directory */

      closedir(dirp);
    }
#endif /* CONFIG_NXPLAYER_INCLUDE_DEVICE_SEARCH */

  /* Device not found */

  auderr("ERROR: Device not found\n");
  pplayer->dev_fd = -1;
  return -ENODEV;
}

/****************************************************************************
 * Name: nxplayer_getmidisubformat
 *
 *   nxplayer_getmidisubformat() reads the MIDI header and determines the
 *   MIDI format of the file.
 *
 ****************************************************************************/

#ifdef CONFIG_AUDIO_FORMAT_MIDI
int nxplayer_getmidisubformat(int fd)
{
  char type[2];
  int  ret;

  /* Seek to location 8 in the file (the format type) */

  lseek(fd, 8, SEEK_SET);
  read(fd, type, sizeof(type));

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

  lseek(fd, 0, SEEK_SET);

  return ret;
}
#endif

/****************************************************************************
 * Name: nxplayer_getmp3subformat
 *
 *   nxplayer_getmp3subformat() just return AUDIO_SUBFMT_PCM_MP3
 *
 ****************************************************************************/

#ifdef CONFIG_AUDIO_FORMAT_MP3
int nxplayer_getmp3subformat(int fd)
{
  return AUDIO_SUBFMT_PCM_MP3;
}
#endif

#ifdef CONFIG_AUDIO_FORMAT_SBC
int nxplayer_getsbcsubformat(int fd)
{
  return AUDIO_FMT_SBC;
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
static inline int nxplayer_fmtfromextension(FAR struct nxplayer_s *pplayer,
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
                      *subfmt = g_known_ext[c].getsubformat(pplayer->fd);
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
#endif /* CONFIG_NXPLAYER_FMT_FROM_EXT */

/****************************************************************************
 * Name: nxplayer_fmtfromheader
 *
 *   nxplayer_fmtfromheader() tries to determine the file format by checking
 *   the file header for known file types.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_FMT_FROM_HEADER
static int nxplayer_fmtfromheader(FAR struct nxplayer_s *pplayer)
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
static int nxplayer_mediasearch(FAR struct nxplayer_s *pplayer,
                                FAR const char *pfilename,
                                FAR const char *path, int pathmax)
{
  return -ENOENT;
}
#endif

/****************************************************************************
 * Name: nxplayer_readbuffer
 *
 *  Read the next block of data from the media file into the specified
 *  buffer.
 *
 ****************************************************************************/

static int nxplayer_readbuffer(FAR struct nxplayer_s *pplayer,
                               FAR struct ap_buffer_s *apb)
{
  int ret;

  /* Validate the file is still open.  It will be closed automatically when
   * we encounter the end of file (or, perhaps, a read error that we cannot
   * handle.
   */

  if (pplayer->fd == -1)
    {
      /* Return -ENODATA to indicate that there is nothing more to read from
       * the file.
       */

      return -ENODATA;
    }

  ret = pplayer->ops->fill_data(pplayer->fd, apb);
  if (ret < 0)
    {
      /* End of file or read error.. We are finished with this file in any
       * event.
       */

      close(pplayer->fd);
      pplayer->fd = -1;
    }

  return OK;
}

/****************************************************************************
 * Name: nxplayer_enqueuebuffer
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

static int nxplayer_enqueuebuffer(FAR struct nxplayer_s *pplayer,
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

#ifdef CONFIG_AUDIO_MULTI_SESSION
  bufdesc.session   = pplayer->session;
#endif
  bufdesc.numbytes  = apb->nbytes;
  bufdesc.u.buffer = apb;

  ret = ioctl(pplayer->dev_fd, AUDIOIOC_ENQUEUEBUFFER,
              (unsigned long)&bufdesc);
  if (ret < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      auderr("ERROR: AUDIOIOC_ENQUEUEBUFFER ioctl failed: %d\n", errcode);
      return -errcode;
    }

  /* Return OK to indicate that we successfully read data from the file
   * (and we are not yet at the end of file)
   */

  return OK;
}

/****************************************************************************
 * Name: nxplayer_jointhread
 ****************************************************************************/

static void nxplayer_jointhread(FAR struct nxplayer_s *pplayer)
{
  FAR void *value;
  int id = 0;

  if (gettid() == pplayer->play_id)
    {
      return;
    }

  pthread_mutex_lock(&pplayer->mutex);

  if (pplayer->play_id > 0)
    {
      id = pplayer->play_id;
      pplayer->play_id = 0;
    }

  pthread_mutex_unlock(&pplayer->mutex);

  if (id > 0)
    {
      pthread_join(id, &value);
    }
}

/****************************************************************************
 * Name: nxplayer_thread_playthread
 *
 *  This is the thread that reads the audio file file and enqueues /
 *  dequeues buffers to the selected and opened audio device.
 *
 ****************************************************************************/

static FAR void *nxplayer_playthread(pthread_addr_t pvarg)
{
  FAR struct nxplayer_s   *pplayer = (FAR struct nxplayer_s *)pvarg;
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

  if ((ret = ioctl(pplayer->dev_fd, AUDIOIOC_GETBUFFERINFO,
                  (unsigned long)&buf_info)) != OK)
    {
      /* Driver doesn't report its buffer size.  Use our default. */

      buf_info.buffer_size = CONFIG_AUDIO_BUFFER_NUMBYTES;
      buf_info.nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  /* Create array of pointers to buffers */

  FAR struct ap_buffer_s *buffers[buf_info.nbuffers];

  /* Create our audio pipeline buffers to use for queueing up data */

  memset(buffers, 0, sizeof(buffers));

  for (x = 0; x < buf_info.nbuffers; x++)
    {
      /* Fill in the buffer descriptor struct to issue an alloc request */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      buf_desc.session = pplayer->session;
#endif

      buf_desc.numbytes = buf_info.buffer_size;
      buf_desc.u.pbuffer = &buffers[x];

      ret = ioctl(pplayer->dev_fd, AUDIOIOC_ALLOCBUFFER,
                 (unsigned long)&buf_desc);
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
      /* Read the next buffer of data */

      ret = nxplayer_readbuffer(pplayer, buffers[x]);
      if (ret != OK)
        {
          /* nxplayer_readbuffer will return an error if there is no further
           * data to be read from the file.  This can happen normally if the
           * file is very small (less than will fit in
           * CONFIG_AUDIO_NUM_BUFFERS) or if an error occurs trying to read
           * from the file.
           */

          /* We are no longer streaming data from the file */

          streaming = false;

          if (x == 0)
            {
              /* No buffers read?  Should never really happen.  Even in the
               * case of a read failure, one empty buffer containing the
               * AUDIO_APB_FINAL indication will be returned.
               */

              running = false;
            }
        }

      /* Enqueue buffer by sending it to the audio driver */

      else
        {
          ret = nxplayer_enqueuebuffer(pplayer, buffers[x]);
          if (ret != OK)
            {
              /* Failed to enqueue the buffer.
               * The driver is not happy with the buffer.
               * Perhaps a decoder has detected something that it
               * does not like in the stream and has stopped streaming.
               * This would happen normally if we send a file in the
               * incorrect format to an audio decoder.
               *
               * We must stop streaming as gracefully as possible.  Close the
               * file so that no further data is read.
               */

              close(pplayer->fd);
              pplayer->fd = -1;

              /* We are no longer streaming data from the file.  Be we will
               * need to wait for any outstanding buffers to be recovered.
               *  We also still expect the audio driver to send a
               * AUDIO_MSG_COMPLETE message after all queued buffers have
               * been returned.
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
    }

  audinfo("%d buffers queued, running=%d streaming=%d\n",
          x, running, streaming);

  /* Start the audio device */

  if (running && !failed)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(pplayer->dev_fd, AUDIOIOC_START,
                 (unsigned long)pplayer->session);
#else
      ret = ioctl(pplayer->dev_fd, AUDIOIOC_START, 0);
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
      /* Indicate we are playing a file */

      pplayer->state = NXPLAYER_STATE_PLAYING;

      /* Set initial parameters such as volume, bass, etc.
       * REVISIT:  Shouldn't this actually be done BEFORE we start playing?
       */

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
      nxplayer_setvolume(pplayer, pplayer->volume);
#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
      nxplayer_setbalance(pplayer, pplayer->balance);
#endif
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
      nxplayer_setbass(pplayer, pplayer->bass);
      nxplayer_settreble(pplayer, pplayer->treble);
#endif
    }

  /* Loop until we specifically break.  running == true means that we are
   * still looping waiting for the playback to complete.  All of the file
   * data may have been sent (if streaming == false), but the playback is
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
   * (3) Terminate playing by sending the AUDIO_MSG_COMPLETE message.
   */

  audinfo("%s\n", running ? "Playing..." : "Not running");
  while (running)
    {
      /* Wait for a signal either from the Audio driver that it needs
       * additional buffer data, or from a user-space signal to pause,
       * stop, etc.
       */

      size = mq_receive(pplayer->mq, (FAR char *)&msg, sizeof(msg), &prio);

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

            /* Read data from the file directly into this buffer and
             * re-enqueue it.  streaming == true means that we have
             * not yet hit the end-of-file.
             */

            if (streaming)
              {
                /* Read the next buffer of data */

                ret = nxplayer_readbuffer(pplayer, msg.u.ptr);
                if (ret != OK)
                  {
                    /* Out of data.  Stay in the loop until the device sends
                     * us a COMPLETE message, but stop trying to play more
                     * data.
                     */

                    streaming = false;
                  }

                /* Enqueue buffer by sending it to the audio driver */

                else
                  {
                    ret = nxplayer_enqueuebuffer(pplayer, msg.u.ptr);
                    if (ret != OK)
                      {
                        /* There is some issue from the audio driver.
                         * Perhaps a problem in the file format?
                         *
                         * We must stop streaming as gracefully as possible.
                         * Close the file so that no further data is read.
                         */

                        close(pplayer->fd);
                        pplayer->fd = -1;

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

          /* Someone wants to stop the playback. */

          case AUDIO_MSG_STOP:

            /* Send a stop message to the device */

#ifdef CONFIG_DEBUG_FEATURES
            audinfo("Stopping! outstanding=%d\n", outstanding);
#endif

#ifdef CONFIG_AUDIO_MULTI_SESSION
            ioctl(pplayer->dev_fd, AUDIOIOC_STOP,
                 (unsigned long)pplayer->session);
#else
            ioctl(pplayer->dev_fd, AUDIOIOC_STOP, 0);
#endif
            /* Stay in the running loop (without sending more data).
             * we will need to recover our audio buffers.  We will
             * loop until AUDIO_MSG_COMPLETE is received.
             */

            streaming = false;
            break;

          /* Message indicating the playback is complete */

          case AUDIO_MSG_COMPLETE:
#ifdef CONFIG_DEBUG_FEATURES
            audinfo("Play complete.  outstanding=%d\n", outstanding);
            DEBUGASSERT(outstanding == 0);
#endif

#ifdef CONFIG_AUDIO_MULTI_SESSION
            ioctl(pplayer->dev_fd, AUDIOIOC_STOP,
                 (unsigned long)pplayer->session);
#else
            ioctl(pplayer->dev_fd, AUDIOIOC_STOP, 0);
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

      if (buffers[x] != NULL)
        {
#ifdef CONFIG_AUDIO_MULTI_SESSION
          buf_desc.session = pplayer->session;
#endif
          buf_desc.u.buffer = buffers[x];
          ioctl(pplayer->dev_fd, AUDIOIOC_FREEBUFFER,
                (unsigned long)&buf_desc);
        }
    }

  /* Unregister the message queue and release the session */

  ioctl(pplayer->dev_fd, AUDIOIOC_UNREGISTERMQ, (unsigned long)pplayer->mq);

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ioctl(pplayer->dev_fd, AUDIOIOC_RELEASE, (unsigned long)pplayer->session);
#else
  ioctl(pplayer->dev_fd, AUDIOIOC_RELEASE, 0);
#endif

  /* Cleanup */

  pthread_mutex_lock(&pplayer->mutex);

  /* Close the files */

  if (0 < pplayer->fd)
    {
      close(pplayer->fd);                 /* Close the file */
      pplayer->fd = -1;                   /* Clear out the FD */
    }

  close(pplayer->dev_fd);                 /* Close the device */
  pplayer->dev_fd = -1;                   /* Mark device as closed */
  mq_close(pplayer->mq);                  /* Close the message queue */
  mq_unlink(pplayer->mqname);             /* Unlink the message queue */
  pplayer->ops   = NULL;                  /* Clear offload parser */
  pplayer->state = NXPLAYER_STATE_IDLE;   /* Go to IDLE */

  pthread_mutex_unlock(&pplayer->mutex);

  /* The playthread is done with the context.  Release it, which may
   * actually cause the context to be freed if the creator has already
   * abandoned (released) the context too.
   */

  nxplayer_release(pplayer);

  audinfo("Exit\n");

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
int nxplayer_setvolume(FAR struct nxplayer_s *pplayer, uint16_t volume)
{
  struct audio_caps_desc_s  cap_desc;
  int ret;

  pthread_mutex_lock(&pplayer->mutex);

  /* If we are currently playing, then we need to post a message to
   * the playthread to perform the volume change operation.  If we
   * are not playing, then just store the volume setting and it will
   * be applied before the next playback begins.
   */

  if (pplayer->state == NXPLAYER_STATE_PLAYING)
    {
      /* Send a CONFIGURE ioctl to the device to set the volume */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session                = pplayer->session;
#endif
      cap_desc.caps.ac_len            = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type           = AUDIO_TYPE_FEATURE;
      cap_desc.caps.ac_format.hw      = AUDIO_FU_VOLUME;
      cap_desc.caps.ac_controls.hw[0] = volume;
      ret = ioctl(pplayer->dev_fd, AUDIOIOC_CONFIGURE,
                  (unsigned long)&cap_desc);
      if (ret < 0)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          auderr("ERROR: AUDIOIOC_CONFIGURE ioctl failed: %d\n", errcode);
          pthread_mutex_unlock(&pplayer->mutex);
          return -errcode;
        }
    }

  /* Store the volume setting */

  pplayer->volume = volume;
  pthread_mutex_unlock(&pplayer->mutex);

  return OK;
}
#endif /* CONFIG_AUDIO_EXCLUDE_VOLUME */

/****************************************************************************
 * Name: nxplayer_setequalization
 *
 *   Sets the level on each band of an equalizer.  Each band setting is
 *   represented in one percent increments, so the range is 0-100.
 *
 * Input Parameters:
 *   pplayer      - Pointer to the context to initialize
 *   equalization - Pointer to array of equalizer settings of size
 *                  CONFIG_AUDIO_EQUALIZER_NBANDS bytes.  Each byte
 *                  represents the setting for one band in the range of
 *                  0-100.
 *
 * Returned Value:
 *   OK if equalization was set correctly.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_EQUALIZER
int nxplayer_setequalization(FAR struct nxplayer_s *pplayer,
                             FAR uint8_t *equalization)
{
#warning Missing logic
  return -ENOSYS;
}
#endif

/****************************************************************************
 * Name: nxplayer_setbass
 *
 *   nxplayer_setbass() sets the bass level and range.
 *
 * Input:
 *   pplayer  - Pointer to the nxplayer context
 *   level    - Bass level in percentage (0-100)
 *   range    - Bass range in percentage (0-100)
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
int nxplayer_setbass(FAR struct nxplayer_s *pplayer, uint8_t level)
{
  struct audio_caps_desc_s  cap_desc;

  pthread_mutex_lock(&pplayer->mutex);

  /* If we are currently playing, then we need to post a message to
   * the playthread to perform the volume change operation.  If we
   * are not playing, then just store the bass setting and it will
   * be applied before the next playback begins.
   */

  if (pplayer->state == NXPLAYER_STATE_PLAYING)
    {
      /* Send a CONFIGURE ioctl to the device to set the volume */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session               = pplayer->session;
#endif
      cap_desc.caps.ac_len           = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type          = AUDIO_TYPE_FEATURE;
      cap_desc.caps.ac_format.hw     = AUDIO_FU_BASS;
      cap_desc.caps.ac_controls.b[0] = level;
      ioctl(pplayer->dev_fd, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
    }

  /* Store the volume setting */

  pplayer->bass = level;

  pthread_mutex_unlock(&pplayer->mutex);

  return -ENOENT;
}
#endif /* CONFIG_AUDIO_EXCLUDE_TONE */

/****************************************************************************
 * Name: nxplayer_settreble
 *
 *   nxplayer_settreble() sets the treble level and range.
 *
 * Input:
 *   pplayer  - Pointer to the nxplayer context
 *   level    - Treble level in percentage (0-100)
 *   range    - Treble range in percentage (0-100)
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
int nxplayer_settreble(FAR struct nxplayer_s *pplayer, uint8_t level)
{
  struct audio_caps_desc_s  cap_desc;

  pthread_mutex_lock(&pplayer->mutex);

  /* If we are currently playing, then we need to post a message to
   * the playthread to perform the volume change operation.  If we
   * are not playing, then just store the treble setting and it will
   * be applied before the next playback begins.
   */

  if (pplayer->state == NXPLAYER_STATE_PLAYING)
    {
      /* Send a CONFIGURE ioctl to the device to set the volume */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session               = pplayer->session;
#endif
      cap_desc.caps.ac_len           = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type          = AUDIO_TYPE_FEATURE;
      cap_desc.caps.ac_format.hw     = AUDIO_FU_TREBLE;
      cap_desc.caps.ac_controls.b[0] = level;
      ioctl(pplayer->dev_fd, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
    }

  /* Store the volume setting */

  pplayer->treble = level;

  pthread_mutex_unlock(&pplayer->mutex);

  return -ENOENT;
}
#endif /* CONFIG_AUDIO_EXCLUDE_TONE */

/****************************************************************************
 * Name: nxplayer_setbalance
 *
 *   nxplayer_setbalance() sets the volume.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
int nxplayer_setbalance(FAR struct nxplayer_s *pplayer, uint16_t balance)
{
  struct audio_caps_desc_s cap_desc;

  pthread_mutex_lock(&pplayer->mutex);

  /* If we are currently playing, then we need to post a message to
   * the playthread to perform the volume change operation.  If we
   * are not playing, then just store the volume setting and it will
   * be applied before the next playback begins.
   */

  if (pplayer->state == NXPLAYER_STATE_PLAYING)
    {
      /* Send a CONFIGURE ioctl to the device to set the volume */

#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session                = pplayer->session;
#endif
      cap_desc.caps.ac_len            = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type           = AUDIO_TYPE_FEATURE;
      cap_desc.caps.ac_format.hw      = AUDIO_FU_BALANCE;
      cap_desc.caps.ac_controls.hw[0] = balance;
      ioctl(pplayer->dev_fd, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
    }

  /* Store the volume setting */

  pplayer->balance = balance;

  pthread_mutex_unlock(&pplayer->mutex);

  return -ENOENT;
}
#endif
#endif

/****************************************************************************
 * Name: nxplayer_pause
 *
 *   nxplayer_pause() pauses playback without cancelling it.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxplayer_pause(FAR struct nxplayer_s *pplayer)
{
  int ret = OK;

  if (pplayer->state == NXPLAYER_STATE_PLAYING)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(pplayer->dev_fd, AUDIOIOC_PAUSE,
          (unsigned long)pplayer->session);
#else
      ret = ioctl(pplayer->dev_fd, AUDIOIOC_PAUSE, 0);
#endif
      if (ret == OK)
        {
          pplayer->state = NXPLAYER_STATE_PAUSED;
        }
    }

  return ret;
}
#endif /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxplayer_resume
 *
 *   nxplayer_resume() resumes playback after a pause operation.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
int nxplayer_resume(FAR struct nxplayer_s *pplayer)
{
  int ret = OK;

  if (pplayer->state == NXPLAYER_STATE_PAUSED)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      ret = ioctl(pplayer->dev_fd, AUDIOIOC_RESUME,
          (unsigned long)pplayer->session);
#else
      ret = ioctl(pplayer->dev_fd, AUDIOIOC_RESUME, 0);
#endif
      if (ret == OK)
        {
          pplayer->state = NXPLAYER_STATE_PLAYING;
        }
    }

  return ret;
}
#endif /* CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME */

/****************************************************************************
 * Name: nxplayer_fforward
 *
 *   Selects to fast forward in the audio data stream.  The fast forward
 *   operation can be canceled by simply selected no sub-sampling with
 *   the AUDIO_SUBSAMPLE_NONE argument returning to normal 1x forward play.
 *   This function may be called multiple times to change fast forward rate.
 *
 *   The preferred way to cancel a fast forward operation is via
 *   nxplayer_cancel_motion() that provides the option to also return to
 *   paused, non-playing state.
 *
 * Input Parameters:
 *   pplayer   - Pointer to the context to initialize
 *   subsample - Identifies the fast forward rate (in terms of sub-sampling,
 *               but does not explicitly require sub-sampling).  See
 *               AUDIO_SUBSAMPLE_* definitions.
 *
 * Returned Value:
 *   OK if fast forward operation successful.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_FFORWARD
int nxplayer_fforward(FAR struct nxplayer_s *pplayer, uint8_t subsample)
{
  struct audio_caps_desc_s cap_desc;
  int ret;

  DEBUGASSERT(pplayer && subsample >= AUDIO_SUBSAMPLE_NONE &&
              subsample <= AUDIO_SUBSAMPLE_MAX);

  /* Send a CONFIGURE ioctl to the device to set the forward rate */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  cap_desc.session                = pplayer->session;
#endif
  cap_desc.caps.ac_len            = sizeof(struct audio_caps_s);
  cap_desc.caps.ac_type           = AUDIO_TYPE_PROCESSING;
  cap_desc.caps.ac_format.hw      = AUDIO_PU_SUBSAMPLE_FORWARD;
  cap_desc.caps.ac_controls.b[0]  = subsample;

  ret = ioctl(pplayer->dev_fd,
              AUDIOIOC_CONFIGURE,
              (unsigned long)&cap_desc);
  if (ret < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      auderr("ERROR: ioctl AUDIOIOC_CONFIGURE failed: %d\n", errcode);
      ret = -errcode;
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: nxplayer_rewind
 *
 *   Selects to rewind in the audio data stream.  The rewind operation must
 *   be cancelled with nxplayer_cancel_motion.  This function may be called
 *   multiple times to change rewind rate.
 *
 *   NOTE that cancellation of the rewind operation differs from
 *   cancellation of the fast forward operation because we must both restore
 *   the sub-sampling rate to 1x and also return to forward play.
 *   AUDIO_SUBSAMPLE_NONE is not a valid argument to this function.
 *
 * Input Parameters:
 *   pplayer   - Pointer to the context to initialize
 *   subsample - Identifies the rewind rate (in terms of sub-sampling, but
 *               does not explicitly require sub-sampling).  See
 *               AUDIO_SUBSAMPLE_* definitions.
 *
 * Returned Value:
 *   OK if rewind operation successfully initiated.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_REWIND
int nxplayer_rewind(FAR struct nxplayer_s *pplayer, uint8_t subsample)
{
  struct audio_caps_desc_s cap_desc;
  int ret;

  DEBUGASSERT(pplayer && subsample >= AUDIO_SUBSAMPLE_MIN &&
              subsample <= AUDIO_SUBSAMPLE_MAX);

  /* Send a CONFIGURE ioctl to the device to set the forward rate */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  cap_desc.session                = pplayer->session;
#endif
  cap_desc.caps.ac_len            = sizeof(struct audio_caps_s);
  cap_desc.caps.ac_type           = AUDIO_TYPE_PROCESSING;
  cap_desc.caps.ac_format.hw      = AUDIO_PU_SUBSAMPLE_REWIND;
  cap_desc.caps.ac_controls.b[0]  = subsample;

  ret = ioctl(pplayer->dev_fd,
              AUDIOIOC_CONFIGURE,
              (unsigned long)&cap_desc);
  if (ret < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      auderr("ERROR: ioctl AUDIOIOC_CONFIGURE failed: %d\n", errcode);
      ret = -errcode;
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: nxplayer_cancel_motion
 *
 *   Cancel a rewind or fast forward operation and return to either the
 *   paused state or to the normal, forward play state.
 *
 * Input Parameters:
 *   pplayer - Pointer to the context to initialize
 *   paused  - True: return to the paused state, False: return to the 1X
 *             forward play state.
 *
 * Returned Value:
 *   OK if rewind operation successfully cancelled.
 *
 ****************************************************************************/

#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
int nxplayer_cancel_motion(FAR struct nxplayer_s *pplayer, bool paused)
{
  int ret;

  /* I think this is equivalent to calling nxplayer_fforward with the
   * argument AUDIO_SUBSAMPLE_NONE:  Forward motion with no sub-sampling.
   *
   * REVISIT: There is no way at present to cancel sub-sampling and return
   * to pause atomically.
   */

  ret = nxplayer_fforward(pplayer, AUDIO_SUBSAMPLE_NONE);
  if (ret < 0)
    {
      auderr("ERROR: nxplayer_fforward failed: %d\n", ret);
      return ret;
    }

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  if (paused)
    {
      ret = nxplayer_pause(pplayer);
      if (ret < 0)
        {
          auderr("ERROR: nxplayer_pause failed: %d\n", ret);
          return ret;
        }
    }
#endif

  return OK;
}
#endif

/****************************************************************************
 * Name: nxplayer_setdevice
 *
 *   nxplayer_setdevice() sets the preferred audio device to use with the
 *   provided nxplayer context.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
int nxplayer_setdevice(FAR struct nxplayer_s *pplayer,
                       FAR const char *pdevice)
{
  int                   temp_fd;
  struct audio_caps_s   caps;

  DEBUGASSERT(pplayer != NULL);
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

  /* Save the path and format capabilities of the preferred device */

  strlcpy(pplayer->prefdevice, pdevice, sizeof(pplayer->prefdevice));
  pplayer->prefformat = caps.ac_format.b[0] | (caps.ac_format.b[1] << 8);
  pplayer->preftype = caps.ac_controls.b[0];

  return OK;
}
#endif /* CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE */

/****************************************************************************
 * Name: nxplayer_stop
 *
 *   nxplayer_stop() stops the current playback and closes the file and
 *   the associated device.
 *
 * Input:
 *   pplayer    Pointer to the initialized MPlayer context
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
int nxplayer_stop(FAR struct nxplayer_s *pplayer)
{
  struct audio_msg_s term_msg;

  DEBUGASSERT(pplayer != NULL);

  /* Validate we are not in IDLE state */

  pthread_mutex_lock(&pplayer->mutex);
  if (pplayer->state == NXPLAYER_STATE_IDLE)
    {
      pthread_mutex_unlock(&pplayer->mutex);
      return OK;
    }

  pthread_mutex_unlock(&pplayer->mutex);

  /* Notify the playback thread that it needs to cancel the playback */

  term_msg.msg_id = AUDIO_MSG_STOP;
  term_msg.u.data = 0;
  mq_send(pplayer->mq, (FAR const char *)&term_msg, sizeof(term_msg),
          CONFIG_NXPLAYER_MSG_PRIO);

  /* Join the thread.  The thread will do all the cleanup. */

  nxplayer_jointhread(pplayer);

  return OK;
}
#endif /* CONFIG_AUDIO_EXCLUDE_STOP */

/****************************************************************************
 * Name: nxplayer_playinternal
 *
 *   nxplayer_playinternal() tries to play the specified file/raw data
 *   using the Audio system.  If a preferred device is specified, it will
 *   try to use that device otherwise it will perform a search of the Audio
 *   device files to find a suitable device.
 *
 * Input:
 *   pplayer    Pointer to the initialized MPlayer context
 *   pfilename  Pointer to the filename to play
 *   filefmt    Format of the file or AUD_FMT_UNDEF if unknown / to be
 *              determined by nxplayer_playfile()
 *   nchannels  channels num (raw data playback needed)
 *   bpsamp     bits pre sample (raw data playback needed)
 *   samprate  samplre rate (raw data playback needed)
 *   chmap      channel map (raw data playback needed)
 *
 * Returns:
 *   OK         File is being played
 *   -EBUSY     The media device is busy
 *   -ENOSYS    The media file is an unsupported type
 *   -ENODEV    No audio device suitable to play the media type
 *   -ENOENT    The media file was not found
 *
 *
 ****************************************************************************/

static int nxplayer_playinternal(FAR struct nxplayer_s *pplayer,
                                 FAR const char *pfilename, int filefmt,
                                 int subfmt, uint8_t nchannels,
                                 uint8_t bpsamp, uint32_t samprate,
                                 uint8_t chmap)
{
  struct mq_attr      attr;
  struct sched_param  sparam;
  pthread_attr_t      tattr;
  struct audio_caps_desc_s cap_desc;
  struct ap_buffer_info_s  buf_info;
  struct audio_caps_s      caps;
  int                      min_channels;
#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
  char                path[PATH_MAX];
#endif
  int                 tmpsubfmt = AUDIO_FMT_UNDEF;
  int                 ret;
  int                 c;

  DEBUGASSERT(pplayer != NULL);
  DEBUGASSERT(pfilename != NULL);

  if (pplayer->state != NXPLAYER_STATE_IDLE)
    {
      return -EBUSY;
    }

  audinfo("==============================\n");
  audinfo("Playing file %s\n", pfilename);
  audinfo("==============================\n");

  /* Test that the specified file exists */

#ifdef CONFIG_NXPLAYER_HTTP_STREAMING_SUPPORT
  if ((pplayer->fd = _open_with_http(pfilename)) == -1)
#else
  if ((pplayer->fd = open(pfilename, O_RDONLY)) == -1)
#endif
    {
      /* File not found.  Test if its in the mediadir */

#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
      snprintf(path, sizeof(path), "%s/%s", pplayer->mediadir, pfilename);

      if ((pplayer->fd = open(path, O_RDONLY)) == -1)
        {
#ifdef CONFIG_NXPLAYER_MEDIA_SEARCH
          /* File not found in the media dir.  Do a search */

          if (nxplayer_mediasearch(pplayer, pfilename, path,
                                   sizeof(path)) != OK)
            {
              auderr("ERROR: Could not find file\n");
              return -ENOENT;
            }
#else
          auderr("ERROR: Could not open %s or %s\n", pfilename, path);
          return -ENOENT;
#endif /* CONFIG_NXPLAYER_MEDIA_SEARCH */
        }

#else   /* CONFIG_NXPLAYER_INCLUDE_MEDIADIR */

      auderr("ERROR: Could not open %s\n", pfilename);
      return -ENOENT;
#endif /* CONFIG_NXPLAYER_INCLUDE_MEDIADIR */
    }

#ifdef CONFIG_NXPLAYER_FMT_FROM_EXT
  /* Try to determine the format of audio file based on the extension */

  if (filefmt == AUDIO_FMT_UNDEF)
    {
      filefmt = nxplayer_fmtfromextension(pplayer, pfilename, &tmpsubfmt);
    }
#endif

#ifdef CONFIG_NXPLAYER_FMT_FROM_HEADER
  /* If type not identified, then test for known header types */

  if (filefmt == AUDIO_FMT_UNDEF)
    {
      filefmt = nxplayer_fmtfromheader(pplayer, &subfmt, &tmpsubfmt);
    }
#endif

  /* Test if we determined the file format */

  if (filefmt == AUDIO_FMT_UNDEF)
    {
      /* Hmmm, it's some unknown / unsupported type */

      auderr("ERROR: Unsupported format: %d\n", filefmt);
      ret = -ENOSYS;
      goto err_out_nodev;
    }

  /* Test if we have a sub format assignment from above */

  if (subfmt == AUDIO_FMT_UNDEF)
    {
      subfmt = tmpsubfmt;
    }

  /* Try to open the device */

  ret = nxplayer_opendevice(pplayer, filefmt, subfmt);
  if (ret < 0)
    {
      /* Error opening the device */

      auderr("ERROR: nxplayer_opendevice failed: %d\n", ret);
      goto err_out_nodev;
    }

  for (c = 0; c < nitems(g_dec_ops); c++)
    {
      if (g_dec_ops[c].format == filefmt)
        {
          pplayer->ops = &g_dec_ops[c];
          break;
        }
    }

  if (!pplayer->ops)
    {
      goto err_out;
    }

  if (pplayer->ops->pre_parse)
    {
      ret = pplayer->ops->pre_parse(pplayer->fd, &samprate,
                                    &nchannels, &bpsamp);
    }

  /* Try to reserve the device */

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(pplayer->dev_fd, AUDIOIOC_RESERVE,
              (unsigned long)&pplayer->session);
#else
  ret = ioctl(pplayer->dev_fd, AUDIOIOC_RESERVE, 0);
#endif
  if (ret < 0)
    {
      /* Device is busy or error */

      auderr("ERROR: Failed to reserve device: %d\n", ret);
      ret = -errno;
      goto err_out;
    }

  caps.ac_len = sizeof(caps);
  caps.ac_type = AUDIO_TYPE_OUTPUT;
  caps.ac_subtype = AUDIO_TYPE_QUERY;

  if (ioctl(pplayer->dev_fd, AUDIOIOC_GETCAPS,
      (unsigned long)&caps) == caps.ac_len)
    {
      min_channels = caps.ac_channels >> 4;

      if (min_channels != 0 && nchannels < min_channels)
        {
          ret = -EINVAL;
          goto err_out;
        }
    }

  if (nchannels && samprate && bpsamp)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      cap_desc.session = pplayer->session;
#endif
      cap_desc.caps.ac_len            = sizeof(struct audio_caps_s);
      cap_desc.caps.ac_type           = AUDIO_TYPE_OUTPUT;
      cap_desc.caps.ac_channels       = nchannels;
      cap_desc.caps.ac_chmap          = chmap;
      cap_desc.caps.ac_controls.hw[0] = samprate;
      cap_desc.caps.ac_controls.b[3]  = samprate >> 16;
      cap_desc.caps.ac_controls.b[2]  = bpsamp;
      cap_desc.caps.ac_subtype        = filefmt;

      ioctl(pplayer->dev_fd, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
    }

  /* Query the audio device for its preferred buffer count */

  if (ioctl(pplayer->dev_fd, AUDIOIOC_GETBUFFERINFO,
            (unsigned long)&buf_info) != OK)
    {
      /* Driver doesn't report its buffer size.  Use our default. */

      buf_info.nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  /* Create a message queue for the playthread */

  attr.mq_maxmsg  = buf_info.nbuffers + 8;
  attr.mq_msgsize = sizeof(struct audio_msg_s);
  attr.mq_curmsgs = 0;
  attr.mq_flags   = 0;

  snprintf(pplayer->mqname, sizeof(pplayer->mqname), "/tmp/%0lx",
           (unsigned long)((uintptr_t)pplayer));

  pplayer->mq = mq_open(pplayer->mqname, O_RDWR | O_CREAT, 0644, &attr);
  if (pplayer->mq == (mqd_t) -1)
    {
      /* Unable to open message queue! */

      ret = -errno;
      auderr("ERROR: mq_open failed: %d\n", ret);
      goto err_out;
    }

  /* Register our message queue with the audio device */

  ioctl(pplayer->dev_fd, AUDIOIOC_REGISTERMQ, (unsigned long)pplayer->mq);

  /* Check if there was a previous thread and join it if there was
   * to perform clean-up.
   */

  nxplayer_jointhread(pplayer);

  /* Start the playfile thread to stream the media file to the
   * audio device.
   */

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 9;
  pthread_attr_setschedparam(&tattr, &sparam);
  pthread_attr_setstacksize(&tattr,
                            CONFIG_NXPLAYER_PLAYTHREAD_STACKSIZE);

  /* Add a reference count to the player for the thread and start the
   * thread.  We increment for the thread to avoid thread start-up
   * race conditions.
   */

  nxplayer_reference(pplayer);
  ret = pthread_create(&pplayer->play_id, &tattr, nxplayer_playthread,
                       (pthread_addr_t) pplayer);
  if (ret != OK)
    {
      auderr("ERROR: Failed to create playthread: %d\n", ret);
      goto err_out;
    }

  /* Name the thread */

  pthread_setname_np(pplayer->play_id, "playthread");
  return OK;

err_out:
  close(pplayer->dev_fd);
  pplayer->dev_fd = -1;

err_out_nodev:
  if (0 < pplayer->fd)
    {
      close(pplayer->fd);
      pplayer->fd = -1;
    }

  return ret;
}

/****************************************************************************
 * Name: nxplayer_playfile
 *
 *   nxplayer_playfile() tries to play the specified file using the Audio
 *   system.  If a preferred device is specified, it will try to use that
 *   device otherwise it will perform a search of the Audio device files
 *   to find a suitable device.
 *
 * Input:
 *   pplayer    Pointer to the initialized MPlayer context
 *   pfilename  Pointer to the filename to play
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

int nxplayer_playfile(FAR struct nxplayer_s *pplayer,
                      FAR const char *pfilename, int filefmt, int subfmt)
{
  return nxplayer_playinternal(pplayer, pfilename, filefmt,
                               subfmt, 0, 0, 0, 0);
}

/****************************************************************************
 * Name: nxplayer_playraw
 *
 *   nxplayer_playraw() tries to play the raw data file using the Audio
 *   system.  If a preferred device is specified, it will try to use that
 *   device otherwise it will perform a search of the Audio device files
 *   to find a suitable device.
 *
 * Input:
 *   pplayer    Pointer to the initialized MPlayer context
 *   pfilename  Pointer to the filename to play
 *   nchannels  channel num
 *   bpsampe    bit width
 *   samprate   sample rate
 *   chmap      channel map
 *
 * Returns:
 *   OK         File is being played
 *   -EBUSY     The media device is busy
 *   -ENOSYS    The media file is an unsupported type
 *   -ENODEV    No audio device suitable to play the media type
 *   -ENOENT    The media file was not found
 *
 ****************************************************************************/

int nxplayer_playraw(FAR struct nxplayer_s *pplayer,
                     FAR const char *pfilename, uint8_t nchannels,
                     uint8_t bpsamp, uint32_t samprate, uint8_t chmap)
{
  if (nchannels == 0)
    {
      nchannels = 2;
    }

  if (bpsamp == 0)
    {
      bpsamp = 16;
    }

  if (samprate == 0)
    {
      samprate = 48000;
    }

  return nxplayer_playinternal(pplayer, pfilename, AUDIO_FMT_PCM, 0,
                               nchannels, bpsamp, samprate, chmap);
}

/****************************************************************************
 * Name: nxplayer_setmediadir
 *
 *   nxplayer_setmediadir() sets the root path for media searches.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
void nxplayer_setmediadir(FAR struct nxplayer_s *pplayer,
     FAR const char *mediadir)
{
  strlcpy(pplayer->mediadir, mediadir, sizeof(pplayer->mediadir));
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
 ****************************************************************************/

FAR struct nxplayer_s *nxplayer_create(void)
{
  FAR struct nxplayer_s *pplayer;

  /* Allocate the memory */

  pplayer = (FAR struct nxplayer_s *)malloc(sizeof(struct nxplayer_s));
  if (pplayer == NULL)
    {
      return NULL;
    }

  /* Initialize the context data */

  pplayer->state = NXPLAYER_STATE_IDLE;
  pplayer->dev_fd = -1;
  pplayer->fd = -1;
#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
  pplayer->prefdevice[0] = '\0';
  pplayer->prefformat = 0;
  pplayer->preftype = 0;
#endif
  pplayer->mq = 0;
  pplayer->play_id = 0;
  pplayer->crefs = 1;

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
  pplayer->bass = 50;
  pplayer->treble = 50;
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  pplayer->volume = 400;
#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
  pplayer->balance = 500;
#endif
#endif

#ifdef CONFIG_AUDIO_MULTI_SESSION
  pplayer->session = NULL;
#endif

#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
  strlcpy(pplayer->mediadir, CONFIG_NXPLAYER_DEFAULT_MEDIADIR,
          sizeof(pplayer->mediadir));
#endif

  pthread_mutex_init(&pplayer->mutex, NULL);

  return pplayer;
}

/****************************************************************************
 * Name: nxplayer_release
 *
 *   nxplayer_release() reduces the reference count by one and if it
 *   reaches zero, frees the context.
 *
 * Input Parameters:
 *   pplayer    Pointer to the NxPlayer context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxplayer_release(FAR struct nxplayer_s *pplayer)
{
  int         refcount;

  /* Check if there was a previous thread and join it if there was */

  nxplayer_jointhread(pplayer);

  pthread_mutex_lock(&pplayer->mutex);

  /* Reduce the reference count */

  refcount = pplayer->crefs--;
  pthread_mutex_unlock(&pplayer->mutex);

  /* If the ref count *was* one, then free the context */

  if (refcount == 1)
    {
      free(pplayer);
    }
}

/****************************************************************************
 * Name: nxplayer_reference
 *
 *   nxplayer_reference() increments the reference count by one.
 *
 * Input Parameters:
 *   pplayer    Pointer to the NxPlayer context
 *
 * Returned values:    None
 *
 ****************************************************************************/

void nxplayer_reference(FAR struct nxplayer_s *pplayer)
{
  pthread_mutex_lock(&pplayer->mutex);

  /* Increment the reference count */

  pplayer->crefs++;
  pthread_mutex_unlock(&pplayer->mutex);
}

/****************************************************************************
 * Name: nxplayer_systemreset
 *
 *   nxplayer_systemreset() performs a HW reset on all registered
 *   audio devices.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_SYSTEM_RESET
int nxplayer_systemreset(FAR struct nxplayer_s *pplayer)
{
  struct dirent *pdevice;
  DIR           *dirp;
  char           path[PATH_MAX];

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
      snprintf(path,  sizeof(path), "/dev/audio/%s", pdevice->d_name);
#endif
      if ((pplayer->dev_fd = open(path, O_RDWR | O_CLOEXEC)) != -1)
        {
          /* We have the device file open.  Now issue an
           * AUDIO ioctls to perform a HW reset
           */

          ioctl(pplayer->dev_fd, AUDIOIOC_HWRESET, 0);

          /* Now close the device */

          close(pplayer->dev_fd);
        }
    }

  pplayer->dev_fd = -1;
  return OK;
}
#endif /* CONFIG_NXPLAYER_INCLUDE_SYSTEM_RESET */
