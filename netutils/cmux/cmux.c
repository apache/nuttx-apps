/****************************************************************************
 * apps/netutils/cmux/cmux.c
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/param.h>
#include <sys/types.h>
#include <pthread.h>
#include <sched.h>
#include <pty.h>
#include <nuttx/crc8.h>

#include "netutils/chat.h"
#include "netutils/cmux.h"
#include "cmux.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define CMUX_MIN_FRAME_LEN (5)
#define CMUX_FRAME_PREFIX (5)
#define CMUX_FRAME_POSFIX (2)

#define CMUX_TASK_NAME ("cmux")
#define CMUX_THREAD_PRIOR (100)
#define CMUX_THREAD_STACK_SIZE (3072)

#define cmux_inc_buffer(buf, p) \
  p++;                          \
  if (p == buf->endp)           \
    p = buf->data;

#define cmux_buffer_length(buf) \
  ((buf->readp > buf->writep) ? (CMUX_BUFFER_SZ - (buf->readp - buf->writep)) : (buf->writep - buf->readp))

#define cmux_buffer_free(buf) \
  ((buf->readp > buf->writep) ? (buf->readp - buf->writep) : (CMUX_BUFFER_SZ - (buf->writep - buf->readp)))

struct cmux_ctl_s
{
  int fd;
  int total_ports;
  struct cmux_parse_s *parse;
  struct cmux_channel_s *channels;
  struct cmux_stream_buffer_s *stream;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmux_calulate_fcs
 *
 * Description:
 *  Calculate the frame checking sequence.
 *
 ****************************************************************************/

static unsigned char cmux_calulate_fcs(const unsigned char *input, int count)
{
  return crc8rohcpart(input, count, 0x00);
}

/****************************************************************************
 * Name: cmux_parse_create
 *
 * Description:
 *  Create a circular buffer to receive incoming packets.
 *
 ****************************************************************************/

static struct cmux_stream_buffer_s *cmux_stream_buffer_create(void)
{
  struct cmux_stream_buffer_s *cmux_buffer =
                    malloc(sizeof(struct cmux_stream_buffer_s));
  if (cmux_buffer)
    {
      memset(cmux_buffer, 0, sizeof(struct cmux_stream_buffer_s));
      cmux_buffer->readp = cmux_buffer->data;
      cmux_buffer->writep = cmux_buffer->data;
      cmux_buffer->endp = cmux_buffer->data + CMUX_BUFFER_SZ;
    }

  return cmux_buffer;
}

/****************************************************************************
 * Name: cmux_buffer_write
 *
 * Description:
 *  Write the input frames to the circular buffer.
 *
 ****************************************************************************/

static int cmux_buffer_write(struct cmux_stream_buffer_s *cmux_buffer,
                              const char *input, int length)
{
  int c = cmux_buffer->endp - cmux_buffer->writep;

  length = MIN(length, cmux_buffer_free(cmux_buffer));
  if (length > c)
    {
      memcpy(cmux_buffer->writep, input, c);
      memcpy(cmux_buffer->data, input + c, length - c);
      cmux_buffer->writep = cmux_buffer->data + (length - c);
    }
  else
    {
      memcpy(cmux_buffer->writep, input, length);
      cmux_buffer->writep += length;
      if (cmux_buffer->writep == cmux_buffer->endp)
        {
          cmux_buffer->writep = cmux_buffer->data;
        }
    }

  return length;
}

/****************************************************************************
 * Name: cmux_parse_reset
 *
 * Description:
 *  Reset data buffer and parse struct.
 *
 ****************************************************************************/

static void cmux_parse_reset(struct cmux_parse_s *cmux_parse)
{
  if (cmux_parse)
    {
      memset(cmux_parse->data, 0x00, CMUX_BUFFER_SZ);
      memset(cmux_parse, 0x00, sizeof(struct cmux_parse_s));
    }
}

static int cmux_decode_frame(struct cmux_stream_buffer_s *cmux_buffer,
                            struct cmux_parse_s *cmux_parse)
{
  /* Minimal length to CMUX Frame : address, type, length, FCS and flag */

  int length = CMUX_MIN_FRAME_LEN;
  unsigned char *data = NULL;
  unsigned char fcs = CMUX_FCS_MAX_VALUE;
  int end = 0;

  if (!cmux_buffer || !cmux_parse)
    {
      return -EACCES;
    }

  while (cmux_buffer_length(cmux_buffer) >= CMUX_MIN_FRAME_LEN)
    {
      cmux_buffer->flag_found = 0;
      length = CMUX_MIN_FRAME_LEN;

      while (!cmux_buffer->flag_found &&
            cmux_buffer_length(cmux_buffer) > 0)
        {
          if (*cmux_buffer->readp == CMUX_OPEN_FLAG)
            {
              cmux_buffer->flag_found = 1;
            }

          cmux_inc_buffer(cmux_buffer, cmux_buffer->readp);
        }

      if (!cmux_buffer->flag_found)
        {
          return ERROR;
        }

      while (cmux_buffer_length(cmux_buffer) > 0 &&
           (*cmux_buffer->readp == CMUX_OPEN_FLAG))
        {
          cmux_inc_buffer(cmux_buffer, cmux_buffer->readp);
        }

      if (cmux_buffer_length(cmux_buffer) < length)
        {
          return ERROR;
        }

      data = cmux_buffer->readp;
      fcs = CMUX_FCS_MAX_VALUE;
      cmux_parse->address = ((*data &
                              CMUX_ADDR_FIELD_CHECK) >> 2);
      fcs = crc8rohcincr(*data, fcs);
      cmux_inc_buffer(cmux_buffer, data);

      cmux_parse->control = *data;
      fcs = crc8rohcincr(*data, fcs);
      cmux_inc_buffer(cmux_buffer, data);

      cmux_parse->data_length = (*data &
                                CMUX_LENGTH_FIELD_OPERATOR) >> 1;
      fcs = crc8rohcincr(*data, fcs);

      /* EA bit, should always have the value 1 */

      if (!(*data & 1))
        {
          cmux_buffer->readp = data;
          cmux_buffer->flag_found = 0;
          continue;
        }

      length += cmux_parse->data_length;
      if (!(cmux_buffer_length(cmux_buffer) >= length))
        {
          return ERROR;
        }

      cmux_inc_buffer(cmux_buffer, data);
      if (cmux_parse->data_length > 0 &&
        cmux_parse->data_length < CMUX_BUFFER_SZ)
        {
          end = cmux_buffer->endp - data;
          if (cmux_parse->data_length > end)
            {
              memcpy(cmux_parse->data, data, end);
              memcpy(cmux_parse->data + end, cmux_buffer->data,
                    cmux_parse->data_length - end);
              data = cmux_buffer->data +
                    (cmux_parse->data_length - end);
            }
          else
            {
              memcpy(cmux_parse->data, data, cmux_parse->data_length);
              data += cmux_parse->data_length;
              if (data == cmux_buffer->endp)
                {
                  data = cmux_buffer->data;
                }
            }

          if (CMUX_FRAME_TYPE(CMUX_FRAME_TYPE_UI, cmux_parse))
            {
              int i;
              for (i = 0; i < cmux_parse->data_length; i++)
                {
                  fcs = crc8rohcincr(cmux_parse->data[i], fcs);
                }
            }
        }

      if (crc8rohcincr(*data, fcs) != CMUX_FCS_OPERATOR)
        {
          cmux_buffer->dropped_count++;
          cmux_buffer->readp = data;
          cmux_parse_reset(cmux_parse);
          continue;
        }

      cmux_inc_buffer(cmux_buffer, data);
      if (*data != CMUX_CLOSE_FLAG)
        {
          cmux_buffer->readp = data;
          cmux_buffer->dropped_count++;
          cmux_parse_reset(cmux_parse);
          continue;
        }

      cmux_buffer->received_count++;
      cmux_inc_buffer(cmux_buffer, data);
      cmux_buffer->readp = data;
      return OK;
    }

  return ERROR;
}

/****************************************************************************
 * Name: cmux_encode_frame
 *
 * Description:
 *  Encode a buffer to the CMUX protocol.
 *
 ****************************************************************************/

static int cmux_encode_frame(int fd, int channel, char *buffer,
                            int frame_size, unsigned char type)
{
  int prefix_len = 4;
  unsigned char frame_prefix[CMUX_FRAME_PREFIX] = {
    CMUX_OPEN_FLAG, (CMUX_ADDR_FIELD_BIT_EA | CMUX_ADDR_FIELD_BIT_CR),
    0x00, 0x00, 0x00
  };

  unsigned char frame_posfix[CMUX_FRAME_POSFIX] = {
    CMUX_FCS_MAX_VALUE,
    CMUX_CLOSE_FLAG
  };

  frame_prefix[CMUX_BIT1] = (frame_prefix[CMUX_BIT1] |
  ((CMUX_ADDR_FIELD_OPERATOR & (unsigned char)channel) << 2));

  frame_prefix[CMUX_BIT2] = type;

  if (frame_size <= CMUX_FRAME_MAX_SIZE)
    {
      frame_prefix[CMUX_BIT3] = CMUX_ADDR_FIELD_BIT_EA | (frame_size << 1);
      prefix_len = 4;
    }
  else
    {
      frame_prefix[CMUX_BIT3] = (frame_size << 1) &
                                CMUX_LENGTH_FIELD_OPERATOR;
      frame_prefix[CMUX_BIT4] = CMUX_ADDR_FIELD_BIT_EA |
                                ((frame_size >> 7) << 1);
      prefix_len = 5;
    }

  frame_posfix[CMUX_BIT0] = cmux_calulate_fcs(frame_prefix + 1,
                                              prefix_len - 1);

  int ret = write(fd, frame_prefix, prefix_len);
  if (ret != prefix_len)
    {
      return ERROR;
    }

  if (frame_size > 0 && buffer != NULL)
    {
      ret = write(fd, buffer, frame_size);
      if (ret != frame_size)
        {
          ninfo("Failed to write buffer (wrote %d, expected %d)\n",
                ret, frame_size);
          return ERROR;
        }
    }

  ret = write(fd, frame_posfix, CMUX_FRAME_POSFIX);
  if (ret != CMUX_FRAME_POSFIX)
    {
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: cmux_open_pseudo_tty
 *
 * Description:
 *  Open pseudo-terminals according to the number of channels.
 *
 ****************************************************************************/

static int cmux_open_pseudo_tty(struct cmux_channel_s *channel,
                                int total_channels)
{
  int ret = 0;
  struct termios options;

  if (!channel)
    {
      return -EACCES;
    }

  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_iflag &= ~(INLCR | ICRNL | IGNCR);

  options.c_oflag &= ~OPOST;
  options.c_oflag &= ~OLCUC;
  options.c_oflag &= ~ONLRET;
  options.c_oflag &= ~ONOCR;
  options.c_oflag &= ~OCRNL;

  for (int i = 0; i < total_channels; i++)
    {
      ret = openpty(&channel[i].master_fd, &channel[i].slave_fd,
                    (FAR char *)&channel[i].slave_path, &options, NULL);
      if (ret < 0)
        {
          perror("Failed to open pseudo terminal \n");
          break;
        }
      else
        {
          ninfo("Open pseudo tty name: %s\n", channel[i].slave_path);

          channel[i].dlci = i + 1;
          channel[i].active = true;
          channel[i].last_activity = time(NULL);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: cmux_open_channels
 *
 * Description:
 *  Open the controller and the logic channels.
 *
 ****************************************************************************/

static int cmux_open_channels(int fd, int total_channels)
{
  int ret = 0;
  for (int i = 0; i < total_channels; i++)
    {
      ret = cmux_encode_frame(fd, i,
            NULL, 0x00,
            (CMUX_FRAME_TYPE_SABM | CMUX_CONTROL_FIELD_BIT_PF));
      if (ret != OK)
        {
          perror("ERROR: Failed to open channel\n");
          break;
        }

      sleep(1);
    }

  return ret;
}

/****************************************************************************
 * Name: cmux_extract
 *
 * Description:
 *  Extract a frame from the circular buffer according
 *  to the input and length.
 *
 ****************************************************************************/

static int cmux_extract(struct cmux_ctl_s *ctl, char *input, int len)
{
  int ret;
  int frames_extracted = 0;

  if (!input)
    {
      return ERROR;
    }

  ret = cmux_buffer_write(ctl->stream, input, len);
  if (ret < 0)
    {
      return ret;
    }

  while (cmux_decode_frame(ctl->stream, ctl->parse) >= 0)
    {
      if (CMUX_FRAME_TYPE(CMUX_FRAME_TYPE_UI, ctl->parse) ||
          CMUX_FRAME_TYPE(CMUX_FRAME_TYPE_UIH, ctl->parse))
        {
          if (ctl->parse->address > 0)
            {
              /* Logic channel */

              ret = write(ctl->channels[ctl->parse->address].master_fd,
                          ctl->parse->data,
                          ctl->parse->data_length);

              if (ret != ctl->parse->data_length)
                {
                  ninfo("Frame length less than expected\n");
                  continue;
                }
            }
          else
            {
              /* Control channel */
            }
        }
      else
        {
          switch ((ctl->parse->control & ~CMUX_CONTROL_FIELD_BIT_PF))
            {
              case CMUX_FRAME_TYPE_UA:
                ninfo("Frame type: UA \n");

                break;
              case CMUX_FRAME_TYPE_DM:
                ninfo("Frame type: DM \n");
                if (ctl->channels[ctl->parse->address].active)
                  {
                    ctl->channels[ctl->parse->address].active = 0;
                  }

                break;
              case CMUX_FRAME_TYPE_DISC:
                ninfo("Frame type: DISC \n");

                if (ctl->channels[ctl->parse->address].active)
                  {
                    ctl->channels[ctl->parse->address].active = false;
                    ret = cmux_encode_frame(ctl->fd,
                          ctl->parse->address, NULL, 0x00,
                          (CMUX_FRAME_TYPE_UA | CMUX_CONTROL_FIELD_BIT_PF));
                  }
                else
                  {
                    ret = cmux_encode_frame(ctl->fd,
                          ctl->parse->address, NULL, 0x00,
                          (CMUX_FRAME_TYPE_DM | CMUX_CONTROL_FIELD_BIT_PF));
                  }

                if (ret < 0)
                  {
                    nwarn("Failed to encode the frame. Address (%d) \n",
                          ctl->parse->address);
                  }

                break;
              case CMUX_FRAME_TYPE_SABM:
                ninfo("Frame type: SABM\n");

                if (!ctl->channels[ctl->parse->address].active)
                  {
                    if (!ctl->parse->address)
                      {
                        ninfo("Control channel opened.\n");
                      }
                    else
                      {
                        ninfo("Logical channel %d opened.\n",
                          ctl->parse->address);
                      }
                  }
                else
                  {
                    nwarn("Even though channel %d was already closed.\n",
                          ctl->parse->address);
                  }

                ctl->channels[ctl->parse->address].active = 1;
                ret = cmux_encode_frame(ctl->fd,
                      ctl->parse->address, NULL, 0x00,
                      (CMUX_FRAME_TYPE_UA | CMUX_CONTROL_FIELD_BIT_PF));
                if (ret < 0)
                  {
                    nwarn("Failed to encode the frame. Address (%d) \n",
                          ctl->parse->address);
                  }

                break;
              default:
                ninfo("Frame type: UNKNOWN\n");
                break;
            }
        }

      frames_extracted++;
    }

  cmux_parse_reset(ctl->parse);

  return frames_extracted;
}

/****************************************************************************
 * Name: cmux_protocol_send
 *
 * Description:
 *  Send encoded messages to a specific address.
 *
 ****************************************************************************/

static int cmux_send(struct cmux_ctl_s *ctl, char *buffer,
                    int length, int address)
{
  int ret;
  if (!buffer)
    {
      return ERROR;
    }

  ret = cmux_encode_frame(ctl->fd,
                          address,
                          buffer,
                          length, CMUX_FRAME_TYPE_UIH);
  return ret;
}

/****************************************************************************
 * Name: cmux_thread
 *
 * Description:
 *   Start cmux thread.
 *
 ****************************************************************************/

static void *cmux_thread(void *args)
{
  struct cmux_ctl_s *ctl = (struct cmux_ctl_s *)args;
  int ret = 0;
  fd_set rfds;
  struct timeval timeout;
  char buffer[CMUX_BUFFER_SZ];

  while (true)
    {
      FD_ZERO(&rfds);
      FD_SET(ctl->fd, &rfds);

      int max_fd = ctl->fd;
      for (int i = 0; i < ctl->total_ports; i++)
        {
          if (ctl->channels[i].active)
            {
              FD_SET(ctl->channels[i].master_fd, &rfds);
              FD_SET(ctl->channels[i].slave_fd, &rfds);

              if (ctl->channels[i].master_fd > max_fd)
                {
                  max_fd = ctl->channels[i].master_fd;
                }

              if (ctl->channels[i].slave_fd > max_fd)
                {
                  max_fd = ctl->channels[i].slave_fd;
                }
            }
        }

      timeout.tv_usec = 100;
      timeout.tv_sec = 0;

      ret = select(max_fd + 1, &rfds, NULL, NULL, &timeout);
      if (ret > 0)
        {
          if (FD_ISSET(ctl->fd, &rfds))
            {
              int bytes_read = read(ctl->fd, buffer, sizeof(buffer) - 1);
              if (bytes_read > 0)
                {
                  buffer[bytes_read] = '\0';
                  ret = cmux_extract(ctl, buffer, bytes_read);
                  if (ret < 0)
                    {
                      perror("ERROR: Failed to extract frames \n");
                    }
                }
            }

          for (int i = 0; i < ctl->total_ports; i++)
            {
              if (ctl->channels[i].active &&
                  FD_ISSET(ctl->channels[i].master_fd, &rfds))
                {
                  memset(buffer, 0, sizeof(buffer));
                  int bytes_read = read(ctl->channels[i].master_fd,
                                    buffer, sizeof(buffer) - 1);
                  if (bytes_read > 0)
                    {
                      ret = cmux_send(ctl, buffer, bytes_read, i);
                      if (ret < 0)
                        {
                          nwarn("WANING: Retransmit from pty/%d\n", i);
                        }
                    }
                }
            }
        }
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmux_create
 *
 * Description:
 *  Create CMUX context.
 *
 ****************************************************************************/

int cmux_create(struct cmux_settings_s *settings)
{
  int ret = 0;
  struct chat_ctl ctl;
  struct cmux_ctl_s *cmux_ctl = NULL;
  pthread_t cmux_thread_id;
  struct sched_param param;
  pthread_attr_t attr;

  cmux_ctl = malloc(sizeof(struct cmux_ctl_s));
  if (!cmux_ctl)
    {
      perror("ERROR: Failed to allocate memory for CMUX daemon\n");
      ret = -ENOMEM;
      return ret;
    }

  memset(cmux_ctl, 0, sizeof(struct cmux_ctl_s));
  cmux_ctl->fd = open(settings->tty_name, O_RDWR | O_NONBLOCK);
  if (cmux_ctl->fd < 0)
    {
      perror("ERROR: Unable to open file %s\n");
      goto exit;
    }

  ctl.echo = false;
  ctl.verbose = false;
  ctl.fd = cmux_ctl->fd;
  ctl.timeout = 30;

  ret = chat(&ctl, settings->script);
  if (ret < 0)
    {
      perror("ERROR:Failed to run cmux script\n");
      goto exit;
    }

  cmux_ctl->channels = malloc(sizeof(struct cmux_channel_s) *
                        settings->total_channels);
  if (!cmux_ctl->channels)
    {
      perror("ERROR:Failed to allocate memory to channels\n");
      ret = -ENOMEM;
      goto exit;
    }

  cmux_ctl->parse = malloc(sizeof(struct cmux_parse_s));
  if (!cmux_ctl->parse)
    {
      perror("ERROR: Failed to allocate memory to parse\n");
      ret = -ENOMEM;
      goto exit;
    }

  cmux_parse_reset(cmux_ctl->parse);

  ret = cmux_open_pseudo_tty(cmux_ctl->channels, settings->total_channels);
  if (ret < 0)
    {
      perror("ERROR: Failed to open pseudo tty.\n");
      goto exit;
    }

  cmux_ctl->stream = cmux_stream_buffer_create();
  if (cmux_ctl->stream == NULL)
    {
      perror("ERROR: Failed to allocate memory to stream\n");
      ret = -ENOMEM;
      goto exit;
    }

  ret = cmux_open_channels(cmux_ctl->fd, settings->total_channels);
  if (ret < 0)
    {
      perror("ERROR: Failed to open virtual channels.\n");
      goto exit;
    }

  cmux_ctl->total_ports = settings->total_channels;

  pthread_attr_init(&attr);
  param.sched_priority = CMUX_THREAD_PRIOR;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, CMUX_THREAD_STACK_SIZE);

  ret = pthread_create(&cmux_thread_id, &attr, cmux_thread, cmux_ctl);

  return ret;

exit:

  if (cmux_ctl->channels)
    {
      for (int i = 0; i < settings->total_channels; i++)
        {
          if (cmux_ctl->channels[i].master_fd > 0)
            {
              close(cmux_ctl->channels[i].master_fd);
            }

          if (cmux_ctl->channels[i].slave_fd > 0)
            {
              close(cmux_ctl->channels[i].slave_fd);
            }
        }

      free(cmux_ctl->channels);
    }

  if (cmux_ctl->stream)
    {
      free(cmux_ctl->stream);
    }

  close(cmux_ctl->fd);

  return ret;
}
