/****************************************************************************
 * examples/lvgdemo/lvgldemo.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <sys/boardctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#ifndef CONFIG_VIDEO_FB
#include <nuttx/lcd/lcd.h>  // lcd_dev_s
#endif

#include <lvgl/lvgl.h>

#include "fbdev.h"
#include "tp.h"
#include "tp_cal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Should we perform board-specific driver initialization?  There are two
 * ways that board initialization can occur:  1) automatically via
 * board_late_initialize() during bootupif CONFIG_BOARD_LATE_INITIALIZE
 * or 2).
 * via a call to boardctl() if the interface is enabled
 * (CONFIG_LIB_BOARDCTL=y).
 * If this task is running as an NSH built-in application, then that
 * initialization has probably already been performed otherwise we do it
 * here.
 */

#undef NEED_BOARDINIT

#if defined(CONFIG_LIB_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define NEED_BOARDINIT 1
#endif

#define DISPLAY_BUFFER_SIZE (CONFIG_LV_HOR_RES * \
                              CONFIG_EXAMPLES_LVGLDEMO_BUFF_SIZE)

/****************************************************************************
 * Public Functions Prototypes
 ****************************************************************************/

void lv_demo_benchmark(void);
void lv_demo_printer(void);
void lv_demo_stress(void);
void lv_demo_widgets(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tick_func
 *
 * Description:
 *   Calls lv_tick_inc(...) every 5ms.
 *
 * Input Parameters:
 *   data
 *
 * Returned Value:
 *   NULL
 *
 ****************************************************************************/

static FAR void *tick_func(void *data)
{
  static long last_ms;
  long    ms;
  struct timespec spec;

  while (1)
    {
      long diff;

      /* Calculate how much time elapsed */

      clock_gettime(CLOCK_REALTIME, &spec);
      ms = (long)spec.tv_nsec / 1000000;
      diff = ms - last_ms;

      /* Handle overflow */

      if (diff < 0)
        {
          diff = 1000 + diff;
        }

      lv_tick_inc(diff);
      usleep(5000);

      last_ms = ms;
    }

  /* Never will reach here */

  return NULL;
}

#ifndef CONFIG_VIDEO_FB

static struct lcd_planeinfo_s pinfo;

static void area_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  for (int32_t y = area->y1; y <= area->y2; y++) {
    for (int32_t x = area->x1; x <= area->x2; x++) {
      pinfo.putrun(y, x, color_p, 2);
      color_p++;
    }
  }

  lv_disp_flush_ready(disp); /* Indicate you are ready with the flushing*/
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main or lvgldemo_main
 *
 * Description:
 *
 * Input Parameters:
 *   Standard argc and argv
 *
 * Returned Value:
 *   Zero on success; a positive, non-zero value on failure.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_disp_drv_t disp_drv;
  pthread_t tick_thread;

  lv_disp_buf_t disp_buf;
  static lv_color_t buf[DISPLAY_BUFFER_SIZE];

#ifndef CONFIG_EXAMPLES_LVGLDEMO_CALIBRATE
  lv_point_t p[4];

  /* top left */

  p[0].x = 0;
  p[0].y = 0;

  /* top right */

  p[1].x = LV_HOR_RES;
  p[1].y = 0;

  /* bottom left */

  p[2].x = 0;
  p[2].y = LV_VER_RES;

  /* bottom right */

  p[3].x = LV_HOR_RES;
  p[3].y = LV_VER_RES;
#endif

#ifdef NEED_BOARDINIT
  /* Perform board-specific driver initialization */

  boardctl(BOARDIOC_INIT, 0);

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif
#endif

  /* LittlevGL initialization */

  lv_init();

  /* Display interface initialization */

#ifdef CONFIG_VIDEO_FB
  fbdev_init();
#else
  struct lcd_dev_s *dev;
  int ret;

  ret = board_lcd_initialize();
  if (ret < 0)
    {
      gerr("ERROR: board_lcd_initialize failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* Get the device instance */

  dev = board_lcd_getdev(0);
  if (!dev)
    {
      gerr("ERROR: board_lcd_getdev failed, devno=%d\n", 0);
      return EXIT_FAILURE;
    }
  
  dev->getplaneinfo(dev, 0, &pinfo);

  /* Turn the LCD on at 75% power */

  int power = ((3 * CONFIG_LCD_MAXPOWER + 3) / 4);

  dev->setpower(dev, power);
#endif

  /* Basic LittlevGL display driver initialization */

  lv_disp_buf_init(&disp_buf, buf, NULL, DISPLAY_BUFFER_SIZE);
  lv_disp_drv_init(&disp_drv);
#ifdef CONFIG_VIDEO_FB
  disp_drv.flush_cb = fbdev_flush;
#else
  disp_drv.flush_cb = area_flush;
#endif
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  /* Tick interface initialization */

  pthread_create(&tick_thread, NULL, tick_func, NULL);

  /* Touchpad Initialization */

  tp_init();
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;

  /* This function will be called periodically (by the library) to get the
   * mouse position and state.
   */

  indev_drv.read_cb = tp_read;
  lv_indev_drv_register(&indev_drv);

#if defined(CONFIG_EXAMPLES_LVGLDEMO_BENCHMARK)
  lv_demo_benchmark();
#elif defined(CONFIG_EXAMPLES_LVGLDEMO_PRINTER)
  lv_demo_printer();
#elif defined(CONFIG_EXAMPLES_LVGLDEMO_STRESS)
  lv_demo_stress();
#elif defined(CONFIG_EXAMPLES_LVGLDEMO_WIDGETS)
  lv_demo_widgets();
#endif

  /* Start TP calibration */

#ifdef CONFIG_EXAMPLES_LVGLDEMO_CALIBRATE
  tp_cal_create();
#else
  tp_set_cal_values(p, p + 1, p + 2, p + 3);
#endif
  /* Handle LVGL tasks */

  while (1)
    {
      lv_task_handler();
      usleep(10000);
    }

  return EXIT_SUCCESS;
}
