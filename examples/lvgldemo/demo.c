/****************************************************************************
 * apps/examples/lvgldemo/demo.c
 *
 *   Copyright (C) 2019 Gábor Kiss-Vámosi. All rights reserved.
 *   Author: Gábor Kiss-Vámosi <kisvegabor@gmail.com>
 *
 * Released under the following BSD-compatible MIT license:
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the “Software”), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <graphics/lvgl.h>

#include "demo.h"

#ifdef CONFIG_EXAMPLES_LVGLDEMO_SIMPLE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void write_create(lv_obj_t *parent);
static lv_res_t keyboard_open_close(lv_obj_t *ta);
static lv_res_t keyboard_hide_action(lv_obj_t *keyboard);
static void kb_hide_anim_end(lv_obj_t *keyboard);
static void list_create(lv_obj_t *parent);
static void chart_create(lv_obj_t *parent);
static lv_res_t slider_action(lv_obj_t *slider);
static lv_res_t list_btn_action(lv_obj_t *slider);

#if LV_DEMO_SLIDE_SHOW
static void tab_switcher(void *tv);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

FAR static lv_obj_t *chart;
FAR static lv_obj_t *ta;
FAR static lv_obj_t *kb;

static lv_style_t style_kb;
static lv_style_t style_kb_rel;
static lv_style_t style_kb_pr;

#if LV_DEMO_WALLPAPER
LV_IMG_DECLARE(img_bubble_pattern);
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: write_create
 *
 * Description:
 *   Create a text-area and keyboard, each composing half of the
 *   parent container.
 *
 * Input Parameters:
 *   parent  - the lv_page_t* container to create the "write" page within
 *
 * Returned Value:
 *   none
 *
 * Assumptions/Limitations:
 *
 *
 ****************************************************************************/

static void write_create(lv_obj_t *parent)
{
  static lv_style_t style_ta;

  lv_page_set_style(parent, LV_PAGE_STYLE_BG, &lv_style_transp_fit);
  lv_page_set_style(parent, LV_PAGE_STYLE_SCRL, &lv_style_transp_fit);

  lv_page_set_sb_mode(parent, LV_SB_MODE_OFF);

  lv_style_copy(&style_ta, &lv_style_pretty);
  style_ta.body.opa     = LV_OPA_30;
  style_ta.body.radius  = 0;
  style_ta.text.color   = LV_COLOR_HEX3(0x222);

  ta = lv_ta_create(parent, NULL);
  lv_obj_set_size(ta, lv_page_get_scrl_width(parent),
                      lv_obj_get_height(parent) / 2);
  lv_ta_set_style(ta, LV_TA_STYLE_BG, &style_ta);
  lv_ta_set_text(ta, "");
  lv_page_set_rel_action(ta, keyboard_open_close);

  lv_style_copy(&style_kb, &lv_style_plain);
  style_kb.body.opa             = LV_OPA_70;
  style_kb.body.main_color      = LV_COLOR_HEX3(0x333);
  style_kb.body.grad_color      = LV_COLOR_HEX3(0x333);
  style_kb.body.padding.hor     = 0;
  style_kb.body.padding.ver     = 0;
  style_kb.body.padding.inner   = 0;

  lv_style_copy(&style_kb_rel, &lv_style_plain);
  style_kb_rel.body.empty           = 1;
  style_kb_rel.body.radius          = 0;
  style_kb_rel.body.border.width    = 1;
  style_kb_rel.body.border.color    = LV_COLOR_SILVER;
  style_kb_rel.body.border.opa      = LV_OPA_50;
  style_kb_rel.body.main_color      = LV_COLOR_HEX3(0x333);
  style_kb_rel.body.grad_color      = LV_COLOR_HEX3(0x333);
  style_kb_rel.text.color           = LV_COLOR_WHITE;

  lv_style_copy(&style_kb_pr, &lv_style_plain);
  style_kb_pr.body.radius       = 0;
  style_kb_pr.body.opa          = LV_OPA_50;
  style_kb_pr.body.main_color   = LV_COLOR_WHITE;
  style_kb_pr.body.grad_color   = LV_COLOR_WHITE;
  style_kb_pr.body.border.width = 1;
  style_kb_pr.body.border.color = LV_COLOR_SILVER;

  keyboard_open_close(ta);
}

/****************************************************************************
 * Name: keyboard_open_close
 *
 * Description:
 *   Toggle the display of the keyboard on the "Write" tab
 *
 * Input Parameters:
 *   text_area - a pointer to the lv_obj_t text area for the keyboard
 *
 * Returned Value:
 *   LV_RES_OK on success, some other LV_RES_ value otherwise.
 *
 * Assumptions/Limitations:
 *
 *
 ****************************************************************************/

static lv_res_t keyboard_open_close(lv_obj_t *text_area)
{
  lv_obj_t *parent;

  (void)text_area; /* Unused */

  /* Text area is on the scrollable part of the page but we need
   * the page itself
   */

  parent = lv_obj_get_parent(lv_obj_get_parent(ta));

  if (kb)
    {
      return keyboard_hide_action(kb);
    }
  else
    {
      kb = lv_kb_create(parent, NULL);

      lv_obj_set_size(kb, lv_page_get_scrl_width(parent),
                      lv_obj_get_height(parent) / 2);
      lv_obj_align(kb, ta, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

      lv_kb_set_ta(kb, ta);

      lv_kb_set_style(kb, LV_KB_STYLE_BG, &style_kb);
      lv_kb_set_style(kb, LV_KB_STYLE_BTN_REL, &style_kb_rel);
      lv_kb_set_style(kb, LV_KB_STYLE_BTN_PR, &style_kb_pr);

      lv_kb_set_hide_action(kb, keyboard_hide_action);
      lv_kb_set_ok_action(kb, keyboard_hide_action);

#if USE_LV_ANIMATION
      lv_obj_animate(kb, LV_ANIM_FLOAT_BOTTOM | LV_ANIM_IN, 300, 0, NULL);
#endif
      return LV_RES_OK;
    }
}

/****************************************************************************
 * Name: keyboard_hide_action
 *
 * Description:
 *   Called when the close or ok button is pressed on the keyboard
 *
 * Input Parameters:
 *   keyboard  - pointer to the keyboard, but unused
 *
 * Returned Value:
 *   LV_RES_OK if the *kb object was not deleted (freed),
 *   LV_RES_INV if it was deleted.
 *
 * Assumptions/Limitations:
 *   The animation routine will call kb_hide_anim_end() to delete (free)
 *   the object. The global lv_obj_t *kb is set to NULL after whether
 *   deleted or not. Assume *kb is valid entering this function.
 *
 ****************************************************************************/

static lv_res_t keyboard_hide_action(lv_obj_t *keyboard)
{
  (void)keyboard; /* Unused */

#if USE_LV_ANIMATION
  lv_obj_animate(kb, LV_ANIM_FLOAT_BOTTOM | LV_ANIM_OUT, 300, 0,
                 kb_hide_anim_end);
  kb = NULL;
  return LV_RES_OK;
#else
  lv_obj_del(kb);
  kb = NULL;
  return LV_RES_INV;
#endif
}

/****************************************************************************
 * Name: kb_hide_anim_end
 *
 * Description:
 *   A callback to delete (free) the object passed as a parameter.
 *
 * Input Parameters:
 *   keyboard - An lv_obj_t * to be deleted.
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *
 *
 ****************************************************************************/

static void kb_hide_anim_end(lv_obj_t *keyboard)
{
  lv_obj_del(keyboard);
}

/****************************************************************************
 * Name: list_create
 *
 * Description:
 *   Create a list object within a parent page, populating it with sample
 *   data.
 *
 * Input Parameters:
 *   parent - The parent lv_page
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *
 *
 ****************************************************************************/

static void list_create(lv_obj_t *parent)
{
  static lv_style_t style_btn_rel;
  static lv_style_t style_btn_pr;

  FAR lv_obj_t *list;
  FAR lv_obj_t *mbox;

  FAR static const char *mbox_btns[] = {
    "Got it", ""
  };

  lv_page_set_style(parent, LV_PAGE_STYLE_BG, &lv_style_transp_fit);
  lv_page_set_style(parent, LV_PAGE_STYLE_SCRL, &lv_style_transp_fit);

  lv_page_set_scrl_fit(parent, false, false);
  lv_page_set_scrl_height(parent, lv_obj_get_height(parent));
  lv_page_set_sb_mode(parent, LV_SB_MODE_OFF);

  /* Create styles for the buttons */

  lv_style_copy(&style_btn_rel, &lv_style_btn_rel);
  style_btn_rel.body.main_color     = LV_COLOR_HEX3(0x333);
  style_btn_rel.body.grad_color     = LV_COLOR_BLACK;
  style_btn_rel.body.border.color   = LV_COLOR_SILVER;
  style_btn_rel.body.border.width   = 1;
  style_btn_rel.body.border.opa     = LV_OPA_50;
  style_btn_rel.body.radius         = 0;

  lv_style_copy(&style_btn_pr, &style_btn_rel);
  style_btn_pr.body.main_color  = LV_COLOR_MAKE(0x55, 0x96, 0xd8);
  style_btn_pr.body.grad_color  = LV_COLOR_MAKE(0x37, 0x62, 0x90);
  style_btn_pr.text.color       = LV_COLOR_MAKE(0xbb, 0xd5, 0xf1);

  list = lv_list_create(parent, NULL);

  lv_obj_set_height(list, 2 * lv_obj_get_height(parent) / 3);

  lv_list_set_style(list, LV_LIST_STYLE_BG,      &lv_style_transp_tight);
  lv_list_set_style(list, LV_LIST_STYLE_SCRL,    &lv_style_transp_tight);
  lv_list_set_style(list, LV_LIST_STYLE_BTN_REL, &style_btn_rel);
  lv_list_set_style(list, LV_LIST_STYLE_BTN_PR,  &style_btn_pr);

  lv_obj_align(list, NULL, LV_ALIGN_IN_TOP_MID, 0, LV_DPI / 4);

  lv_list_add(list, SYMBOL_FILE,      "New",    list_btn_action);
  lv_list_add(list, SYMBOL_DIRECTORY, "Open",   list_btn_action);
  lv_list_add(list, SYMBOL_TRASH,     "Delete", list_btn_action);
  lv_list_add(list, SYMBOL_EDIT,      "Edit",   list_btn_action);
  lv_list_add(list, SYMBOL_SAVE,      "Save",   list_btn_action);
  lv_list_add(list, SYMBOL_WIFI,      "WiFi",   list_btn_action);
  lv_list_add(list, SYMBOL_GPS,       "GPS",    list_btn_action);

  mbox = lv_mbox_create(parent, NULL);

  lv_mbox_set_text(mbox, "Click a button to copy its text to the Text area ");
  lv_obj_set_width(mbox, LV_HOR_RES - LV_DPI);

  /* The default action is close */

  lv_mbox_add_btns(mbox, mbox_btns, NULL);
  lv_obj_align(mbox, parent, LV_ALIGN_IN_TOP_MID, 0, LV_DPI / 2);
}

/****************************************************************************
 * Name: list_btn_action
 *
 * Description:
 *   Callback for the buttons within the list on the list page. Appends
 *   the button text to the content of the text area.
 *
 * Input Parameters:
 *   btn - The button object that triggered the action
 *
 * Returned Value:
 *   LV_RES_OK - the object is still valid
 *
 * Assumptions/Limitations:
 *
 *
 ****************************************************************************/

static lv_res_t list_btn_action(lv_obj_t *btn)
{
  lv_ta_add_text(ta, lv_list_get_btn_text(btn));
  lv_ta_add_char(ta, '\n');

  return LV_RES_OK;
}

/****************************************************************************
 * Name: chart_create
 *
 * Description:
 *   Create a chart object within a parent page, populating it with sample
 *   data and a slider to adjust the Y-axis.
 *
 * Input Parameters:
 *   parent - The parent lv_page
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *
 *
 ****************************************************************************/

static void chart_create(lv_obj_t *parent)
{
  static lv_style_t style_chart;
  static lv_style_t style_bar;
  static lv_style_t style_indic;
  static lv_style_t style_knob;

  FAR lv_chart_series_t *ser1;

  FAR lv_obj_t *slider;

  lv_page_set_style(parent, LV_PAGE_STYLE_BG,   &lv_style_transp_fit);
  lv_page_set_style(parent, LV_PAGE_STYLE_SCRL, &lv_style_transp_fit);

  /* Make the parent a fixed size */

  lv_page_set_scrl_fit(parent, false, false);
  lv_page_set_scrl_height(parent, lv_obj_get_height(parent));
  lv_page_set_sb_mode(parent, LV_SB_MODE_OFF);

  lv_style_copy(&style_chart, &lv_style_pretty);
  style_chart.body.opa      = LV_OPA_60;
  style_chart.body.radius   = 0;
  style_chart.line.color    = LV_COLOR_GRAY;

  chart = lv_chart_create(parent, NULL);
  lv_obj_set_size(chart, 2 * lv_obj_get_width(parent) / 3,
                  lv_obj_get_height(parent) / 2);
  lv_obj_align(chart, NULL,  LV_ALIGN_IN_TOP_MID, 0, LV_DPI / 4);
  lv_chart_set_type(chart, LV_CHART_TYPE_COLUMN);
  lv_chart_set_style(chart, &style_chart);
  lv_chart_set_series_opa(chart, LV_OPA_70);

  ser1 = lv_chart_add_series(chart, LV_COLOR_RED);
  lv_chart_set_next(chart, ser1, 40);
  lv_chart_set_next(chart, ser1, 30);
  lv_chart_set_next(chart, ser1, 47);
  lv_chart_set_next(chart, ser1, 59);
  lv_chart_set_next(chart, ser1, 59);
  lv_chart_set_next(chart, ser1, 31);
  lv_chart_set_next(chart, ser1, 55);
  lv_chart_set_next(chart, ser1, 70);
  lv_chart_set_next(chart, ser1, 82);
  lv_chart_set_next(chart, ser1, 91);

  /* Create a bar, an indicator and a knob style */

  lv_style_copy(&style_bar, &lv_style_pretty);
  style_bar.body.main_color     = LV_COLOR_BLACK;
  style_bar.body.grad_color     = LV_COLOR_GRAY;
  style_bar.body.radius         = LV_RADIUS_CIRCLE;
  style_bar.body.border.color   = LV_COLOR_WHITE;
  style_bar.body.opa            = LV_OPA_60;
  style_bar.body.padding.hor    = 0;
  style_bar.body.padding.ver    = LV_DPI / 10;

  lv_style_copy(&style_indic, &lv_style_pretty);
  style_indic.body.grad_color   = LV_COLOR_MAROON;
  style_indic.body.main_color   = LV_COLOR_RED;
  style_indic.body.radius       = LV_RADIUS_CIRCLE;
  style_indic.body.shadow.width = LV_DPI / 10;
  style_indic.body.shadow.color = LV_COLOR_RED;
  style_indic.body.padding.hor  = LV_DPI / 30;
  style_indic.body.padding.ver  = LV_DPI / 30;

  lv_style_copy(&style_knob, &lv_style_pretty);
  style_knob.body.radius    = LV_RADIUS_CIRCLE;
  style_knob.body.opa       = LV_OPA_70;

  /* Create a second slider, align to bottom of the chart */

  slider = lv_slider_create(parent, NULL);

  lv_slider_set_style(slider, LV_SLIDER_STYLE_BG, &style_bar);
  lv_slider_set_style(slider, LV_SLIDER_STYLE_INDIC, &style_indic);
  lv_slider_set_style(slider, LV_SLIDER_STYLE_KNOB, &style_knob);

  lv_obj_set_size(slider, lv_obj_get_width(chart), LV_DPI / 3);

  lv_obj_align(slider, chart, LV_ALIGN_OUT_BOTTOM_MID, 0,
          (LV_VER_RES - chart->coords.y2 - lv_obj_get_height(slider)) / 2);

  /* Simulate a user value set the refresh the chart */

  lv_slider_set_action(slider, slider_action);
  lv_slider_set_range(slider, 10, 1000);
  lv_slider_set_value(slider, 700);

  slider_action(slider);
}

/****************************************************************************
 * Name: slider_action
 *
 * Description:
 *   Callback of the slider on the chart page. Adjusts the Y-axis range
 *
 * Input Parameters:
 *   slider - the slider object that triggered this action
 *
 * Returned Value:
 *   LV_RES_OK - the object is still valid
 *
 * Assumptions/Limitations:
 *   Anything else that one might need to know to use this function.
 *
 ****************************************************************************/

static lv_res_t slider_action(lv_obj_t *slider)
{
  int16_t v = lv_slider_get_value(slider);

  v = 1000 * 100 / v;   /* Convert to range modify values linearly */
  lv_chart_set_range(chart, 0, v);

  return LV_RES_OK;
}

#if LV_DEMO_SLIDE_SHOW
/****************************************************************************
 * Name: tab_switcher
 *
 * Description:
 *   Cycle to the next tab in a tab view.
 *
 * Input Parameters:
 *   tv - The tab view object to cycle
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   Assumes only 3 tabs in the tab view.
 *
 ****************************************************************************/

/**
 * Called periodically (lv_task) to switch to the next tab
 */

static void tab_switcher(void *tv)
{
  static uint8_t tab = 0;

  tab++;
  if (tab >= 3)
    {
      tab = 0;
    }

  lv_tabview_set_tab_act(tv, tab, true);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: demo_create
 *
 * Description:
 *   Initialize the LVGL demo screen
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *
 *
 ****************************************************************************/

void demo_create(void)
{
  static lv_style_t style_tv_btn_bg;
  static lv_style_t style_tv_btn_rel;
  static lv_style_t style_tv_btn_pr;

  FAR lv_obj_t *tv;
  FAR lv_obj_t *tab1;
  FAR lv_obj_t *tab2;
  FAR lv_obj_t *tab3;

#if LV_DEMO_WALLPAPER
  FAR lv_obj_t *wp;

  wp = lv_img_create(lv_scr_act(), NULL);
  lv_img_set_src(wp, &img_bubble_pattern);
  lv_obj_set_width(wp, LV_HOR_RES * 4);
  lv_obj_set_protect(wp, LV_PROTECT_POS);
#endif

  lv_style_copy(&style_tv_btn_bg, &lv_style_plain);
  style_tv_btn_bg.body.main_color   = LV_COLOR_HEX(0x487fb7);
  style_tv_btn_bg.body.grad_color   = LV_COLOR_HEX(0x487fb7);
  style_tv_btn_bg.body.padding.ver  = 0;

  lv_style_copy(&style_tv_btn_rel, &lv_style_btn_rel);
  style_tv_btn_rel.body.empty           = 1;
  style_tv_btn_rel.body.border.width    = 0;

  lv_style_copy(&style_tv_btn_pr, &lv_style_btn_pr);
  style_tv_btn_pr.body.radius       = 0;
  style_tv_btn_pr.body.opa          = LV_OPA_50;
  style_tv_btn_pr.body.main_color   = LV_COLOR_WHITE;
  style_tv_btn_pr.body.grad_color   = LV_COLOR_WHITE;
  style_tv_btn_pr.body.border.width = 0;
  style_tv_btn_pr.text.color        = LV_COLOR_GRAY;

  tv = lv_tabview_create(lv_scr_act(), NULL);

#if LV_DEMO_WALLPAPER
  lv_obj_set_parent(wp, ((lv_tabview_ext_t*)tv->ext_attr)->content);
  lv_obj_set_pos(wp, 0, -5);
#endif

  tab1 = lv_tabview_add_tab(tv, "Write");
  tab2 = lv_tabview_add_tab(tv, "List");
  tab3 = lv_tabview_add_tab(tv, "Chart");

#if LV_DEMO_WALLPAPER == 0
  /* Blue bg instead of wallpaper */

  lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BG, &style_tv_btn_bg);
#endif
  lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_BG, &style_tv_btn_bg);
  lv_tabview_set_style(tv, LV_TABVIEW_STYLE_INDIC, &lv_style_plain);
  lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_REL, &style_tv_btn_rel);
  lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_PR, &style_tv_btn_pr);
  lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_TGL_REL, &style_tv_btn_rel);
  lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_TGL_PR, &style_tv_btn_pr);

  write_create(tab1);
  list_create(tab2);
  chart_create(tab3);

#if LV_DEMO_SLIDE_SHOW
  lv_task_create(tab_switcher, 3000, LV_TASK_PRIO_MID, tv);
#endif
}

#endif /* CONFIG_EXAMPLES_LVGLDEMO_SIMPLE */
