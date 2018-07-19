/****************************************************************************
 * examples/touchscreen/tp_cal.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gábor Kiss-Vámosi <kisvegabor@gmail.com>
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

#include <stdio.h>

#include <graphics/lvgl.h>
#include "tp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CIRCLE_SIZE      20
#define TP_MAX_VALUE     5000

/****************************************************************************
 * Private Type Definitions
 ****************************************************************************/

enum tp_cal_state_e
{
  TP_CAL_STATE_INIT,
  TP_CAL_STATE_WAIT_TOP_LEFT,
  TP_CAL_STATE_WAIT_TOP_RIGHT,
  TP_CAL_STATE_WAIT_BOTTOM_RIGHT,
  TP_CAL_STATE_WAIT_BOTTOM_LEFT,
  TP_CAL_STATE_WAIT_LEAVE,
  TP_CAL_STATE_READY,
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static lv_res_t btn_click_action(lv_obj_t * scr);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static enum tp_cal_state_e state;
static lv_point_t p[4];
static lv_obj_t *prev_scr;
static lv_obj_t *big_btn;
static lv_obj_t *label_main;
static lv_obj_t *circ_area;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: btn_click_action
 *
 * Description:
 *
 * Input Parameters:
 *   scr
 *
 * Returned Value:
 *   ?
 *
 ****************************************************************************/

static lv_res_t btn_click_action(FAR lv_obj_t *scr)
{
  if (state == TP_CAL_STATE_WAIT_TOP_LEFT)
    {
      lv_indev_t *indev = lv_indev_get_act();
      char buf[64];
      lv_anim_t a;

      lv_indev_get_point(indev, &p[0]);

      sprintf(buf, "x: %d\ny: %d", p[0].x, p[0].y);
      lv_obj_t *label_coord = lv_label_create(lv_scr_act(), NULL);
      lv_label_set_text(label_coord, buf);

      lv_label_set_text(label_main, "Click the circle in\n"
                        "upper right-hand corner");

      lv_obj_set_pos(label_main,
                     (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                     (LV_VER_RES - lv_obj_get_height(label_main)) / 2);

      a.var            = circ_area;
      a.start          = 0;
      a.end            = LV_HOR_RES - CIRCLE_SIZE;
      a.fp             = (lv_anim_fp_t) lv_obj_set_x;
      a.path           = lv_anim_path_linear;
      a.end_cb         = NULL;
      a.act_time       = 0;
      a.time           = 200;
      a.playback       = 0;
      a.playback_pause = 0;
      a.repeat         = 0;
      a.repeat_pause   = 0;
      lv_anim_create(&a);

      a.start          = 0;
      a.end            = 0;
      a.fp             = (lv_anim_fp_t)lv_obj_set_y;
      a.end_cb         = NULL;
      a.time           = 200;
      lv_anim_create(&a);

      state            = TP_CAL_STATE_WAIT_TOP_RIGHT;
    }
  else if (state == TP_CAL_STATE_WAIT_TOP_RIGHT)
    {
      lv_indev_t *indev = lv_indev_get_act();
      char buf[64];
      lv_anim_t a;

      lv_indev_get_point(indev, &p[1]);

      sprintf(buf, "x: %d\ny: %d", p[1].x, p[1].y);
      lv_obj_t *label_coord = lv_label_create(lv_scr_act(), NULL);
      lv_label_set_text(label_coord, buf);
      lv_obj_set_pos(label_coord, LV_HOR_RES - lv_obj_get_width(label_coord),
                     0);

      lv_label_set_text(label_main, "Click the circle in\n"
                        "lower right-hand corner");

      lv_obj_set_pos(label_main,
                     (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                     (LV_VER_RES - lv_obj_get_height(label_main)) / 2);

      a.var            = circ_area;
      a.start          = LV_HOR_RES - CIRCLE_SIZE;
      a.end            = LV_HOR_RES - CIRCLE_SIZE;
      a.fp             = (lv_anim_fp_t)lv_obj_set_x;
      a.path           = lv_anim_path_linear;
      a.end_cb         = NULL;
      a.act_time       = 0;
      a.time           = 200;
      a.playback       = 0;
      a.playback_pause = 0;
      a.repeat         = 0;
      a.repeat_pause   = 0;
      lv_anim_create(&a);

      a.start          = 0;
      a.end            = LV_VER_RES - CIRCLE_SIZE;
      a.fp             = (lv_anim_fp_t) lv_obj_set_y;
      a.end_cb         = NULL;
      a.time           = 200;
      lv_anim_create(&a);

      state            = TP_CAL_STATE_WAIT_BOTTOM_RIGHT;
    }
  else if (state == TP_CAL_STATE_WAIT_BOTTOM_RIGHT)
    {
      lv_indev_t *indev = lv_indev_get_act();
      char buf[64];
      lv_anim_t a;

      lv_indev_get_point(indev, &p[2]);

      sprintf(buf, "x: %d\ny: %d", p[2].x, p[2].y);
      lv_obj_t *label_coord = lv_label_create(lv_scr_act(), NULL);
      lv_label_set_text(label_coord, buf);
      lv_obj_set_pos(label_coord, LV_HOR_RES - lv_obj_get_width(label_coord),
                     LV_VER_RES - lv_obj_get_height(label_coord));

      lv_label_set_text(label_main, "Click the circle in\n"
                        "lower left-hand corner");

      lv_obj_set_pos(label_main,
                     (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                     (LV_VER_RES - lv_obj_get_height(label_main)) / 2);

      a.var            = circ_area;
      a.start          = LV_HOR_RES - CIRCLE_SIZE;
      a.end            = 0;
      a.fp             = (lv_anim_fp_t)lv_obj_set_x;
      a.path           = lv_anim_path_linear;
      a.end_cb         = NULL;
      a.act_time       = 0;
      a.time           = 200;
      a.playback       = 0;
      a.playback_pause = 0;
      a.repeat         = 0;
      a.repeat_pause   = 0;
      lv_anim_create(&a);

      a.start         = LV_VER_RES - CIRCLE_SIZE;
      a.end           = LV_VER_RES - CIRCLE_SIZE;
      a.fp            = (lv_anim_fp_t) lv_obj_set_y;
      a.end_cb        = NULL;
      a.time          = 200;
      lv_anim_create(&a);

      state           = TP_CAL_STATE_WAIT_BOTTOM_LEFT;
    }

  else if (state == TP_CAL_STATE_WAIT_BOTTOM_LEFT)
    {
      lv_indev_t *indev = lv_indev_get_act();
      char buf[64];

      lv_indev_get_point(indev, &p[3]);

      lv_label_set_text(label_main, "Click the screen\n"
                        "to leave calibration");

      lv_obj_set_pos(label_main,
                     (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                     (LV_VER_RES - lv_obj_get_height(label_main)) / 2);

      sprintf(buf, "x: %d\ny: %d", p[3].x, p[3].y);
      lv_obj_t *label_coord = lv_label_create(lv_scr_act(), NULL);
      lv_label_set_text(label_coord, buf);
      lv_obj_set_pos(label_coord, 0,
                     LV_VER_RES - lv_obj_get_height(label_coord));

      lv_obj_del(circ_area);

      state = TP_CAL_STATE_WAIT_LEAVE;
    }
  else if (state == TP_CAL_STATE_WAIT_LEAVE)
    {
      lv_scr_load(prev_scr);
      tp_set_cal_values(&p[0], &p[1], &p[2], &p[3]);
      state = TP_CAL_STATE_READY;
    }
  else if (state == TP_CAL_STATE_READY)
    {
    }

  return LV_RES_OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tp_cal_create
 *
 * Description:
 *   Create a touchpad calibration screen
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void tp_cal_create(void)
{
  static lv_style_t style_circ;
  lv_anim_t a;

  state = TP_CAL_STATE_INIT;

  prev_scr = lv_scr_act();

  lv_obj_t *scr = lv_obj_create(NULL, NULL);
  lv_obj_set_size(scr, TP_MAX_VALUE, TP_MAX_VALUE);
  lv_scr_load(scr);

  /* Create a big transparent button screen to receive clicks */

  big_btn = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_size(big_btn, TP_MAX_VALUE, TP_MAX_VALUE);
  lv_btn_set_style(big_btn, LV_BTN_STYLE_REL, &lv_style_transp);
  lv_btn_set_style(big_btn, LV_BTN_STYLE_PR, &lv_style_transp);
  lv_btn_set_action(big_btn, LV_BTN_ACTION_CLICK, btn_click_action);
  lv_btn_set_layout(big_btn, LV_LAYOUT_OFF);

  label_main = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(label_main, "Click the circle in\n"
                    "upper left-hand corner");
  lv_label_set_align(label_main, LV_LABEL_ALIGN_CENTER);

  lv_obj_set_pos(label_main, (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                 (LV_VER_RES - lv_obj_get_height(label_main)) / 2);

  lv_style_copy(&style_circ, &lv_style_pretty_color);
  style_circ.body.radius = LV_RADIUS_CIRCLE;

  circ_area = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_set_size(circ_area, CIRCLE_SIZE, CIRCLE_SIZE);
  lv_obj_set_style(circ_area, &style_circ);
  lv_obj_set_click(circ_area, false);

  a.var            = circ_area;
  a.start          = LV_HOR_RES / 2;
  a.end            = 0;
  a.fp             = (lv_anim_fp_t) lv_obj_set_x;
  a.path           = lv_anim_path_linear;
  a.end_cb         = NULL;
  a.act_time       = -500;
  a.time           = 200;
  a.playback       = 0;
  a.playback_pause = 0;
  a.repeat         = 0;
  a.repeat_pause   = 0;
  lv_anim_create(&a);

  a.start          = LV_VER_RES / 2;
  a.end            = 0;
  a.fp             = (lv_anim_fp_t) lv_obj_set_y;
  a.end_cb         = NULL;
  a.time           = 200;
  lv_anim_create(&a);

  state = TP_CAL_STATE_WAIT_TOP_LEFT;
}
