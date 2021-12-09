/****************************************************************************
 * apps/examples/gpio/gpio_main.c
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
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include <nuttx/ioexpander/gpio.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE: %s [-t <pintype>] [-w <signo>] [-o <value>] "
          "<driver-path>\n", progname);
  fprintf(stderr, "       %s -h\n", progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t<driver-path>: The full path to the GPIO pin "
          "driver.\n");
  fprintf(stderr, "\t-t <pintype>:  Change the pin to this pintype "
          "(0-10):\n");
  fprintf(stderr, "\t-w <signo>:    Wait for a signal if this is an "
          "interrupt pin.\n");
  fprintf(stderr, "\t-o <value>:    Write this value (0 or 1) if this is an "
          "output pin.\n");
  fprintf(stderr, "\t-h: Print this usage information and exit.\n");
  fprintf(stderr, "Pintypes:\n");
  fprintf(stderr, "\t 0: GPIO_INPUT_PIN\n");
  fprintf(stderr, "\t 1: GPIO_INPUT_PIN_PULLUP\n");
  fprintf(stderr, "\t 2: GPIO_INPUT_PIN_PULLDOWN\n");
  fprintf(stderr, "\t 3: GPIO_OUTPUT_PIN\n");
  fprintf(stderr, "\t 4: GPIO_OUTPUT_PIN_OPENDRAIN\n");
  fprintf(stderr, "\t 5: GPIO_INTERRUPT_PIN\n");
  fprintf(stderr, "\t 6: GPIO_INTERRUPT_HIGH_PIN\n");
  fprintf(stderr, "\t 7: GPIO_INTERRUPT_LOW_PIN\n");
  fprintf(stderr, "\t 8: GPIO_INTERRUPT_RISING_PIN\n");
  fprintf(stderr, "\t 9: GPIO_INTERRUPT_FALLING_PIN\n");
  fprintf(stderr, "\t10: GPIO_INTERRUPT_BOTH_PIN\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * gpio_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *devpath = NULL;
  enum gpio_pintype_e pintype;
  enum gpio_pintype_e newpintype;
  bool havenewtype = false;
  bool havesigno = false;
  bool invalue;
  bool outvalue = false;
  bool haveout = false;
  int signo = 0;
  int ndx;
  int ret;
  int fd;

  /* Parse command line */

  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Missing required arguments\n");
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  ndx = 1;
  if (strcmp(argv[ndx], "-h") == 0)
    {
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (strcmp(argv[ndx], "-t") == 0)
    {
      havenewtype = true;

      if (++ndx >= argc)
        {
          fprintf(stderr, "ERROR: Missing argument to -t\n");
          show_usage(argv[0]);
          return EXIT_FAILURE;
        }

      newpintype = atoi(argv[ndx]);

      if (++ndx >= argc)
        {
          fprintf(stderr, "ERROR: Missing required <driver-path>\n");
          show_usage(argv[0]);
          return EXIT_FAILURE;
        }
    }

  if (ndx < argc && strcmp(argv[ndx], "-w") == 0)
    {
      havesigno = true;

      if (++ndx >= argc)
        {
          fprintf(stderr, "ERROR: Missing argument to -w\n");
          show_usage(argv[0]);
          return EXIT_FAILURE;
        }

      signo = atoi(argv[ndx]);

      if (++ndx >= argc)
        {
          fprintf(stderr, "ERROR: Missing required <driver-path>\n");
          show_usage(argv[0]);
          return EXIT_FAILURE;
        }
    }

  if (ndx < argc && strcmp(argv[ndx], "-o") == 0)
    {
      if (++ndx >= argc)
        {
          fprintf(stderr, "ERROR: Missing argument to -o\n");
          show_usage(argv[0]);
          return EXIT_FAILURE;
        }

      if (strcmp(argv[ndx], "0") == 0)
        {
          outvalue = false;
          haveout = true;
        }
      else if (strcmp(argv[ndx], "1") == 0)
        {
          outvalue = true;
          haveout = true;
        }
      else
        {
          fprintf(stderr, "ERROR: Invalid argument to -o\n");
          show_usage(argv[0]);
          return EXIT_FAILURE;
        }

      if (++ndx >= argc)
        {
          fprintf(stderr, "ERROR: Missing required <driver-path>\n");
          show_usage(argv[0]);
          return EXIT_FAILURE;
        }
    }

  devpath = argv[ndx];
  printf("Driver: %s\n", devpath);

  /* Open the pin driver */

  fd = open(devpath, O_RDWR);
  if (fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", devpath, errcode);
      return EXIT_FAILURE;
    }

  /* Set the new pintype */

  if (havenewtype)
    {
      ret = ioctl(fd, GPIOC_SETPINTYPE, (unsigned long) newpintype);
      if (ret < 0)
        {
          int errcode = errno;
          fprintf(stderr, "ERROR: Failed to set pintype on %s: %d\n",
                  devpath, errcode);
          close(fd);
          return EXIT_FAILURE;
        }
    }

  /* Get the pin type */

  ret = ioctl(fd, GPIOC_PINTYPE, (unsigned long)((uintptr_t)&pintype));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to read pintype from %s: %d\n", devpath,
              errcode);
      close(fd);
      return EXIT_FAILURE;
    }

  /* Read the pin value */

  ret = ioctl(fd, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to read value from %s: %d\n",
              devpath, errcode);
      close(fd);
      return EXIT_FAILURE;
    }

  /* Perform the test based on the pintype and on command line options */

  switch (pintype)
    {
      case GPIO_INPUT_PIN:
        {
          printf("  Input pin:     Value=%u\n",
                 (unsigned int)invalue);
        }
        break;

      case GPIO_INPUT_PIN_PULLUP:
        {
          printf("  Input pin (pull-up):     Value=%u\n",
                 (unsigned int)invalue);
        }
        break;

      case GPIO_INPUT_PIN_PULLDOWN:
        {
          printf("  Input pin (pull-down):     Value=%u\n",
                 (unsigned int)invalue);
        }
        break;

      case GPIO_OUTPUT_PIN:
      case GPIO_OUTPUT_PIN_OPENDRAIN:
        {
          printf("  Output pin:    Value=%u\n", (unsigned int)invalue);

          if (haveout)
            {
              printf("  Writing:       Value=%u\n", (unsigned int)outvalue);

              /* Write the pin value */

              ret = ioctl(fd, GPIOC_WRITE, (unsigned long)outvalue);
              if (ret < 0)
               {
                 int errcode = errno;
                 fprintf(stderr,
                         "ERROR: Failed to write value %u from %s: %d\n",
                         (unsigned int)outvalue, devpath, errcode);
                 close(fd);
                 return EXIT_FAILURE;
               }

              /* Re-read the pin value */

              ret = ioctl(fd, GPIOC_READ,
                          (unsigned long)((uintptr_t)&invalue));
              if (ret < 0)
                {
                  int errcode = errno;
                  fprintf(stderr,
                          "ERROR: Failed to re-read value from %s: %d\n",
                          devpath, errcode);
                  close(fd);
                  return EXIT_FAILURE;
                }

              printf("  Verify:        Value=%u\n", (unsigned int)invalue);
            }
        }
        break;

      case GPIO_INTERRUPT_PIN:
      case GPIO_INTERRUPT_HIGH_PIN:
      case GPIO_INTERRUPT_LOW_PIN:
      case GPIO_INTERRUPT_RISING_PIN:
      case GPIO_INTERRUPT_FALLING_PIN:
      case GPIO_INTERRUPT_BOTH_PIN:
        {
          printf("  Interrupt pin: Value=%u\n", invalue);

          if (havesigno)
            {
              struct sigevent notify;
              struct timespec ts;
              sigset_t set;

              notify.sigev_notify = SIGEV_SIGNAL;
              notify.sigev_signo  = signo;

              /* Set up to receive signal */

              ret = ioctl(fd, GPIOC_REGISTER, (unsigned long)&notify);
              if (ret < 0)
                {
                  int errcode = errno;

                  fprintf(stderr,
                          "ERROR: Failed to setup for signal from %s: %d\n",
                          devpath, errcode);

                  close(fd);
                  return EXIT_FAILURE;
                }

              /* Wait up to 5 seconds for the signal */

              sigemptyset(&set);
              sigaddset(&set, signo);

              ts.tv_sec  = 5;
              ts.tv_nsec = 0;

              ret = sigtimedwait(&set, NULL, &ts);
              ioctl(fd, GPIOC_UNREGISTER, 0);

              if (ret < 0)
                {
                  int errcode = errno;
                  if (errcode == EAGAIN)
                    {
                      printf("  [Five second timeout with no signal]\n");
                      close(fd);
                      return EXIT_SUCCESS;
                    }
                  else
                    {
                      fprintf(stderr, "ERROR: Failed to wait signal %d "
                              "from %s: %d\n", signo, devpath, errcode);
                      close(fd);
                      return EXIT_FAILURE;
                    }
                }

              /* Re-read the pin value */

              ret = ioctl(fd, GPIOC_READ,
                          (unsigned long)((uintptr_t)&invalue));
              if (ret < 0)
                {
                  int errcode = errno;
                  fprintf(stderr,
                          "ERROR: Failed to re-read value from %s: %d\n",
                          devpath, errcode);
                  close(fd);
                  return EXIT_FAILURE;
                }

              printf("  Verify:        Value=%u\n", (unsigned int)invalue);
            }
        }
        break;

      default:
        fprintf(stderr, "ERROR: Unrecognized pintype: %d\n", (int)pintype);
        close(fd);
        return EXIT_FAILURE;
    }

  close(fd);
  return EXIT_SUCCESS;
}
