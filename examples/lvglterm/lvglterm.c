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
 * Code shared by both input variants: it starts the NSH shell on a
 * pseudo-terminal, renders the shell output in an LVGL text area, and
 * delegates the input source to the selected variant (on-screen keyboard in
 * lvglterm_touch.c, physical keyboard in lvglterm_kbd.c).
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/boardctl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <nuttx/debug.h>
#include <poll.h>
#include <pty.h>
#include <spawn.h>
#include <lvgl/lvgl.h>

#include "lvglterm.h"

/* NSH Task requires posix_spawn() */

#ifndef CONFIG_LIBC_EXECFUNCS
#  error posix_spawn() should be enabled in the configuration
#endif

/* The shell runs on a pseudo-terminal rather than on plain pipes because
 * that is what makes its standard streams look like a console.  Interactive
 * programs decide from isatty() whether to echo, to prompt and to
 * line-buffer their output and the terminal driver is what echoes back what
 * the user types.
 */

#ifndef CONFIG_PSEUDOTERM
#  error Pseudo-Terminal (PTY) support should be enabled in the configuration
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

/* How many reads the output poll may perform on a single timer tick */

#define MAX_READS_PER_TICK 16

/* How many keystrokes may wait for the shell to consume them.  A shell line
 * is shorter than this, and the shell truncates the over-long ones anyway.
 */

#define INPUT_QUEUE_SIZE 256

/* Trim the output text area once it grows past this many characters */

#define TERM_MAXCHARS  4096
#define TERM_KEEPCHARS 3072

/* NSH Task to be started */

/* The shell to run.  A name is looked up on PATH, a path is taken as
 * given, which matters where the shell is installed under another name,
 * as it is when NSH is built as the system's init.
 */

#ifdef CONFIG_EXAMPLES_LVGLTERM_SHELL
#  define NSH_TASK CONFIG_EXAMPLES_LVGLTERM_SHELL
#else
#  define NSH_TASK "nsh"
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static bool has_output(void);
static void flush_input(void);
static int create_widgets(void);
static void timer_callback(lv_timer_t *timer);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* LVGL Column Container and NSH Output Text Area (shared with the variant) */

lv_obj_t *g_col;
lv_obj_t *g_output;

/* LVGL Font Style for NSH Input and Output */

lv_style_t g_terminal_style;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* PTY master:  the terminal's end of the shell's console.  Keystrokes are
 * written to it and the shell output is read from it.
 */

static int g_nsh_fd = -1;

/* Keystrokes waiting for room in the terminal (see lvglterm_send_input) */

static char g_input_queue[INPUT_QUEUE_SIZE];
static int g_input_queued;

/* LVGL Timer for polling NSH Output */

static lv_timer_t *g_timer;

/* Arguments for NSH Task */

static char * const g_nsh_argv[] =
{
  NSH_TASK, NULL
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: has_output
 *
 * Description:
 *   Return true if the shell has produced output that can be read without
 *   blocking.
 *
 ****************************************************************************/

static bool has_output(void)
{
  struct pollfd fdp;

  fdp.fd     = g_nsh_fd;
  fdp.events = POLLIN;
  return poll(&fdp, 1, 0) > 0 && (fdp.revents & POLLIN) != 0;
}

/****************************************************************************
 * Name: flush_input
 *
 * Description:
 *   Hand the queued keystrokes to the terminal, but no more of them than it
 *   can take right now:  a write that has to wait for room blocks the LVGL
 *   thread, and that deadlocks the terminal, since the shell stops reading
 *   as soon as the echo it produces has filled the output buffer that only
 *   this thread drains.  Runs in the LVGL thread.
 *
 ****************************************************************************/

static void flush_input(void)
{
  int nwritten;
  int space;

  if (g_input_queued == 0)
    {
      return;
    }

  if (ioctl(g_nsh_fd, FIONSPACE, &space) < 0 || space <= 0)
    {
      return;
    }

  if (space > g_input_queued)
    {
      space = g_input_queued;
    }

  nwritten = write(g_nsh_fd, g_input_queue, space);
  if (nwritten <= 0)
    {
      return;
    }

  g_input_queued -= nwritten;
  memmove(g_input_queue, g_input_queue + nwritten, g_input_queued);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lvglterm_send_input
 *
 * Description:
 *   Send keystrokes to the shell.  Whatever the terminal cannot take yet is
 *   queued and handed over by the terminal's periodic timer.  Must run in
 *   the LVGL thread.
 *
 ****************************************************************************/

void lvglterm_send_input(FAR const char *buf, int len)
{
  int room = INPUT_QUEUE_SIZE - g_input_queued;

  if (len > room)
    {
      /* The queue drains no faster than the shell reads.  Dropping what does
       * not fit keeps the terminal responsive, and a line that long would be
       * truncated by the shell in any case.
       */

      gwarn("WARNING: dropping %d input bytes\n", len - room);
      len = room;
    }

  memcpy(g_input_queue + g_input_queued, buf, len);
  g_input_queued += len;

  flush_input();
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
 *   Start the NSH shell on a pseudo-terminal, create the shared widgets and
 *   the output-polling timer, and set up the input variant.
 *
 ****************************************************************************/

static int create_terminal(int argc, FAR char *argv[])
{
  int ret;
  pid_t pid;
  int slave;

  /* Create the pseudo-terminal.  The slave keeps the driver defaults, ECHO
   * and the \n -> \r\n output translation among them, which is what a
   * console provides and what the shell's line editor expects.
   */

  if (openpty(&g_nsh_fd, &slave, NULL, NULL, NULL) < 0)
    {
      fprintf(stderr, "openpty failed: %d\n", errno);
      return ERROR;
    }

  /* Close default stdin, stdout and stderr and assign the terminal slave,
   * which is the shell's console once it is spawned below.
   */

  close(0);
  close(1);
  close(2);

  dup2(slave, 0);
  dup2(slave, 1);
  dup2(slave, 2);
  close(slave);

  /* Start the NSH Shell and inherit stdin, stdout and stderr */

  ret = posix_spawn(&pid, NSH_TASK, NULL, NULL, g_nsh_argv, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "posix_spawn failed: %d\n", errcode);
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
 *   Poll the terminal for shell output and render it, then let the input
 *   variant perform its periodic work.  Runs in the LVGL thread.
 *
 ****************************************************************************/

static void timer_callback(lv_timer_t *timer)
{
  static char buf[64];
  int reads;
  int ret;

  /* Collect the keystrokes and hand them over first, so that what the user
   * typed is rendered (the terminal echoes it back to us) before the
   * resulting shell output.
   */

  lvglterm_input_poll();
  flush_input();

  /* Drain what the shell has produced.  A writer blocks once it has filled
   * the terminal buffer, so read as long as there is something to read, but
   * keep a cap:  a program producing output faster than the screen can take
   * it must not keep the LVGL thread out of lv_timer_handler().
   */

  for (reads = 0; reads < MAX_READS_PER_TICK; reads++)
    {
      if (!has_output())
        {
          break;
        }

      ret = read(g_nsh_fd, buf, sizeof(buf));
      if (ret <= 0)
        {
          break;
        }

      lvglterm_add_output(buf, ret);
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
