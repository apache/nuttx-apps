/****************************************************************************
 * apps/examples/lvgldemo/lv_test_theme_2.c
 *
 *   Copyright (C) 2016 2016 Gábor Kiss-Vámosi. All rights reserved.
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

#include "lv_test_theme_2.h"
#include "lv_misc/lv_math.h"

#ifdef CONFIG_EXAMPLES_LVGLDEMO_THEME_2

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void header_create(void);
static void sb_create(void);
static void content_create(void);
static lv_res_t theme_select_action(lv_obj_t *roller);
static lv_res_t hue_select_action(lv_obj_t *roller);
static void init_all_themes(uint16_t hue);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t   *header;
static lv_obj_t   *sb;
static lv_obj_t   *content;
static lv_theme_t *th_act;

static const char *th_options = {
#if USE_LV_THEME_NIGHT
  "Night"
#endif
#if USE_LV_THEME_MATERIAL
  "\nMaterial"
#endif
#if USE_LV_THEME_ALIEN
  "\nAlien"
#endif
#if USE_LV_THEME_ZEN
  "\nZen"
#endif
#if USE_LV_THEME_NEMO
  "\nNemo"
#endif
#if USE_LV_THEME_MONO
  "\nMono"
#endif
#if USE_LV_THEME_DEFAULT
  "\nDefault"
#endif
  ""
};

static lv_theme_t *themes[8];

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: header_create
 *
 * Description:
 *   Initialize the global header object, populating with various icons
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *
 ****************************************************************************/

static void header_create(void)
{
  lv_obj_t *sym;
  lv_obj_t *clock;

  /* Create the global header object */

  header = lv_cont_create(lv_scr_act(), NULL);
  lv_obj_set_width(header, LV_HOR_RES);

  /* Add a few symbols */

  sym = lv_label_create(header, NULL);
  lv_label_set_text(sym,
                    SYMBOL_GPS " "
                    SYMBOL_WIFI " "
                    SYMBOL_BLUETOOTH " "
                    SYMBOL_VOLUME_MAX);

  lv_obj_align(sym, NULL, LV_ALIGN_IN_RIGHT_MID, -LV_DPI / 10, 0);

  /* Create a fake clock */

  clock = lv_label_create(header, NULL);
  lv_label_set_text(clock, "14:21");
  lv_obj_align(clock, NULL, LV_ALIGN_IN_LEFT_MID, LV_DPI / 10, 0);

  /* Let the height set automatically */

  lv_cont_set_fit(header, false, true);
  lv_obj_set_pos(header, 0, 0);
}

/****************************************************************************
 * Name: sb_create
 *
 * Description:
 *   Initialize the global sb (sidebar) object.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *
 ****************************************************************************/

static void sb_create(void)
{
  lv_obj_t *th_label;
  lv_obj_t *th_roller;
  lv_obj_t *hue_label;
  lv_obj_t *hue_roller;

  sb = lv_page_create(lv_scr_act(), NULL);
  lv_page_set_scrl_layout(sb, LV_LAYOUT_COL_M);
  lv_page_set_style(sb, LV_PAGE_STYLE_BG, &lv_style_transp_tight);
  lv_page_set_style(sb, LV_PAGE_STYLE_SCRL, &lv_style_transp);

  th_label = lv_label_create(sb, NULL);
  lv_label_set_text(th_label, "Theme");

  th_roller = lv_roller_create(sb, NULL);
  lv_roller_set_options(th_roller, th_options);
  lv_roller_set_action(th_roller, theme_select_action);

  hue_label = lv_label_create(sb, NULL);
  lv_label_set_text(hue_label, "\nColor");

  hue_roller = lv_roller_create(sb, NULL);
  lv_roller_set_options(hue_roller,
                "0\n30\n60\n90\n120\n150\n180\n210\n240\n270\n300\n330");
  lv_roller_set_action(hue_roller, hue_select_action);

#if LV_HOR_RES > LV_VER_RES
  lv_obj_set_height(sb, LV_VER_RES - lv_obj_get_height(header));
  lv_cont_set_fit(sb, true, false);
  lv_page_set_scrl_fit(sb, true, false);
  lv_obj_align(sb, header, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_page_set_sb_mode(sb, LV_SB_MODE_DRAG);
#else
  lv_obj_set_size(sb, LV_HOR_RES, LV_VER_RES / 2 - lv_obj_get_height(header));
  lv_obj_align(sb, header, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_page_set_sb_mode(sb, LV_SB_MODE_AUTO);
#endif
}

/****************************************************************************
 * Name: content_create
 *
 * Description:
 *   Initialize the global content object, create a number of objects
 *   within it
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *
 ****************************************************************************/

static void content_create(void)
{
  FAR lv_obj_t *btn;
  FAR lv_obj_t *label;
  FAR lv_obj_t *sw;
  FAR lv_obj_t *bar;
  FAR lv_obj_t *slider;
  FAR lv_obj_t *lmeter;
  FAR lv_obj_t *gauge;
  FAR lv_obj_t *ta;
  FAR lv_obj_t *ddlist;
  FAR lv_obj_t *kb;
  FAR lv_obj_t *mbox;
  FAR lv_obj_t *roller;

  lv_coord_t max_w;

  lv_anim_t a;

  static const char *nums = "One\nTwo\nThree\nFour";

  static const char *days =
    "Monday\nTuesday\nWednesday\nThursday\nFriday\nSaturday\nSunday";

  static const char *mbox_btns[] =
  {
    "Ok", ""
  };

  content = lv_page_create(lv_scr_act(), NULL);

#if LV_HOR_RES > LV_VER_RES
  lv_obj_set_size(content, LV_HOR_RES - lv_obj_get_width(
                    sb), LV_VER_RES - lv_obj_get_height(header));
  lv_obj_set_pos(content,  lv_obj_get_width(sb), lv_obj_get_height(header));
#else
  lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES / 2);
  lv_obj_set_pos(content,  0, LV_VER_RES / 2);
#endif

  lv_page_set_scrl_layout(content, LV_LAYOUT_PRETTY);

  max_w = lv_page_get_fit_width(content);

  /* Button */

  btn = lv_btn_create(content, NULL);
  lv_btn_set_ink_in_time(btn, 200);
  lv_btn_set_ink_wait_time(btn, 100);
  lv_btn_set_ink_out_time(btn, 500);

  label = lv_label_create(btn, NULL);
  lv_label_set_text(label, "Button");

  /* Switch */

  sw = lv_sw_create(content, NULL);

#if USE_LV_ANIMATION
#if LVGL_VERSION_MAJOR == 5 && LVGL_VERSION_MINOR >= 3
  lv_sw_set_anim_time(sw, 250);
#endif
#endif

  /* Check box */

  lv_cb_create(content, NULL);

  /* Bar */

  bar = lv_bar_create(content, NULL);
  lv_obj_set_width(bar, LV_MATH_MIN(max_w, 3 * LV_DPI / 2));

#if USE_LV_ANIMATION
  a.var             = bar;
  a.start           = 0;
  a.end             = 100;
  a.fp              = (lv_anim_fp_t)lv_bar_set_value;
  a.path            = lv_anim_path_linear;
  a.end_cb          = NULL;
  a.act_time        = 0;
  a.time            = 1000;
  a.playback        = 1;
  a.playback_pause  = 100;
  a.repeat          = 1;
  a.repeat_pause    = 100;
  lv_anim_create(&a);
#endif

  /* Slider */

  slider = lv_slider_create(content, NULL);
  lv_obj_set_width(slider, LV_MATH_MIN(max_w, 3 * LV_DPI / 2));
  lv_slider_set_value(slider, 30);

  /* Roller */

  roller = lv_roller_create(content, NULL);
  lv_roller_set_options(roller, days);

  /* Drop down list */

  ddlist   = lv_ddlist_create(content, NULL);
  lv_ddlist_set_options(ddlist, nums);

  /* Line meter */

  lmeter = lv_lmeter_create(content, NULL);
  lv_obj_set_click(lmeter, false);
#if USE_LV_ANIMATION
  a.var             = lmeter;
  a.start           = 0;
  a.end             = 100;
  a.fp              = (lv_anim_fp_t)lv_lmeter_set_value;
  a.path            = lv_anim_path_linear;
  a.end_cb          = NULL;
  a.act_time        = 0;
  a.time            = 1000;
  a.playback        = 1;
  a.playback_pause  = 100;
  a.repeat          = 1;
  a.repeat_pause    = 100;
  lv_anim_create(&a);
#endif

  /* Gauge */

  gauge = lv_gauge_create(content, NULL);
  lv_gauge_set_value(gauge, 0, 47);
  lv_obj_set_size(gauge, LV_MATH_MIN(max_w, LV_DPI * 3 / 2),
                  LV_MATH_MIN(max_w, LV_DPI * 3 / 2));
  lv_obj_set_click(gauge, false);

  /* Text area */

  ta = lv_ta_create(content, NULL);
  lv_obj_set_width(ta, LV_MATH_MIN(max_w, LV_DPI * 3 / 2));
  lv_ta_set_one_line(ta, true);
  lv_ta_set_text(ta, "Type...");

  /* Keyboard */

  kb = lv_kb_create(content, NULL);
  lv_obj_set_width(kb, LV_MATH_MIN(max_w, LV_DPI * 3));
  lv_kb_set_ta(kb, ta);

  /* Message Box */

  mbox = lv_mbox_create(lv_scr_act(), NULL);
  lv_obj_set_drag(mbox, true);
  lv_mbox_set_text(mbox, "Choose a theme and a color on the left!");

  lv_mbox_add_btns(mbox, mbox_btns, NULL);

  lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
}

/****************************************************************************
 * Name: theme_select_action
 *
 * Description:
 *   Callback for the theme selection roller
 *
 * Input Parameters:
 *   roller - the roller object that triggered the action
 *
 * Returned Value:
 *   LV_RES_OK in all cases
 *
 * Assumptions/Limitations:
 *
 ****************************************************************************/

static lv_res_t theme_select_action(lv_obj_t *roller)
{
  uint16_t opt;

  opt = lv_roller_get_selected(roller);

  th_act = themes[opt];
  lv_theme_set_current(th_act);

  lv_obj_align(header, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
  lv_obj_align(sb, header, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

#if LV_HOR_RES > LV_VER_RES
  lv_obj_set_size(content,
                  LV_HOR_RES - lv_obj_get_width(sb),
                  LV_VER_RES - lv_obj_get_height(header));
  lv_obj_set_pos(content,  lv_obj_get_width(sb), lv_obj_get_height(header));
#else
  lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES / 2);
  lv_obj_set_pos(content,  0, LV_VER_RES / 2);
#endif

  lv_page_focus(sb, roller, 200);

  return LV_RES_OK;
}

/****************************************************************************
 * Name: hue_select_action
 *
 * Description:
 *   Callback for the hue roller object upon selection change
 *
 * Input Parameters:
 *   roller - the roller object that triggered the action
 *
 * Returned Value:
 *   LV_RES_OK in all cases
 *
 * Assumptions/Limitations:
 *
 ****************************************************************************/

static lv_res_t hue_select_action(lv_obj_t * roller)
{
  uint16_t hue;

  hue = lv_roller_get_selected(roller) * 30;

  init_all_themes(hue);

  lv_theme_set_current(th_act);

  lv_page_focus(sb, roller, 200);

  return LV_RES_OK;
}

/****************************************************************************
 * Name: init_all_themes
 *
 * Description:
 *   Initialize all compiled-in themes to a hue
 *
 * Input Parameters:
 *   hue - The HSV hue to use for the theme's color
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   This must be adjusted if more themes are added.
 *
 ****************************************************************************/

static void init_all_themes(uint16_t hue)
{
  int i = 0;

#if USE_LV_THEME_NIGHT
  themes[i++] = lv_theme_night_init(hue, NULL);
#endif

#if USE_LV_THEME_MATERIAL
  themes[i++] = lv_theme_material_init(hue, NULL);
#endif

#if USE_LV_THEME_ALIEN
  themes[i++] = lv_theme_alien_init(hue, NULL);
#endif

#if USE_LV_THEME_ZEN
  themes[i++] = lv_theme_zen_init(hue, NULL);
#endif

#if USE_LV_THEME_NEMO
  themes[i++] = lv_theme_nemo_init(hue, NULL);
#endif

#if USE_LV_THEME_MONO
  themes[i++] = lv_theme_mono_init(hue, NULL);
#endif

#if USE_LV_THEME_DEFAULT
  themes[i++] = lv_theme_default_init(hue, NULL);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_test_theme_2
 *
 * Description:
 *   Setup the theme-switching demo application objects
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *
 ****************************************************************************/

void lv_test_theme_2(void)
{
  lv_obj_t *scr;

  /* By doing this, we hide the first (empty) option. */

  if (th_options[0] == '\n')
    {
      th_options++;
    }

  init_all_themes(0);
  th_act = themes[0];
  if (th_act == NULL)
    {
      LV_LOG_WARN("lv_test_theme_2: no theme is enabled. Check lv_conf.h");
      return;
    }

  lv_theme_set_current(th_act);

  scr = lv_obj_create(NULL, NULL);
  lv_scr_load(scr);

  header_create();
  sb_create();
  content_create();
}

#endif /* CONFIG_EXAMPLES_LVGLDEMO_THEME_2 */
