/****************************************************************************
 * apps/system/input/input.c
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
#include <unistd.h>

#include <nuttx/input/buttons.h>
#include <nuttx/input/touchscreen.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFATLT_INTERVAL 30
#define DEFATLT_DISTANCE 10

#define DELAY_MS(ms) usleep((ms) * 1000)
#define ABS(a)       ((a) > 0 ? (a) : -(a))

#ifndef MAX
#  define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct input_cmd_s
{
  const char *cmd;                    /* Command name */
  int (*func)(int argc, char **argv); /* Function corresponding to command */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int  input_utouch_tap(int argc, char **argv);
static int  input_utouch_swipe(int argc, char **argv);
static int  input_sleep(int argc, char **argv);
static int  input_help(int argc, char **argv);
static int  input_button(int argc, char **argv);
static void input_utouch_move(int fd, int x, int y, int pendown);
/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct input_cmd_s g_input_cmd_list[] =
{
  {"help",   input_help        },
  {"sleep",  input_sleep       },
  {"tap",    input_utouch_tap  },
  {"swipe",  input_utouch_swipe},
  {"button", input_button      },
  {NULL,     NULL              }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: input_utouch_move
 ****************************************************************************/

static void input_utouch_move(int fd, int x, int y, int pendown)
{
  struct touch_sample_s sample; /* Sampled touch point data */

  /* Handle the change from pen down to pen up */

  if (pendown != TOUCH_DOWN)
    {
      sample.point[0].flags = TOUCH_UP | TOUCH_ID_VALID;
    }
  else
    {
      sample.point[0].x        = x;
      sample.point[0].y        = y;
      sample.point[0].pressure = 42;
      sample.point[0].flags    = TOUCH_DOWN | TOUCH_ID_VALID |
                                 TOUCH_POS_VALID | TOUCH_PRESSURE_VALID;
    }

  sample.npoints    = 1;
  sample.point[0].h = 1;
  sample.point[0].w = 1;

  write(fd, &sample, sizeof(struct touch_sample_s));
}

/****************************************************************************
 * Name: input_sleep
 ****************************************************************************/

static int input_sleep(int argc, char **argv)
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

static int input_utouch_tap(int argc, char **argv)
{
  int fd;
  int x;
  int y;
  int interval = DEFATLT_INTERVAL;

  if (argc < 3 || argc > 4)
    {
      return -EINVAL;
    }

  if (argc == 4)
    {
      interval = atoi(argv[3]);
    }

  x = atoi(argv[1]);
  y = atoi(argv[2]);

  fd = open("/dev/utouch", O_WRONLY);
  if (fd < 0)
    {
      return -errno;
    }

  input_utouch_move(fd, x, y, TOUCH_DOWN);
  DELAY_MS(interval);
  input_utouch_move(fd, 0, 0, TOUCH_UP);
  close(fd);

  return 0;
}

/****************************************************************************
 * Name: input_utouch_swipe
 ****************************************************************************/

static int input_utouch_swipe(int argc, char **argv)
{
  int i;
  int fd;
  int x0;
  int x1;
  int y0;
  int y1;
  int count;
  int duration;
  int distance = DEFATLT_DISTANCE;
  int interval = DEFATLT_INTERVAL;

  if (argc < 5 || argc > 7)
    {
      return -EINVAL;
    }

  x0 = atoi(argv[1]);
  x1 = atoi(argv[3]);
  y0 = atoi(argv[2]);
  y1 = atoi(argv[4]);

  fd = open("/dev/utouch", O_WRONLY);
  if (fd < 0)
    {
      return -errno;
    }

  /* Number of times to calculate reports */

  if (argc >= 5)
    {
      duration = atoi(argv[5]);
    }

  if (argc >= 6)
    {
      distance = atoi(argv[6]);
      distance = distance == 0 ? DEFATLT_DISTANCE : distance;
    }

  count    = MAX(ABS(x0 - x1), ABS(y0 - y1)) / distance;
  interval = duration == 0 ? DEFATLT_INTERVAL : duration / count;

  for (i = 0; i <= count; i++)
    {
      int x = x0 + (x1 - x0) / count * i;
      int y = y0 + (y1 - y0) / count * i;

      input_utouch_move(fd, x, y, TOUCH_DOWN);
      DELAY_MS(interval);
    }

  input_utouch_move(fd, 0, 0, TOUCH_UP);
  close(fd);
  return 0;
}

static int input_button(int argc, char **argv)
{
  int fd;
  btn_buttonset_t button;

  if (argc != 2)
    {
      return -EINVAL;
    }

  button = strtoul(argv[1], NULL, 0);
  fd = open("/dev/ubutton", O_WRONLY);
  if (fd < 0)
    {
      return -errno;
    }

  write(fd, &button, sizeof(button));
  close(fd);

  return 0;
}

/****************************************************************************
 * Name: intput_help
 ****************************************************************************/

static int input_help(int argc, char **argv)
{
  printf("Usage: input <command> [<arg>...]\n");
  printf("The commands and default sources are:\n"
         "\thelp\n"
         "\tsleep <time(ms)>\n"
         "\ttap <x> <y> [interval(ms)]\n"
         "\tswipe <x1> <y1> <x2> <y2> [duration(ms)] [distance(pix)]\n"
         "\tbutton <buttonvalue>\n"
         "\tinterval: Time interval between two reports of sample\n"
         "\tduration: Duration of sliding\n"
         "\tdistance: Report the pixel distance of the sample twice\n"
         "\tbuttonvalue: button value in hex format\n");

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

  if (argv[1] == NULL)
    {
      goto errout;
    }

  for (i = 0; g_input_cmd_list[i].cmd != NULL; i++)
    {
      /* Matching the function corresponding to the command */

      if (strcmp(g_input_cmd_list[i].cmd, argv[1]) == 0)
        {
          ret = g_input_cmd_list[i].func(argc - 1, &argv[1]);
          break;
        }
    }

errout:
  if (ret != 0)
    {
      input_help(argc, argv);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
