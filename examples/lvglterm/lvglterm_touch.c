/****************************************************************************
 * apps/examples/lvglterm/lvglterm_touch.c
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

/* Touch input variant of the LVGL terminal: an on-screen LVGL keyboard feeds
 * an input text area; pressing Enter sends the line to NSH stdin.  This is
 * the original PinePhone behaviour and suits touchscreen boards.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <unistd.h>
#include <string.h>
#include <nuttx/debug.h>
#include <lvgl/lvgl.h>

#include "lvglterm.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void input_callback(lv_event_t *e);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* LVGL Text Area for NSH Input and the on-screen Keyboard Widget */

static lv_obj_t *g_input;
static lv_obj_t *g_kb;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: input_callback
 *
 * Description:
 *   Callback for the NSH input text area.  When the on-screen keyboard Enter
 *   key is pressed, send the input command to NSH stdin.
 *
 ****************************************************************************/

static void input_callback(lv_event_t *e)
{
  int ret;
  const lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_VALUE_CHANGED)
    {
      const uint16_t id = lv_keyboard_get_selected_button(g_kb);
      const char *key = lv_keyboard_get_button_text(g_kb, id);

      if (key == NULL)
        {
          return;
        }

      /* If the key pressed is Enter, send the command to NSH stdin */

      if (key[0] == 0xef && key[1] == 0xa2 && key[2] == 0xa2)
        {
          const char *cmd;
          int len;

          DEBUGASSERT(g_input != NULL);
          cmd = lv_textarea_get_text(g_input);
          if (cmd == NULL || cmd[0] == 0)
            {
              return;
            }

          /* Echo the command on the output so the prompt line reads
           * "nsh> <command>", then send it to the shell.
           */

          len = strlen(cmd);
          lvglterm_add_output(cmd, len);
          if (cmd[len - 1] != '\n')
            {
              lvglterm_add_output("\n", 1);
            }

          DEBUGASSERT(g_nsh_stdin[WRITE_PIPE] != 0);
          ret = write(g_nsh_stdin[WRITE_PIPE], cmd, len);
          DEBUGASSERT(ret == len);

          lv_textarea_set_text(g_input, "");
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lvglterm_input_create
 *
 * Description:
 *   Create the on-screen keyboard and input text area under the shared
 *   column container.
 *
 ****************************************************************************/

void lvglterm_input_create(int argc, FAR char *argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  /* Create an LVGL Text Area Widget for NSH Input */

  g_input = lv_textarea_create(g_col);
  DEBUGASSERT(g_input != NULL);
  lv_obj_add_style(g_input, &g_terminal_style, 0);
  lv_obj_set_size(g_input, LV_PCT(100), LV_SIZE_CONTENT);

  /* Create an LVGL Keyboard Widget */

  g_kb = lv_keyboard_create(g_col);
  DEBUGASSERT(g_kb != NULL);
  lv_obj_set_style_pad_all(g_kb, 0, 0);

  /* Wire the keyboard to the input text area */

  lv_obj_add_event_cb(g_input, input_callback, LV_EVENT_ALL, NULL);
  lv_keyboard_set_textarea(g_kb, g_input);
}

/****************************************************************************
 * Name: lvglterm_input_poll
 *
 * Description:
 *   No periodic work is needed for the touch variant.
 *
 ****************************************************************************/

void lvglterm_input_poll(void)
{
}
