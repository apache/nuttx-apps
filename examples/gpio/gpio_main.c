/****************************************************************************
 * examples/gpio/gpio_main.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
  fprintf(stderr, "USAGE: %s [-w <signo>] [-o <value>] <driver-path>\n",
          progname);
  fprintf(stderr, "       %s -h\n", progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t<driver-path>: The full path to the GPIO pin "
          "driver.\n");
  fprintf(stderr,
          "\t-w <signo>: Wait for an signal if this is an interrupt pin.\n");
  fprintf(stderr,
          "\t-o <value>:  Write this value (0 or 1) if this is an output "
          "pin.\n");
  fprintf(stderr, "\t-h: Print this usage information and exit.\n");
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

  if (strcmp(argv[ndx], "-w") == 0)
    {
      havesigno = true;

      if (++ndx >= argc)
        {
          fprintf(stderr, "ERROR: Missing argument to -o\n");
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

  /* Get the pin type */

  ret = ioctl(fd, GPIOC_PINTYPE, (unsigned long)((uintptr_t)&pintype));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to read pintype from %s: %d\n",
              devpath, errcode);
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
                         devpath, (unsigned int)outvalue, errcode);
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
