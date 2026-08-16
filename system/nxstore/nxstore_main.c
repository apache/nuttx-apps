/****************************************************************************
 * apps/system/nxstore/nxstore_main.c
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

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <lvgl/lvgl.h>

#include <system/nxpkg.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Warn the user after this many seconds to indicate that install has not
 * hung.
 */

#define NXSTORE_INSTALL_WARN_SECONDS (60)

#define NXSTORE_BAR_HEIGHT 36

/* Shared UI colors. */

#define NXSTORE_COLOR_BG          0x0b0d10  /* Screen background */
#define NXSTORE_COLOR_HEADER_BG   0x14171c  /* Header bar surface */
#define NXSTORE_COLOR_HEADER_LINE 0x22262e  /* Header bottom divider */
#define NXSTORE_COLOR_CARD_BG     0x1b1f26  /* Row card surface */
#define NXSTORE_COLOR_CARD_BORDER 0x282d36  /* Row card border */
#define NXSTORE_COLOR_TEXT        0xf2f4f7  /* Primary text */
#define NXSTORE_COLOR_TEXT_MUTED  0x99a1ad  /* Secondary/meta text */
#define NXSTORE_COLOR_ACCENT      0x3d8bff  /* "Install" affordance */
#define NXSTORE_COLOR_SUCCESS     0x34c77b  /* Installed / launch */
#define NXSTORE_COLOR_WARNING     0xf5a623  /* In progress / slow */
#define NXSTORE_COLOR_ERROR       0xf0554c  /* Failed */

/* Reject unreasonably large icon dimensions. */

#define NXSTORE_ICON_MAX_DIM      128

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum install_state_e
{
  INSTALL_STATE_IDLE = 0,
  INSTALL_STATE_INSTALLING,
  INSTALL_STATE_LAUNCHING,
  INSTALL_STATE_DONE_OK,
  INSTALL_STATE_INSTALL_FAILED,
  INSTALL_STATE_LAUNCH_FAILED,
};

/* State shared with the single install worker. */

struct install_ctx_s
{
  FAR const struct pkg_manifest_s *manifest;
  lv_obj_t *btn;
  lv_obj_t *label;
  lv_obj_t *progress_bar;
  char orig_text[192];
  _Atomic int state;
  _Atomic int install_error;
  pthread_t thread;
  bool joinable;
  time_t start_time;
  bool warned_slow;

  /* Published before INSTALL_STATE_DONE_OK. */

  pid_t launched_pid;
};

/* Child process currently using the framebuffer. */

struct running_app_s
{
  pid_t pid;
  char name[64];
  bool active;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_list;

/* Keep the large package index out of the task stack. */

static FAR struct pkg_index_s *g_index;
static struct install_ctx_s g_active;

/* App list and child-process supervisor screens. */

static lv_obj_t *g_main_scr;
static lv_obj_t *g_run_scr;
static lv_obj_t *g_run_label;
static struct running_app_s g_running;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void nxstore_toast(bool is_error, FAR const char *fmt, ...);
static lv_obj_t *nxstore_progress_bar_start(lv_obj_t *card);
static void nxstore_progress_bar_stop(lv_obj_t *bar);
static void nxstore_enter_running_screen(FAR const char *name);
static void close_running_app_event_cb(lv_event_t *e);
static void nxstore_poll_running_app(void);
static void build_run_screen(void);

/****************************************************************************
 * Name: nxstore_install_error_str
 *
 * Description:
 *   Return a user-facing package error.
 *
 ****************************************************************************/

static FAR const char *nxstore_install_error_str(int err)
{
  switch (err)
    {
      case -EILSEQ:
        return "checksum mismatch";

      case -ENOEXEC:
        return "wrong architecture for this device";

      case -EXDEV:
        return "not built for this board";

      case -ENETUNREACH:
      case -ENETDOWN:
      case -ETIMEDOUT:
      case -ECONNREFUSED:
      case -EHOSTUNREACH:
      case -EPROTO:
        return "network error";

      case -ENOSPC:
        return "not enough storage space";

      case -EFBIG:
        return "download too large";

      case -EBUSY:
        return "another install is already in progress for this package";

      case -EINVAL:
        return "invalid or untrusted package data";

      default:
        return "install failed";
    }
}

/****************************************************************************
 * Name: nxstore_toast
 *
 * Description:
 *   Show a temporary status banner.
 *
 ****************************************************************************/

static void nxstore_toast(bool is_error, FAR const char *fmt, ...)
{
  lv_obj_t *label;
  char text[192];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(text, sizeof(text), fmt, ap);
  va_end(ap);

  label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, text);
  lv_obj_add_flag(label, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_bg_color(label,
                            lv_color_hex(is_error ? NXSTORE_COLOR_ERROR
                                                  : NXSTORE_COLOR_CARD_BG),
                            0);
  lv_obj_set_style_bg_opa(label, LV_OPA_90, 0);
  lv_obj_set_style_radius(label, 10, 0);
  lv_obj_set_style_pad_hor(label, 14, 0);
  lv_obj_set_style_pad_ver(label, 8, 0);
  lv_obj_set_style_border_width(label, 0, 0);
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -14);
  lv_obj_move_foreground(label);

  lv_obj_delete_delayed(label, 2500);
}

/****************************************************************************
 * Name: nxstore_progress_bar_anim_cb
 ****************************************************************************/

static void nxstore_progress_bar_anim_cb(void *var, int32_t v)
{
  lv_obj_t *bar = var;
  int32_t end = v + 30 > 100 ? 100 : v + 30;

  lv_bar_set_start_value(bar, v, LV_ANIM_OFF);
  lv_bar_set_value(bar, end, LV_ANIM_OFF);
}

/****************************************************************************
 * Name: nxstore_progress_bar_start
 *
 * Description:
 *   Start an indeterminate bar because pkg_install() reports no progress.
 *
 ****************************************************************************/

static lv_obj_t *nxstore_progress_bar_start(lv_obj_t *card)
{
  lv_obj_t *text_col = lv_obj_get_child(card, 1);
  lv_obj_t *bar;
  lv_anim_t a;

  if (text_col == NULL)
    {
      return NULL;
    }

  bar = lv_bar_create(text_col);
  lv_obj_set_size(bar, lv_pct(100), 5);
  lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
  lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(bar, lv_color_hex(NXSTORE_COLOR_CARD_BORDER),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, lv_color_hex(NXSTORE_COLOR_ACCENT),
                            LV_PART_INDICATOR);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);
  lv_bar_set_mode(bar, LV_BAR_MODE_RANGE);
  lv_bar_set_range(bar, 0, 100);

  lv_anim_init(&a);
  lv_anim_set_var(&a, bar);
  lv_anim_set_exec_cb(&a, nxstore_progress_bar_anim_cb);
  lv_anim_set_values(&a, 0, 70);
  lv_anim_set_time(&a, 900);
  lv_anim_set_playback_time(&a, 900);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);

  return bar;
}

/****************************************************************************
 * Name: nxstore_progress_bar_stop
 ****************************************************************************/

static void nxstore_progress_bar_stop(lv_obj_t *bar)
{
  if (bar == NULL)
    {
      return;
    }

  lv_anim_delete(bar, nxstore_progress_bar_anim_cb);
  lv_obj_del(bar);
}

/****************************************************************************
 * Name: nxstore_enter_running_screen
 *
 * Description:
 *   Show and flush the child screen before it uses the framebuffer.  The
 *   caller records the PID after a successful spawn.
 *
 ****************************************************************************/

static void nxstore_enter_running_screen(FAR const char *name)
{
  snprintf(g_running.name, sizeof(g_running.name), "%s", name);

  lv_label_set_text(g_run_label, g_running.name);
  lv_screen_load(g_run_scr);
  lv_refr_now(NULL);
}

/****************************************************************************
 * Name: close_running_app_event_cb
 *
 * Description:
 *   Request a cooperative shutdown with SIGTERM.  Keep the child screen
 *   active if the process does not exit; forced deletion may leave shared
 *   framebuffer or heap state in use.
 *
 ****************************************************************************/

static void close_running_app_event_cb(lv_event_t *e)
{
  char name[64];
  int tries;
  int status;
  pid_t wret;
  bool reaped = false;
  bool gone = false;

  UNUSED(e);

  syslog(LOG_WARNING, "nxstore: close cb fired, active=%d pid=%d\n",
        g_running.active, (int)g_running.pid);

  if (!g_running.active)
    {
      return;
    }

  lv_label_set_text(g_run_label, "Closing...");
  lv_timer_handler();

  snprintf(name, sizeof(name), "%s", g_running.name);

  if (kill(g_running.pid, SIGTERM) < 0 && errno == ESRCH)
    {
      /* The child exited before SIGTERM. */

      syslog(LOG_WARNING, "nxstore: close pid %d already gone (ESRCH)\n",
            (int)g_running.pid);
      gone = true;
    }
  else
    {
      syslog(LOG_WARNING, "nxstore: close SIGTERM sent to %d\n",
            (int)g_running.pid);
    }

  for (tries = 0; tries < 40 && !reaped && !gone; tries++)
    {
      wret = waitpid(g_running.pid, &status, WNOHANG);
      if (wret == g_running.pid)
        {
          reaped = true;
        }
      else if (wret < 0 && errno == ECHILD)
        {
          /* Another path already reaped the child. */

          gone = true;
        }
      else
        {
          usleep(50 * 1000);
        }
    }

  syslog(LOG_WARNING,
        "nxstore: close reaped=%d gone=%d after %d tries\n",
        reaped, gone, tries);

  if (!reaped && !gone)
    {
      lv_label_set_text(g_run_label, "Still closing - try again");
      return;
    }

  g_running.active = false;
  lv_screen_load(g_main_scr);

  /* Redraw after the child wrote directly to the framebuffer. */

  lv_refr_now(NULL);

  nxstore_toast(false, "%s closed", name);
}

/****************************************************************************
 * Name: nxstore_poll_running_app
 *
 * Description:
 *   Return to the app list when the child exits.
 *
 ****************************************************************************/

static void nxstore_poll_running_app(void)
{
  int status;
  pid_t wret;

  if (!g_running.active)
    {
      return;
    }

  wret = waitpid(g_running.pid, &status, WNOHANG);
  if (wret == g_running.pid || (wret < 0 && errno == ECHILD))
    {
      char name[64];

      snprintf(name, sizeof(name), "%s", g_running.name);
      g_running.active = false;
      lv_screen_load(g_main_scr);
      lv_refr_now(NULL);
      nxstore_toast(false, "%s closed", name);
    }
}

/****************************************************************************
 * Name: build_run_screen
 *
 * Description:
 *   Build the supervisor bar shown while a child owns the framebuffer.
 *
 ****************************************************************************/

static void build_run_screen(void)
{
  lv_obj_t *bar;
  lv_obj_t *close_btn;
  lv_obj_t *close_label;

  g_run_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(g_run_scr, lv_color_hex(0x000000), 0);
  lv_obj_set_style_border_width(g_run_scr, 0, 0);
  lv_obj_clear_flag(g_run_scr, LV_OBJ_FLAG_SCROLLABLE);

  bar = lv_obj_create(g_run_scr);
  lv_obj_set_size(bar, lv_pct(100), NXSTORE_BAR_HEIGHT);
  lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(bar, lv_color_hex(NXSTORE_COLOR_HEADER_BG), 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_pad_hor(bar, 12, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);

  g_run_label = lv_label_create(bar);
  lv_label_set_text(g_run_label, "Running");
  lv_obj_set_style_text_font(g_run_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(g_run_label, lv_color_hex(NXSTORE_COLOR_TEXT),
                              0);
  lv_obj_align(g_run_label, LV_ALIGN_LEFT_MID, 0, 0);

  close_btn = lv_obj_create(bar);
  lv_obj_set_size(close_btn, 68, 26);
  lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_set_style_radius(close_btn, 8, 0);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(NXSTORE_COLOR_ERROR), 0);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xb03830),
                            LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(close_btn, -2, LV_STATE_PRESSED);
  lv_obj_set_style_transform_height(close_btn, -2, LV_STATE_PRESSED);
  lv_obj_set_style_border_width(close_btn, 0, 0);
  lv_obj_clear_flag(close_btn, LV_OBJ_FLAG_SCROLLABLE);

  close_label = lv_label_create(close_btn);
  lv_label_set_text(close_label, LV_SYMBOL_CLOSE " Close");
  lv_obj_set_style_text_font(close_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(close_label, lv_color_hex(0xffffff), 0);
  lv_obj_clear_flag(close_label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_center(close_label);

  lv_obj_add_event_cb(close_btn, close_running_app_event_cb,
                      LV_EVENT_CLICKED, NULL);
}

/* Packaged framebuffer apps need more than the default spawn stack. */

#define NXSTORE_LAUNCH_STACKSIZE 32768

/****************************************************************************
 * Name: nxstore_launch
 *
 * Description:
 *   Launch the installed version and return its pid in pid_out.
 *
 ****************************************************************************/

static int nxstore_launch(FAR const struct pkg_manifest_s *manifest,
                          FAR pid_t *pid_out)
{
  FAR struct pkg_installed_db_s *db;
  FAR struct pkg_installed_entry_s *entry;
  FAR struct pkg_manifest_s *installed;
  posix_spawnattr_t attr;
  char path[PATH_MAX];
  FAR char *argv[PKG_LAUNCH_ARGS_MAX + 2];
  size_t i;
  pid_t pid;
  int ret;

  /* The installed database is too large for the UI task stack. */

  db = calloc(1, sizeof(*db));
  if (db == NULL)
    {
      syslog(LOG_ERR, "nxstore: unable to allocate installed db buffer");
      return -ENOMEM;
    }

  installed = calloc(1, sizeof(*installed));
  if (installed == NULL)
    {
      free(db);
      return -ENOMEM;
    }

  ret = pkg_metadata_load_installed(db);
  if (ret < 0)
    {
      syslog(LOG_ERR, "nxstore: failed to load installed db: %d", ret);
      free(installed);
      free(db);
      return ret;
    }

  entry = pkg_metadata_find_installed(db, manifest->name);
  if (entry == NULL)
    {
      syslog(LOG_ERR, "nxstore: %s not found in installed db",
             manifest->name);
      free(installed);
      free(db);
      return -ENOENT;
    }

  ret = pkg_store_format_manifest_path(path, sizeof(path), manifest->name,
                                       entry->current);
  if (ret >= 0)
    {
      ret = pkg_metadata_load_manifest_path(path, installed);
    }

  if (ret >= 0 &&
      (strcmp(installed->name, manifest->name) != 0 ||
       strcmp(installed->version, entry->current) != 0))
    {
      ret = -EINVAL;
    }

  if (ret >= 0)
    {
      ret = pkg_store_format_payload_path(path, sizeof(path),
                                          installed->name,
                                          installed->version,
                                          installed->artifact);
    }

  free(db);
  if (ret < 0)
    {
      free(installed);
      return ret;
    }

  argv[0] = path;
  for (i = 0; i < installed->launch_argc; i++)
    {
      argv[i + 1] = installed->launch_args[i];
    }

  argv[i + 1] = NULL;

  ret = posix_spawnattr_init(&attr);
  if (ret != 0)
    {
      free(installed);
      return -ret;
    }

  ret = posix_spawnattr_setstacksize(&attr, NXSTORE_LAUNCH_STACKSIZE);
  if (ret == 0)
    {
      ret = posix_spawn(&pid, path, NULL, &attr, argv, NULL);
    }

  posix_spawnattr_destroy(&attr);
  free(installed);
  if (ret != 0)
    {
      syslog(LOG_ERR, "nxstore: posix_spawn(%s) failed: %d", path, ret);
      return -ret;
    }

  if (pid_out != NULL)
    {
      *pid_out = pid;
    }

  return 0;
}

/****************************************************************************
 * Name: nxstore_is_installed
 ****************************************************************************/

static bool nxstore_is_installed(FAR const struct pkg_manifest_s *manifest)
{
  FAR struct pkg_installed_db_s *db;
  bool found;

  db = calloc(1, sizeof(*db));
  if (db == NULL)
    {
      return false;
    }

  if (pkg_metadata_load_installed(db) < 0)
    {
      free(db);
      return false;
    }

  found = pkg_metadata_find_installed(db, manifest->name) != NULL;
  free(db);
  return found;
}

/****************************************************************************
 * Name: nxstore_is_up_to_date
 *
 * Description:
 *   Check whether the installed version matches the manifest.
 *
 ****************************************************************************/

static bool nxstore_is_up_to_date(FAR const struct pkg_manifest_s *manifest)
{
  FAR struct pkg_installed_db_s *db;
  FAR struct pkg_installed_entry_s *entry;
  bool up_to_date;

  db = calloc(1, sizeof(*db));
  if (db == NULL)
    {
      return false;
    }

  if (pkg_metadata_load_installed(db) < 0)
    {
      free(db);
      return false;
    }

  entry = pkg_metadata_find_installed(db, manifest->name);
  up_to_date = entry != NULL &&
              strcmp(entry->current, manifest->version) == 0;
  free(db);
  return up_to_date;
}

/****************************************************************************
 * Name: install_worker
 *
 * Description:
 *   Run blocking install and launch operations off the LVGL thread.
 *
 ****************************************************************************/

static FAR void *install_worker(FAR void *arg)
{
  FAR struct install_ctx_s *ctx = arg;
  int ret;

  ret = pkg_install(ctx->manifest->name);
  if (ret != 0)
    {
      ctx->install_error = ret;
      ctx->state = INSTALL_STATE_INSTALL_FAILED;
      return NULL;
    }

  ctx->state = INSTALL_STATE_LAUNCHING;

  ret = nxstore_launch(ctx->manifest, &ctx->launched_pid);
  ctx->state = ret == 0 ? INSTALL_STATE_DONE_OK :
                           INSTALL_STATE_LAUNCH_FAILED;
  return NULL;
}

/****************************************************************************
 * Name: nxstore_poll_active_install
 *
 * Description:
 *   Apply worker state changes and join the completed thread.
 *
 ****************************************************************************/

static void nxstore_poll_active_install(void)
{
  char text[256];
  FAR const char *result_text;

  if (g_active.manifest == NULL)
    {
      return;
    }

  if (g_active.state == INSTALL_STATE_INSTALLING ||
      g_active.state == INSTALL_STATE_LAUNCHING)
    {
      bool launching = g_active.state == INSTALL_STATE_LAUNCHING;
      time_t elapsed = time(NULL) - g_active.start_time;

      if (!g_active.warned_slow && elapsed > NXSTORE_INSTALL_WARN_SECONDS)
        {
          g_active.warned_slow = true;
        }

      if (g_active.label != NULL)
        {
          /* Show elapsed time when byte progress is unavailable. */

          if (elapsed < 3)
            {
              snprintf(text, sizeof(text), "%s...",
                      launching ? "Launching" : "Installing");
            }
          else if (g_active.warned_slow)
            {
              snprintf(text, sizeof(text), "%s... %lds (taking a while)",
                      launching ? "Launching" : "Installing",
                      (long)elapsed);
            }
          else
            {
              snprintf(text, sizeof(text), "%s... %lds",
                      launching ? "Launching" : "Installing",
                      (long)elapsed);
            }

          lv_label_set_text(g_active.label, text);
        }

      return;
    }

  switch (g_active.state)
    {
      case INSTALL_STATE_DONE_OK:
        if (g_active.manifest->description[0] != '\0')
          {
            snprintf(text, sizeof(text), "%s",
                    g_active.manifest->description);
          }
        else
          {
            snprintf(text, sizeof(text), "Installed - tap to launch");
          }

        result_text = text;
        break;

      case INSTALL_STATE_INSTALL_FAILED:
        snprintf(text, sizeof(text), "%s, tap to retry",
                 nxstore_install_error_str(g_active.install_error));
        result_text = text;
        break;

      case INSTALL_STATE_LAUNCH_FAILED:
        snprintf(text, sizeof(text), "Installed - launch failed, tap to "
                "retry");
        result_text = text;
        break;

      default:
        return;
    }

  if (g_active.joinable)
    {
      pthread_join(g_active.thread, NULL);
      g_active.joinable = false;
    }

  if (g_active.progress_bar != NULL)
    {
      nxstore_progress_bar_stop(g_active.progress_bar);
      g_active.progress_bar = NULL;
    }

  if (g_active.label != NULL)
    {
      lv_label_set_text(g_active.label, result_text);
    }

  /* Mark the package as installed. */

  if (g_active.state == INSTALL_STATE_DONE_OK && g_active.btn != NULL)
    {
      lv_obj_t *status_bar = lv_obj_get_child(g_active.btn, 1);

      if (status_bar != NULL)
        {
          lv_obj_set_style_bg_color(status_bar,
                                    lv_color_hex(NXSTORE_COLOR_SUCCESS), 0);
        }
    }

  if (g_active.btn != NULL)
    {
      lv_obj_clear_state(g_active.btn, LV_STATE_DISABLED);
    }

  switch (g_active.state)
    {
      case INSTALL_STATE_DONE_OK:

        /* The running screen confirms a successful install. */

        g_running.pid = g_active.launched_pid;
        g_running.active = true;
        nxstore_enter_running_screen(g_active.manifest->name);
        break;

      case INSTALL_STATE_INSTALL_FAILED:
      case INSTALL_STATE_LAUNCH_FAILED:
        nxstore_toast(true, "%s: %s", g_active.manifest->name, result_text);
        break;

      default:
        break;
    }

  memset(&g_active, 0, sizeof(g_active));
}

/****************************************************************************
 * Name: nxstore_card_subtitle
 *
 * Description:
 *   Return the subtitle label for a package card.
 *
 ****************************************************************************/

static lv_obj_t *nxstore_card_subtitle(lv_obj_t *card)
{
  lv_obj_t *text_col = lv_obj_get_child(card, 1);

  return text_col != NULL ? lv_obj_get_child(text_col, 1) : NULL;
}

/****************************************************************************
 * Name: uninstall_btn_event_cb
 *
 * Description:
 *   Remove an installed package on long press.
 *
 ****************************************************************************/

static void uninstall_btn_event_cb(lv_event_t *e)
{
  FAR const struct pkg_manifest_s *manifest = lv_event_get_user_data(e);
  lv_obj_t *card;
  lv_obj_t *subtitle;
  char text[256];
  int ret;

  if (manifest == NULL || !nxstore_is_installed(manifest))
    {
      return;
    }

  /* Ignore long presses emitted while the list is scrolling. */

  if (lv_indev_get_scroll_obj(lv_indev_active()) != NULL)
    {
      return;
    }

  card = lv_event_get_target(e);

  /* Prevent concurrent operations on this row. */

  if (lv_obj_has_state(card, LV_STATE_DISABLED))
    {
      return;
    }

  subtitle = nxstore_card_subtitle(card);

  lv_obj_add_state(card, LV_STATE_DISABLED);
  if (subtitle != NULL)
    {
      lv_label_set_text(subtitle, "Removing...");
      lv_timer_handler();
    }

  ret = pkg_uninstall(manifest->name);
  if (subtitle != NULL)
    {
      if (ret == EXIT_SUCCESS)
        {
          lv_obj_t *status_bar;

          snprintf(text, sizeof(text), "Removed - tap to reinstall");

          /* Mark the package as not installed. */

          status_bar = lv_obj_get_child(card, 1);

          if (status_bar != NULL)
            {
              lv_obj_set_style_bg_color(status_bar,
                                        lv_color_hex(NXSTORE_COLOR_ACCENT),
                                        0);
            }
        }
      else
        {
          snprintf(text, sizeof(text),
                  "Remove failed - long-press to retry");
        }

      lv_label_set_text(subtitle, text);
    }

  nxstore_toast(ret != EXIT_SUCCESS, "%s %s", manifest->name,
               ret == EXIT_SUCCESS ? "removed" : "failed to remove");

  lv_obj_clear_state(card, LV_STATE_DISABLED);
}

/****************************************************************************
 * Name: install_btn_event_cb
 *
 * Description:
 *   Launch an installed package or install its latest version first.
 *
 ****************************************************************************/

static void install_btn_event_cb(lv_event_t *e)
{
  FAR const struct pkg_manifest_s *manifest = lv_event_get_user_data(e);
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *card;
  lv_obj_t *subtitle;
  pthread_attr_t attr;

  if (code != LV_EVENT_CLICKED || manifest == NULL)
    {
      return;
    }

  card = lv_event_get_target(e);
  subtitle = nxstore_card_subtitle(card);

  if (nxstore_is_installed(manifest) && nxstore_is_up_to_date(manifest))
    {
      char orig[192];
      char text[256];
      pid_t pid = 0;
      int ret;

      /* Only one child or pending install may use the framebuffer. */

      if (g_running.active || g_active.manifest != NULL)
        {
          nxstore_toast(true, "Close the running app first");
          return;
        }

      if (lv_obj_has_state(card, LV_STATE_DISABLED))
        {
          return;
        }

      lv_obj_add_state(card, LV_STATE_DISABLED);

      orig[0] = '\0';
      if (subtitle != NULL)
        {
          snprintf(orig, sizeof(orig), "%s", lv_label_get_text(subtitle));
          lv_label_set_text(subtitle, "Launching...");
          lv_timer_handler();
        }

      /* Flush the supervisor screen before the child starts drawing. */

      nxstore_enter_running_screen(manifest->name);

      ret = nxstore_launch(manifest, &pid);
      if (subtitle != NULL)
        {
          lv_label_set_text(subtitle, orig);
        }

      lv_obj_clear_state(card, LV_STATE_DISABLED);

      if (ret == 0)
        {
          g_running.pid = pid;
          g_running.active = true;
        }
      else
        {
          g_running.active = false;
          lv_screen_load(g_main_scr);
          lv_refr_now(NULL);

          if (subtitle != NULL)
            {
              snprintf(text, sizeof(text),
                      "%s - launch failed, tap to retry", orig);
              lv_label_set_text(subtitle, text);
            }

          nxstore_toast(true, "%s failed to launch", manifest->name);
        }

      return;
    }

  if (g_active.manifest != NULL)
    {
      /* Only one install worker may run at a time. */

      return;
    }

  if (g_running.active)
    {
      /* Installs auto-launch, so wait for the current child to exit. */

      nxstore_toast(true, "Close the running app first");
      return;
    }

  memset(&g_active, 0, sizeof(g_active));
  g_active.manifest = manifest;
  g_active.btn = card;
  g_active.label = subtitle;
  g_active.state = INSTALL_STATE_INSTALLING;
  g_active.start_time = time(NULL);

  if (subtitle != NULL)
    {
      snprintf(g_active.orig_text, sizeof(g_active.orig_text), "%s",
              lv_label_get_text(subtitle));
    }

  lv_obj_add_state(card, LV_STATE_DISABLED);

  if (subtitle != NULL)
    {
      lv_label_set_text(subtitle, "Installing...");
    }

  g_active.progress_bar = nxstore_progress_bar_start(card);

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 16384);

  if (pthread_create(&g_active.thread, &attr, install_worker,
                     &g_active) != 0)
    {
      syslog(LOG_ERR, "nxstore: failed to spawn install worker");
      nxstore_progress_bar_stop(g_active.progress_bar);

      lv_obj_clear_state(card, LV_STATE_DISABLED);
      if (subtitle != NULL)
        {
          lv_label_set_text(subtitle, "Install failed");
        }

      nxstore_toast(true, "%s failed to start install", manifest->name);

      memset(&g_active, 0, sizeof(g_active));
      pthread_attr_destroy(&attr);
      return;
    }

  g_active.joinable = true;
  pthread_attr_destroy(&attr);
}

/****************************************************************************
 * Name: nxstore_load_icon
 *
 * Description:
 *   Load and cache an RGB565 package icon.  The row owns the returned image.
 *
 ****************************************************************************/

static FAR lv_image_dsc_t *
nxstore_load_icon(FAR const struct pkg_manifest_s *manifest)
{
  char cache_path[PATH_MAX];
  char source[PATH_MAX];
  FAR lv_image_dsc_t *dsc;
  FAR uint8_t *buf;
  struct stat st;
  int fd;
  ssize_t nread;
  size_t offset;
  uint16_t w;
  uint16_t h;
  uint16_t stride;

  if (manifest->icon[0] == '\0')
    {
      return NULL;
    }

  snprintf(cache_path, sizeof(cache_path),
           PKG_ROOT_DIR "/icons/%s-%s.bin",
           manifest->name, manifest->version);

  if (stat(cache_path, &st) < 0)
    {
      mkdir(PKG_ROOT_DIR "/icons", 0755);

      if (pkg_resolve_icon_source(source, sizeof(source), manifest) < 0 ||
          pkg_acquire_source(source, cache_path) < 0 ||
          stat(cache_path, &st) < 0)
        {
          return NULL;
        }
    }

  if (st.st_size <= 12 || st.st_size > 64 * 1024)
    {
      return NULL;
    }

  buf = malloc((size_t)st.st_size);
  if (buf == NULL)
    {
      return NULL;
    }

  fd = open(cache_path, O_RDONLY);
  if (fd < 0)
    {
      free(buf);
      return NULL;
    }

  offset = 0;
  while (offset < (size_t)st.st_size)
    {
      nread = read(fd, buf + offset, (size_t)st.st_size - offset);
      if (nread < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          break;
        }

      if (nread == 0)
        {
          break;
        }

      offset += nread;
    }

  close(fd);

  if (offset != (size_t)st.st_size ||
      buf[0] != LV_IMAGE_HEADER_MAGIC ||
      buf[1] != LV_COLOR_FORMAT_RGB565)
    {
      free(buf);
      unlink(cache_path);
      return NULL;
    }

  w = (uint16_t)(buf[4] | (buf[5] << 8));
  h = (uint16_t)(buf[6] | (buf[7] << 8));
  stride = (uint16_t)(buf[8] | (buf[9] << 8));

  if (w == 0 || h == 0 || w > NXSTORE_ICON_MAX_DIM ||
      h > NXSTORE_ICON_MAX_DIM || stride != w * 2 ||
      12 + (size_t)stride * h != (size_t)st.st_size)
    {
      free(buf);
      unlink(cache_path);
      return NULL;
    }

  dsc = calloc(1, sizeof(*dsc));
  if (dsc == NULL)
    {
      free(buf);
      return NULL;
    }

  dsc->header.magic = buf[0];
  dsc->header.cf = buf[1];
  dsc->header.flags = (uint16_t)(buf[2] | (buf[3] << 8));
  dsc->header.w = w;
  dsc->header.h = h;
  dsc->header.stride = stride;
  dsc->data_size = (uint32_t)stride * h;
  dsc->data = buf + 12;

  return dsc;
}

/****************************************************************************
 * Name: populate_app_list
 *
 * Description:
 *   Build one LVGL list entry per manifest already loaded into g_index.
 *
 ****************************************************************************/

static void populate_app_list(void)
{
  char seen_names[PKG_INDEX_MAX][PKG_NAME_MAX + 1];
  size_t seen_count = 0;
  size_t i;

  for (i = 0; i < g_index->count; i++)
    {
      FAR const struct pkg_manifest_s *manifest;
      bool installed;
      bool up_to_date;
      bool dup = false;
      size_t j;
      lv_obj_t *card;
      lv_obj_t *icon;
      lv_obj_t *icon_label;
      lv_obj_t *status_bar;
      lv_obj_t *text_col;
      lv_obj_t *title_label;
      lv_obj_t *subtitle_label;
      lv_obj_t *chevron;
      FAR lv_image_dsc_t *icon_dsc;
      char title_text[PKG_NAME_MAX + PKG_VERSION_MAX + 5];
      char subtitle_text[192];

      /* Show only the latest version of each package. */

      for (j = 0; j < seen_count; j++)
        {
          if (strcmp(seen_names[j], g_index->manifests[i].name) == 0)
            {
              dup = true;
              break;
            }
        }

      if (dup)
        {
          continue;
        }

      snprintf(seen_names[seen_count], sizeof(seen_names[seen_count]), "%s",
              g_index->manifests[i].name);
      seen_count++;

      manifest = pkg_metadata_find_latest(g_index,
                                          g_index->manifests[i].name);
      if (manifest == NULL)
        {
          continue;
        }

      installed = nxstore_is_installed(manifest);
      up_to_date = !installed || nxstore_is_up_to_date(manifest);
      icon_dsc = nxstore_load_icon(manifest);

      card = lv_obj_create(g_list);
      lv_obj_set_size(card, lv_pct(100), LV_SIZE_CONTENT);
      lv_obj_set_style_radius(card, 14, 0);
      lv_obj_set_style_bg_color(card, lv_color_hex(NXSTORE_COLOR_CARD_BG),
                                0);
      lv_obj_set_style_border_width(card, 1, 0);
      lv_obj_set_style_border_color(card,
                                    lv_color_hex(NXSTORE_COLOR_CARD_BORDER),
                                    0);
      lv_obj_set_style_pad_all(card, 12, 0);
      lv_obj_set_style_pad_column(card, 12, 0);

      /* Show immediate touch feedback. */

      lv_obj_set_style_bg_color(card,
                                lv_color_hex(NXSTORE_COLOR_CARD_BORDER),
                                LV_STATE_PRESSED);
      lv_obj_set_style_border_color(card, lv_color_hex(NXSTORE_COLOR_ACCENT),
                                    LV_STATE_PRESSED);
      lv_obj_set_style_transform_width(card, -3, LV_STATE_PRESSED);
      lv_obj_set_style_transform_height(card, -3, LV_STATE_PRESSED);

      lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER);
      lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

      icon = lv_obj_create(card);
      lv_obj_set_size(icon, 44, 44);
      lv_obj_set_style_radius(icon, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_bg_color(icon,
                                lv_color_hex(NXSTORE_COLOR_CARD_BORDER), 0);
      lv_obj_set_style_border_width(icon, 0, 0);
      lv_obj_set_style_pad_all(icon, 0, 0);
      lv_obj_set_style_clip_corner(icon, true, 0);
      lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);

      if (icon_dsc != NULL)
        {
          lv_obj_t *icon_img = lv_image_create(icon);

          lv_image_set_src(icon_img, icon_dsc);
          lv_image_set_scale(icon_img, 200);
          lv_obj_center(icon_img);
        }
      else
        {
          icon_label = lv_label_create(icon);
          lv_label_set_text(icon_label, LV_SYMBOL_FILE);
          lv_obj_set_style_text_color(icon_label,
                                      lv_color_hex(NXSTORE_COLOR_TEXT_MUTED),
                                      0);
          lv_obj_center(icon_label);
        }

      /* Indicate missing, current, or outdated package state. */

      status_bar = lv_obj_create(card);
      lv_obj_set_size(status_bar, 6, 36);
      lv_obj_set_style_radius(status_bar, 3, 0);
      lv_obj_set_style_bg_color(status_bar,
                                lv_color_hex(!installed
                                            ? NXSTORE_COLOR_ACCENT
                                            : up_to_date
                                            ? NXSTORE_COLOR_SUCCESS
                                            : NXSTORE_COLOR_WARNING), 0);
      lv_obj_set_style_border_width(status_bar, 0, 0);
      lv_obj_set_style_pad_all(status_bar, 0, 0);
      lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_CLICKABLE);

      text_col = lv_obj_create(card);
      lv_obj_set_style_bg_opa(text_col, LV_OPA_TRANSP, 0);
      lv_obj_set_style_border_width(text_col, 0, 0);
      lv_obj_set_style_pad_all(text_col, 0, 0);
      lv_obj_set_style_pad_row(text_col, 2, 0);
      lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_grow(text_col, 1);
      lv_obj_set_height(text_col, LV_SIZE_CONTENT);
      lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_clear_flag(text_col, LV_OBJ_FLAG_CLICKABLE);

      title_label = lv_label_create(text_col);
      snprintf(title_text, sizeof(title_text), "%s  v%s",
              manifest->name, manifest->version);
      lv_label_set_text(title_label, title_text);
      lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(title_label,
                                  lv_color_hex(NXSTORE_COLOR_TEXT), 0);

      subtitle_label = lv_label_create(text_col);
      if (installed && !up_to_date)
        {
          snprintf(subtitle_text, sizeof(subtitle_text),
                  "Update available - tap to update");
        }
      else if (manifest->description[0] != '\0')
        {
          snprintf(subtitle_text, sizeof(subtitle_text), "%s",
                  manifest->description);
        }
      else
        {
          snprintf(subtitle_text, sizeof(subtitle_text),
                  installed ? "Installed - tap to launch" :
                              "Tap to install");
        }

      lv_label_set_text(subtitle_label, subtitle_text);
      lv_obj_set_style_text_font(subtitle_label, &lv_font_montserrat_12, 0);
      lv_obj_set_style_text_color(subtitle_label,
                                  lv_color_hex(NXSTORE_COLOR_TEXT_MUTED), 0);
      lv_label_set_long_mode(subtitle_label, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(subtitle_label, lv_pct(100));

      chevron = lv_label_create(card);
      lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
      lv_obj_set_style_text_color(chevron,
                                  lv_color_hex(NXSTORE_COLOR_TEXT_MUTED), 0);

      lv_obj_add_event_cb(card, install_btn_event_cb, LV_EVENT_CLICKED,
                          (void *)manifest);
      lv_obj_add_event_cb(card, uninstall_btn_event_cb,
                          LV_EVENT_LONG_PRESSED, (void *)manifest);
    }
}

/****************************************************************************
 * Name: build_app_store_ui
 ****************************************************************************/

static void build_app_store_ui(FAR const char *repo_url)
{
  lv_obj_t *scr = lv_scr_act();
  lv_obj_t *header;
  lv_obj_t *title;
  lv_obj_t *subtitle;
  lv_obj_t *status;
  int ret;

  g_main_scr = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(NXSTORE_COLOR_BG), 0);

  g_index = calloc(1, sizeof(*g_index));
  if (g_index == NULL)
    {
      status = lv_label_create(scr);
      lv_obj_add_flag(status, LV_OBJ_FLAG_IGNORE_LAYOUT);
      lv_label_set_text(status, "Out of memory building catalog.");
      lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_center(status);
      lv_obj_set_style_text_color(status, lv_color_hex(NXSTORE_COLOR_ERROR),
                                  0);
      return;
    }

  /* Keep the screen fixed while allowing the package list to scroll. */

  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLL_CHAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_set_scroll_dir(scr, LV_DIR_NONE);
  lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr, 0, 0);
  lv_obj_set_style_pad_row(scr, 0, 0);

  header = lv_obj_create(scr);
  lv_obj_set_size(header, lv_pct(100), 64);
  lv_obj_set_style_bg_color(header,
                            lv_color_hex(NXSTORE_COLOR_HEADER_BG), 0);
  lv_obj_set_style_radius(header, 0, 0);
  lv_obj_set_style_border_width(header, 2, 0);
  lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_color(header,
                                lv_color_hex(NXSTORE_COLOR_HEADER_LINE), 0);
  lv_obj_set_style_pad_all(header, 0, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_CLICKABLE);

  title = lv_label_create(header);
  lv_label_set_text(title, "App Store");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(NXSTORE_COLOR_TEXT), 0);
  lv_obj_align(title, LV_ALIGN_LEFT_MID, 16, -10);

  subtitle = lv_label_create(header);
  lv_label_set_text(subtitle, "NuttX package manager");
  lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(subtitle,
                              lv_color_hex(NXSTORE_COLOR_TEXT_MUTED), 0);
  lv_obj_align(subtitle, LV_ALIGN_LEFT_MID, 16, 12);

  g_list = lv_list_create(scr);
  lv_obj_set_width(g_list, lv_pct(100));
  lv_obj_set_flex_grow(g_list, 1);
  lv_obj_set_style_bg_color(g_list, lv_color_hex(NXSTORE_COLOR_BG), 0);
  lv_obj_set_style_border_width(g_list, 0, 0);
  lv_obj_set_style_pad_all(g_list, 12, 0);
  lv_obj_set_style_pad_row(g_list, 10, 0);

  /* Allow deliberate vertical scrolling without momentum or chaining. */

  lv_obj_clear_flag(g_list, LV_OBJ_FLAG_SCROLL_CHAIN);
  lv_obj_clear_flag(g_list, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_clear_flag(g_list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(g_list, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_set_scroll_dir(g_list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(g_list, LV_SCROLLBAR_MODE_AUTO);

  /* Refresh the catalog when a repository URL is provided. */

  if (repo_url != NULL && repo_url[0] != '\0')
    {
      lv_obj_t *spinner;
      time_t retry_deadline;

      status = lv_label_create(scr);
      lv_obj_add_flag(status, LV_OBJ_FLAG_IGNORE_LAYOUT);
      lv_label_set_text(status, "Waiting for network...");
      lv_obj_align(status, LV_ALIGN_CENTER, 0, -30);
      lv_obj_set_style_text_color(status,
                                  lv_color_hex(NXSTORE_COLOR_WARNING), 0);

      spinner = lv_spinner_create(scr);
      lv_obj_add_flag(spinner, LV_OBJ_FLAG_IGNORE_LAYOUT);
      lv_obj_set_size(spinner, 40, 40);
      lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 20);
      lv_obj_set_style_arc_color(spinner, lv_color_hex(NXSTORE_COLOR_ACCENT),
                                 LV_PART_INDICATOR);

      lv_label_set_text(status, "Downloading listing from server...");

      /* Paint the progress state before the blocking fetch. */

      lv_timer_handler();

      /* The UI may start before the board has a route. */

      retry_deadline = time(NULL) + 15;
      do
        {
          ret = pkg_sync(repo_url);
          if (ret != -ENETUNREACH && ret != -ENETDOWN &&
              ret != -EHOSTUNREACH)
            {
              break;
            }

          lv_timer_handler();
          usleep(250 * 1000);
        }
      while (time(NULL) < retry_deadline);

      lv_obj_del(spinner);
      if (ret != 0)
        {
          lv_label_set_text(status, "Server download failed.\n"
                                    "Showing last cached listing.");
          lv_obj_set_style_text_color(status,
                                      lv_color_hex(NXSTORE_COLOR_WARNING),
                                      0);
          lv_obj_align(status, LV_ALIGN_CENTER, 0, 0);
          lv_timer_handler();
        }
      else
        {
          lv_obj_del(status);
        }
    }

  ret = pkg_metadata_load_index(g_index);
  if (ret < 0)
    {
      status = lv_label_create(scr);
      lv_obj_add_flag(status, LV_OBJ_FLAG_IGNORE_LAYOUT);
      lv_label_set_text(status, "No package index available.\n"
                                "Connect Wi-Fi and pass a repo URL,\n"
                                "or run 'nxpkg sync <repo>' first.");
      lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_center(status);
      lv_obj_set_style_text_color(status, lv_color_hex(NXSTORE_COLOR_ERROR),
                                  0);
      return;
    }

  if (g_index->count == 0)
    {
      /* Distinguish an empty catalog from a load failure. */

      status = lv_label_create(scr);
      lv_obj_add_flag(status, LV_OBJ_FLAG_IGNORE_LAYOUT);
      lv_label_set_text(status, "No packages available for this device.");
      lv_obj_center(status);
      lv_obj_set_style_text_color(status,
                                  lv_color_hex(NXSTORE_COLOR_WARNING), 0);
      return;
    }

  populate_app_list();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  FAR const char *repo_url = NULL;

  /* An optional first argument overrides the cached repository index. */

  if (argc > 1 && argv[1] != NULL && argv[1][0] != '\0')
    {
      repo_url = argv[1];
    }

  if (lv_is_initialized())
    {
      printf("nxstore: LVGL already initialized! aborting.\n");
      return -1;
    }

  lv_init();

  lv_nuttx_dsc_init(&info);
  info.fb_path = CONFIG_SYSTEM_NXSTORE_FBDEVPATH;
  info.input_path = CONFIG_SYSTEM_NXSTORE_INPUT_DEVPATH;
  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      printf("nxstore: lv_nuttx_init failure!\n");
      return 1;
    }

  /* Require a deliberate drag and disable momentum on noisy touch input. */

  if (result.indev != NULL)
    {
      lv_indev_set_scroll_limit(result.indev, 20);
      lv_indev_set_scroll_throw(result.indev, 0);
    }

  build_app_store_ui(repo_url);
  build_run_screen();

  while (1)
    {
      uint32_t idle;

      idle = lv_timer_handler();

      nxstore_poll_active_install();
      nxstore_poll_running_app();

      idle = idle ? idle : 1;
      usleep(idle * 1000);
    }

  lv_nuttx_deinit(&result);
  lv_deinit();
  return 0;
}
