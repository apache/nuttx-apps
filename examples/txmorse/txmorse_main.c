/****************************************************************************
 * apps/examples/txmorse/txmorse_main.c
 *
 * SPDX-License-Identifer: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership. The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License. You may obtain a copy of the License at
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

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef CONFIG_DEV_GPIO
#include <nuttx/ioexpander/gpio.h>
#endif

#include <audioutils/morsey.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Default millisecond duration of dot (2.5WPM) */

#define DEFAULT_DURATION (240)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: transmit_console
 *
 * Description:
 *   Transmit Morse code as printed characters to the console.
 ****************************************************************************/

static void transmit_console(int on, unsigned duration_ms, void *arg)
{
  unsigned ms_per_unit = *((unsigned *)arg);

  if (on)
    {
      /* We are printing a dot or a dash */

      if (duration_ms == ms_per_unit * MORSEY_UNITS_DOT)
        {
          putchar('.');
        }
      else if (duration_ms == ms_per_unit * MORSEY_UNITS_DASH)
        {
          putchar('-');
        }
    }
  else
    {
      /* We are indicating separation */

      if (duration_ms == ms_per_unit * MORSEY_UNITS_INTERELEM)
        {
          return; /* Inter-element gap, do nothing */
        }
      else if (duration_ms == ms_per_unit * MORSEY_UNITS_INTERLET)
        {
          putchar(' '); /* Inter-letter gap, print a space */
        }
      else if (duration_ms == ms_per_unit * MORSEY_UNITS_INTERWORD)
        {
          putchar('/'); /* Inter-word gap, print / delimiter */
        }
    }
}

/****************************************************************************
 * Name: cleanup_console
 *
 * Description:
 *   Default cleanup function; does nothing.
 *
 * Input Parameters:
 *   arg - User argument.
 *
 ****************************************************************************/

static void cleanup_console(void *arg)
{
  UNUSED(arg);
  putchar('\n'); /* Newline character after message is done */
}

#ifdef CONFIG_DEV_GPIO

/****************************************************************************
 * Name: transmit_gpio
 *
 * Description:
 *   Transmit Morse over a GPIO pin.
 *
 * Input Parameters:
 *   arg - Pointer to file descriptor of GPIO device
 *
 ****************************************************************************/

static void transmit_gpio(int on, unsigned duration_ms, void *arg)
{
  int err;
  int fd = *((int *)arg);

  if (on)
    {
      err = ioctl(fd, GPIOC_WRITE, 1);
      if (err < 0)
        {
          fprintf(stderr, "Failed to turn on GPIO: %d\n", errno);
          return;
        }
    }

  usleep(duration_ms * 1000);

  err = ioctl(fd, GPIOC_WRITE, 0);
  if (err < 0)
    {
      fprintf(stderr, "Failed to turn off GPIO: %d\n", errno);
    }
}

/****************************************************************************
 * Name: cleanup_gpio
 *
 * Description:
 *   Clean up a GPIO Morse sink.
 *
 * Input Parameters:
 *   arg - User argument (pointer to file descriptor)
 *
 ****************************************************************************/

static void cleanup_gpio(void *arg)
{
  int fd = *((int *)arg);
  close(fd);
}

#endif /* CONFIG_DEV_GPIO */

/****************************************************************************
 * Name: print_usage
 *
 * Description:
 *   Prints out the usage information for the program.
 *
 * Input Parameters:
 *   sink - The stream to output the usage information
 *
 ****************************************************************************/

static void print_usage(FILE *sink)
{
  fprintf(sink, "USAGE: " CONFIG_EXAMPLES_TXMORSE_PROGNAME
                " [-d dot_duration_ms] \"my text\" [path/to/device]\n");
  fprintf(sink,
          "Ex: " CONFIG_EXAMPLES_TXMORSE_PROGNAME " \"sos\" /dev/gpio1\n");
  fprintf(sink, "Default dot duration is 240 ms\n");
  fprintf(sink, "Default device is output to console\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  int err;
  int c;
  int fd;
  struct morsey_state_s state;

  transmit_f txfunc = transmit_console;          /* Default transmit */
  void (*cleanupfunc)(void *) = cleanup_console; /* Default cleanup */
  void *arg = NULL;

  unsigned duration = DEFAULT_DURATION;
  char *devpath = NULL;
  char *message = NULL;

  arg = &duration; /* Default argument */

  /* Parse command line arguments */

  while ((c = getopt(argc, argv, ":hd:")) != -1)
    {
      switch (c)
        {
        case 'h':
          print_usage(stdout);
          return EXIT_SUCCESS;
        case 'd':
          duration = strtoul(optarg, NULL, 10);
          break;
        case '?':
          fprintf(stderr, "Unknown option -%c\n", optopt);
          print_usage(stderr);
          return EXIT_FAILURE;
        }
    }

  /* Get text to encode */

  if (argc <= optind)
    {
      fprintf(stderr, "Missing text to encode!\n");
      return EXIT_FAILURE;
    }

  message = argv[optind++];

  /* Check for optional device path */

  if (argc > optind)
    {
      devpath = argv[optind++];
    }

  /* Choose transmit function based on device path */

  if (devpath == NULL)
    {
      txfunc = transmit_console;
      cleanupfunc = cleanup_console;
      arg = &duration;
    }
#ifdef CONFIG_DEV_GPIO
  else if (strstr(devpath, "gpio"))
    {
      txfunc = transmit_gpio;
      cleanupfunc = cleanup_gpio;
      fd = open(devpath, O_WRONLY);
      if (fd < 0)
        {
          fprintf(stderr, "Couldn't open '%s': %d\n", devpath, errno);
          return EXIT_FAILURE;
        }

      arg = &fd;
    }
#endif
  else
    {
      fprintf(stderr, "Unrecognized device '%s', defaulting to console.\n",
              devpath);
    }

  /* Set up library to play */

  morsey_init(&state, duration, 0);

  /* Transmit the text as Morse code */

  err = morsey_transmit(message, strlen(message), &state, txfunc, arg);
  if (err == 0)
    {
      cleanupfunc(arg);
      return 0;
    }

  fprintf(stderr, "Error transmitting: %d\n", err);
  cleanupfunc(arg);
  return err;
}
