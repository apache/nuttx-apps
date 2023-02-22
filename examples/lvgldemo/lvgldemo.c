/****************************************************************************
 * apps/examples/lvgldemo/lvgldemo.c
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

#include <sys/boardctl.h>
#include <sys/param.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <debug.h>

#include <lvgl/lvgl.h>
#include <port/lv_port.h>
#include <lvgl/demos/lv_demos.h>

#ifdef CONFIG_LIBUV
#  include <uv.h>
#  include <port/lv_port_libuv.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Should we perform board-specific driver initialization?  There are two
 * ways that board initialization can occur:  1) automatically via
 * board_late_initialize() during bootupif CONFIG_BOARD_LATE_INITIALIZE
 * or 2).
 * via a call to boardctl() if the interface is enabled
 * (CONFIG_BOARDCTL=y).
 * If this task is running as an NSH built-in application, then that
 * initialization has probably already been performed otherwise we do it
 * here.
 */

#undef NEED_BOARDINIT

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define NEED_BOARDINIT 1
#endif

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef CODE void (*demo_create_func_t)(void);

struct func_key_pair_s
{
  FAR const char *name;
  demo_create_func_t func;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct func_key_pair_s func_key_pair[] =
{
#ifdef CONFIG_LV_USE_DEMO_WIDGETS
  { "widgets",        lv_demo_widgets        },
#endif

#ifdef CONFIG_LV_USE_DEMO_KEYPAD_AND_ENCODER
  { "keypad_encoder", lv_demo_keypad_encoder },
#endif

#ifdef CONFIG_LV_USE_DEMO_BENCHMARK
  { "benchmark",      lv_demo_benchmark      },
#endif

#ifdef CONFIG_LV_USE_DEMO_STRESS
  { "stress",         lv_demo_stress         },
#endif

#ifdef CONFIG_LV_USE_DEMO_MUSIC
  { "music",          lv_demo_music          },
#endif
  { "", NULL }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(void)
{
  int i;
  const int len = nitems(func_key_pair) - 1;

  if (len == 0)
    {
      printf("lvgldemo: no demo available!\n");
      exit(EXIT_FAILURE);
      return;
    }

  printf("\nUsage: lvgldemo demo_name\n");
  printf("\ndemo_name:\n");

  for (i = 0; i < len; i++)
    {
      printf("  %s\n", func_key_pair[i].name);
    }

  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Name: find_demo_create_func
 ****************************************************************************/

static demo_create_func_t find_demo_create_func(FAR const char *name)
{
  int i;
  const int len = nitems(func_key_pair) - 1;

  for (i = 0; i < len; i++)
    {
      if (strcmp(name, func_key_pair[i].name) == 0)
        {
          return func_key_pair[i].func;
        }
    }

  printf("lvgldemo: '%s' not found.\n", name);
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

int main(int argc, FAR char *argv[])
{
  demo_create_func_t demo_create_func;
  FAR const char *demo = NULL;
  const int func_key_pair_len = nitems(func_key_pair);

#ifdef CONFIG_LIBUV
  uv_loop_t ui_loop;
#endif

  /* If no arguments are specified and only 1 demo exists, select the demo */

  if (argc == 1 && func_key_pair_len == 2)  /* 2 because of NULL sentinel */
    {
      demo = func_key_pair[0].name;
    }
  else if (argc != 2)
    {
      show_usage();
      return EXIT_FAILURE;
    }
  else
    {
      demo = argv[1];
    }

  demo_create_func = find_demo_create_func(demo);

  if (demo_create_func == NULL)
    {
      show_usage();
      return EXIT_FAILURE;
    }

#ifdef NEED_BOARDINIT
  /* Perform board-specific driver initialization */

  boardctl(BOARDIOC_INIT, 0);

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif
#endif

  /* LVGL initialization */

  lv_init();

  /* LVGL port initialization */

  lv_port_init();

  /* LVGL demo creation */

  demo_create_func();

  /* Handle LVGL tasks */

#ifdef CONFIG_LIBUV
  uv_loop_init(&ui_loop);
  lv_port_libuv_init(&ui_loop);
  uv_run(&ui_loop, UV_RUN_DEFAULT);
#else
  while (1)
    {
      uint32_t idle;
      idle = lv_timer_handler();

      /* Minimum sleep of 1ms */

      idle = idle ? idle : 1;
      usleep(idle * 1000);
    }
#endif

  return EXIT_SUCCESS;
}
