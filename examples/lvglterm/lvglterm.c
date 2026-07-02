/****************************************************************************
 * apps/examples/lvglterm/lvglterm.c
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

/* Reference:
 * "NuttX RTOS for PinePhone: LVGL Terminal for NSH Shell"
 * https://lupyuen.github.io/articles/terminal
 *
 * Code shared by both input variants: it starts the NSH shell with its
 * standard streams redirected through pipes, renders the shell output in an
 * LVGL text area, and delegates the input source to the selected variant
 * (on-screen keyboard in lvglterm_touch.c, physical keyboard in
 * lvglterm_kbd.c).
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/boardctl.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <nuttx/debug.h>
#include <poll.h>
#include <spawn.h>
#include <lvgl/lvgl.h>

#include "lvglterm.h"

/* NSH Task requires posix_spawn() */

#ifndef CONFIG_LIBC_EXECFUNCS
#  error posix_spawn() should be enabled in the configuration
#endif

/* NSH Redirection requires Pipes */

#ifndef CONFIG_DEV_PIPE_SIZE
#  error FIFO and Named Pipe Drivers should be enabled in the configuration
#endif

/* NSH Output requires a Monospaced Font.  The size is selectable so that
 * low-resolution displays can use the smaller UNSCII 8.
 */

#if defined(CONFIG_EXAMPLES_LVGLTERM_FONT_UNSCII_8)
#  ifndef CONFIG_LV_FONT_UNSCII_8
#    error LVGL Font UNSCII 8 should be enabled in the configuration
#  endif
#  define LVGLTERM_FONT lv_font_unscii_8
#else
#  ifndef CONFIG_LV_FONT_UNSCII_16
#    error LVGL Font UNSCII 16 should be enabled in the configuration
#  endif
#  define LVGLTERM_FONT lv_font_unscii_16
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* How often to poll for output from NSH Shell (milliseconds) */

#define TIMER_PERIOD_MS 20

/* Trim the output text area once it grows past this many characters */

#define TERM_MAXCHARS  4096
#define TERM_KEEPCHARS 3072

/* NSH Task to be started */

#define NSH_TASK "nsh"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int create_widgets(void);
static void timer_callback(lv_timer_t *timer);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Pipe to NSH stdin, written by the selected input variant */

int g_nsh_stdin[2];

/* LVGL Column Container and NSH Output Text Area (shared with the variant) */

lv_obj_t *g_col;
lv_obj_t *g_output;

/* LVGL Font Style for NSH Input and Output */

lv_style_t g_terminal_style;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Pipes for NSH stdout and stderr */

static int g_nsh_stdout[2];
static int g_nsh_stderr[2];

/* LVGL Timer for polling NSH Output */

static lv_timer_t *g_timer;

/* Arguments for NSH Task */

static char * const g_nsh_argv[] =
{
  NSH_TASK, NULL
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lvglterm_has_input
 *
 * Description:
 *   Return true if the file descriptor has data to be read.
 *
 ****************************************************************************/

bool lvglterm_has_input(int fd)
{
  struct pollfd fdp;

  fdp.fd     = fd;
  fdp.events = POLLIN;
  return poll(&fdp, 1, 0) > 0 && (fdp.revents & POLLIN) != 0;
}

/****************************************************************************
 * Name: lvglterm_add_output
 *
 * Description:
 *   Append text to the NSH output text area, dropping VT100 escape sequences
 *   and carriage returns, honouring backspace, and trimming the buffer once
 *   it grows too large.  Must run in the LVGL thread.
 *
 ****************************************************************************/

void lvglterm_add_output(FAR const char *buf, int len)
{
  char clean[64];
  int  ci = 0;
  int  i;

  for (i = 0; i < len; i++)
    {
      char c = buf[i];

      if (c == 0x1b)                /* ESC: skip the escape sequence */
        {
          i++;
          if (i < len && buf[i] == '[')
            {
              for (i++; i < len; i++)
                {
                  if ((buf[i] >= 'A' && buf[i] <= 'Z') ||
                      (buf[i] >= 'a' && buf[i] <= 'z'))
                    {
                      break;
                    }
                }
            }

          continue;
        }

      if (c == '\r')                /* Drop carriage return */
        {
          continue;
        }

      if (c == 0x08 || c == 0x7f)   /* Backspace/DEL: erase last character */
        {
          if (ci > 0)
            {
              clean[ci] = '\0';
              lv_textarea_add_text(g_output, clean);
              ci = 0;
            }

          lv_textarea_delete_char(g_output);
          continue;
        }

      if (c == '\n' || c == '\t' || (c >= 0x20 && c < 0x7f))
        {
          clean[ci++] = c;
          if (ci >= (int)sizeof(clean) - 1)
            {
              clean[ci] = '\0';
              lv_textarea_add_text(g_output, clean);
              ci = 0;
            }
        }
    }

  if (ci > 0)
    {
      clean[ci] = '\0';
      lv_textarea_add_text(g_output, clean);
    }

  /* Trim the text area if it has grown too large */

  if ((int)strlen(lv_textarea_get_text(g_output)) > TERM_MAXCHARS)
    {
      FAR const char *txt = lv_textarea_get_text(g_output);
      int total = strlen(txt);
      lv_textarea_set_text(g_output, txt + (total - TERM_KEEPCHARS));
      lv_textarea_set_cursor_pos(g_output, LV_TEXTAREA_CURSOR_LAST);
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: create_widgets
 *
 * Description:
 *   Create the shared LVGL widgets: a column container and the NSH output
 *   text area.  The input source is added by the selected variant.
 *
 ****************************************************************************/

static int create_widgets(void)
{
  /* Set the Font Style for NSH Input and Output to a Monospaced Font */

  lv_style_init(&g_terminal_style);
  lv_style_set_text_font(&g_terminal_style, &LVGLTERM_FONT);

  /* Create an LVGL Container with Column Flex Direction */

  g_col = lv_obj_create(lv_scr_act());
  DEBUGASSERT(g_col != NULL);
  lv_obj_set_size(g_col, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(g_col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(g_col, 0, 0);

  /* Create an LVGL Text Area Widget for NSH Output */

  g_output = lv_textarea_create(g_col);
  DEBUGASSERT(g_output != NULL);
  lv_obj_add_style(g_output, &g_terminal_style, 0);
  lv_obj_set_width(g_output, LV_PCT(100));
  lv_obj_set_flex_grow(g_output, 1);

  return OK;
}

/****************************************************************************
 * Name: create_terminal
 *
 * Description:
 *   Start the NSH shell with its streams redirected to pipes, create the
 *   shared widgets and the output-polling timer, and set up the input
 *   variant.
 *
 ****************************************************************************/

static int create_terminal(int argc, FAR char *argv[])
{
  int ret;
  pid_t pid;

  /* Create the pipes for NSH Shell: stdin, stdout and stderr */

  if (pipe(g_nsh_stdin) < 0 || pipe(g_nsh_stdout) < 0 ||
      pipe(g_nsh_stderr) < 0)
    {
      _err("pipe failed: %d\n", errno);
      return ERROR;
    }

  /* Close default stdin, stdout and stderr and assign the new pipes */

  close(0);
  close(1);
  close(2);

  dup2(g_nsh_stdin[READ_PIPE], 0);
  dup2(g_nsh_stdout[WRITE_PIPE], 1);
  dup2(g_nsh_stderr[WRITE_PIPE], 2);

  /* Start the NSH Shell and inherit stdin, stdout and stderr */

  ret = posix_spawn(&pid, NSH_TASK, NULL, NULL, g_nsh_argv, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      _err("posix_spawn failed: %d\n", errcode);
      return -errcode;
    }

  /* Create an LVGL Timer to poll for output from NSH Shell */

  g_timer = lv_timer_create(timer_callback, TIMER_PERIOD_MS, NULL);
  DEBUGASSERT(g_timer != NULL);

  /* Create the shared widgets and the input source for the variant */

  ret = create_widgets();
  if (ret < 0)
    {
      return ret;
    }

  lvglterm_input_create(argc, argv);
  return OK;
}

/****************************************************************************
 * Name: timer_callback
 *
 * Description:
 *   Poll NSH stdout and stderr for output and render it, then let the input
 *   variant perform its periodic work.  Runs in the LVGL thread.
 *
 ****************************************************************************/

static void timer_callback(lv_timer_t *timer)
{
  static char buf[64];
  int ret;

  /* Drain the input variant first (local echo, scroll) so that what the user
   * typed is rendered before the resulting shell output.
   */

  lvglterm_input_poll();

  if (lvglterm_has_input(g_nsh_stdout[READ_PIPE]))
    {
      ret = read(g_nsh_stdout[READ_PIPE], buf, sizeof(buf));
      if (ret > 0)
        {
          lvglterm_add_output(buf, ret);
        }
    }

  if (lvglterm_has_input(g_nsh_stderr[READ_PIPE]))
    {
      ret = read(g_nsh_stderr[READ_PIPE], buf, sizeof(buf));
      if (ret > 0)
        {
          lvglterm_add_output(buf, ret);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main or lvglterm_main
 *
 * Description:
 *   Start an LVGL Terminal that runs interactive commands with NSH.
 *   NSH output is rendered in an LVGL text area; the input comes from an
 *   on-screen keyboard (touch) or a physical keyboard, selected at build
 *   time.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  int ret;

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  uv_loop_t ui_loop;
#endif

#ifdef CONFIG_BOARDCTL_FINALINIT
  boardctl(BOARDIOC_FINALINIT, 0);
#endif

  lv_init();
  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif

  lv_nuttx_init(&info, &result);
  if (result.disp == NULL)
    {
      LV_LOG_ERROR("lv_nuttx_init failure!");
      return 1;
    }

  ret = create_terminal(argc, argv);
  if (ret < 0)
    {
      return EXIT_FAILURE;
    }

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  lv_nuttx_uv_loop(&ui_loop, &result);
#else
  while (1)
    {
      uint32_t idle;
      idle = lv_timer_handler();
      idle = idle ? idle : 1;
      usleep(idle * 1000);
    }
#endif

  return EXIT_SUCCESS;
}
