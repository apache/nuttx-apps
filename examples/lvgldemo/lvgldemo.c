/****************************************************************************
 * examples/lvgdemo/lvgldemo.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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
#include <graphics/lvgl.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "fbdev.h"
#include "tp.h"
#include "demo.h"
#include "tp_cal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tick_func
 *
 * Description:
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

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int lvgldemo_main(int argc, char *argv[])
#endif
{
  lv_init();

  /* DISPLAY INTERFACE INIT */

  fbdev_init();

  /* Basic initialization */

  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.disp_flush = fbdev_flush;
  lv_disp_drv_register(&disp_drv);

  /* TICK INTERFACE INIT */

  pthread_t tick_thread;
  pthread_create(&tick_thread, NULL, tick_func, NULL);

  /* Touchpad Initialization */

  tp_init();
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;

  /* This function will be called periodically (by the library) to get the
   * mouse position and state.
   */

  indev_drv.read_fp = tp_read;
  lv_indev_drv_register(&indev_drv);

  /* Demo initialization */

  demo_init();

  /* Start TP calibration */

  tp_cal_create();

  /* Handle LittlevGL tasks */

  while (1)
    {
      lv_task_handler();
      usleep(10000);
    }

  return EXIT_SUCCESS;
}
