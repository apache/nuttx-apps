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
#include <time.h>
#include <debug.h>
#include <poll.h>
#include <spawn.h>
#include <lvgl/lvgl.h>

/* NSH Task requires posix_spawn() */

#ifndef CONFIG_LIBC_EXECFUNCS
#  error posix_spawn() should be enabled in the configuration
#endif

/* NSH Redirection requires Pipes */

#ifndef CONFIG_DEV_PIPE_SIZE
#  error FIFO and Named Pipe Drivers should be enabled in the configuration
#endif

/* NSH Output requires a Monospaced Font */

#ifndef CONFIG_LV_FONT_UNSCII_16
#  error LVGL Font UNSCII 16 should be enabled in the configuration
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

/* How often to poll for output from NSH Shell (milliseconds) */

#define TIMER_PERIOD_MS 100

/* Read and Write Pipes for NSH stdin, stdout and stderr */

#define READ_PIPE  0
#define WRITE_PIPE 1

/* NSH Task to be started */

#define NSH_TASK "nsh"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int create_widgets(void);
static void timer_callback(lv_timer_t * timer);
static void input_callback(lv_event_t * e);
static bool has_input(int fd);
static void remove_escape_codes(char *buf, int len);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Pipes for NSH Shell: stdin, stdout, stderr */

static int g_nsh_stdin[2];
static int g_nsh_stdout[2];
static int g_nsh_stderr[2];

/* LVGL Column Container for NSH Widgets */

static lv_obj_t *g_col;

/* LVGL Text Area Widgets for NSH Input and Output */

static lv_obj_t *g_input;
static lv_obj_t *g_output;

/* LVGL Keyboard Widget for NSH Terminal */

static lv_obj_t *g_kb;

/* LVGL Font Style for NSH Input and Output */

static lv_style_t g_terminal_style;

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
 * Name: create_terminal
 *
 * Description:
 *   Create the LVGL Terminal. Start the NSH Shell and redirect the NSH
 *   stdin, stdout and stderr to the LVGL Widgets.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value is returned on any failure.
 *
 ****************************************************************************/

static int create_terminal(void)
{
  int ret;
  pid_t pid;

  /* Create the pipes for NSH Shell: stdin, stdout and stderr */

  ret = pipe(g_nsh_stdin);
  if (ret < 0)
    {
      _err("stdin pipe failed: %d\n", errno);
      return ERROR;
    }

  ret = pipe(g_nsh_stdout);
  if (ret < 0)
    {
      _err("stdout pipe failed: %d\n", errno);
      return ERROR;
    }

  ret = pipe(g_nsh_stderr);
  if (ret < 0)
    {
      _err("stderr pipe failed: %d\n", errno);
      return ERROR;
    }

  /* Close default stdin, stdout and stderr */

  close(0);
  close(1);
  close(2);

  /* Assign the new pipes as stdin, stdout and stderr */

  dup2(g_nsh_stdin[READ_PIPE], 0);
  dup2(g_nsh_stdout[WRITE_PIPE], 1);
  dup2(g_nsh_stderr[WRITE_PIPE], 2);

  /* Start the NSH Shell and inherit stdin, stdout and stderr */

  ret = posix_spawn(&pid,        /* Returned Task ID */
                    NSH_TASK,    /* NSH Path */
                    NULL,        /* Inherit stdin, stdout and stderr */
                    NULL,        /* Default spawn attributes */
                    g_nsh_argv,  /* Arguments */
                    NULL);       /* No environment */
  if (ret < 0)
    {
      int errcode = errno;
      _err("posix_spawn failed: %d\n", errcode);
      return -errcode;
    }

  /* Create an LVGL Timer to poll for output from NSH Shell */

  g_timer = lv_timer_create(timer_callback,  /* Callback Function */
                            TIMER_PERIOD_MS, /* Timer Period (millisec) */
                            NULL);           /* Callback Argument */
  DEBUGASSERT(g_timer != NULL);

  /* Create the LVGL Terminal Widgets */

  ret = create_widgets();
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: create_widgets
 *
 * Description:
 *   Create the LVGL Widgets for LVGL Terminal.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value is returned on any failure.
 *
 ****************************************************************************/

static int create_widgets(void)
{
  /* Set the Font Style for NSH Input and Output to a Monospaced Font */

  lv_style_init(&g_terminal_style);
  lv_style_set_text_font(&g_terminal_style, &lv_font_unscii_16);

  /* Create an LVGL Container with Column Flex Direction */

  g_col = lv_obj_create(lv_scr_act());
  DEBUGASSERT(g_col != NULL);
  lv_obj_set_size(g_col, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(g_col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(g_col, 0, 0);  /* No padding */

  /* Create an LVGL Text Area Widget for NSH Output */

  g_output = lv_textarea_create(g_col);
  DEBUGASSERT(g_output != NULL);
  lv_obj_add_style(g_output, &g_terminal_style, 0);
  lv_obj_set_width(g_output, LV_PCT(100));
  lv_obj_set_flex_grow(g_output, 1);  /* Fill the column */

  /* Create an LVGL Text Area Widget for NSH Input */

  g_input = lv_textarea_create(g_col);
  DEBUGASSERT(g_input != NULL);
  lv_obj_add_style(g_input, &g_terminal_style, 0);
  lv_obj_set_size(g_input, LV_PCT(100), LV_SIZE_CONTENT);

  /* Create an LVGL Keyboard Widget */

  g_kb = lv_keyboard_create(g_col);
  DEBUGASSERT(g_kb != NULL);
  lv_obj_set_style_pad_all(g_kb, 0, 0);  /* No padding */

  /* Register the Callback Function for NSH Input */

  lv_obj_add_event_cb(g_input, input_callback, LV_EVENT_ALL, NULL);

  /* Set the Keyboard to populate the NSH Input Text Area */

  lv_keyboard_set_textarea(g_kb, g_input);

  return OK;
}

/****************************************************************************
 * Name: timer_callback
 *
 * Description:
 *   Callback Function for LVGL Timer. Poll NSH stdout and stderr for output
 *   and display the output.
 *
 * Input Parameters:
 *   timer - LVGL Timer for the callback
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void timer_callback(lv_timer_t *timer)
{
  int ret;
  static char buf[64];

  DEBUGASSERT(g_nsh_stdout[READ_PIPE] != 0);
  DEBUGASSERT(g_nsh_stderr[READ_PIPE] != 0);

  /* Poll NSH stdout to check if there's output to be processed */

  if (has_input(g_nsh_stdout[READ_PIPE]))
    {
      /* Read the output from NSH stdout */

      ret = read(g_nsh_stdout[READ_PIPE], buf, sizeof(buf) - 1);
      if (ret > 0)
        {
          /* Add to NSH Output Text Area */

          buf[ret] = 0;
          remove_escape_codes(buf, ret);

          DEBUGASSERT(g_output != NULL);
          lv_textarea_add_text(g_output, buf);
        }
    }

  /* Poll NSH stderr to check if there's output to be processed */

  if (has_input(g_nsh_stderr[READ_PIPE]))
    {
      /* Read the output from NSH stderr */

      ret = read(g_nsh_stderr[READ_PIPE], buf, sizeof(buf) - 1);
      if (ret > 0)
        {
          /* Add to NSH Output Text Area */

          buf[ret] = 0;
          remove_escape_codes(buf, ret);

          DEBUGASSERT(g_output != NULL);
          lv_textarea_add_text(g_output, buf);
        }
    }
}

/****************************************************************************
 * Name: input_callback
 *
 * Description:
 *   Callback Function for NSH Input Text Area. If Enter Key was pressed,
 *   send the NSH Input Command to NSH stdin.
 *
 * Input Parameters:
 *   e - LVGL Event for the callback
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void input_callback(lv_event_t *e)
{
  int ret;

  /* Decode the LVGL Event */

  const lv_event_code_t code = lv_event_get_code(e);

  /* If NSH Input Text Area has changed, get the Key Pressed */

  if (code == LV_EVENT_VALUE_CHANGED)
    {
      /* Get the Button Index of the Keyboard Button Pressed */

      const uint16_t id = lv_keyboard_get_selected_button(g_kb);

      /* Get the Text of the Keyboard Button */

      const char *key = lv_keyboard_get_button_text(g_kb, id);
      if (key == NULL)
        {
          return;
        }

      /* If Key Pressed is Enter, send the Command to NSH stdin */

      if (key[0] == 0xef && key[1] == 0xa2 && key[2] == 0xa2)
        {
          /* Read the NSH Input */

          const char *cmd;
          DEBUGASSERT(g_input != NULL);
          cmd = lv_textarea_get_text(g_input);
          if (cmd == NULL || cmd[0] == 0)
            {
              return;
            }

          /* Send the Command to NSH stdin */

          DEBUGASSERT(g_nsh_stdin[WRITE_PIPE] != 0);
          ret = write(g_nsh_stdin[WRITE_PIPE], cmd, strlen(cmd));
          DEBUGASSERT(ret == strlen(cmd));

          /* Erase the NSH Input */

          lv_textarea_set_text(g_input, "");
        }
    }
}

/****************************************************************************
 * Name: has_input
 *
 * Description:
 *   Return true if the File Descriptor has data to be read.
 *
 * Input Parameters:
 *   fd - File Descriptor to be checked
 *
 * Returned Value:
 *   True if File Descriptor has data to be read; False otherwise
 *
 ****************************************************************************/

static bool has_input(int fd)
{
  int ret;

  /* Poll the File Descriptor for input */

  struct pollfd fdp;
  fdp.fd = fd;
  fdp.events = POLLIN;
  ret = poll(&fdp,  /* File Descriptors */
             1,     /* Number of File Descriptors */
             0);    /* Poll Timeout (Milliseconds) */

  if (ret > 0)
    {
      /* If poll is OK and there is input */

      if ((fdp.revents & POLLIN) != 0)
        {
          /* Report that there is input */

          return true;
        }

      /* Else report no input */

      return false;
    }
  else if (ret == 0)
    {
      /* If timeout, report no input */

      return false;
    }
  else if (ret < 0)
    {
      /* Handle error */

      _err("poll failed: %d, fd=%d\n", ret, fd);
      return false;
    }

  /* Never comes here */

  DEBUGPANIC();
  return false;
}

/****************************************************************************
 * Name: remove_escape_codes
 *
 * Description:
 *   Remove ANSI Escape Codes from the string. Assumes that buf[len] is 0.
 *
 * Input Parameters:
 *   buf - String to be processed
 *   len - Length of string
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void remove_escape_codes(char *buf, int len)
{
  int i;
  int j;

  for (i = 0; i < len; i++)
    {
      /* Escape Code looks like 0x1b 0x5b 0x4b */

      if (buf[i] == 0x1b)
        {
          /* Remove 3 bytes */

          for (j = i; j + 2 < len; j++)
            {
              buf[j] = buf[j + 3];
            }
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
 *   Start an LVGL Terminal that runs interactive commands with NSH Shell.
 *   NSH Commands are entered through an LVGL Keyboard. NSH Output is
 *   rendered in an LVGL Widget.
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
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  int ret;

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  uv_loop_t ui_loop;
#endif

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

  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif

  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      LV_LOG_ERROR("lv_demos initialization failure!");
      return 1;
    }

  /* Create the LVGL Widgets */

  ret = create_terminal();
  if (ret < 0)
    {
      return EXIT_FAILURE;
    }

  /* Handle LVGL tasks */

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  lv_nuttx_uv_loop(&ui_loop, &result);
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
