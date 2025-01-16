/****************************************************************************
 * apps/examples/lp503x/lp503x_main.c
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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "system/readline.h"

#include <nuttx/leds/lp503x.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LP503X_HELP_TEXT(x) x

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef CODE int (*lp503x_func)(FAR char *pargs);

struct lp503x_cmd_s
{
  FAR const char *cmd;     /* The command text           */
  FAR const char *arghelp; /* Text describing the args   */
  lp503x_func  pfunc;      /* Pointer to command handler */
  FAR const char *help;    /* The help text              */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int lp503x_cmd_quit(FAR char *parg);
static int lp503x_cmd_help(FAR char *parg);
static int lp503x_cmd_set_rgbled(FAR char *parg);
static int lp503x_cmd_setled(FAR char *parg);
static int lp503x_cmd_setcolour(FAR char *parg);
static int lp503x_cmd_pattern(FAR char *parg);
static int lp503x_cmd_brightness(FAR char *parg);
static int lp503x_cmd_mode(FAR char *parg);
static int lp503x_cmd_disable(FAR char *parg);
static int lp503x_cmd_set_bank_mode(FAR char *parg);
static int lp503x_cmd_set_banka_colour(FAR char *parg);
static int lp503x_cmd_set_bankb_colour(FAR char *parg);
static int lp503x_cmd_set_bankc_colour(FAR char *parg);
static int lp503x_cmd_set_bank_brightness(FAR char *parg);
static int lp503x_cmd_set_individual_mode(FAR char *parg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

int fd;

static struct lp503x_cmd_s g_lp503x_cmds[] =
{
  {
    "q",
    "",
    lp503x_cmd_quit,
    LP503X_HELP_TEXT("Exit lp503x")
  },
  {
    "h",
    "",
    lp503x_cmd_help,
    LP503X_HELP_TEXT("Display help for commands")
  },
  {
    "?",
    "",
    lp503x_cmd_help,
    LP503X_HELP_TEXT("Display help for commands")
  },
  {
    "help",
    "",
    lp503x_cmd_help,
    LP503X_HELP_TEXT("Display help for commands")
  },
  {
    "m",
    "",
    lp503x_cmd_mode,
    LP503X_HELP_TEXT("send default mode to the device")
  },
  {
    "c",
    "hex",
    lp503x_cmd_setcolour,
    LP503X_HELP_TEXT("Hex RGB colour required")
  },
  {
    "l",
    "led, 0..11",
    lp503x_cmd_set_rgbled,
    LP503X_HELP_TEXT("set RGB LED selected to current colour")
  },
  {
    "n",
    "led, 0..35",
    lp503x_cmd_setled,
    LP503X_HELP_TEXT("set individual LED to current brightness X")
  },
  {
    "p",
    "num of leds in pattern (1..12)",
    lp503x_cmd_pattern,
    LP503X_HELP_TEXT("play LED pattern")
  },
  {
    "b",
    "brightness, 0-255",
    lp503x_cmd_brightness,
    LP503X_HELP_TEXT("set brightness of all leds")
  },
  {
    "d",
    "disable=1",
    lp503x_cmd_disable,
    LP503X_HELP_TEXT("globally disable ALL leds")
  },
  {
    "k",
    "led num",
    lp503x_cmd_set_bank_mode,
    LP503X_HELP_TEXT("enable bank mode for LED ")
  },
  {
    "i",
    "led num 0..11",
    lp503x_cmd_set_individual_mode,
    LP503X_HELP_TEXT("enable individual mode for LED ")
  },
  {
    "A",
    "",
    lp503x_cmd_set_banka_colour,
    LP503X_HELP_TEXT("set bankA colour")
  },
  {
    "B",
    "",
    lp503x_cmd_set_bankb_colour,
    LP503X_HELP_TEXT("set bankB colour")
  },
    {
    "C",
    "",
    lp503x_cmd_set_bankc_colour,
    LP503X_HELP_TEXT("set bankC colour")
  },
  {
    "X",
    "0..255",
    lp503x_cmd_set_bank_brightness,
    LP503X_HELP_TEXT("Bank or individual LED brightness required")
  },
};

static const int g_lp503x_cmd_count = sizeof(g_lp503x_cmds) /
                                      sizeof(struct lp503x_cmd_s);
FAR static struct lp503x_config_s *lp503x_config;
FAR static struct ioctl_arg_s *lp503x_ioctl_args;
int current_colour;
int current_led_brightness;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name:lp503x_cmd_mode
 *
 * send default (Kconfig)  mode/config to the device
 ****************************************************************************/

static int lp503x_cmd_mode(FAR char *parg)
{
  return ioctl(fd, PWMIOC_CONFIG, lp503x_config);
}

/****************************************************************************
 * Name:lp503x_cmd_quit
 *
 *   lp503x_cmd_quit() terminates the application
 ****************************************************************************/

static int lp503x_cmd_quit(FAR char *parg)
{
  return OK;
}

/****************************************************************************
 * Name:lp503x_cmd_disable
 *
 *   Globally disable all LEDS
 ****************************************************************************/

static int lp503x_cmd_disable(FAR char *parg)
{
  return ioctl(fd, PWMIOC_ENABLE, atoi(parg)) ;
}

/****************************************************************************
 * Name:lp503x_setled
 *
 *   sets the chosen led (0..35) to the current brightness (0..255) (T)
 ****************************************************************************/

static int lp503x_cmd_setled(FAR char *parg)
{
  int ret;
  int lednum;
  lednum = atoi(parg);
  if (lednum > MAX_LEDS)
    {
      printf("ERROR: led number must be in range 0..35\n");
      ret = -EINVAL;
    }
  else
    {
      lp503x_ioctl_args->lednum = lednum;
      lp503x_ioctl_args->param = current_led_brightness;
      ret = ioctl(fd, PWMIOC_SET_LED_COLOUR, lp503x_ioctl_args);
    }

  return ret;
}

/****************************************************************************
 * Name:lp503x_setRGBled
 *
 *   sets the chosen RGB led (0..11) to the current RGB colour
 ****************************************************************************/

static int lp503x_cmd_set_rgbled(FAR char *parg)
{
  int ret;
  int lednum;

  lednum = atoi(parg);
  if (lednum > MAX_RGB_LEDS)
    {
      printf("ERROR: RGB led must be in range 0..11\n");
      ret = -EINVAL;
    }
  else
    {
      lp503x_ioctl_args->lednum = lednum;
      lp503x_ioctl_args->param = current_colour;
      ret = ioctl(fd, PWMIOC_SET_RGB_COLOUR, lp503x_ioctl_args);
    }

  return ret;
}

/****************************************************************************
 * Name:lp503x_cmd_set_bank_mode
 *
 *   sets requested led to use "bank" mode
 ****************************************************************************/

static int lp503x_cmd_set_bank_mode(FAR char *parg)
{
  int ret;
  int lednum;

  lednum = atoi(parg);
  if (lednum > MAX_RGB_LEDS)
    {
      printf("ERROR: RGB led must be in range 0..11\n");
      ret = -EINVAL;
    }
  else
    {
      lp503x_ioctl_args->param = LP503X_LED_BANK_MODE_ENABLED;
      lp503x_ioctl_args->lednum = lednum;
      ret = ioctl(fd, PWMIOC_ENABLE_LED_BANK_MODE, lp503x_ioctl_args);
    }

  return ret;
}

/****************************************************************************
 * Name:lp503x_cmd_set_individual_mode
 *
 *   sets requested led to use "individual" mode (default)
 ****************************************************************************/

static int lp503x_cmd_set_individual_mode(FAR char *parg)
{
  int ret;
  int lednum;

  lednum = atoi(parg);
  if (lednum > MAX_RGB_LEDS)
    {
      printf("ERROR: RGB led must be in range 0..11\n");
      ret = -EINVAL;
    }
  else
    {
      lp503x_ioctl_args->param = LP503X_LED_BANK_MODE_DISABLED;
      lp503x_ioctl_args->lednum = lednum;
      ret = ioctl(fd, PWMIOC_ENABLE_LED_BANK_MODE, lp503x_ioctl_args);
    }

  return ret;
}

/****************************************************************************
 * Name:lp503x_cmd_set_bank?_colour
 *
 *   sets bank A/B/C to the current brightness (T)
 ****************************************************************************/

static int lp503x_cmd_set_banka_colour(FAR char *parg)
{
  uint8_t requested_colour;

  requested_colour = atoi(parg);

  if ((requested_colour > MAX_BRIGHTNESS) || (requested_colour < 0))
    {
      printf("ERR: Bank Colour (mix percent) must be in range 0..255\n");
      return -EINVAL;
    }
  else
    {
      lp503x_ioctl_args->param = requested_colour;
      lp503x_ioctl_args->lednum = 'A';
      return ioctl(fd, PWMIOC_SET_BANK_MIX_COLOUR, lp503x_ioctl_args);
    }
}

static int lp503x_cmd_set_bankb_colour(FAR char *parg)
{
  uint8_t requested_colour;

  requested_colour = atoi(parg);

  if ((requested_colour > MAX_BRIGHTNESS) || (requested_colour < 0))
    {
      printf("ERR: Bank Colour (mix percent) must be in range 0..255\n");
      return -EINVAL;
    }
  else
    {
      lp503x_ioctl_args->param = requested_colour;
      lp503x_ioctl_args->lednum = 'B';
      return ioctl(fd, PWMIOC_SET_BANK_MIX_COLOUR, lp503x_ioctl_args);
    }
}

static int lp503x_cmd_set_bankc_colour(FAR char *parg)
{
  uint8_t requested_colour;

  requested_colour = atoi(parg);

  if ((requested_colour > MAX_BRIGHTNESS) || (requested_colour < 0))
    {
      printf("ERR: Bank Colour (mix percent) must be in range 0..255\n");
      return -EINVAL;
    }
  else
    {
      lp503x_ioctl_args->param = requested_colour;
      lp503x_ioctl_args->lednum = 'C';
      return ioctl(fd, PWMIOC_SET_BANK_MIX_COLOUR, lp503x_ioctl_args);
    }
}

/****************************************************************************
 * Name:lp503x_cmd_brightness
 *
 *   sets all RGB led to the requested brightness
 ****************************************************************************/

static int lp503x_cmd_brightness(FAR char *parg)
{
  int ret;
  int lednum;
  int requested_brightness;

  requested_brightness = ((int)strtol(parg, NULL, 0)) & 0xff;
  if ((requested_brightness > 255) || (requested_brightness < 0))
    {
      printf("ERROR: brightness must be in range 0..255\n");
      ret = -EINVAL;
    }
  else
    {
      lp503x_ioctl_args->param = requested_brightness;

      for (lednum = 0; lednum < 12; lednum++)
        {
          lp503x_ioctl_args->lednum = lednum;
          ret = ioctl(fd, PWMIOC_SET_RGB_BRIGHTNESS, lp503x_ioctl_args);
        }
    }

  return ret;
}

/****************************************************************************
 * Name:lp503x_cmd_pattern
 *
 * run a pattern for "arg[0]" leds.
 * Repeats 5 times at different brightness levels
 ****************************************************************************/

static int lp503x_cmd_pattern(FAR char *parg)
{
  int ret;
  int numleds;
  int i;
  int j;
  uint8_t k;

  int colour_lookup[12] =
  {
    0x0000ff, 0x00ff00, 0xff0000,
    0x00ffff, 0xff00ff, 0xffff33,
    0xffffff, 0xffa500, 0xff00a5,
    0x8080ff, 0x80ff80, 0xff8080
  };

  int pattern_brightness[5] =
  {
    50, 100, 150, 200, 255
  };

  int curcolour;

  numleds = atoi(parg);
  if ((numleds > 12) || (numleds < 1))
    {
      printf("Error: number of leds should be between 1 and 12\n");
      return -EINVAL;
    }

  curcolour = 0;

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < numleds; j++)
        {
          lp503x_ioctl_args->lednum = j;
          lp503x_ioctl_args->param = pattern_brightness[i];
          ret = ioctl(fd, PWMIOC_SET_RGB_BRIGHTNESS, lp503x_ioctl_args);
          curcolour++;
          if (curcolour >= numleds)
            {
              curcolour = 0;
            }

          lp503x_ioctl_args->param = colour_lookup[curcolour];
          ret = ioctl(fd, PWMIOC_SET_RGB_COLOUR, lp503x_ioctl_args);
        }

      usleep(500000);
      curcolour++;
    }

  lp503x_ioctl_args->param = 0;
  for (k = 0; k < numleds; k++)
    {
      lp503x_ioctl_args->lednum = k;
      ret = ioctl(fd, PWMIOC_SET_RGB_COLOUR, lp503x_ioctl_args);
    }

  return ret;
}

/****************************************************************************
 * Name:lp503x_cmd_set_bank_brightness
 *
 *   sets the bank brightness to be used
 ****************************************************************************/

static int lp503x_cmd_set_bank_brightness(FAR char *parg)
{
  int bank_brightness;
  if (parg != NULL || *parg != '\0')
    {
      bank_brightness = atoi(parg);

      if ((bank_brightness > MAX_BRIGHTNESS) || (bank_brightness < 0))
        {
          printf("bank brightness range is 0..255\n");
          bank_brightness = 0;
          return -EINVAL;
        }
      else
        {
          lp503x_ioctl_args->param = bank_brightness;
          current_led_brightness = bank_brightness;
          return ioctl(fd, PWMIOC_SET_BANK_BRIGHTNESS, lp503x_ioctl_args);
        }
    }

  return OK;
}

/****************************************************************************
 * Name:lp503x_setcolour
 *
 *   sets the current RGB colour to be used
 ****************************************************************************/

static int lp503x_cmd_setcolour(FAR char *parg)
{
  int colour;
  if (parg == NULL || *parg != '\0')
    {
      printf("current colour is: 0x%06x\n", current_colour);
    }
  else
    {
      colour = (int)strtol(parg, NULL, 16);

      if (colour > MAX_RGB_COLOUR)
        {
          printf("colour range is 0...0xFFFFFF\n");
          current_colour = 0;
          return -EINVAL;
        }
      else
        {
          current_colour = colour;
          printf("colour set to: 0x%06x\n", current_colour);
        }
    }

  return OK;
}

/****************************************************************************
 * Name: lp503x_cmd_help
 *
 *   lp503x_cmd_help() displays the application's help information on
 *   supported commands and command syntax.
 ****************************************************************************/

static int lp503x_cmd_help(FAR char *parg)
{
  int len;
  int maxlen = 0;
  int x;
  int c;

  /* Calculate length of longest cmd + arghelp */

  for (x = 0; x < g_lp503x_cmd_count; x++)
    {
      len = strlen(g_lp503x_cmds[x].cmd) +
                   strlen(g_lp503x_cmds[x].arghelp);
      if (len > maxlen)
        {
          maxlen = len;
        }
    }

  printf("lp503x commands\n================\n");
  for (x = 0; x < g_lp503x_cmd_count; x++)
    {
      /* Print the command and it's arguments */

      printf("  %s %s", g_lp503x_cmds[x].cmd,
                        g_lp503x_cmds[x].arghelp);

      /* Calculate number of spaces to print before the help text */

      len = maxlen - (strlen(g_lp503x_cmds[x].cmd) +
                      strlen(g_lp503x_cmds[x].arghelp));
      for (c = 0; c < len; c++)
        {
          printf(" ");
        }

      printf("  : %s\n", g_lp503x_cmds[x].help);
    }

  return OK;
}

/****************************************************************************
 * lp503x_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char buffer[LINE_MAX];
  FAR char *cmd;
  FAR char *arg;
  bool running;
  int len;
  int x;

  fd = open(CONFIG_EXAMPLES_LP503X_DEVPATH, O_CREAT);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_LP503X_DEVPATH, errno);
      return -ENODEV;
    }

  running = true;
  while (running)
    {
      printf("lp503x> ");
      fflush(stdout);

      /* read a line from the terminal */

      len = readline_stream(buffer, sizeof(buffer),
                            stdin, stdout);
      buffer[len] = '\0';
      if (len > 0)
        {
          if (strncmp(buffer, "!", 1) != 0)
            {
              /* a command */

              if (buffer[len - 1] == '\n')
                {
                  buffer[len - 1] = '\0';
                }

              /* Parse the command from the argument */

              cmd = strtok_r(buffer, " \n", &arg);
              if (cmd == NULL)
                {
                  continue;
                }

              /* Remove leading spaces from arg */

              while (*arg == ' ')
                {
                  arg++;
                }

              /* Find the command in our cmd array */

              for (x = 0; x < g_lp503x_cmd_count; x++)
                {
                  if (strcmp(cmd, g_lp503x_cmds[x].cmd) == 0)
                    {
                      /* Command found.  Call it's handler if not NULL */

                      if (g_lp503x_cmds[x].pfunc != NULL)
                        {
                          g_lp503x_cmds[x].pfunc(arg);
                        }

                      /* Test if it is a quit command */

                      if (g_lp503x_cmds[x].pfunc == lp503x_cmd_quit)
                        {
                          running = FALSE;
                        }

                      break;
                    }
                }
            }
          else
            {
#ifdef CONFIG_SYSTEM_SYSTEM
              /* Transfer nuttx shell */

              system(buffer + 1);
#else
              printf("%s:  unknown lp503x command\n", buffer);
#endif
            }
        }
    }

  close(fd);

  return 0;
}
