/****************************************************************************
 * apps/examples/ws2812esp32rmt/ws2812esp32rmt_main.c
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

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APB_PERIOD (12.5)

#define T0H ((uint16_t)(350 / APB_PERIOD))   // ns
#define T0L ((uint16_t)(900 / APB_PERIOD))   // ns
#define T1H ((uint16_t)(900 / APB_PERIOD))   // ns
#define T1L ((uint16_t)(350 / APB_PERIOD))   // ns
#define RES ((uint16_t)(60000 / APB_PERIOD)) // ns

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct led_s
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int fd;
static uint32_t *rmt_buffer;
static uint32_t rmt_buffer_len_in_words;
static uint32_t leds_buffer_len_in_bytes;
static struct led_s *leds_buffer;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint32_t map_byte_to_words(uint8_t byte, uint32_t *dst)
{
  uint32_t ret = 0;
  uint8_t mask = 0x80;
  for (int i = 0; i < 8; i++)
    {
      uint8_t bit = byte&mask;
      mask >>= 1;
      uint32_t word;
      if (bit)
        {
          word = (T1L << 16) | (0x8000 | T1H);
        }
        else
        {
          word = (T0L << 16) | (0x8000 | T0H);
        }

      *dst = word;
      dst++;
      ret++;
    }

  return ret;
}

static int map_leds_to_words(struct led_s *leds,
            uint32_t n_leds,
            uint32_t *dst,
            uint32_t dst_len_in_words
          )
{
  int ret = OK;

  if (!dst || !leds)
    {
      return -1;
    }

  uint32_t required_words = n_leds * 3 * 8;

  if (required_words >= dst_len_in_words)
    {
      _err("Required %d words, but buffer has only:%d",
        required_words, dst_len_in_words
      );
      return -1;
    }

  uint32_t dst_offset = 0;
  for (uint32_t led_idx = 0; led_idx < n_leds; led_idx++)
    {
      dst_offset += map_byte_to_words(leds[led_idx].g, dst + dst_offset);
      dst_offset += map_byte_to_words(leds[led_idx].r, dst + dst_offset);
      dst_offset += map_byte_to_words(leds[led_idx].b, dst + dst_offset);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ws2812esp32rmt_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc < 2)
    {
      printf("usage: %s channel led_count\n", argv[0]);
      exit(1);
    }

  int ch_idx = atoi(argv[1]);
  if (ch_idx >= 8)
    {
      printf("channel must be int the range 0~7\n");
      exit(1);
    }

  uint32_t n_leds = atoi(argv[2]);

  char devname[50];
  snprintf(devname, sizeof(devname), "/dev/rmt%d", ch_idx);
  fd = open(devname, O_WRONLY);
  if (fd < 0)
    {
      fprintf(stderr,
              "%s: open %s failed: %d\n",
              argv[0],
              devname,
              errno);
      goto errout_with_dev;
    }

  _info("size of (struct led_s) is:%d", sizeof(struct led_s));

  leds_buffer_len_in_bytes = sizeof(struct led_s)*n_leds;
  leds_buffer = (struct led_s *)malloc(leds_buffer_len_in_bytes);
  _info("leds:%d - leds_buffer %p len:%d\n",
    n_leds,
    leds_buffer,
    leds_buffer_len_in_bytes
  );

  if (!leds_buffer)
    {
      _err("failure allocating %d bytes for LED buffer",
        leds_buffer_len_in_bytes
      );
      goto errout;
    }

  rmt_buffer_len_in_words = (leds_buffer_len_in_bytes * 8 + 1);
  rmt_buffer = (uint32_t *)malloc(rmt_buffer_len_in_words * 4);
  _info("words:%d - rmt_buffer %p rmt_buffer_len_in_words:%d\n",
    rmt_buffer_len_in_words,
    rmt_buffer,
    rmt_buffer_len_in_words
  );

  if (!rmt_buffer)
    {
      _err("failure allocating %d words for RMT word buffer",
        rmt_buffer_len_in_words
      );
      goto errout;
    }

  int pos = 0;
  int direction = 0;

  /* Run the display loop */

  while (1)
    {
      int n_loops = 30;
      for (int i = 0; i < n_loops; ++i)
        {
          for (int l = 0; l < n_leds; l++)
            {
              if (!direction)
                leds_buffer[l].r = i;
              else
                leds_buffer[l].r = 30 - i;

              leds_buffer[l].g = 0x00;
              leds_buffer[l].b = 0x00;

              if (l == pos)
                {
                  leds_buffer[l].r = 0x00;
                  leds_buffer[l].g = 0xff;
                  leds_buffer[l].b = 0x00;
                }
            }

          if (pos++ >= n_leds)
              pos = 0;
          int ret;
          ret = map_leds_to_words(leds_buffer,
            n_leds,
            rmt_buffer,
            rmt_buffer_len_in_words
          );

          if (ret)
            {
              _err("error mapping leds to words");
              goto errout;
            }

          ret = write(fd, rmt_buffer, rmt_buffer_len_in_words * 4);

          if (ret < 0)
            {
              _err("error writing to device, %d errno:%d", ret, errno);
              goto errout_with_dev;
            }

/****************************************************************************
 * Must sleep so the WS2812 can understand the EOT
 ****************************************************************************/

          usleep(50000);
        }

      direction = !direction;
    }

  free(leds_buffer);
  free(rmt_buffer);
  close(fd);
  fflush(stdout);
  return OK;

errout_with_dev:
  close(fd);

errout:
  fflush(stdout);
  return ERROR;
}
