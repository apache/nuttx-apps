/****************************************************************************
 * apps/examples/comp/comp_main.c
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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <nuttx/analog/ioctl.h>

#ifdef CONFIG_DAC
#  include <nuttx/analog/dac.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_COMP
#  error "COMP device support is not enabled (CONFIG_COMP)"
#endif

#ifndef CONFIG_EXAMPLES_COMP_DEVPATH
#  define CONFIG_EXAMPLES_COMP_DEVPATH "/dev/comp0"
#endif

#ifdef CONFIG_DAC
#  ifndef CONFIG_EXAMPLES_COMP_DACPATH
#    define CONFIG_EXAMPLES_COMP_DACPATH "/dev/dac0"
#  endif
#endif

#define DEFAULT_RAMP_START 0
#define DEFAULT_RAMP_END   4095
#define DEFAULT_RAMP_STEP  100
#define DEFAULT_RAMP_DELAY 5

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct comp_state_s
{
  FAR const char *devpath;
#ifdef CONFIG_DAC
  FAR const char *dacpath;
  int             ramp_start;
  int             ramp_end;
  int             ramp_step;
  int             ramp_delay;
#endif
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct comp_state_s g_comp_state =
{
  CONFIG_EXAMPLES_COMP_DEVPATH
#ifdef CONFIG_DAC
  ,
  CONFIG_EXAMPLES_COMP_DACPATH,
  DEFAULT_RAMP_START,
  DEFAULT_RAMP_END,
  DEFAULT_RAMP_STEP,
  DEFAULT_RAMP_DELAY
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: comp_help
 ****************************************************************************/

static void comp_help(FAR const char *progname)
{
  printf("Usage: %s [OPTIONS] [COMMAND]\n", progname);
  printf("\nCommands:\n");
  printf("  enable       Enable the comparator\n");
  printf("  disable      Disable the comparator\n");
  printf("  read         Read comparator output state (0 or 1)\n");
  printf("  test         Run test sequence (ioctls, enable/disable)\n");
#ifdef CONFIG_DAC
  printf("  ramp         Generate DAC voltage ramp to test trip\n");
#endif
  printf("  help         Show this help message\n");
  printf("\nOptions:\n");
  printf("  -p <devpath> Comparator device path (default: %s)\n",
         CONFIG_EXAMPLES_COMP_DEVPATH);
#ifdef CONFIG_DAC
  printf("  -D <dacpath> DAC device path (default: %s)\n",
         CONFIG_EXAMPLES_COMP_DACPATH);
  printf("  -s <start>   Ramp start value (default: %d)\n",
         DEFAULT_RAMP_START);
  printf("  -f <end>     Ramp end value (default: %d)\n",
         DEFAULT_RAMP_END);
  printf("  -i <step>    Ramp step increment (default: %d)\n",
         DEFAULT_RAMP_STEP);
  printf("  -w <delay>   Delay per step in ms (default: %d)\n",
         DEFAULT_RAMP_DELAY);
#endif
  printf("  -e           Same as 'enable'\n");
  printf("  -d           Same as 'disable'\n");
  printf("  -r           Same as 'read'\n");
  printf("  -t           Same as 'test'\n");
  printf("  -h           Show this help message\n");
}

/****************************************************************************
 * Name: comp_do_enable
 ****************************************************************************/

static int comp_do_enable(FAR const char *devpath)
{
  int errcode;
  int fd;
  int ret;

  fd = open(devpath, O_RDONLY);
  if (fd < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", devpath, errcode);
      return -errcode;
    }

  ret = ioctl(fd, ANIOC_COMP_ENABLE, 0);
  if (ret < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: ioctl(ANIOC_COMP_ENABLE) failed: %d\n",
              errcode);
      close(fd);
      return -errcode;
    }

  printf("%s: Comparator enabled successfully\n", devpath);
  close(fd);
  return OK;
}

/****************************************************************************
 * Name: comp_do_disable
 ****************************************************************************/

static int comp_do_disable(FAR const char *devpath)
{
  int errcode;
  int fd;
  int ret;

  fd = open(devpath, O_RDONLY);
  if (fd < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", devpath, errcode);
      return -errcode;
    }

  ret = ioctl(fd, ANIOC_COMP_DISABLE, 0);
  if (ret < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: ioctl(ANIOC_COMP_DISABLE) failed: %d\n",
              errcode);
      close(fd);
      return -errcode;
    }

  printf("%s: Comparator disabled successfully\n", devpath);
  close(fd);
  return OK;
}

/****************************************************************************
 * Name: comp_do_read
 ****************************************************************************/

static int comp_do_read(FAR const char *devpath, FAR uint8_t *val)
{
  int errcode;
  int fd;
  ssize_t nbytes;
  uint8_t output = 0;

  fd = open(devpath, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", devpath, errcode);
      return -errcode;
    }

  nbytes = read(fd, &output, 1);
  if (nbytes < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: read from %s failed: %d\n", devpath, errcode);
      close(fd);
      return -errcode;
    }

  close(fd);

  if (val != NULL)
    {
      *val = output;
    }

  return OK;
}

/****************************************************************************
 * Name: comp_do_test
 ****************************************************************************/

static int comp_do_test(FAR const char *devpath)
{
  int errcode;
  int fd;
  int ret;
  uint8_t val;
  ssize_t nbytes;

  printf("Running comparator test on %s...\n", devpath);

  fd = open(devpath, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", devpath, errcode);
      return -errcode;
    }

  /* 1. Read initial state */

  nbytes = read(fd, &val, 1);
  if (nbytes < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Initial read failed: %d\n", errcode);
      close(fd);
      return -errcode;
    }

  printf("  1. Initial output state: %u\n", (unsigned int)val);

  /* 2. Test ANIOC_COMP_DISABLE */

  ret = ioctl(fd, ANIOC_COMP_DISABLE, 0);
  if (ret < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: ioctl(ANIOC_COMP_DISABLE) failed: %d\n",
              errcode);
      close(fd);
      return -errcode;
    }

  printf("  2. ANIOC_COMP_DISABLE: OK\n");

  /* 3. Read state while disabled */

  nbytes = read(fd, &val, 1);
  if (nbytes < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Read after disable failed: %d\n", errcode);
      close(fd);
      return -errcode;
    }

  printf("  3. Output state when disabled: %u\n", (unsigned int)val);

  /* 4. Test ANIOC_COMP_ENABLE */

  ret = ioctl(fd, ANIOC_COMP_ENABLE, 0);
  if (ret < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: ioctl(ANIOC_COMP_ENABLE) failed: %d\n",
              errcode);
      close(fd);
      return -errcode;
    }

  printf("  4. ANIOC_COMP_ENABLE: OK\n");

  /* 5. Read state after re-enable */

  nbytes = read(fd, &val, 1);
  if (nbytes < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Read after enable failed: %d\n", errcode);
      close(fd);
      return -errcode;
    }

  printf("  5. Output state when enabled: %u\n", (unsigned int)val);

  close(fd);
  printf("Comparator test PASSED on %s\n", devpath);
  return OK;
}

#ifdef CONFIG_DAC
/****************************************************************************
 * Name: comp_do_ramp
 *
 * Description:
 *   Generate a DAC voltage ramp and monitor comparator output transitions.
 *   This performs a real analog threshold comparison on hardware.
 *
 ****************************************************************************/

static int comp_do_ramp(FAR const char *comp_path,
                        FAR const char *dac_path,
                        int start, int end, int step, int delay_ms)
{
  int comp_fd;
  int dac_fd;
  int errcode;
  int ret;
  int val;
  int transitions = 0;
  int prev_state = -1;
  uint8_t cur_state = 0;
  struct dac_msg_s msg;
  ssize_t nbytes;

  printf("=== Comparator Ramp Test ===\n");
  printf("  COMP device : %s\n", comp_path);
  printf("  DAC device  : %s\n", dac_path);
  printf("  Sweep range : %d -> %d (step %d, delay %d ms)\n\n",
         start, end, step, delay_ms);

  /* Open comparator device */

  comp_fd = open(comp_path, O_RDONLY | O_NONBLOCK);
  if (comp_fd < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", comp_path, errcode);
      return -errcode;
    }

  /* Ensure comparator is enabled */

  ret = ioctl(comp_fd, ANIOC_COMP_ENABLE, 0);
  if (ret < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Failed to enable comparator: %d\n", errcode);
      close(comp_fd);
      return -errcode;
    }

  /* Open DAC device */

  dac_fd = open(dac_path, O_WRONLY | O_NONBLOCK);
  if (dac_fd < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Failed to open DAC %s: %d\n",
              dac_path, errcode);
      close(comp_fd);
      return -errcode;
    }

  /* Sweep DAC values */

  for (val = start; val <= end; val += step)
    {
      uint32_t mv;

      msg.am_channel = 0;
      msg.am_data = val;

      nbytes = write(dac_fd, &msg, sizeof(msg));
      if (nbytes < 0 && errno != EAGAIN)
        {
          errcode = errno;
          fprintf(stderr, "ERROR: DAC write failed: %d\n", errcode);
          break;
        }

      usleep(delay_ms * 1000);

      nbytes = read(comp_fd, &cur_state, 1);
      if (nbytes < 0)
        {
          errcode = errno;
          fprintf(stderr, "ERROR: COMP read failed: %d\n", errcode);
          break;
        }

      mv = (uint32_t)((val * 3300ULL) / 4095ULL);

      if (prev_state >= 0 && (int)cur_state != prev_state)
        {
          transitions++;
          printf("  -> [TRIP EVENT #%d] DAC=%4d (~%4lu mV) | "
                 "COMP output: %d -> %d\n",
                 transitions, val, (unsigned long)mv, prev_state, cur_state);
        }
      else if (val == start || val + step > end || (val % (step * 5)) == 0)
        {
          printf("     DAC=%4d (~%4lu mV) | COMP output: %d\n",
                 val, (unsigned long)mv, (int)cur_state);
        }

      prev_state = (int)cur_state;
    }

  close(dac_fd);
  close(comp_fd);

  printf("\n=== Test Summary ===\n");
  if (transitions > 0)
    {
      printf("SUCCESS: Detected %d comparator transition(s)!\n",
             transitions);
      ret = OK;
    }
  else
    {
      printf("COMPLETED: Output stayed at %d across entire sweep range.\n",
             prev_state);
      printf("Note: Ensure INP pin is biased within 0..3.3V.\n");
      ret = OK;
    }

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int opt;
  int ret = OK;
  uint8_t val = 0;
  FAR const char *cmd = NULL;

  g_comp_state.devpath = CONFIG_EXAMPLES_COMP_DEVPATH;
#ifdef CONFIG_DAC
  g_comp_state.dacpath = CONFIG_EXAMPLES_COMP_DACPATH;
  g_comp_state.ramp_start = DEFAULT_RAMP_START;
  g_comp_state.ramp_end = DEFAULT_RAMP_END;
  g_comp_state.ramp_step = DEFAULT_RAMP_STEP;
  g_comp_state.ramp_delay = DEFAULT_RAMP_DELAY;
#endif

  if (argc > 1 && argv[1][0] != '-')
    {
      cmd = argv[1];
      argc--;
      argv++;
    }

#ifdef CONFIG_DAC
  while ((opt = getopt(argc, argv, "p:D:s:f:i:w:edrth")) != -1)
#else
  while ((opt = getopt(argc, argv, "p:edrth")) != -1)
#endif
    {
      switch (opt)
        {
          case 'p':
            g_comp_state.devpath = optarg;
            break;

#ifdef CONFIG_DAC
          case 'D':
            g_comp_state.dacpath = optarg;
            break;

          case 's':
            g_comp_state.ramp_start = atoi(optarg);
            break;

          case 'f':
            g_comp_state.ramp_end = atoi(optarg);
            break;

          case 'i':
            g_comp_state.ramp_step = atoi(optarg);
            break;

          case 'w':
            g_comp_state.ramp_delay = atoi(optarg);
            break;
#endif

          case 'e':
            cmd = "enable";
            break;

          case 'd':
            cmd = "disable";
            break;

          case 'r':
            cmd = "read";
            break;

          case 't':
            cmd = "test";
            break;

          case 'h':
            comp_help(argv[0]);
            return OK;

          default:
            comp_help(argv[0]);
            return -EINVAL;
        }
    }

  if (cmd == NULL && optind < argc)
    {
      cmd = argv[optind];
    }

  if (cmd == NULL || strcmp(cmd, "test") == 0)
    {
      ret = comp_do_test(g_comp_state.devpath);
#ifdef CONFIG_DAC
      if (ret == OK)
        {
          printf("\n");
          ret = comp_do_ramp(g_comp_state.devpath,
                             g_comp_state.dacpath,
                             g_comp_state.ramp_start,
                             g_comp_state.ramp_end,
                             g_comp_state.ramp_step,
                             g_comp_state.ramp_delay);
        }
#endif
    }
#ifdef CONFIG_DAC
  else if (strcmp(cmd, "ramp") == 0)
    {
      ret = comp_do_ramp(g_comp_state.devpath,
                         g_comp_state.dacpath,
                         g_comp_state.ramp_start,
                         g_comp_state.ramp_end,
                         g_comp_state.ramp_step,
                         g_comp_state.ramp_delay);
    }
#endif
  else if (strcmp(cmd, "enable") == 0)
    {
      ret = comp_do_enable(g_comp_state.devpath);
    }
  else if (strcmp(cmd, "disable") == 0)
    {
      ret = comp_do_disable(g_comp_state.devpath);
    }
  else if (strcmp(cmd, "read") == 0)
    {
      ret = comp_do_read(g_comp_state.devpath, &val);
      if (ret == OK)
        {
          printf("%s: output = %u\n", g_comp_state.devpath,
                 (unsigned int)val);
        }
    }
  else if (strcmp(cmd, "help") == 0)
    {
      comp_help(argv[0]);
      ret = OK;
    }
  else
    {
      fprintf(stderr, "Unknown command: %s\n", cmd);
      comp_help(argv[0]);
      ret = -EINVAL;
    }

  return ret;
}
