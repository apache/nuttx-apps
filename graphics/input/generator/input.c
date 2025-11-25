/****************************************************************************
 * apps/graphics/input/generator/input.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdbool.h>

#include <graphics/input_gen.h>
#include <nuttx/input/buttons.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SWIPE_DEFAULT_DURATION 300
#define DRAG_DEFAULT_DURATION 500
#define BUTTON_LONGPRESS_DEFAULT_DURATION 1000

#define DELAY_MS(ms) usleep((ms) * 1000)
#define ABS(a)       ((a) > 0 ? (a) : -(a))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct input_cmd_s
{
  /* Command name */

  const char *cmd;

  /* Function corresponding to command */

  int (*func)(input_gen_ctx_t ctx, int argc, char **argv);
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int input_button(input_gen_ctx_t ctx, int argc, char **argv);
static int input_help(input_gen_ctx_t ctx, int argc, char **argv);
static int input_sleep(input_gen_ctx_t ctx, int argc, char **argv);
static int input_utouch_tap(input_gen_ctx_t ctx, int argc, char **argv);
static int input_utouch_swipe(input_gen_ctx_t ctx, int argc, char **argv);
static int input_wheel(input_gen_ctx_t ctx, int argc, char **argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct input_cmd_s g_input_cmd_list[] =
{
  {"help",        input_help        },
  {"sleep",       input_sleep       },
  {"tap",         input_utouch_tap  },
  {"swipe",       input_utouch_swipe},
  {"drag",        input_utouch_swipe},
  {"draganddrop", input_utouch_swipe},
  {"button",      input_button      },
  {"wheel",       input_wheel       },
  {NULL,          NULL              }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: parse_arg_to_int16
 ****************************************************************************/

static int parse_arg_to_int16(const char *arg, int16_t *result)
{
  char *endptr;
  long  val;

  if (!arg || arg[0] == '\0')
    {
      return 1;
    }

  errno = 0;
  val   = strtol(arg, &endptr, 10);

  if (errno != 0 || *endptr != '\0' || arg == endptr ||
      val < -32768 || val > 32767)
    {
      return 1;
    }

  *result = (int16_t)val;
  return 0;
}

/****************************************************************************
 * Name: parse_arg_to_uint32
 ****************************************************************************/

static int parse_arg_to_uint32(const char *arg, uint32_t *result)
{
  char *endptr;
  uintmax_t val;

  if (!arg || arg[0] == '\0')
    {
      return 1;
    }

  errno = 0;
  val   = strtoumax(arg, &endptr, 0);

  if (errno != 0 || *endptr != '\0' || arg == endptr || val > UINT32_MAX)
    {
      return 1;
    }

  *result = (uint32_t)val;
  return 0;
}

/****************************************************************************
 * Name: input_sleep
 ****************************************************************************/

static int input_sleep(input_gen_ctx_t ctx, int argc, char **argv)
{
  if (argc != 2)
    {
      return -EINVAL;
    }

  DELAY_MS(atoi(argv[1]));
  return 0;
}

/****************************************************************************
 * Name: input_utouch_tap
 ****************************************************************************/

static int input_utouch_tap(input_gen_ctx_t ctx, int argc, char **argv)
{
  int16_t x;
  int16_t y;

  if (argc != 3)
    {
      return -EINVAL;
    }

  if (parse_arg_to_int16(argv[1], &x) || parse_arg_to_int16(argv[2], &y))
    {
      return -EINVAL;
    }

  return input_gen_tap(ctx, x, y);
}

/****************************************************************************
 * Name: input_utouch_swipe
 ****************************************************************************/

static int input_utouch_swipe(input_gen_ctx_t ctx, int argc, char **argv)
{
  int16_t  x1;
  int16_t  x2;
  int16_t  y1;
  int16_t  y2;
  bool     is_swipe = strcmp(argv[0], "swipe") == 0;
  uint32_t duration = is_swipe ? SWIPE_DEFAULT_DURATION
                               : DRAG_DEFAULT_DURATION;

  if (argc < 5 || argc > 6)
    {
      return -EINVAL;
    }

  if (parse_arg_to_int16(argv[1], &x1) || parse_arg_to_int16(argv[3], &x2) ||
      parse_arg_to_int16(argv[2], &y1) || parse_arg_to_int16(argv[4], &y2))
    {
      return -EINVAL;
    }

  /* Number of times to calculate reports */

  if (argc > 5)
    {
      if (parse_arg_to_uint32(argv[5], &duration))
        {
          return -EINVAL;
        }
    }

  return is_swipe ? input_gen_swipe(ctx, x1, y1, x2, y2, duration)
                  : input_gen_drag(ctx, x1, y1, x2, y2, duration);
}

/****************************************************************************
 * Name: input_button
 ****************************************************************************/

static int input_button(input_gen_ctx_t ctx, int argc, char **argv)
{
  btn_buttonset_t button;
  uint32_t duration = 0;
  int result = -EINVAL;

  if (argc < 2 || argc > 3)
    {
      return result;
    }

  if (parse_arg_to_uint32(argv[1], &button))
    {
      return result;
    }

  if (argc == 3)
    {
      if (parse_arg_to_uint32(argv[2], &duration))
        {
          return -EINVAL;
        }
    }

  if (duration)
    {
      result = input_gen_button_longpress(ctx, button, duration);
    }
  else
    {
      result = input_gen_button_click(ctx, button);
    }

  return result;
}

/****************************************************************************
 * Name: input_wheel
 ****************************************************************************/

static int input_wheel(input_gen_ctx_t ctx, int argc, char **argv)
{
  int16_t wheel;

  if (argc != 2)
    {
      return -EINVAL;
    }

  if (parse_arg_to_int16(argv[1], &wheel))
    {
      return -EINVAL;
    }

  return input_gen_mouse_wheel(ctx, wheel);
}

/****************************************************************************
 * Name: intput_help
 ****************************************************************************/

static int input_help(input_gen_ctx_t ctx, int argc, char **argv)
{
  printf("Usage: input <command> [<arg>...]\n");
  printf("The commands are:\n"
         "\thelp\n"
         "\tsleep <time(ms)>\n"
         "\ttap <x> <y>\n"
         "\tswipe <x1> <y1> <x2> <y2> [duration(ms)]\n"
         "\tdrag | draganddrop <x1> <y1> <x2> <y2> [duration(ms)]\n"
         "\tbutton <buttonvalue> [duration(ms)]\n"
         "\twheel <wheelvalue>\n"
         "\tduration: Duration of sliding\n"
         "\tbuttonvalue: button value in hex format\n"
         );

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: input_main
 *
 * Description:
 *   Main entry point for the serial terminal example.
 *
 ****************************************************************************/

int main(int argc, char **argv)
{
  int i;
  int ret = -EINVAL;
  input_gen_ctx_t input_gen_ctx;

  if (argv[1] == NULL)
    {
      goto errout;
    }

  if (input_gen_create(&input_gen_ctx, INPUT_GEN_DEV_ALL) < 0)
    {
      printf("Failed to create input generator\n");
      return EXIT_FAILURE;
    }

  for (i = 0; g_input_cmd_list[i].cmd != NULL; i++)
    {
      /* Matching the function corresponding to the command */

      if (strcmp(g_input_cmd_list[i].cmd, argv[1]) == 0)
        {
          ret = g_input_cmd_list[i].func(input_gen_ctx, argc - 1, &argv[1]);
          break;
        }
    }

  if (input_gen_destroy(input_gen_ctx) < 0)
    {
      return EXIT_FAILURE;
    }

errout:
  if (ret != 0)
    {
      input_help(NULL, argc, argv);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
