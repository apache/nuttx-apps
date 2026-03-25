/****************************************************************************
 * apps/graphics/input/monkey/monkey_main.c
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
#include <nuttx/video/fb.h>
#include <nuttx/input/touchscreen.h>
#include <nuttx/input/mouse.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "monkey.h"
#include "monkey_utils.h"
#include "monkey_log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MONKEY_PREFIX "monkey"

#define CONSTRAIN(x, low, high) \
  ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))

#define OPTARG_TO_VALUE(value, type, base)                             \
  do                                                                   \
    {                                                                  \
      FAR char *ptr;                                                   \
      (value) = (type)strtoul(optarg, &ptr, (base));                   \
      if (*ptr != '\0')                                                \
        {                                                              \
          MONKEY_LOG_ERROR("Parameter error: -%c %s", ch, optarg);     \
          show_usage(argv[0], EXIT_FAILURE);                           \
        }                                                              \
    } while (0)

#define OPTARG_TO_RANGE(value_1, value_2, delimiter)                   \
  do                                                                   \
    {                                                                  \
      int converted;                                                   \
      int v1;                                                          \
      int v2;                                                          \
      converted = sscanf(optarg, "%d" delimiter "%d", &v1, &v2);       \
      if (converted == 2 && v1 >= 0 && v2 >= 0)                        \
        {                                                              \
          value_1 = v1;                                                \
          value_2 = v2;                                                \
        }                                                              \
      else                                                             \
        {                                                              \
          MONKEY_LOG_ERROR("Error range: %s", optarg);                 \
          show_usage(argv[0], EXIT_FAILURE);                           \
        }                                                              \
    } while (0)

/* Default parameters */

#if defined(CONFIG_VIDEO_FB)
#  define MONKEY_SCREEN_DEV "/dev/fb0"
#  define MONKEY_SCREEN_GETVIDEOINFO FBIOGET_VIDEOINFO
#elif defined(CONFIG_LCD)
#  define MONKEY_SCREEN_DEV "/dev/lcd0"
#  define MONKEY_SCREEN_GETVIDEOINFO LCDDEVIO_GETVIDEOINFO
#endif

#define MONKEY_RUNNING_MINUTES_MAX (UINT32_MAX / 60 / 1000)

#define MONKEY_EXIT_DEV_PATH_DEFAULT "/dev/input0"
#define MONKEY_EXIT_LONGPRESS_MS     2500

#define MONKEY_SCREEN_HOR_RES_DEFAULT 480
#define MONKEY_SCREEN_VER_RES_DEFAULT 480

#define MONKEY_PERIOD_MIN_DEFAULT 100
#define MONKEY_PERIOD_MAX_DEFAULT 500
#define MONKEY_BUTTON_BIT_DEFAULT 0

#define MONKEY_EVENT_CLICK_WEIGHT_DEFAULT 70
#define MONKEY_EVENT_LONG_PRESS_WEIGHT_DEFAULT 10
#define MONKEY_EVENT_DRAG_WEIGHT_DEFAULT 20

#define MONKEY_EVENT_CLICK_DURATION_MIN_DEFAULT 50
#define MONKEY_EVENT_CLICK_DURATION_MAX_DEFAULT 200
#define MONKEY_EVENT_LONG_PRESS_DURATION_MIN_DEFAULT 400
#define MONKEY_EVENT_LONG_PRESS_DURATION_MAX_DEFAULT 600
#define MONKEY_EVENT_DRAG_DURATION_MIN_DEFAULT 100
#define MONKEY_EVENT_DRAG_DURATION_MAX_DEFAULT 400

#define MONKEY_EVENT_PARAM_INIT(name)                      \
  do                                                       \
    {                                                      \
      param->event[MONKEY_EVENT_##name].weight =           \
        MONKEY_EVENT_##name##_WEIGHT_DEFAULT;              \
      param->event[MONKEY_EVENT_##name].duration_min =     \
        MONKEY_EVENT_##name##_DURATION_MIN_DEFAULT;        \
      param->event[MONKEY_EVENT_##name].duration_max =     \
        MONKEY_EVENT_##name##_DURATION_MAX_DEFAULT;        \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct monkey_param_s
{
  int dev_type_mask;
  FAR const char *file_path;
  FAR const char *exit_dev_path;
  int x_offset;
  int y_offset;
  int hor_res;
  int ver_res;
  int period_min;
  int period_max;
  int btn_bit;
  int log_level;
  struct monkey_event_config_s event[MONKEY_EVENT_LAST];
  uint32_t running_minutes;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("\nUsage: %s"
         " -t <hex-value>"
         " -f <string>"
         " -p <string>"
         " -s <string>"
         " -b <decimal-value>"
         " -l <decimal-value>"
         " -e <string>\n"
         " --weight-click <decimal-value>"
         " --weight-longpress <decimal-value>"
         " --weight-drag <decimal-value>\n"
         " --duration-click <string>"
         " --duration-longpress <string>"
         " --duration-drag <string>\n"
         " --screen-offset <string>\n"
         " --running-minutes <decimal-value>\n",
         progname);

  printf("\nWhere:\n");

  printf("  -t <hex-value> Device type mask: uinput = 0x%02X;"
         " touch = 0x%02X; button = 0x%02X.\n",
         MONKEY_UINPUT_TYPE_MASK,
         MONKEY_DEV_TYPE_TOUCH,
         MONKEY_DEV_TYPE_BUTTON);
  printf("  -f <string> Recorder playback file path.\n");
  printf("  -p <string> Period(ms) range: "
         "<decimal-value min>-<decimal-value max>\n");
  printf("  -s <string> Screen resolution: "
         "<decimal-value hor_res>x<decimal-value ver_res>\n");
  printf("  -b <decimal-value> Button bit: 0 ~ 31\n");
  printf("  -l <decimal-value> Log level: 0 ~ 3\n");
  printf("  -e <string> Exit device path for long-press detection"
         " (default: %s).\n", MONKEY_EXIT_DEV_PATH_DEFAULT);

  printf("  --weight-click <decimal-value> Click event weight.\n");
  printf("  --weight-longpress <decimal-value> Long press event weight.\n");
  printf("  --weight-drag <decimal-value> Drag event weight.\n");

  printf("  --duration-click <string> Click duration(ms) range: "
         "<decimal-value min>-<decimal-value max>.\n");
  printf("  --duration-longpress <string> Long press duration(ms) range: "
         "<decimal-value min>-<decimal-value max>.\n");
  printf("  --duration-drag <string> Drag duration(ms) range: "
         "<decimal-value min>-<decimal-value max>.\n");
  printf("  --screen-offset <string> Screen offset: "
         "<decimal-value x_offset>,<decimal-value y_offset>\n");
  printf("  --running-minutes <decimal-value> Running minutes.\n");

  exit(exitcode);
}

/****************************************************************************
 * Name: monkey_get_screen_resolution
 ****************************************************************************/

static int monkey_get_screen_resolution(FAR int *hor_res, FAR int *ver_res)
{
#ifdef MONKEY_SCREEN_DEV
  struct fb_videoinfo_s vinfo;
  int fd;
  int ret;
  FAR const char *dev_path = MONKEY_SCREEN_DEV;

  *hor_res = MONKEY_SCREEN_HOR_RES_DEFAULT;
  *ver_res = MONKEY_SCREEN_VER_RES_DEFAULT;
  fd = open(dev_path, 0);

  if (fd < 0)
    {
      MONKEY_LOG_ERROR("screen dev %s open failed: %d", dev_path, errno);
      return ERROR;
    }

  ret = ioctl(fd, MONKEY_SCREEN_GETVIDEOINFO,
              (unsigned long)((uintptr_t)&vinfo));

  if (ret < 0)
    {
      MONKEY_LOG_ERROR("get video info failed: %d", errno);
    }
  else
    {
      *hor_res = vinfo.xres;
      *ver_res = vinfo.yres;
    }

  close(fd);
  return ret;
#else
  *hor_res = MONKEY_SCREEN_HOR_RES_DEFAULT;
  *ver_res = MONKEY_SCREEN_VER_RES_DEFAULT;
  return OK;
#endif /* MONKEY_SCREEN_DEV */
}

/****************************************************************************
 * Name: monkey_set_timer
 ****************************************************************************/

static void monkey_set_timer(int tfd, uint32_t ms)
{
  struct itimerspec its;

  memset(&its, 0, sizeof(its));
  its.it_value.tv_sec = ms / 1000;
  its.it_value.tv_nsec = (ms % 1000) * 1000000;
  timerfd_settime(tfd, 0, &its, NULL);
}

/****************************************************************************
 * Name: monkey_check_longpress
 ****************************************************************************/

static bool monkey_check_longpress(int dev_fd,
                                   FAR struct monkey_s *monkey)
{
  bool is_pressed = false;

  /* Drain all buffered events, use the latest state */

  if (monkey->exit_ctx.is_mouse)
    {
      struct mouse_report_s report;

      while (read(dev_fd, &report, sizeof(report)) == sizeof(report))
        {
          is_pressed = (report.buttons & MOUSE_BUTTON_1) != 0;
        }
    }
  else
    {
      struct touch_sample_s sample;

      while (read(dev_fd, &sample, sizeof(sample)) == sizeof(sample))
        {
          is_pressed = (sample.point[0].flags
                       & (TOUCH_DOWN | TOUCH_MOVE)) != 0;
        }
    }

  if (is_pressed)
    {
      if (monkey->exit_ctx.tick == 0)
        {
          monkey->exit_ctx.tick = monkey_tick_get();
        }

      if (monkey_tick_elaps(monkey_tick_get(), monkey->exit_ctx.tick)
          >= MONKEY_EXIT_LONGPRESS_MS)
        {
          MONKEY_LOG_WARN("long press detected, exiting");
          return true;
        }
    }
  else
    {
      monkey->exit_ctx.tick = 0;
    }

  return false;
}

/****************************************************************************
 * Name: monkey_init_fds
 ****************************************************************************/

static void monkey_init_fds(
    FAR struct monkey_s *monkey,
    FAR const char *dev_path)
{
  FAR struct pollfd *pfds = monkey->pfds;
  int grab_cmd = TSIOC_GRAB;
  sigset_t mask;
  int i;

  monkey->exit_ctx.is_mouse = false;
  monkey->exit_ctx.tick = 0;

  for (i = 0; i < MONKEY_POLL_COUNT; i++)
    {
      pfds[i].fd = -1;
      pfds[i].events = POLLIN;
    }

  /* Create timerfd for monkey_update interval */

  pfds[MONKEY_POLL_TIMER].fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (pfds[MONKEY_POLL_TIMER].fd < 0)
    {
      MONKEY_LOG_ERROR("timerfd_create failed: %d", errno);
    }

  /* Create signalfd for SIGTSTP/SIGCONT/SIGTERM */

  sigemptyset(&mask);
  sigaddset(&mask, SIGTSTP);
  sigaddset(&mask, SIGCONT);
  sigaddset(&mask, SIGTERM);

  /* Override default stop action before blocking, otherwise
   * nxsig_stop_task will suspend the process on SIGTSTP.
   */

  signal(SIGTSTP, SIG_IGN);
  signal(SIGCONT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  sigprocmask(SIG_BLOCK, &mask, NULL);

  pfds[MONKEY_POLL_SIGNAL].fd = signalfd(-1, &mask, 0);
  if (pfds[MONKEY_POLL_SIGNAL].fd < 0)
    {
      MONKEY_LOG_WARN("signalfd failed: %d", errno);
    }

  /* Open input device for long press detection */

  pfds[MONKEY_POLL_INPUT].fd = open(dev_path, O_RDONLY | O_NONBLOCK);
  if (pfds[MONKEY_POLL_INPUT].fd < 0)
    {
      MONKEY_LOG_WARN("open %s failed: %d", dev_path, errno);
      return;
    }

  if (strstr(dev_path, "mouse") != NULL)
    {
      monkey->exit_ctx.is_mouse = true;
      grab_cmd = MSIOC_GRAB;
    }

  if (ioctl(pfds[MONKEY_POLL_INPUT].fd, grab_cmd, 1) < 0)
    {
      MONKEY_LOG_WARN("GRAB %s failed: %d", dev_path, errno);
      close(pfds[MONKEY_POLL_INPUT].fd);
      pfds[MONKEY_POLL_INPUT].fd = -1;
      return;
    }

  MONKEY_LOG_NOTICE("exit monitor: dev=%s(%s), threshold=%dms",
                    dev_path,
                    monkey->exit_ctx.is_mouse ? "mouse" : "touch",
                    MONKEY_EXIT_LONGPRESS_MS);
}

/****************************************************************************
 * Name: monkey_init
 ****************************************************************************/

static FAR struct monkey_s *monkey_init(
                            FAR const struct monkey_param_s *param)
{
  FAR struct monkey_s *monkey;
  struct monkey_config_s config;
  int i;

  if (!param->dev_type_mask)
    {
      show_usage(MONKEY_PREFIX, EXIT_FAILURE);
    }

  monkey = monkey_create(param->dev_type_mask);

  if (!monkey)
    {
      goto failed;
    }

  monkey_config_default_init(&config);
  config.screen.x_offset = param->x_offset;
  config.screen.y_offset = param->y_offset;
  config.screen.hor_res = param->hor_res;
  config.screen.ver_res = param->ver_res;
  config.period.min = param->period_min;
  config.period.max = param->period_max;
  config.btn_bit = param->btn_bit;
  memcpy(config.event, param->event, sizeof(config.event));
  monkey_set_config(monkey, &config);
  monkey_log_set_level(param->log_level);

  MONKEY_LOG_NOTICE("Screen: %dx%d, offset: %d,%d",
                    config.screen.hor_res,
                    config.screen.ver_res,
                    config.screen.x_offset,
                    config.screen.y_offset);
  MONKEY_LOG_NOTICE("Period: %" PRIu32 " ~ %" PRIu32 "ms",
                    config.period.min,
                    config.period.max);
  MONKEY_LOG_NOTICE("Button bit: %d", config.btn_bit);
  MONKEY_LOG_NOTICE("Log level: %d", monkey_log_get_level());

  for (i = 0; i < MONKEY_EVENT_LAST; i++)
    {
      MONKEY_LOG_NOTICE("Event %d(%s): weight=%d"
                        " duration %d ~ %dms",
                        i,
                        monkey_event_type2name(i),
                        config.event[i].weight,
                        config.event[i].duration_min,
                        config.event[i].duration_max);
    }

  MONKEY_LOG_NOTICE("Running minutes: %" PRIu32, param->running_minutes);

  if (MONKEY_IS_UINPUT_TYPE(param->dev_type_mask))
    {
      if (param->file_path)
        {
          monkey_set_mode(monkey, MONKEY_MODE_PLAYBACK);
          if (!monkey_set_recorder_path(monkey, param->file_path))
            {
              goto failed;
            }
        }
      else
        {
          monkey_set_mode(monkey, MONKEY_MODE_RANDOM);
        }
    }
  else
    {
      monkey_set_mode(monkey, MONKEY_MODE_RECORD);
      if (!monkey_set_recorder_path(monkey,
                                  CONFIG_GRAPHICS_INPUT_MONKEY_REC_DIR_PATH))
        {
          goto failed;
        }

      monkey_set_period(monkey, config.period.min);
    }

  monkey_init_fds(monkey, param->exit_dev_path);
  monkey_set_timer(monkey->pfds[MONKEY_POLL_TIMER].fd, 1);

  return monkey;

failed:
  if (monkey)
    {
      monkey_delete(monkey);
    }

  return NULL;
}

/****************************************************************************
 * Name: parse_long_commandline
 ****************************************************************************/

static void parse_long_commandline(int argc, FAR char **argv,
                                   int ch,
                                   FAR const struct option *longopts,
                                   int longindex,
                                   FAR struct monkey_param_s *param)
{
  int event_index;

  switch (longindex)
    {
      case 0:
      case 1:
      case 2:
        event_index = longindex;
        OPTARG_TO_VALUE(param->event[event_index].weight, uint8_t, 10);
        break;

      case 3:
      case 4:
      case 5:
        event_index = longindex - 3;
        OPTARG_TO_RANGE(param->event[event_index].duration_min,
                        param->event[event_index].duration_max,
                        "-");
        break;

      case 6:
        OPTARG_TO_RANGE(param->x_offset,
                        param->y_offset,
                        ",");
        break;

      case 7:
        OPTARG_TO_VALUE(param->running_minutes, uint32_t, 10);

        if (param->running_minutes > MONKEY_RUNNING_MINUTES_MAX)
          {
            MONKEY_LOG_WARN("Running minutes must be less than %d",
                            MONKEY_RUNNING_MINUTES_MAX);
            param->running_minutes = MONKEY_RUNNING_MINUTES_MAX;
          }
        break;

      default:
        MONKEY_LOG_WARN("Unknown longindex: %d", longindex);
        show_usage(argv[0], EXIT_FAILURE);
        break;
    }
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(int argc, FAR char **argv,
                              FAR struct monkey_param_s *param)
{
  int ch;
  int longindex = 0;
  FAR const char *optstring = "t:f:p:s:b:l:e:";
  const struct option longopts[] =
    {
      {"weight-click",       required_argument, NULL, 0 },
      {"weight-longpress",   required_argument, NULL, 0 },
      {"weight-drag",        required_argument, NULL, 0 },
      {"duration-click",     required_argument, NULL, 0 },
      {"duration-longpress", required_argument, NULL, 0 },
      {"duration-drag",      required_argument, NULL, 0 },
      {"screen-offset",      required_argument, NULL, 0 },
      {"running-minutes",    required_argument, NULL, 0 },
      {0,                    0,                 NULL, 0 }
    };

  memset(param, 0, sizeof(struct monkey_param_s));
  monkey_get_screen_resolution(&param->hor_res, &param->ver_res);
  param->period_min = MONKEY_PERIOD_MIN_DEFAULT;
  param->period_max = MONKEY_PERIOD_MAX_DEFAULT;
  param->btn_bit = MONKEY_BUTTON_BIT_DEFAULT;
  param->log_level = MONKEY_LOG_LEVEL_NOTICE;
  param->exit_dev_path = MONKEY_EXIT_DEV_PATH_DEFAULT;
  MONKEY_EVENT_PARAM_INIT(CLICK);
  MONKEY_EVENT_PARAM_INIT(LONG_PRESS);
  MONKEY_EVENT_PARAM_INIT(DRAG);

  while ((ch = getopt_long(argc, argv,
                           optstring, longopts, &longindex)) != ERROR)
    {
      switch (ch)
        {
          case 0:
            parse_long_commandline(argc, argv, ch,
                                   longopts, longindex, param);
            break;

          case 't':
            OPTARG_TO_VALUE(param->dev_type_mask, int, 16);
            break;

          case 'f':
            param->file_path = optarg;
            break;

          case 'p':
            OPTARG_TO_RANGE(param->period_min, param->period_max, "-");
            break;

          case 's':
            OPTARG_TO_RANGE(param->hor_res, param->ver_res, "x");
            break;

          case 'b':
            OPTARG_TO_VALUE(param->btn_bit, int, 10);
            param->btn_bit = CONSTRAIN(param->btn_bit, 0, 31);
            break;

          case 'l':
            OPTARG_TO_VALUE(param->log_level, int, 10);
            param->log_level = CONSTRAIN(param->log_level, 0,
                                         MONKEY_LOG_LEVEL_LAST - 1);
            break;

          case 'e':
            param->exit_dev_path = optarg;
            break;

          case '?':
            MONKEY_LOG_WARN("Unknown option: %c", optopt);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: monkey_main
 *
 * Description:
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
  struct monkey_param_s param;
  FAR struct monkey_s *monkey;
  uint32_t start_tick;
  bool paused = false;
  int sleep_ms = 0;
  int i;

  parse_commandline(argc, argv, &param);

  monkey = monkey_init(&param);
  if (!monkey)
    {
      return ERROR;
    }

  start_tick = monkey_tick_get();

  for (; ; )
    {
      if (poll(monkey->pfds, MONKEY_POLL_COUNT, 100) < 0)
        {
          MONKEY_LOG_ERROR("poll error: %d", errno);
          break;
        }

      /* Check long press on every poll wakeup */

      if (monkey->exit_ctx.tick > 0
          && monkey_tick_elaps(monkey_tick_get(), monkey->exit_ctx.tick)
          >= MONKEY_EXIT_LONGPRESS_MS)
        {
          MONKEY_LOG_WARN("long press detected, exiting");
          break;
        }

      /* Handle signal events */

      if (monkey->pfds[MONKEY_POLL_SIGNAL].revents & POLLIN)
        {
          struct signalfd_siginfo ssi;

          read(monkey->pfds[MONKEY_POLL_SIGNAL].fd, &ssi, sizeof(ssi));

          if (ssi.ssi_signo == SIGTERM)
            {
              break;
            }
          else if (ssi.ssi_signo == SIGTSTP)
            {
              MONKEY_LOG_NOTICE("pause");
              paused = true;
              monkey_set_timer(monkey->pfds[MONKEY_POLL_TIMER].fd, 0);
            }
          else if (ssi.ssi_signo == SIGCONT && paused)
            {
              MONKEY_LOG_NOTICE("continue...");
              paused = false;
              monkey_set_timer(monkey->pfds[MONKEY_POLL_TIMER].fd,
                               sleep_ms == 0 ? 1 : sleep_ms);
            }
        }

      /* Handle timer events - time to run monkey_update */

      if (monkey->pfds[MONKEY_POLL_TIMER].revents & POLLIN)
        {
          uint64_t exp;

          read(monkey->pfds[MONKEY_POLL_TIMER].fd, &exp, sizeof(exp));

          if (param.running_minutes > 0)
            {
              uint32_t elaps = monkey_tick_elaps(monkey_tick_get(),
                                                 start_tick);

              if (elaps > param.running_minutes * 60 * 1000)
                {
                  MONKEY_LOG_WARN("Running time is over: %"
                                  PRIu32 " minutes, exit...",
                                  param.running_minutes);
                  break;
                }
            }

          sleep_ms = monkey_update(monkey);
          if (sleep_ms < 0)
            {
              break;
            }

          monkey_set_timer(monkey->pfds[MONKEY_POLL_TIMER].fd,
                           sleep_ms == 0 ? 1 : sleep_ms);
        }

      /* Handle input events - long press detection */

      if (monkey->pfds[MONKEY_POLL_INPUT].revents & POLLIN)
        {
          if (monkey_check_longpress(monkey->pfds[MONKEY_POLL_INPUT].fd,
                                     monkey))
            {
              break;
            }
        }
    }

  for (i = 0; i < MONKEY_POLL_COUNT; i++)
    {
      if (monkey->pfds[i].fd >= 0)
        {
          close(monkey->pfds[i].fd);
        }
    }

  monkey_delete(monkey);
  return OK;
}
