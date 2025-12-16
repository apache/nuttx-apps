/****************************************************************************
 * apps/system/stty/stty_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define stty_error(...) dprintf(STDERR_FILENO, __VA_ARGS__)
#define FLAG_STR(flags, flag) ((flags) & (flag) ? "" : "-")

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_usage(FAR const char *progname)
{
  printf("Usage: %s [-F device] [settings...]\n", progname);
  printf("\nSettings:\n");
  printf("  raw           - Set raw mode (no processing)\n");
  printf("  cooked        - Set cooked mode (canonical)\n");
  printf("  echo          - Enable echo\n");
  printf("  -echo         - Disable echo\n");
  printf("  icanon        - Enable canonical mode\n");
  printf("  -icanon       - Disable canonical mode\n");
  printf("  icrnl         - Map CR to NL on input\n");
  printf("  -icrnl        - Don't map CR to NL\n");
  printf("  onlcr         - Map NL to CR-NL on output\n");
  printf("  -onlcr        - Don't map NL to CR-NL\n");
  printf("  opost         - Enable output processing\n");
  printf("  -opost        - Disable output processing\n");
  printf("  isig          - Enable signal characters\n");
  printf("  -isig         - Disable signal characters\n");
  printf("  speed N       - Set baud rate\n");
  printf("  min N         - Set minimum characters for read (0-255)\n");
  printf("  time N        - Set read timeout in 0.1s units (0-255)\n");
  printf("\nExamples:\n");
  printf("  %s -F /dev/ttyS0 raw -echo", progname);
  printf(" # Raw mode, no echo\n");
  printf("  %s -F /dev/ttyS0 cooked", progname);
  printf(" # Interactive terminal mode\n");
}

static int parse_numeric_param(int argc, FAR char *argv[], FAR int *idx,
                               int min, int max)
{
  int val;
  if (*idx + 1 >= argc)
    {
      stty_error("ERROR: %s requires a value\n", argv[*idx]);
      return -EINVAL;
    }

  val = atoi(argv[++(*idx)]);
  if (val < min || val > max)
    {
      stty_error("ERROR: Invalid value: %d (must be %d-%d)\n",
                 val, min, max);
      return -EINVAL;
    }

  return val;
}

static int apply_settings(int fd, int argc, FAR char *argv[], int idx)
{
  struct termios tio;
  int val;
  int ret;

  /* Get current settings */

  ret = tcgetattr(fd, &tio);
  if (ret < 0)
    {
      stty_error("ERROR: tcgetattr failed: %d\n", errno);
      return -errno;
    }

  /* Parse and apply settings */

  for (; idx < argc; idx++)
    {
      FAR const char *arg = argv[idx];

      if (strcmp(arg, "raw") == 0)
        {
          cfmakeraw(&tio);
        }
      else if (strcmp(arg, "cooked") == 0)
        {
          /* Set cooked mode (canonical with echo) */

          tio.c_lflag |= ICANON | ECHO | ISIG;
          tio.c_oflag |= OPOST  | ONLCR;
          tio.c_iflag |= ICRNL;
        }
      else if (strcmp(arg, "echo") == 0)
        {
          tio.c_lflag |= ECHO;
        }
      else if (strcmp(arg, "-echo") == 0)
        {
          tio.c_lflag &= ~ECHO;
        }
      else if (strcmp(arg, "icanon") == 0)
        {
          tio.c_lflag |= ICANON;
        }
      else if (strcmp(arg, "-icanon") == 0)
        {
          tio.c_lflag &= ~ICANON;
        }
      else if (strcmp(arg, "icrnl") == 0)
        {
          tio.c_iflag |= ICRNL;
        }
      else if (strcmp(arg, "-icrnl") == 0)
        {
          tio.c_iflag &= ~ICRNL;
        }
      else if (strcmp(arg, "onlcr") == 0)
        {
          tio.c_oflag |= ONLCR;
        }
      else if (strcmp(arg, "-onlcr") == 0)
        {
          tio.c_oflag &= ~ONLCR;
        }
      else if (strcmp(arg, "opost") == 0)
        {
          tio.c_oflag |= OPOST;
        }
      else if (strcmp(arg, "-opost") == 0)
        {
          tio.c_oflag &= ~OPOST;
        }
      else if (strcmp(arg, "isig") == 0)
        {
          tio.c_lflag |= ISIG;
        }
      else if (strcmp(arg, "-isig") == 0)
        {
          tio.c_lflag &= ~ISIG;
        }
      else if (strcmp(arg, "inlcr") == 0)
        {
          tio.c_iflag |= INLCR;
        }
      else if (strcmp(arg, "-inlcr") == 0)
        {
          tio.c_iflag &= ~INLCR;
        }
      else if (strcmp(arg, "igncr") == 0)
        {
          tio.c_iflag |= IGNCR;
        }
      else if (strcmp(arg, "-igncr") == 0)
        {
          tio.c_iflag &= ~IGNCR;
        }
      else if (strcmp(arg, "ocrnl") == 0)
        {
          tio.c_oflag |= OCRNL;
        }
      else if (strcmp(arg, "-ocrnl") == 0)
        {
          tio.c_oflag &= ~OCRNL;
        }
      else if (strcmp(arg, "speed") == 0)
        {
          val = parse_numeric_param(argc, argv, &idx, 0, 4000000);
          if (val < 0)
            {
              return val;
            }

          if (cfsetspeed(&tio, val) < 0)
            {
              stty_error("ERROR: Invalid speed value: %d\n", val);
              return -errno;
            }
        }
      else if (strcmp(arg, "min") == 0)
        {
          val = parse_numeric_param(argc, argv, &idx, 0, 255);
          if (val < 0)
            {
              return val;
            }

          tio.c_cc[VMIN] = val;
        }
      else if (strcmp(arg, "time") == 0)
        {
          val = parse_numeric_param(argc, argv, &idx, 0, 255);
          if (val < 0)
            {
              return val;
            }

          tio.c_cc[VTIME] = val;
        }
      else
        {
          stty_error("WARNING: Unknown setting: %s\n", arg);
        }
    }

  /* Apply the new settings */

  ret = tcsetattr(fd, TCSANOW, &tio);
  if (ret < 0)
    {
      stty_error("ERROR: tcsetattr failed: %d\n", errno);
      return -errno;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *device = NULL;
  int fd = STDIN_FILENO;
  int ret;
  int opt;
  int idx;

  /* Parse -F option for device path */

  while ((opt = getopt(argc, argv, "F:h")) != -1)
    {
      switch (opt)
        {
          case 'F':
            device = optarg;
            break;

          case 'h':
            print_usage(argv[0]);
            return 0;

          default:
            print_usage(argv[0]);
            return -EINVAL;
        }
    }

  idx = optind;

  /* Open device if specified, or use stdin as default */

  if (device == NULL)
    {
      device = "stdin";
    }
  else
    {
      fd = open(device, O_RDWR | O_NONBLOCK);
      if (fd < 0)
        {
          stty_error("ERROR: Failed to open %s: %d\n", device, errno);
          return -errno;
        }
    }

  if (!isatty(fd))
    {
      stty_error("ERROR: %s is not a TTY\n", device);
      if (fd != STDIN_FILENO)
        {
          close(fd);
        }

      return -EINVAL;
    }

  /* If no settings provided, just display current settings */

  if (idx >= argc)
    {
      struct termios tio;
      speed_t speed;

      memset(&tio, 0, sizeof(struct termios));
      ret = tcgetattr(fd, &tio);
      if (ret < 0)
        {
          stty_error("ERROR: tcgetattr failed: %d\n", errno);
          if (fd != STDIN_FILENO)
            {
              close(fd);
            }

          return -errno;
        }

      speed = cfgetispeed(&tio);

      printf("Terminal settings for %s:\n", device);
      if (speed <= 0)
        {
          printf("  speed unknown or not supported by driver; ");
        }
      else
        {
          printf("  speed %lu baud; ", speed);
        }

      printf("min = %d; time = %d;\n", tio.c_cc[VMIN], tio.c_cc[VTIME]);

      printf("  c_iflag: 0x%08x %sicrnl %signcr %sinlcr\n", tio.c_iflag,
             FLAG_STR(tio.c_iflag, ICRNL),
             FLAG_STR(tio.c_iflag, IGNCR),
             FLAG_STR(tio.c_iflag, INLCR));
      printf("  c_oflag: 0x%08x %sopost %sonlcr %socrnl\n", tio.c_oflag,
             FLAG_STR(tio.c_oflag, OPOST),
             FLAG_STR(tio.c_oflag, ONLCR),
             FLAG_STR(tio.c_oflag, OCRNL));
      printf("  c_lflag: 0x%08x %secho %sicanon %sisig\n", tio.c_lflag,
             FLAG_STR(tio.c_lflag, ECHO),
             FLAG_STR(tio.c_lflag, ICANON),
             FLAG_STR(tio.c_lflag, ISIG));
      printf("  c_cflag: 0x%08x %sclocal %scread %scstopb\n", tio.c_cflag,
             FLAG_STR(tio.c_cflag, CLOCAL),
             FLAG_STR(tio.c_cflag, CREAD),
             FLAG_STR(tio.c_cflag, CSTOPB));
    }
  else
    {
      /* Apply settings */

      ret = apply_settings(fd, argc, argv, idx);
      if (ret < 0)
        {
          if (fd != STDIN_FILENO)
            {
              close(fd);
            }

          return ret;
        }

      printf("Settings applied to %s\n", device);
    }

  if (fd != STDIN_FILENO)
    {
      close(fd);
    }

  return 0;
}
