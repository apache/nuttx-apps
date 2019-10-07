/****************************************************************************
 * apps/system/lm75/lm75.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <fixedmath.h>

#include <nuttx/sensors/lm75.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_SYSTEM_LM75_DEVNAME
#  warning CONFIG_SYSTEM_LM75_DEVNAME is not defined
#  define CONFIG_SYSTEM_LM75_DEVNAME "/dev/temp"
#endif

#if !defined(CONFIG_SYSTEM_LM75_FAHRENHEIT) && !defined(CONFIG_SYSTEM_LM75_CELSIUS)
#  warning one of CONFIG_SYSTEM_LM75_FAHRENHEIT or CONFIG_SYSTEM_LM75_CELSIUS must be defined
#  defeind CONFIG_SYSTEM_LM75_FAHRENHEIT 1
#endif

#if defined(CONFIG_SYSTEM_LM75_FAHRENHEIT) && defined(CONFIG_SYSTEM_LM75_CELSIUS)
#  warning both of CONFIG_SYSTEM_LM75_FAHRENHEIT and CONFIG_SYSTEM_LM75_CELSIUS defined
#  undef CONFIG_SYSTEM_LM75_CELSIUS
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_samples = 1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lm75_help
 ****************************************************************************/

static void lm75_help(void)
{
  printf("Usage: temp [OPTIONS]\n");
  printf("  [-n count] selects the samples to collect.  "
         "Default: 1 Current: %d\n", g_samples);
  printf("  [-h] shows this message and exits\n");
}

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

static int arg_string(FAR char **arg, FAR char **value)
{
  FAR char *ptr = *arg;

  if (ptr[2] == '\0')
    {
      *value = arg[1];
      return 2;
    }
  else
    {
      *value = &ptr[2];
      return 1;
    }
}

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

static int arg_decimal(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(int argc, FAR char **argv)
{
  FAR char *ptr;
  long value;
  int index;
  int nargs;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          printf("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
          case 'n':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0)
              {
                printf("Count must be non-negative: %ld\n", value);
                exit(1);
              }

            g_samples = (int)value;
            index += nargs;
            break;

          case 'h':
            lm75_help();
            exit(0);

          default:
            printf("Unsupported option: %s\n", ptr);
            lm75_help();
            exit(1);
        }
    }
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_LIBC_FLOATINGPOINT
  double temp;
#endif
  b16_t temp16;
  ssize_t nbytes;
  int fd;
  int ret;
  int i;

  /* Open the temperature sensor device */

  fd  = open(CONFIG_SYSTEM_LM75_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_SYSTEM_LM75_DEVNAME, errno);
      return EXIT_FAILURE;
    }

#ifdef CONFIG_SYSTEM_LM75_FAHRENHEIT
  /* Select Fahrenheit scaling */

  ret = ioctl(fd, SNIOC_FAHRENHEIT, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(SNIOC_FAHRENHEIT) failed: %d\n",
              errno);
      return EXIT_FAILURE;
    }
#else
  /* Select Centigrade scaling */

  ret = ioctl(fd, SNIOC_CENTIGRADE, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(SNIOC_CENTIGRADE) failed: %d\n",
              errno);
      return EXIT_FAILURE;
    }
#endif

  /* Parse the command line */

  parse_args(argc, argv);

  for (i = 0; i < g_samples; i++)
    {
      /* Read the current temperature as a fixed precision number */

      nbytes = read(fd, &temp16, sizeof(b16_t));

      if (nbytes < 0)
        {
          fprintf(stderr, "ERROR: read(%d) failed: %d\n",
                  sizeof(b16_t), errno);
          return EXIT_FAILURE;
        }

      if (nbytes != sizeof(b16_t))
        {
          fprintf(stderr, "ERROR: Unexpected read size: %Ld vs %d\n",
                  (long)nbytes, sizeof(b16_t));
          return EXIT_FAILURE;
        }

      /* Print the current temperature on stdout */

#ifdef CONFIG_LIBC_FLOATINGPOINT
      temp = (double)temp16 / 65536.0;
#  ifdef CONFIG_SYSTEM_LM75_FAHRENHEIT
      printf("%3.2f degrees Fahrenheit\n", temp);
#  else
      printf("%3.2f degrees Celsius\n", temp);
#  endif

#else
#  ifdef CONFIG_SYSTEM_LM75_FAHRENHEIT
      printf("0x%04x.%04x degrees Fahrenheit\n", temp16 >> 16, temp16 & 0xffff);
#  else
      printf("0x%04x.%04x degrees Celsius\n", temp16 >> 16, temp16 & 0xffff);
#  endif
#endif

      /* Force it to print out the lines */

      fflush(stdout);

      /* Wait 500 ms */

      usleep(500000);
    }

  close(fd);

  return EXIT_SUCCESS;
}
