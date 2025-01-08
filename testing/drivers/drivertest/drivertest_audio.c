/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_audio.c
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/param.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <cmocka.h>
#include <errno.h>
#include <dirent.h>
#include <mqueue.h>
#include <debug.h>
#include <unistd.h>

#include <nuttx/audio/audio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define OPTARG_TO_VALUE(value, type, base)                            \
  do                                                                  \
    {                                                                 \
      FAR char *ptr;                                                  \
      value = (type)strtoul(optarg, &ptr, base);                      \
      if (*ptr != '\0')                                               \
        {                                                             \
          printf("Parameter error: %s\n", optarg);                    \
          audio_test_help(argv[0], EXIT_FAILURE);                     \
        }                                                             \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct audio_state_s
{
  char       outfile[PATH_MAX];
  char       outdev[PATH_MAX];
  char       indev[PATH_MAX];
  int        outdev_fd;
  int        indev_fd;
  int        out_fd;

  uint32_t   samprate;
  uint32_t   format;
  uint32_t   bpsamp;
  uint32_t   chans;
  uint32_t   chmap;

  char       mqname[16];
  int        direction;
  uint32_t   duration;
  uint32_t   idx;
  mqd_t      mq;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  FAR void    *session;
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: audio_test_writebuffer
 ****************************************************************************/

static int audio_test_writebuffer(FAR struct audio_state_s *state,
                                  FAR struct ap_buffer_s *apb)
{
  int ret = 0;

  if (state->out_fd == -1)
    {
      return -ENODATA;
    }

  ret = write(state->out_fd, apb->samp, apb->nbytes);
  if (ret < 0)
    {
      return ret;
    }

  apb->curbyte = 0;
  apb->flags   = 0;

  return OK;
}

/****************************************************************************
 * Name: player_readbuffer
 ****************************************************************************/

static int audio_test_readbuffer(FAR struct audio_state_s *state,
                                 FAR struct ap_buffer_s *apb)
{
  if (state->out_fd == -1)
    {
      return -ENODATA;
    }

  apb->nbytes  = read(state->out_fd, apb->samp, apb->nmaxbytes);
  apb->curbyte = 0;
  apb->flags   = 0;

  if (apb->nbytes < apb->nmaxbytes)
    {
      close(state->out_fd);
      state->out_fd = -1;
      apb->flags |= AUDIO_APB_FINAL;
    }

  return OK;
}

/****************************************************************************
 * Name: audio_test_enqueuebuffer
 ****************************************************************************/

static int audio_test_enqueuebuffer(FAR struct audio_state_s *state,
                                    FAR struct ap_buffer_s *apb,
                                    int direction)
{
  struct audio_buf_desc_s bufdesc;
  int ret = 0;
  int fd;

  fd = direction == AUDIO_TYPE_OUTPUT ?
       state->outdev_fd :
       state->indev_fd;

  apb->nbytes = apb->nmaxbytes;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  bufdesc.session  = state->session;
#endif

  bufdesc.numbytes = apb->nbytes;
  bufdesc.u.buffer = apb;

  ret = ioctl(fd, AUDIOIOC_ENQUEUEBUFFER, (unsigned long)&bufdesc);
  if (ret < 0)
    {
      return -errno;
    }

  return OK;
}

static int audio_test_alloc_buffer(FAR struct audio_state_s *state,
                                   FAR struct ap_buffer_s **bufs,
                                   FAR struct ap_buffer_info_s *buf_info,
                                   int direction)
{
  struct audio_buf_desc_s buf_desc;
  int ret = 0;
  int fd;
  int x;

  fd = direction == AUDIO_TYPE_OUTPUT ?
       state->outdev_fd :
       state->indev_fd;

  for (x = 0; x < buf_info->nbuffers; x++)
    {
#ifdef CONFIG_AUDIO_MULTI_SESSION
      buf_desc.session = state->session;
#endif
      buf_desc.numbytes = buf_info->buffer_size;
      buf_desc.u.pbuffer = &bufs[x];

      ret = ioctl(fd, AUDIOIOC_ALLOCBUFFER, (unsigned long)&buf_desc);
      if (ret != sizeof(buf_desc))
        {
          goto err_out;
        }
    }

  return OK;

err_out:
  free(bufs);
  return ret;
}

static int audio_test_free_buffer(FAR struct audio_state_s *state,
                                  FAR struct ap_buffer_s **bufs,
                                  FAR struct ap_buffer_info_s *buf_info,
                                  int direction)
{
  struct audio_buf_desc_s buf_desc;
  int ret = 0;
  int fd;
  int x;

  fd = direction == AUDIO_TYPE_OUTPUT ?
       state->outdev_fd :
       state->indev_fd;

  for (x = 0; x < buf_info->nbuffers; x++)
    {
      if (bufs[x] != NULL)
        {
#ifdef CONFIG_AUDIO_MULTI_SESSION
          buf_desc.session = state->session;
#endif
          buf_desc.u.buffer = bufs[x];
          ret = ioctl(fd, AUDIOIOC_FREEBUFFER, (unsigned long)&buf_desc);
          if (ret < 0)
            {
              break;
            }
        }
    }

  return ret;
}

static void audio_test_help(FAR const char *progname, int exitcode)
{
  printf("Usage: %s\n"
         " -a <test case>\n"
         " -t <duration>\n"
         " -i <input  device e.g./dev/audio/pcm0c>\n"
         " -o <output device e.g./dev/audio/pcm0p>\n"
         " -p <output file>\n"
         " -f <format>\n"
         " -b <bytes per sample> \n"
         " -s <sample rate> \n"
         " -c <channles> \n",
         progname);
  printf(" [-a testcase] selects the testcase\n"
         " Case 1: Capture\n"
         " Case 2: Playack\n"
         " Case 3: First Capture, and then Playback\n"
         );
  printf(" -h shows this message and exits\n");

  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct audio_state_s *state, int argc,
                              FAR char **argv)
{
  int option;

  while ((option = getopt(argc, argv, "a:t::o::i::f::p:c::b::s::")) != ERROR)
    {
      switch (option)
        {
          case 'a':
            {
              OPTARG_TO_VALUE(state->direction, uint32_t, 10);
              break;
            }

          case 't':
            {
              OPTARG_TO_VALUE(state->duration, uint32_t, 10);
              break;
            }

          case 'o':
            {
              strcpy(state->outdev, optarg);
              break;
            }

          case 'i':
            {
              strcpy(state->indev, optarg);
              break;
            }

          case 'f':
            {
              if (!strcmp(optarg, "mp3"))
                {
                  state->format = AUDIO_FMT_MP3;
                }
              else
                {
                  state->format = AUDIO_FMT_PCM;
                }
              break;
            }

          case 'p':
            {
              strcpy(state->outfile, optarg);
              break;
            }

         case 'c':
            {
              OPTARG_TO_VALUE(state->chans, uint32_t, 10);
              break;
            }

          case 'b':
            {
              OPTARG_TO_VALUE(state->bpsamp, uint32_t, 10);
              break;
            }

          case 's':
            {
              OPTARG_TO_VALUE(state->samprate, uint32_t, 10);
              break;
            }

          case '?':
            printf("Unknown option: %c\n", optopt);
            audio_test_help(argv[0], EXIT_FAILURE);
            break;
        }
    }
}

/****************************************************************************
 * Name: audio_test_prepare
 ****************************************************************************/

static int audio_test_prepare(FAR struct audio_state_s *state,
                              FAR struct ap_buffer_info_s *buf_info,
                              int direction)
{
  struct audio_caps_desc_s cap_desc;
  struct audio_caps_s caps;
  int ret = 0;
  int fd;

  fd = direction == AUDIO_TYPE_OUTPUT ?
       state->outdev_fd :
       state->indev_fd;

  if (direction == AUDIO_TYPE_OUTPUT)
    {
      state->out_fd = open(state->outfile, O_RDONLY);
    }
  else
    {
      state->out_fd = open(state->outfile, O_WRONLY | O_CREAT | O_TRUNC);
    }

  if (state->out_fd == -1)
    {
      return -ENOENT;
    }

  caps.ac_len  = sizeof(caps);
  caps.ac_type = AUDIO_TYPE_QUERY;
  caps.ac_subtype = AUDIO_TYPE_QUERY;

  ret = ioctl(fd, AUDIOIOC_GETCAPS, (unsigned long)&caps);
  if (ret != caps.ac_len)
    {
      return ret;
    }

  if ((caps.ac_controls.b[0] & direction) == 0 ||
      (caps.ac_format.hw & (1 << (state->format - 1))) == 0)
    {
      return -EINVAL;
    }

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(fd, AUDIOIOC_RESERVE, (unsigned long)&state->session);
#else
  ret = ioctl(fd, AUDIOIOC_RESERVE, 0);
#endif

#ifdef CONFIG_AUDIO_MULTI_SESSION
  cap_desc.session = state->session:
#endif
  cap_desc.caps.ac_len            = sizeof(struct audio_caps_s);
  cap_desc.caps.ac_type           = direction;
  cap_desc.caps.ac_channels       = state->chans;
  cap_desc.caps.ac_chmap          = state->chmap;
  cap_desc.caps.ac_controls.hw[0] = state->samprate;
  cap_desc.caps.ac_controls.b[3]  = state->samprate >> 16;
  cap_desc.caps.ac_controls.b[2]  = state->bpsamp;
  cap_desc.caps.ac_subtype        = state->format;
  ret = ioctl(fd, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);
  if (ret < 0)
    {
      return -errno;
    }

  ret = ioctl(fd, AUDIOIOC_REGISTERMQ, (unsigned long)state->mq);
  if (ret < 0)
    {
      return -errno;
    }

  ret = ioctl(fd, AUDIOIOC_GETBUFFERINFO, (unsigned long)buf_info);
  if (ret < 0)
    {
      buf_info->buffer_size = CONFIG_AUDIO_BUFFER_NUMBYTES;
      buf_info->nbuffers = CONFIG_AUDIO_NUM_BUFFERS;
    }

  return OK;
}

static int audio_test_start(FAR struct audio_state_s *state, int direction)
{
  int ret = 0;
  int fd;

  fd = direction == AUDIO_TYPE_OUTPUT ?
       state->outdev_fd :
       state->indev_fd;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(fd, AUDIOIOC_START,
                    (unsigned long)state->session);
#else
  ret = ioctl(fd, AUDIOIOC_START, 0);
#endif

  return ret;
}

static bool audio_test_timeout(FAR struct audio_state_s *state,
                               int direction, struct timeval start)
{
  struct timeval now;
  struct timeval delta;
  struct timeval wait;

  if (direction == AUDIO_TYPE_OUTPUT)
    {
      return false;
    }

  wait.tv_sec = state->duration;
  wait.tv_usec = 0;

  gettimeofday(&now, NULL);
  timersub(&now, &start, &delta);
  return timercmp(&delta, &wait, > /* For checkpatch */);
}

static int audio_test_stop(FAR struct audio_state_s *state, int direction)
{
  int ret = 0;
  int fd;

  fd = direction == AUDIO_TYPE_OUTPUT ?
       state->outdev_fd :
       state->indev_fd;

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ret = ioctl(fd, AUDIOIOC_STOP,
                    (unsigned long)state->session);
#else
  ret = ioctl(fd, AUDIOIOC_STOP, 0);
#endif

  return ret;
}

static int audio_test_cleanup(FAR struct audio_state_s *state, int direction)
{
  int fd;

  fd = direction == AUDIO_TYPE_OUTPUT ?
       state->outdev_fd :
       state->indev_fd;

  ioctl(fd, AUDIOIOC_UNREGISTERMQ, (unsigned long)state->mq);

#ifdef CONFIG_AUDIO_MULTI_SESSION
  ioctl(fd, AUDIOIOC_RELEASE, (unsigned long)state->session);
#else
  ioctl(fd, AUDIOIOC_RELEASE, 0);
#endif

  if (state->out_fd >= 0)
    {
      close(state->out_fd);
      state->out_fd = -1;
    }

  return 0;
}

static void drivertest_audio(void **audio_state)
{
  FAR struct audio_state_s *state;
  FAR struct ap_buffer_s **bufs = NULL;
  struct ap_buffer_info_s buf_info;
  struct audio_msg_s msg;
  struct timeval start;
  int directions[3] =
    {
      AUDIO_TYPE_INPUT,
      AUDIO_TYPE_OUTPUT,
      -1
    };

  struct mq_attr attr;
  int unconsumed = 0;
  unsigned int prio;
  bool streaming;
  bool running;
  int ret = 0;
  int direct;
  int x = 0;
  int i;

  state = (struct audio_state_s *)*audio_state;

  while (directions[x] != -1)
    {
      /* first round, test capture.
       * second round, test playback.
       * */

      streaming = running = true;
      if (!(directions[x] & state->direction))
        {
          x++;
          continue;
        }

      direct = directions[x];

      ret = audio_test_prepare(state, &buf_info, direct);
      assert_false(ret < 0);

      bufs = (FAR struct ap_buffer_s **)
              calloc(buf_info.nbuffers, sizeof(FAR void *));
      assert_true(bufs != NULL);

      ret = audio_test_alloc_buffer(state, bufs, &buf_info, direct);
      assert_false(ret < 0);

      for (i = 0; i < buf_info.nbuffers; i++)
        {
          if (direct == AUDIO_TYPE_OUTPUT)
              ret = audio_test_readbuffer(state, bufs[i]);

          if (ret < 0)
            {
              streaming = false;
            }
          else
            {
              ret = audio_test_enqueuebuffer(state, bufs[i], direct);
              assert_false(ret);
            }
        }

      if (running)
        {
          ret = audio_test_start(state, direct);
          assert_false(ret < 0);
          gettimeofday(&start, NULL);
        }

      printf("Start %s. \n", direct == AUDIO_TYPE_OUTPUT ?
                             "Playback" : "Capture");

      unconsumed = buf_info.nbuffers;

      while (running)
        {
          ret = mq_receive(state->mq, (FAR char *)&msg, sizeof(msg), &prio);
          if (ret != sizeof(msg))
            {
              continue;
            }

          switch (msg.msg_id)
            {
              case AUDIO_MSG_DEQUEUE:
                unconsumed--;
                if (streaming)
                  {
                    if (direct == AUDIO_TYPE_INPUT)
                      {
                        ret = audio_test_writebuffer(state, msg.u.ptr);
                      }
                    else
                      {
                        ret = audio_test_readbuffer(state, msg.u.ptr);
                      }

                    if (ret != OK)
                      {
                        streaming = false;
                      }
                    else
                      {
                        ret = audio_test_enqueuebuffer(state,
                                                       msg.u.ptr, direct);
                        if (ret != OK)
                          {
                            close(state->out_fd);
                            state->out_fd = -1;
                            streaming = false;
                          }
                        else
                          {
                            unconsumed++;
                          }
                      }
                  }
                break;

                case AUDIO_MSG_COMPLETE:
                  running = false;
                  break;

                default:
                  break;
            }

          /* Capture stopped */

          if (audio_test_timeout(state, direct, start) || !streaming)
            {
              ret = audio_test_stop(state, direct);
              assert_false(ret < 0);
              running = false;
            }
        }

      do
        {
          ret = mq_getattr(state->mq, &attr);
          assert_false(ret < 0);

          if (attr.mq_curmsgs == 0 && unconsumed <= 0)
            {
              break;
            }

          mq_receive(state->mq, (FAR char *)&msg, sizeof(msg), &prio);
          if (msg.msg_id == AUDIO_MSG_DEQUEUE)
            {
              unconsumed--;
            }
        }
      while (ret >= 0);

      ret = audio_test_free_buffer(state, bufs, &buf_info, direct);
      assert_false(ret < 0);

      ret = audio_test_cleanup(state, direct);
      assert_false(ret < 0);

      memset(&buf_info, 0, sizeof(buf_info));
      free(bufs);
      bufs = NULL;

      x++;
    }

  return;
}

static int audio_test_setup(FAR void **audio_state)
{
  FAR struct audio_state_s *state;
  struct ap_buffer_info_s buf_info;
  struct mq_attr attr;
  int maxmsg = 0;
  int ret = 0;

  state = (struct audio_state_s *)*audio_state;

  if (state->direction & AUDIO_TYPE_OUTPUT)
    {
      state->outdev_fd = open(state->outdev, O_RDWR | O_CLOEXEC);
      assert_false(state->outdev_fd < 0);

      ret = ioctl(state->outdev_fd, AUDIOIOC_GETBUFFERINFO,
                  (unsigned long)&buf_info);
      maxmsg = MAX(maxmsg,
                   ret >= 0 ? buf_info.nbuffers : CONFIG_AUDIO_NUM_BUFFERS);
    }

  if (state->direction & AUDIO_TYPE_INPUT)
    {
      state->indev_fd = open(state->indev, O_RDWR | O_CLOEXEC);
      assert_false(state->indev_fd < 0);

      ret = ioctl(state->indev_fd, AUDIOIOC_GETBUFFERINFO,
                  (unsigned long)&buf_info);
      maxmsg = MAX(maxmsg,
                   ret >= 0 ? buf_info.nbuffers : CONFIG_AUDIO_NUM_BUFFERS);
    }

  attr.mq_maxmsg  = maxmsg + 8;
  attr.mq_msgsize = sizeof(struct audio_msg_s);
  attr.mq_curmsgs = 0;
  attr.mq_flags   = 0;

  snprintf(state->mqname, sizeof(state->mqname), "/tmp/%p",
           ((void *)state));

  state->mq = mq_open(state->mqname, O_RDWR | O_CREAT, 0644, &attr);
  assert_false(state->mq < 0);

  return OK;
}

static int audio_test_teardown(FAR void **audio_state)
{
  FAR struct audio_state_s *state;
  state = (struct audio_state_s *)*audio_state;

  if (state->outdev_fd >= 0)
    {
      close(state->outdev_fd);
      state->outdev_fd = -1;
    }

  if (state->outdev_fd >= 0)
    {
      close(state->indev_fd);
      state->indev_fd = -1;
    }

  mq_close(state->mq);
  mq_unlink(state->mqname);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: loopback_test_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct audio_state_s state = {
    .outdev    = "/dev/audio/pcm0p",
    .indev     = "/dev/audio/pcm0c",
    .duration  = 10,
    .chans     = 2,
    .bpsamp    = 16,
    .samprate  = 44100,
    .format    = AUDIO_FMT_PCM,
    .chmap     = 0,
    .outdev_fd = -1,
    .indev_fd  = -1,
    .out_fd    = -1,
  };

  parse_commandline(&state, argc, argv);

  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(drivertest_audio,
                                               audio_test_setup,
                                               audio_test_teardown,
                                               &state),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
