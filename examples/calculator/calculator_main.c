/****************************************************************************
 * apps/examples/calculator/calculator_main.c
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/boardctl.h>

#include <lvgl/lvgl.h>

#include <system/nxstore_chrome.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef NEED_BOARDINIT

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define NEED_BOARDINIT 1
#endif

#define CALC_DISPLAY_MAX 16

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Set only by quit_signal_handler(), an async-signal-safe flag - the
 * actual shutdown happens from the main loop, a known-safe point,
 * rather than from the handler itself.  See games/NXDoom's
 * i_install_quit_signal()/i_poll_quit_signal() for the same pattern:
 * an external supervisor (nxstore) has no in-app quit path to drive,
 * so it can only ask this process to exit from the outside via
 * SIGTERM.
 */

static volatile sig_atomic_t g_quit_requested;

static lv_obj_t *g_display;
static char g_display_buf[CALC_DISPLAY_MAX + 1] = "0";
static double g_accumulator;
static char g_pending_op;
static bool g_start_new_entry = true;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void quit_signal_handler(int signo)
{
  (void)signo;
  g_quit_requested = 1;
}

/* nxstore's own "Close" bar is confined to the top 36px of the display
 * so it can coexist with full-screen children like this one - a
 * convention that only works if the child app leaves that strip alone.
 * This app's display/keypad fill the entire canvas, so nxstore's bar
 * gets overdrawn as soon as this screen loads.  Give the calculator its
 * own close affordance instead: tapping it just sets the same quit flag
 * SIGTERM does, so nxstore's nxstore_poll_running_app() sees the process
 * exit on its own and returns to the app list automatically.
 */

static void close_event_cb(lv_event_t *e)
{
  UNUSED(e);
  g_quit_requested = 1;
}

static void calc_update_display(void)
{
  lv_label_set_text(g_display, g_display_buf);
}

static void calc_set_display_number(double value)
{
  snprintf(g_display_buf, sizeof(g_display_buf), "%.10g", value);
  calc_update_display();
}

static double calc_apply(double a, double b, char op, bool *ok)
{
  *ok = true;

  switch (op)
    {
      case '+':
        return a + b;

      case '-':
        return a - b;

      case '*':
        return a * b;

      case '/':
        if (b == 0)
          {
            *ok = false;
            return 0;
          }

        return a / b;

      default:
        return b;
    }
}

static void calc_clear(void)
{
  g_accumulator = 0;
  g_pending_op = 0;
  g_start_new_entry = true;
  snprintf(g_display_buf, sizeof(g_display_buf), "0");
  calc_update_display();
}

static void calc_handle_digit(char digit)
{
  size_t len;

  if (g_start_new_entry)
    {
      g_display_buf[0] = '\0';
      g_start_new_entry = false;
    }

  if (strcmp(g_display_buf, "0") == 0)
    {
      g_display_buf[0] = '\0';
    }

  len = strlen(g_display_buf);
  if (len + 1 >= sizeof(g_display_buf))
    {
      /* Display is already at capacity - drop the keypress rather than
       * overflow g_display_buf or silently truncate mid-number.
       */

      return;
    }

  g_display_buf[len] = digit;
  g_display_buf[len + 1] = '\0';
  calc_update_display();
}

static void calc_handle_point(void)
{
  size_t len;

  if (g_start_new_entry)
    {
      snprintf(g_display_buf, sizeof(g_display_buf), "0");
      g_start_new_entry = false;
    }

  if (strchr(g_display_buf, '.') != NULL)
    {
      return;
    }

  len = strlen(g_display_buf);
  if (len + 1 >= sizeof(g_display_buf))
    {
      return;
    }

  g_display_buf[len] = '.';
  g_display_buf[len + 1] = '\0';
  calc_update_display();
}

static void calc_handle_op(char op)
{
  double current = atof(g_display_buf);
  bool ok;

  if (g_pending_op != 0)
    {
      g_accumulator = calc_apply(g_accumulator, current, g_pending_op, &ok);
      if (!ok)
        {
          snprintf(g_display_buf, sizeof(g_display_buf), "Error");
          calc_update_display();
          g_pending_op = 0;
          g_start_new_entry = true;
          return;
        }
    }
  else
    {
      g_accumulator = current;
    }

  g_pending_op = op;
  g_start_new_entry = true;
  calc_set_display_number(g_accumulator);
}

static void calc_handle_equals(void)
{
  double current = atof(g_display_buf);
  bool ok;

  if (g_pending_op == 0)
    {
      g_accumulator = current;
    }
  else
    {
      g_accumulator = calc_apply(g_accumulator, current, g_pending_op, &ok);
      if (!ok)
        {
          snprintf(g_display_buf, sizeof(g_display_buf), "Error");
          calc_update_display();
          g_pending_op = 0;
          g_start_new_entry = true;
          return;
        }
    }

  g_pending_op = 0;
  g_start_new_entry = true;
  calc_set_display_number(g_accumulator);
}

static void button_event_cb(lv_event_t *e)
{
  FAR const char *code = lv_event_get_user_data(e);

  switch (code[0])
    {
      case 'C':
        calc_clear();
        break;

      case '.':
        calc_handle_point();
        break;

      case '=':
        calc_handle_equals();
        break;

      case '+':
      case '-':
      case '*':
      case '/':
        calc_handle_op(code[0]);
        break;

      default:
        calc_handle_digit(code[0]);
        break;
    }
}

static lv_obj_t *make_button(lv_obj_t *parent, FAR const char *label,
                             FAR const char *code, uint32_t color)
{
  lv_obj_t *btn;
  lv_obj_t *btn_label;

  btn = lv_obj_create(parent);
  lv_obj_set_style_radius(btn, 10, 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0xffffff),
                            LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(btn, LV_OPA_30, LV_STATE_PRESSED);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

  btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, label);
  lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);
  lv_obj_center(btn_label);

  lv_obj_add_event_cb(btn, button_event_cb, LV_EVENT_CLICKED,
                      (void *)code);

  return btn;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  lv_obj_t *screen;
  lv_obj_t *grid;
  lv_obj_t *close_btn;
  lv_obj_t *close_label;
  struct sigaction sa;
  int32_t display_height;
  int32_t display_width;
  static const char *const keys[5][4] =
  {
    { "7", "8", "9", "/" },
    { "4", "5", "6", "*" },
    { "1", "2", "3", "-" },
    { "C", "0", ".", "+" },
    { "=", "=", "=", "=" },
  };

  static const int32_t col_dsc[] =
  {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };

  static const int32_t row_dsc[] =
  {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
  };

  g_quit_requested = 0;
  g_display_buf[0] = '0';
  g_display_buf[1] = '\0';
  g_accumulator = 0;
  g_pending_op = 0;
  g_start_new_entry = true;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = quit_signal_handler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGTERM, &sa, NULL) < 0)
    {
      LV_LOG_ERROR("calculator: failed to install SIGTERM handler");
      return 1;
    }

  if (lv_is_initialized())
    {
      LV_LOG_ERROR("LVGL already initialized! aborting.");
      return -1;
    }

#ifdef NEED_BOARDINIT
  boardctl(BOARDIOC_INIT, 0);
#endif

  lv_init();

  lv_nuttx_dsc_init(&info);
  info.fb_path = CONFIG_EXAMPLES_CALCULATOR_FBDEVPATH;
  info.input_path = CONFIG_EXAMPLES_CALCULATOR_INPUT_DEVPATH;

  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      LV_LOG_ERROR("calculator: LVGL display init failed!");
      lv_nuttx_deinit(&result);
      lv_deinit();
      return 1;
    }

  if (result.indev == NULL)
    {
      LV_LOG_ERROR("calculator: touchscreen init failed!");
      lv_nuttx_deinit(&result);
      lv_deinit();
      return 1;
    }

  display_width = lv_display_get_horizontal_resolution(result.disp);
  display_height = lv_display_get_vertical_resolution(result.disp);
  if (display_width <= 0 || display_height <= NXSTORE_BAR_HEIGHT)
    {
      LV_LOG_ERROR("calculator: display is too small");
      lv_nuttx_deinit(&result);
      lv_deinit();
      return 1;
    }

  screen = lv_obj_create(NULL);
  lv_obj_set_size(screen, display_width,
                  display_height - NXSTORE_BAR_HEIGHT);
  lv_obj_set_pos(screen, 0, NXSTORE_BAR_HEIGHT);
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x101827), 0);
  lv_obj_set_style_border_width(screen, 0, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  g_display = lv_label_create(screen);
  lv_obj_set_width(g_display, LV_PCT(90));
  lv_obj_set_style_text_font(g_display, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(g_display, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(g_display, LV_TEXT_ALIGN_RIGHT, 0);
  lv_label_set_text(g_display, g_display_buf);
  lv_obj_align(g_display, LV_ALIGN_TOP_MID, 0, 48);

  close_btn = lv_obj_create(screen);
  lv_obj_set_size(close_btn, 36, 36);
  lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -8, 8);
  lv_obj_set_style_radius(close_btn, 8, 0);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xf0554c), 0);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xb03830),
                            LV_STATE_PRESSED);
  lv_obj_set_style_border_width(close_btn, 0, 0);
  lv_obj_clear_flag(close_btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(close_btn, LV_OBJ_FLAG_CLICKABLE);

  close_label = lv_label_create(close_btn);
  lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(close_label, lv_color_hex(0xffffff), 0);
  lv_obj_clear_flag(close_label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_center(close_label);

  lv_obj_add_event_cb(close_btn, close_event_cb, LV_EVENT_CLICKED, NULL);

  grid = lv_obj_create(screen);
  lv_obj_set_size(grid, LV_PCT(90), LV_PCT(70));
  lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -16);
  lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(grid, 0, 0);
  lv_obj_set_style_pad_all(grid, 4, 0);
  lv_obj_set_style_pad_gap(grid, 8, 0);
  lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
  lv_obj_set_layout(grid, LV_LAYOUT_GRID);

  for (int row = 0; row < 5; row++)
    {
      for (int col = 0; col < 4; col++)
        {
          FAR const char *code = keys[row][col];
          uint32_t color = 0x2a3648;
          lv_obj_t *btn;

          if (row == 4)
            {
              /* Single wide "=" button spans the whole row - only
               * instantiate it once, on the first column.
               */

              if (col > 0)
                {
                  continue;
                }
            }

          if (strchr("+-*/=", code[0]) != NULL)
            {
              color = 0x3d8bff;
            }
          else if (code[0] == 'C')
            {
              color = 0xf0554c;
            }

          btn = make_button(grid, code, code, color);
          if (row == 4)
            {
              lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 0, 4,
                                   LV_GRID_ALIGN_STRETCH, row, 1);
            }
          else
            {
              lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1,
                                   LV_GRID_ALIGN_STRETCH, row, 1);
            }
        }
    }

  lv_screen_load(screen);

  while (!g_quit_requested)
    {
      uint32_t idle;

      idle = lv_timer_handler();

      /* Minimum sleep of 1ms */

      idle = idle ? idle : 1;
      usleep(idle * 1000);
    }

  lv_nuttx_deinit(&result);
  lv_deinit();

  return 0;
}
