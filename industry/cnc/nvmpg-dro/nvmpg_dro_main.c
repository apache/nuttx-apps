/****************************************************************************
 * apps/industry/cnc/nvmpg-dro/nvmpg_dro_main.c
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

/* DRO for the NVMPG pendant.  Drop-in replacement for the original
 * firmware: it speaks the same serial protocol with the NVUM/NVEM
 * controller.
 *
 * Controller -> pendant:
 *   5A 5A <51 bytes>   status frame, no checksum (layout below)
 *   5A 5B              identify request, answered with 01 13
 *
 * Pendant -> controller:
 *   key index + 2            key pressed
 *   (key index + 2) | 0x80   key released
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <nuttx/ioexpander/gpio.h>

#include "nvmpg_dro.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NVDRO_SYNC          0x5a
#define NVDRO_IDENT         0x5b
#define NVDRO_FRAME_LEN     51

/* Status frame layout (little-endian, packed).  The meaning of each field
 * comes from how the original firmware draws it.
 */

#define FRM_POS(axis)       (4 * (axis)) /* int32, 0.0001 units, axes X..C */
#define FRM_RESET_LAMP      0x19         /* u8: 0 = green, else red */
#define FRM_AUX_VALUE       0x1b         /* int32, 5 digits, lower right */
#define FRM_COMM_LAMP       0x1f         /* u8: 0 = off, else yellow */
#define FRM_OVERRIDE(n)     (0x20 + (n)) /* u8: FRO, SJR, SRO in % */
#define FRM_OVR_SELECT      0x24         /* u8: 0 = none, 1..3 = FRO..SRO */
#define FRM_AXIS_SELECT     0x25         /* u8: 0..5 = X..C, else none */
#define FRM_STEP            0x26         /* u8: 0..3 = x1 .. x1000 */

#define NVDRO_NAXES         6
#define NVDRO_NROWS         3
#define NVDRO_KEYCODE(i)    ((uint8_t)((i) + 2))
#define NVDRO_RELEASE       0x80
#define NVDRO_POLL_MS       10
#define NVDRO_LINK_TIMEOUT  CONFIG_INDUSTRY_CNC_NVMPG_DRO_LINK_TIMEOUT

/* Screen layout (220x176) */

#define ROW_Y(r)            (6 + (r) * 42)
#define ROW_H               36
#define AXIS_X              4
#define AXIS_W              30
#define VALUE_X             38
#define VALUE_W             134
#define VALUE_CHARS         12
#define OVR_X               176
#define OVR_W               40
#define BOTTOM_Y            132
#define BOTTOM_H            38

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nvdro_s
{
  int ttyfd;
  int keyfd[CONFIG_INDUSTRY_CNC_NVMPG_DRO_NKEYS];
  pthread_mutex_t lock;          /* Protects rxframe/rxseq and TTY writes */

  uint8_t rxframe[NVDRO_FRAME_LEN];
  uint32_t rxseq;                /* Incremented for every complete frame */

  /* Owned by the main thread */

  uint8_t frame[NVDRO_FRAME_LEN];
  uint8_t shown[NVDRO_FRAME_LEN];
  bool redraw;                   /* Ignore 'shown', draw everything */
  bool linkup;
  bool shown_linkup;
  int page;                      /* 0 = XYZ, 1 = ABC */
  bool keydown[CONFIG_INDUSTRY_CNC_NVMPG_DRO_NKEYS];
  bool keyraw[CONFIG_INDUSTRY_CNC_NVMPG_DRO_NKEYS];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct nvdro_s g_dro;

static const char g_axisname[NVDRO_NAXES] =
{
  'X', 'Y', 'Z', 'A', 'B', 'C'
};

static const char *const g_ovrname[NVDRO_NROWS] =
{
  "FRO%", "SJR%", "SRO%"
};

static const char *const g_stepname[] =
{
  "x1   ", "x10  ", "x100 ", "x1000"
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int32_t get_le32(FAR const uint8_t *p)
{
  return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                   ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}

static bool field_changed(FAR struct nvdro_s *dro, int off, int len)
{
  return dro->redraw ||
         memcmp(&dro->frame[off], &dro->shown[off], len) != 0;
}

static uint32_t now_ms(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/****************************************************************************
 * Serial link
 ****************************************************************************/

static void tty_send(FAR struct nvdro_s *dro, FAR const uint8_t *buf,
                     size_t len)
{
  pthread_mutex_lock(&dro->lock);
  write(dro->ttyfd, buf, len);
  pthread_mutex_unlock(&dro->lock);
}

static int tty_open(FAR struct nvdro_s *dro)
{
  struct termios tio;

  dro->ttyfd = open(CONFIG_INDUSTRY_CNC_NVMPG_DRO_TTY, O_RDWR | O_NOCTTY);
  if (dro->ttyfd < 0)
    {
      return -errno;
    }

  /* The board sets USART1 to 115200 8N1, but make sure the port is raw */

  if (tcgetattr(dro->ttyfd, &tio) == 0)
    {
      cfmakeraw(&tio);
      cfsetspeed(&tio, B115200);
      tcsetattr(dro->ttyfd, TCSANOW, &tio);
    }

  return 0;
}

/* Receive thread: same state machine as the original firmware */

static FAR void *rx_thread(FAR void *arg)
{
  FAR struct nvdro_s *dro = arg;
  uint8_t frame[NVDRO_FRAME_LEN];
  uint8_t buf[16];
  int state = 0;
  int idx = 0;

  for (; ; )
    {
      ssize_t n = read(dro->ttyfd, buf, sizeof(buf));
      ssize_t i;

      if (n <= 0)
        {
          usleep(10000);
          continue;
        }

      for (i = 0; i < n; i++)
        {
          uint8_t c = buf[i];

          switch (state)
            {
              case 0:  /* Waiting for the first sync byte */
                if (c == NVDRO_SYNC)
                  {
                    state = 1;
                  }
                break;

              case 1:  /* Second byte selects frame or identify */
                if (c == NVDRO_SYNC)
                  {
                    state = 2;
                    idx = 0;
                  }
                else
                  {
                    state = 0;
                    if (c == NVDRO_IDENT)
                      {
                        static const uint8_t ident[2] =
                        {
                          0x01, 0x13
                        };

                        tty_send(dro, ident, sizeof(ident));
                      }
                  }
                break;

              default: /* Payload */
                frame[idx++] = c;
                if (idx >= NVDRO_FRAME_LEN)
                  {
                    pthread_mutex_lock(&dro->lock);
                    memcpy(dro->rxframe, frame, NVDRO_FRAME_LEN);
                    dro->rxseq++;
                    pthread_mutex_unlock(&dro->lock);
                    state = 0;
                  }
                break;
            }
        }
    }

  return NULL;
}

/****************************************************************************
 * Keys
 ****************************************************************************/

static int keys_open(FAR struct nvdro_s *dro)
{
  char path[16];
  int i;

  for (i = 0; i < CONFIG_INDUSTRY_CNC_NVMPG_DRO_NKEYS; i++)
    {
      snprintf(path, sizeof(path), "/dev/gpio%d", i);
      dro->keyfd[i] = open(path, O_RDONLY);
      if (dro->keyfd[i] < 0)
        {
          syslog(LOG_ERR, "nvmpg_dro: cannot open %s: %d\n", path, errno);
        }
    }

  return 0;
}

/* Poll the keys, debounce over two samples and report changes */

static void keys_poll(FAR struct nvdro_s *dro)
{
  int i;

  for (i = 0; i < CONFIG_INDUSTRY_CNC_NVMPG_DRO_NKEYS; i++)
    {
      bool released = true;
      bool pressed;

      if (dro->keyfd[i] < 0 ||
          ioctl(dro->keyfd[i], GPIOC_READ,
                (unsigned long)((uintptr_t)&released)) < 0)
        {
          continue;
        }

      pressed = !released;
      if (pressed == dro->keyraw[i] && pressed != dro->keydown[i])
        {
          uint8_t code = NVDRO_KEYCODE(i);

          if (!pressed)
            {
              code |= NVDRO_RELEASE;
            }

          tty_send(dro, &code, 1);
          dro->keydown[i] = pressed;
        }

      dro->keyraw[i] = pressed;
    }
}

/****************************************************************************
 * Display
 ****************************************************************************/

static void draw_static(void)
{
  int r;

  nvdro_fill(0, 0, nvdro_lcd_width(), nvdro_lcd_height(), NVDRO_BG);

  for (r = 0; r < NVDRO_NROWS; r++)
    {
      nvdro_fill(VALUE_X, ROW_Y(r), VALUE_W, ROW_H, NVDRO_PANEL);
      nvdro_fill(OVR_X, ROW_Y(r), OVR_W, ROW_H, NVDRO_BOX);
      nvdro_text(OVR_X + 8, ROW_Y(r) + 3, g_ovrname[r], 1, 1,
                 NVDRO_BLACK, NVDRO_BOX);
    }

  /* Bottom: MPG step | RESET lamp | COMM lamp | aux value */

  nvdro_fill(4, BOTTOM_Y, 62, BOTTOM_H, NVDRO_BOX);
  nvdro_text(72, BOTTOM_Y, "RESET", 1, 1, NVDRO_WHITE, NVDRO_BG);
  nvdro_text(116, BOTTOM_Y, "COMM", 1, 1, NVDRO_WHITE, NVDRO_BG);
  nvdro_fill(156, BOTTOM_Y, 60, BOTTOM_H, NVDRO_BOX);
}

static void draw_axis_letter(FAR struct nvdro_s *dro, int row)
{
  int axis = dro->page * NVDRO_NROWS + row;
  int sel = dro->frame[FRM_AXIS_SELECT];
  uint16_t border = (sel == axis) ? NVDRO_YELLOW : NVDRO_BOX;
  char name[2];

  name[0] = g_axisname[axis];
  name[1] = '\0';

  nvdro_fill(AXIS_X, ROW_Y(row), AXIS_W, ROW_H, NVDRO_BOX);
  nvdro_frame(AXIS_X, ROW_Y(row), AXIS_W, ROW_H, 2, border);
  nvdro_text(AXIS_X + 8, ROW_Y(row) + 8, name, 3, 0,
             NVDRO_BLACK, NVDRO_BOX);
}

static void draw_position(FAR struct nvdro_s *dro, int row)
{
  int axis = dro->page * NVDRO_NROWS + row;
  int64_t v = get_le32(&dro->frame[FRM_POS(axis)]);
  uint16_t fg = dro->linkup ? NVDRO_BLACK : NVDRO_DIM;
  char num[16];
  char str[VALUE_CHARS + 4];
  bool neg = v < 0;

  if (neg)
    {
      v = -v;
    }

  snprintf(num, sizeof(num), "%s%ld.%04ld", neg ? "-" : "",
           (long)(v / 10000), (long)(v % 10000));
  snprintf(str, sizeof(str), "%*s", VALUE_CHARS, num);

  nvdro_text(VALUE_X + 1, ROW_Y(row) + 11, str, 2, 1, fg, NVDRO_PANEL);
}

static void draw_override(FAR struct nvdro_s *dro, int row)
{
  bool sel = dro->frame[FRM_OVR_SELECT] == row + 1;
  char str[8];

  snprintf(str, sizeof(str), "%3u", dro->frame[FRM_OVERRIDE(row)]);
  nvdro_text(OVR_X + 3, ROW_Y(row) + 14, str, 2, 1,
             NVDRO_BLACK, NVDRO_BOX);
  nvdro_fill(OVR_X + 4, ROW_Y(row) + ROW_H - 4, OVR_W - 8, 3,
             sel ? NVDRO_GREEN : NVDRO_BOX);
}

static void draw_bottom(FAR struct nvdro_s *dro)
{
  uint8_t step;
  int32_t aux;
  char str[8];

  if (field_changed(dro, FRM_STEP, 1))
    {
      step = dro->frame[FRM_STEP];
      nvdro_text(7, BOTTOM_Y + 12, step < 4 ? g_stepname[step] : "  ?  ",
                 2, 1, NVDRO_BLACK, NVDRO_BOX);
    }

  if (field_changed(dro, FRM_RESET_LAMP, 1))
    {
      nvdro_fill(72, BOTTOM_Y + 14, 34, 12,
                 dro->frame[FRM_RESET_LAMP] ? NVDRO_RED : NVDRO_GREEN);
    }

  if (field_changed(dro, FRM_COMM_LAMP, 1))
    {
      nvdro_fill(116, BOTTOM_Y + 14, 34, 12,
                 dro->frame[FRM_COMM_LAMP] ? NVDRO_YELLOW : NVDRO_LAMP_OFF);
    }

  if (field_changed(dro, FRM_AUX_VALUE, 4))
    {
      aux = get_le32(&dro->frame[FRM_AUX_VALUE]);
      if (aux > 99999)
        {
          aux = 99999;
        }
      else if (aux < -9999)
        {
          aux = -9999;
        }

      snprintf(str, sizeof(str), "%5ld", (long)aux);
      nvdro_text(158, BOTTOM_Y + 12, str, 2, 1, NVDRO_BLACK, NVDRO_BOX);
    }
}

/* Redraw only what changed since the last call, like the original */

static void ui_update(FAR struct nvdro_s *dro)
{
  uint8_t sel = dro->frame[FRM_AXIS_SELECT];
  int page = (sel >= NVDRO_NROWS && sel < NVDRO_NAXES) ? 1 : 0;
  bool axischanged;
  bool posforce;
  int r;

  if (page != dro->page)
    {
      dro->page = page;
      dro->redraw = true;
    }

  if (dro->redraw)
    {
      draw_static();
    }

  axischanged = field_changed(dro, FRM_AXIS_SELECT, 1);
  posforce = dro->redraw || dro->linkup != dro->shown_linkup;

  for (r = 0; r < NVDRO_NROWS; r++)
    {
      int axis = page * NVDRO_NROWS + r;

      if (axischanged)
        {
          draw_axis_letter(dro, r);
        }

      if (posforce || field_changed(dro, FRM_POS(axis), 4))
        {
          draw_position(dro, r);
        }

      if (field_changed(dro, FRM_OVERRIDE(r), 1) ||
          field_changed(dro, FRM_OVR_SELECT, 1))
        {
          draw_override(dro, r);
        }
    }

  draw_bottom(dro);

  memcpy(dro->shown, dro->frame, NVDRO_FRAME_LEN);
  dro->shown_linkup = dro->linkup;
  dro->redraw = false;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct nvdro_s *dro = &g_dro;
  pthread_attr_t attr;
  pthread_t rxtid;
  uint32_t seq = 0;
  uint32_t lastrx;
  int ret;

  memset(dro, 0, sizeof(*dro));
  pthread_mutex_init(&dro->lock, NULL);

  ret = nvdro_lcd_open(CONFIG_INDUSTRY_CNC_NVMPG_DRO_LCD);
  if (ret < 0)
    {
      syslog(LOG_ERR, "nvmpg_dro: cannot open %s: %d\n",
              CONFIG_INDUSTRY_CNC_NVMPG_DRO_LCD, ret);
      return EXIT_FAILURE;
    }

  ret = tty_open(dro);
  if (ret < 0)
    {
      syslog(LOG_ERR, "nvmpg_dro: cannot open %s: %d\n",
              CONFIG_INDUSTRY_CNC_NVMPG_DRO_TTY, ret);
      nvdro_lcd_close();
      return EXIT_FAILURE;
    }

#ifdef CONFIG_INDUSTRY_CNC_NVMPG_DRO_SPLASH
  nvdro_logo();
  usleep(CONFIG_INDUSTRY_CNC_NVMPG_DRO_SPLASH_MS * 1000);
#endif

  keys_open(dro);

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 1024);
  ret = pthread_create(&rxtid, &attr, rx_thread, dro);
  if (ret != 0)
    {
      syslog(LOG_ERR, "nvmpg_dro: pthread_create failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* No axis selected and link down until the first frame arrives */

  dro->frame[FRM_AXIS_SELECT] = 0xff;
  dro->redraw = true;
  ui_update(dro);
  lastrx = now_ms();

  for (; ; )
    {
      bool newframe = false;

      keys_poll(dro);

      pthread_mutex_lock(&dro->lock);
      if (dro->rxseq != seq)
        {
          seq = dro->rxseq;
          memcpy(dro->frame, dro->rxframe, NVDRO_FRAME_LEN);
          newframe = true;
        }

      pthread_mutex_unlock(&dro->lock);

      if (newframe)
        {
          lastrx = now_ms();
          dro->linkup = true;
        }
      else if (dro->linkup && now_ms() - lastrx > NVDRO_LINK_TIMEOUT)
        {
          dro->linkup = false;
        }

      if (newframe || dro->linkup != dro->shown_linkup)
        {
          ui_update(dro);
        }

      usleep(NVDRO_POLL_MS * 1000);
    }

  return EXIT_SUCCESS;
}
