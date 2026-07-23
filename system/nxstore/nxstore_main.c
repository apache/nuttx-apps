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
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <lvgl/lvgl.h>

#include <system/nxstore_chrome.h>

#include "../nxpkg/pkg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* No cancellation mechanism exists here (forcibly killing a thread mid
 * SD-card write risks corrupting more state than it fixes), so a hang
 * can't be recovered from automatically.  This threshold only controls
 * when the UI starts telling the user something is stuck, rather than
 * showing a spinner that just silently never stops.
 *
 * A legitimately large package (e.g. a game WAD) over a real Wi-Fi
 * link has been observed taking up to ~90s - the previous 120s
 * threshold meant that install could finish successfully without the
 * UI ever having reassured the user it hadn't hung.  60s comfortably
 * covers normal large installs while still catching a genuine stall
 * well before someone gives up and walks away.
 */

#define NXSTORE_INSTALL_WARN_SECONDS (60)

/* Color palette, named rather than inlined as hex literals throughout,
 * so the whole screen reads as one consistent visual system instead of
 * ad hoc per-widget colors.
 */

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

/* Sanity bound on a package icon's declared width/height (see
 * nxstore_load_icon()) - purely a defense against a garbage or
 * malicious icon file driving an oversized allocation/render, not a
 * real design constraint (icons are shown well under this size).
 */

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

/* Tracks the one install/launch operation nxstore allows at a time.  The
 * worker thread only ever writes `state` - `_Atomic` (rather than a bare
 * `volatile int`) gives that a real cross-thread happens-before guarantee
 * instead of relying on "an int-sized store happens to be atomic on this
 * target" as an unenforced assumption.  Every LVGL object touch happens
 * back on the main thread out of the polling loop in main(), since LVGL
 * itself is not thread-safe.
 */

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

  /* Written by install_worker() (background thread) once nxstore_launch()
   * returns, read by nxstore_poll_active_install() (main/LVGL thread) once
   * it observes INSTALL_STATE_DONE_OK - safe without extra synchronization
   * because `state`'s own _Atomic store/load already establishes the
   * happens-before ordering between the two.
   */

  pid_t launched_pid;
};

/* Tracks the single external process nxstore has handed the screen to
 * (e.g. nxdoom).  Only ever touched from the main/LVGL thread - see
 * nxstore_enter_running_screen().
 */

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

/* Heap-allocated (nxstore_main()'s first-run allocation in
 * build_app_store_ui()) rather than a plain static struct - each manifest
 * slot is ~1.7KB and PKG_INDEX_MAX has grown past what's comfortable to
 * carve out of internal DRAM as a fixed BSS array (see the comment on
 * PKG_INDEX_MAX in pkg.h).  calloc()/pkg_zalloc() on this board's tasks
 * draws from the PSRAM-backed user heap instead.
 */

static FAR struct pkg_index_s *g_index;
static struct install_ctx_s g_active;

/* g_main_scr is the normal app-list screen (built once in
 * build_app_store_ui()); g_run_scr is the supervisor screen shown while an
 * external app owns the framebuffer directly - see build_run_screen().
 */

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
 *   Translate a pkg_install() failure code into a message a user can act
 *   on, instead of one generic "install failed" string for every cause
 *   (network down, bad checksum, wrong board, out of space, ...).
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
 *   Transient bottom-aligned status banner, auto-dismissed after a couple
 *   seconds - a stronger, momentary "this just happened" signal than the
 *   durable per-row subtitle text, for actions (install/uninstall/launch
 *   failures) that benefit from an unmissable confirmation that a tap was
 *   registered and acted on.
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
 *   pkg_install() has no byte-level progress callback, so real percentage
 *   progress isn't available - a sliding-segment bar (the standard
 *   hand-rolled "indeterminate" pattern, since LVGL's lv_bar has no built
 *   in indeterminate mode) reads far more clearly as "actively working"
 *   than the small corner spinner it replaces, without fabricating a fake
 *   percentage.  Placed under the card's subtitle rather than over the
 *   chevron, so (unlike the spinner it replaces) it doesn't need to hide
 *   any other row content to make room for itself.
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
 *   Switches to the supervisor screen and forces that switch to actually
 *   reach the physical framebuffer before returning.  Deliberately does
 *   NOT record a pid or set g_running.active - callers that can control
 *   spawn order (the direct-launch path in install_btn_event_cb()) must
 *   call this *before* nxstore_launch(), then set g_running.pid/active
 *   themselves only once the spawn actually succeeds.  Getting this
 *   backwards (spawn first, switch screens after) leaves a real window,
 *   observed on hardware, where the newly-spawned app starts drawing
 *   into the framebuffer immediately while this task's own pending
 *   screen switch only reaches the display whenever its LVGL loop next
 *   happens to run - which could be starved indefinitely by a busy-looping
 *   child, leaving the old app-list screen visibly stuck underneath/around
 *   whatever the child draws for as long as it keeps running.
 *
 *   The async install-then-launch path (nxstore_poll_active_install(),
 *   reacting to a background thread that already called nxstore_launch()
 *   itself) can't avoid that ordering - LVGL calls aren't safe off the
 *   main thread, so the screen switch can only happen after the worker
 *   reports done - but forcing the flush here still shrinks that window
 *   to one polling tick instead of leaving it open indefinitely.
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
 *   Sends SIGTERM and waits for the app to reap itself.  This board's
 *   build has CONFIG_SIG_DEFAULT disabled (sched/Kconfig, off by
 *   default), so NuttX has *no* default action registered for SIGTERM
 *   (see the CONFIG_SIG_SIGKILL_ACTION-gated table in
 *   sched/signal/sig_default.c) - delivery only does anything for an app
 *   that explicitly installs its own handler via sigaction(), which
 *   NXDoom now does (apps/games/NXDoom/src/i_system.c,
 *   i_install_quit_signal()/i_poll_quit_signal()).
 *
 *   An earlier version of this function fell back to task_delete()
 *   (NuttX's forced-termination API) when SIGTERM didn't get a reap
 *   quickly enough.  On real hardware that force-kill landed while
 *   NXDoom was mid framebuffer/heap access and hung the *entire board*,
 *   not just the one task - confirmed by the board going completely
 *   silent on the serial console, requiring a physical power cycle to
 *   recover.  That fallback has been removed entirely: there is no safe
 *   way to force-terminate an arbitrary task from the outside on this
 *   system, so an app can only be closed if it cooperates.  If SIGTERM
 *   doesn't get reaped within the timeout, this leaves the running
 *   screen up rather than pretending success - switching back to the
 *   app list while the app might secretly still be alive and drawing
 *   into the framebuffer underneath it would just reproduce the original
 *   header-bleed-through bug.
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
      /* Nothing to signal - the child is already gone (e.g. reaped by
       * nxstore_poll_running_app()'s own waitpid() between this tap
       * landing and this handler running).  Fall straight through to
       * cleanup instead of sending a signal to a stale pid and then
       * looping on a waitpid() that can now never match.
       */

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
          /* No such child left to wait for - it already exited and was
           * reaped by someone else (again, most likely
           * nxstore_poll_running_app()'s own concurrent waitpid()).
           * This is not "still running, keep polling": there is
           * nothing left to reap, ever, so stop and clean up now
           * rather than spinning for the rest of the tries and leaving
           * the user stuck on "Still closing" for an app that is
           * already gone.
           */

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

  /* NXDoom just left pixels directly in /dev/fb0 that no LVGL object on
   * the app-list screen naturally overlaps (its own header/list content
   * doesn't cover the same area doom's viewport did) - lv_screen_load()
   * marks the screen dirty, but that only actually reaches the physical
   * framebuffer on LVGL's own schedule.  Force it to flush right now
   * instead of trusting a later lv_timer_handler() call gets to it
   * before something else (a toast, another tap) does.
   */

  lv_refr_now(NULL);

  nxstore_toast(false, "%s closed", name);
}

/****************************************************************************
 * Name: nxstore_poll_running_app
 *
 * Description:
 *   Called from the main LVGL loop.  Catches an app that exits on its own
 *   (a plain CLI demo that just runs and returns, as opposed to nxdoom
 *   which has to be force-closed via close_running_app_event_cb) so the
 *   screen returns to the app list automatically instead of being left
 *   showing a dead app's last frame with no way back short of the Close
 *   button.
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
 *   Builds the supervisor screen shown while an external app (nxdoom,
 *   etc.) owns /dev/fb0 directly.  Confined to the top 36px, which is
 *   inside the border NXDoom's centered/scaled viewport never writes to
 *   (800x480 screen, 320x200 game buffer scaled x2 = 640x400, leaving an
 *   80px left/right and 40px top/bottom margin) - so this bar coexists
 *   with whatever the child process draws into the rest of the frame
 *   buffer without either side clobbering the other.  Built once and
 *   reused rather than rebuilt per launch, since nothing about it changes
 *   other than the label text.
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

/* posix_spawn()'s default stack size (CONFIG_POSIX_SPAWN_DEFAULT_STACKSIZE,
 * used whenever the spawn attributes don't explicitly override it) is a
 * mere 2048 bytes - nowhere near enough for a real app like nxdoom, which
 * needs CONFIG_GAMES_NXDOOM_STACKSIZE=16384 just for itself.  A package
 * manifest has no field to carry its own required stack size, and there
 * is no reliable way to recover an app's Makefile-configured stack size
 * from its installed ELF file at spawn time - a generous fixed size for
 * every nxstore-launched app is the only practical option here, and costs
 * nothing at rest (stack is only reserved, not zero-filled/touched, until
 * actually used).  Getting this wrong doesn't fail loudly: the app
 * silently overflows its stack into adjacent heap memory, corrupting
 * whatever happens to live there - which on real hardware surfaced as a
 * deterministic-looking crash deep inside an unrelated kernel semaphore
 * function, nowhere near the actual bug.
 */

#define NXSTORE_LAUNCH_STACKSIZE 32768

/****************************************************************************
 * Name: nxstore_launch
 *
 * Description:
 *   Resolve the just-installed package's current payload path and run it.
 *   Reuses nxpkg's own installed-package bookkeeping rather than guessing
 *   at paths.  On success, *pid_out is set to the spawned child's pid so
 *   the caller can hand it to nxstore_enter_running_screen() for
 *   supervision (reap-on-exit / force-close).
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

  /* struct pkg_installed_db_s is ~8KB - too large for a plain stack
   * local given this function is called from the main UI thread (only
   * an 8KB task stack, CONFIG_SYSTEM_NXSTORE_STACKSIZE) as well as the
   * install worker thread (16KB).  Heap-allocate it instead.
   */

  db = pkg_zalloc(sizeof(*db));
  if (db == NULL)
    {
      pkg_error("nxstore: unable to allocate installed db buffer");
      return -ENOMEM;
    }

  installed = pkg_zalloc(sizeof(*installed));
  if (installed == NULL)
    {
      pkg_free(db);
      return -ENOMEM;
    }

  ret = pkg_metadata_load_installed(db);
  if (ret < 0)
    {
      pkg_error("nxstore: failed to load installed db: %d", ret);
      pkg_free(installed);
      pkg_free(db);
      return ret;
    }

  entry = pkg_metadata_find_installed(db, manifest->name);
  if (entry == NULL)
    {
      pkg_error("nxstore: %s not found in installed db", manifest->name);
      pkg_free(installed);
      pkg_free(db);
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

  pkg_free(db);
  if (ret < 0)
    {
      pkg_free(installed);
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
      pkg_free(installed);
      return -ret;
    }

  ret = posix_spawnattr_setstacksize(&attr, NXSTORE_LAUNCH_STACKSIZE);
  if (ret == 0)
    {
      ret = posix_spawn(&pid, path, NULL, &attr, argv, NULL);
    }

  posix_spawnattr_destroy(&attr);
  pkg_free(installed);
  if (ret != 0)
    {
      pkg_error("nxstore: posix_spawn(%s) failed: %d", path, ret);
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

  db = pkg_zalloc(sizeof(*db));
  if (db == NULL)
    {
      return false;
    }

  if (pkg_metadata_load_installed(db) < 0)
    {
      pkg_free(db);
      return false;
    }

  found = pkg_metadata_find_installed(db, manifest->name) != NULL;
  pkg_free(db);
  return found;
}

/****************************************************************************
 * Name: nxstore_is_up_to_date
 *
 * Description:
 *   nxstore_is_installed() only checks the package *name*, not which
 *   version is actually on disk.  If an older version is installed and
 *   the catalog's latest manifest for that name is newer, that made a
 *   card read as plain "Installed" with no way to tell the two apart -
 *   tapping it launched whatever old version was actually on disk, with
 *   no update indication or action at all.  Returns true only when the
 *   installed version string exactly matches manifest->version.
 *
 ****************************************************************************/

static bool nxstore_is_up_to_date(FAR const struct pkg_manifest_s *manifest)
{
  FAR struct pkg_installed_db_s *db;
  FAR struct pkg_installed_entry_s *entry;
  bool up_to_date;

  db = pkg_zalloc(sizeof(*db));
  if (db == NULL)
    {
      return false;
    }

  if (pkg_metadata_load_installed(db) < 0)
    {
      pkg_free(db);
      return false;
    }

  entry = pkg_metadata_find_installed(db, manifest->name);
  up_to_date = entry != NULL &&
              strcmp(entry->current, manifest->version) == 0;
  pkg_free(db);
  return up_to_date;
}

/****************************************************************************
 * Name: install_worker
 *
 * Description:
 *   Runs on its own thread so the LVGL loop keeps animating the progress
 *   bar while the (blocking, network- and SD-bound) install/launch calls
 *   run.
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
 *   Called from the main LVGL loop.  Reflects g_active's worker-thread
 *   state onto the button/label/progress bar, and reaps the thread once
 *   it finishes.
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
          /* A larger package (e.g. a game WAD) can legitimately take a
           * minute or more over Wi-Fi - a static "Installing..." with
           * no further feedback for that whole stretch reads as
           * frozen.  A live elapsed-time counter costs nothing (no
           * real byte-progress is threaded up from the download layer)
           * but gives continuous reassurance that something is still
           * happening, well before the "(taking a while)" threshold.
           */

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

  /* The title (name + version) was never touched by any of this - only
   * the subtitle changes here - so unlike the old single-label design
   * there's no need to reconstruct "name vVersion - ..." from scratch;
   * each case only has to say what actually changed.
   */

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

  /* A fresh successful install started out with the "not installed"
   * blue status bar (set at populate_app_list() time) - flip it to the
   * green "installed" affordance now that it's true, instead of leaving
   * it stale until the next reboot repopulates the list.  The icon
   * itself (child 0) is never state-colored - see populate_app_list()'s
   * own comment on why a real app icon can't double as that indicator -
   * status_bar (child 1) is the only thing that needs flipping here.
   */

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

        /* No "installed" toast here - the screen switch to the running
         * app itself is the confirmation, and a toast created just
         * before lv_screen_load() would be created on the screen that's
         * about to be hidden and never actually seen.
         */

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
 *   Card children are [icon][text_col][chevron]; text_col's are
 *   [title][subtitle] - see populate_app_list().  Centralizes that
 *   layout knowledge in one place rather than repeating the child
 *   indices at every call site.
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
 *   Long-press on an installed row removes it via nxpkg's "remove".
 *   Long-press (rather than a second confirmation tap) is used
 *   specifically to avoid the ambiguity of whether this LVGL version
 *   also fires LV_EVENT_CLICKED on release after a long-press - a
 *   two-step "tap again to confirm" scheme risks the same physical
 *   gesture both arming and immediately confirming itself.  A sustained
 *   long-press is already a deliberate, hard-to-trigger-by-accident
 *   gesture on its own.
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

  /* LV_EVENT_LONG_PRESSED can fire for a touch that is actually driving
   * a scroll of the enclosing list: if the drag starts slowly enough
   * that the long-press timer (400ms) elapses before the finger has
   * moved past the scroll-lock distance, LVGL hasn't committed the
   * gesture to scrolling yet and still delivers the long-press event.
   * This is what reads to a user as "it just happens when scrolling",
   * not a deliberate long-press.  Ignore the event if this input
   * device is currently attributed to scrolling any object.
   */

  if (lv_indev_get_scroll_obj(lv_indev_active()) != NULL)
    {
      return;
    }

  card = lv_event_get_target(e);

  /* Reuses the same LV_STATE_DISABLED reentrancy guard as
   * install/launch: this must not run concurrently with itself, or
   * with an in-flight install/launch for this same row.
   */

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

          /* status_bar (child 1) reverts to the "not installed"
           * affordance now that it genuinely isn't - the icon itself
           * (child 0) is never state-colored, same reasoning as the
           * install success path above.
           */

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
 *   Tapped list entry: launch directly if already installed *and*
 *   up-to-date, otherwise kick off an async install+launch (this
 *   fetches whatever version the catalog currently advertises and
 *   launches it once done, which for an already-installed-but-outdated
 *   package is exactly the "update" action) and show a progress bar
 *   while it runs.  Long-press (uninstall_btn_event_cb) removes an
 *   installed entry.
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

      /* This board's display is a single shared framebuffer that only
       * one app can own at a time, and g_running is nxstore's only
       * record of who that is.  A rapid double-tap on this exact button
       * is guarded below, but that alone isn't enough: this path used
       * to run unconditionally even while another app was already
       * running (g_running.active), or while an unrelated install
       * elsewhere in the list was about to auto-launch its own package
       * once done (install_worker() -> nxstore_launch() runs on its own
       * thread and g_active.manifest stays non-NULL for the whole
       * install+launch+settle window - see nxstore_poll_active_install()
       * - so checking it here closes the same race as checking
       * g_running.active, without needing to know which state the
       * install is currently in).  Letting either through overwrote
       * g_running with whichever launch finished last, leaving the
       * other process alive, unsupervised, and drawing into the same
       * framebuffer with no way to close it from this UI again.
       */

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

      /* Switch to (and force-flush) the running screen *before* spawning
       * - see nxstore_enter_running_screen()'s own comment for why doing
       * it the other way around left the old app-list screen visibly
       * stuck under/around whatever the newly-launched app draws.
       */

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
      /* Only one *install* can run at a time (single worker slot). */

      return;
    }

  if (g_running.active)
    {
      /* This install will auto-launch its package once done (see
       * install_worker()) - starting it while another app already owns
       * the framebuffer would just reproduce the dual-launch race
       * guarded against above, once this install finishes.
       */

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
      pkg_error("nxstore: failed to spawn install worker");
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
 *   Best-effort load of a package's optional icon (manifest->icon) into
 *   an lv_image_dsc_t, downloading and caching it under PKG_ROOT_DIR
 *   first if it isn't already cached.  The icon file format is a raw,
 *   uncompressed image matching lv_image_header_t's own 12-byte layout
 *   (magic/cf/flags, then w/h, then stride/reserved, all little-endian)
 *   followed immediately by RGB565 pixel data - deliberately the
 *   simplest thing LVGL can render directly with no decoder, since this
 *   board has no PNG/JPEG decode capability wired up.  tools/
 *   export_pkg_repo.py's --icon option writes this exact format.
 *
 *   Every failure mode (no icon set, download failed, corrupt/oversized
 *   file) just returns NULL - callers fall back to the plain colored
 *   circle + symbol glyph, so a bad icon never blocks the app from being
 *   listed or installed.
 *
 *   On success, both the returned descriptor and its data pointer are
 *   heap-allocated and intentionally never freed - the descriptor is
 *   handed straight to a permanent lv_image row widget (which keeps a
 *   pointer to it, not a copy) that lives for nxstore's whole runtime,
 *   so there's no point it's ever safe to free at.  Must be heap, not a
 *   caller's stack local: it has to outlive the populate_app_list() loop
 *   iteration that creates it.
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
  uint16_t w;
  uint16_t h;
  uint16_t stride;

  if (manifest->icon[0] == '\0')
    {
      return NULL;
    }

  snprintf(cache_path, sizeof(cache_path), PKG_ROOT_DIR "/icons/%s.bin",
          manifest->name);

  if (stat(cache_path, &st) < 0)
    {
      mkdir(PKG_ROOT_DIR "/icons", 0755);

      if (pkg_resolve_icon_source(source, sizeof(source), manifest) < 0 ||
          pkg_acquire_source(source, cache_path, NULL) < 0 ||
          stat(cache_path, &st) < 0)
        {
          return NULL;
        }
    }

  if (st.st_size <= 12 || st.st_size > 64 * 1024)
    {
      return NULL;
    }

  buf = pkg_malloc((size_t)st.st_size);
  if (buf == NULL)
    {
      return NULL;
    }

  fd = open(cache_path, O_RDONLY);
  if (fd < 0)
    {
      pkg_free(buf);
      return NULL;
    }

  nread = read(fd, buf, (size_t)st.st_size);
  close(fd);

  if (nread != st.st_size || buf[0] != LV_IMAGE_HEADER_MAGIC)
    {
      pkg_free(buf);
      return NULL;
    }

  w = (uint16_t)(buf[4] | (buf[5] << 8));
  h = (uint16_t)(buf[6] | (buf[7] << 8));
  stride = (uint16_t)(buf[8] | (buf[9] << 8));

  if (w == 0 || h == 0 || w > NXSTORE_ICON_MAX_DIM ||
      h > NXSTORE_ICON_MAX_DIM || stride != w * 2 ||
      12 + (size_t)stride * h > (size_t)nread)
    {
      pkg_free(buf);
      return NULL;
    }

  dsc = pkg_zalloc(sizeof(*dsc));
  if (dsc == NULL)
    {
      pkg_free(buf);
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

      /* The index can list multiple versions of the same package as
       * separate entries (that's exactly how `nxpkg update` finds a
       * newer version to install) - without this, every version in the
       * index got its own row ("nxdoom" showing up 3 times).  One card
       * per unique name; pkg_metadata_find_latest() (already used by
       * pkg_install.c for bare-name installs/updates, see pkg.h) picks
       * the actual newest version rather than just whichever entry
       * happened to appear first in the index.
       */

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

      /* Card: one flex row [icon circle][title+subtitle column][chevron],
       * styled as a distinct surface (rounded corners, subtle border)
       * rather than a bare list row - this is what actually reads as
       * "a real app" per entry instead of a plain text menu.
       */

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

      /* No LVGL theme is loaded (this file styles every widget itself),
       * so without an explicit LV_STATE_PRESSED variant a tap gives no
       * visual feedback at all - this is what actually confirms to the
       * user that a touch registered, before any install/launch state
       * change has had a chance to show up elsewhere on the row.
       */

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

      /* Icon circle: always shows the package's own art (icon_dsc) when
       * available, on a neutral background - install state used to be
       * conveyed by coloring this same circle, but that only worked for
       * the plain-glyph fallback; a real (opaque, same-size) icon image
       * just covered the ring color entirely, silently losing the only
       * install-state indicator in the row.  status_bar below is a
       * dedicated indicator instead, so state stays legible regardless
       * of whether a row has real art or just the fallback glyph.
       */

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

      /* Status bar: a small vertical stripe next to the icon, the only
       * thing that carries installed/not-installed state now - legible
       * at a glance without depending on the icon itself ever being
       * colorable (a real app icon's artwork is whatever it is).  A
       * third color distinguishes "installed, but an older version than
       * the catalog's latest" from a fully up-to-date install - without
       * it, an outdated install looked identical to a current one and
       * gave no hint that tapping would still launch the old payload.
       */

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

      /* Text column: title never gets overwritten by install/launch
       * status (unlike the previous single-label design) - only the
       * subtitle line changes, so which package the progress bar below
       * it belongs to stays legible the whole time.
       */

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
          /* Takes priority over the static description - an available
           * update is actionable state the user needs to see, and a
           * fixed description text would never convey it.
           */

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

      /* Chevron: a plain tap-affordance hint, decorative only - not
       * touched again after creation.  Unlike the old spinner design,
       * the install-progress bar lives in text_col below the subtitle
       * rather than over this, so nothing needs to hide it during an
       * active install.
       */

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

  g_index = pkg_zalloc(sizeof(*g_index));
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

  /* The indev-level scroll_limit/scroll_throw settings in main() only
   * raise the bar for a scroll gesture to *start* - they don't stop one
   * from happening once that bar is cleared.  Without also locking the
   * screen object itself down (examples/lvgldemo/lvgldemo.c does this
   * for its own screen; this file previously only cleared SCROLLABLE on
   * the list, not on scr), an accepted gesture scrolls the whole
   * screen's content, which looks exactly like "the display steps down"
   * on a tap and silently misaligns every row's real position from
   * where it was drawn when the user aimed - explaining reports of
   * tapping one entry and a different one installing.
   */

  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLL_CHAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_set_scroll_dir(scr, LV_DIR_NONE);
  lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

  /* Screen is a flex column [header][list]: header gets a fixed pixel
   * height, list gets flex_grow to take all remaining vertical space -
   * this resizes correctly regardless of screen resolution, unlike
   * doing height arithmetic on lv_pct() values (which encode percentage
   * specially and cannot be combined with plain pixel math).
   */

  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr, 0, 0);
  lv_obj_set_style_pad_row(scr, 0, 0);

  /* Header bar: a distinct surface (not just a label floating on the
   * background) with a bottom divider, giving the screen an actual
   * top-level structure instead of a title line directly above a list.
   */

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

  /* The list needs to scroll once there are more rows than fit on
   * screen - it was previously locked down entirely (matching scr
   * above) because a noisy touch driver could turn a tap into an
   * accidental scroll drag.  That protection actually lives at the
   * indev level (lv_indev_set_scroll_limit(20)/scroll_throw(0) in
   * main() - a real drag has to travel much further than any touch
   * jitter before a scroll starts at all), so it's safe to leave this
   * SCROLLABLE and still get that protection; only vertical dragging is
   * allowed, momentum/elastic overscroll stay off so a fast swipe can't
   * bounce past the last row on this same noisy touch driver, and
   * SCROLL_CHAIN stays off since there's nothing above this to chain
   * into (scr itself is not scrollable).
   */

  lv_obj_clear_flag(g_list, LV_OBJ_FLAG_SCROLL_CHAIN);
  lv_obj_clear_flag(g_list, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_clear_flag(g_list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(g_list, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_set_scroll_dir(g_list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(g_list, LV_SCROLLBAR_MODE_AUTO);

  /* If a repository URL was supplied, download the package listing from the
   * server over Wi-Fi before showing it.  This is the "fetch the catalog
   * from a hosted server" step of the store flow.  Falls back to whatever
   * local index already exists if the download fails (e.g. offline).
   */

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

      /* Yield once so the "Downloading..." label and spinner are painted
       * before the blocking HTTP fetch below.
       */

      lv_timer_handler();

      /* A cold boot can reach nxstore before DHCP has installed a route.
       * Retry only readiness failures for up to 15 seconds.  Checking the
       * operation's result keeps nxstore independent of board-specific
       * Wi-Fi state and works for Ethernet or other network interfaces too.
       */

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
      /* The index parsed fine but has zero usable entries (empty
       * catalog, or every entry was for a different arch/board and got
       * filtered out) - distinct from the "couldn't load an index at
       * all" case above, which needs a different message.
       */

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

  /* Optional first argument: a repository URL (http://host:port/index.json)
   * to download the package listing from over Wi-Fi.  With no argument,
   * nxstore just shows the locally-synced index.
   */

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

  /* Touch-drift mitigation: require a modest drag before scroll starts,
   * so this board's noisy touch driver can't turn a tap into an
   * accidental scroll.  examples/lvgldemo/lvgldemo.c uses 255 for this
   * same setting, which this file originally copied verbatim - but
   * unlike that demo, this screen's list is something a user actually
   * scrolls constantly, and requiring a drag that large before anything
   * visibly responds reads as broken/laggy scrolling, not drift
   * protection.  20px is enough to reject typical touch-driver jitter
   * (single-digit pixels) while still recognizing a real scroll gesture
   * almost immediately.  Momentum stays off (scroll_throw 0) - see
   * nxstore_enter_running_screen()'s and populate_app_list()'s own
   * comments on why a coasting scroll misaligning row positions caused
   * real "tapped one entry, a different one installed" reports; that
   * risk is about *momentum* continuing after release, not about the
   * gesture-start threshold, so it's independent of this value.
   */

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
