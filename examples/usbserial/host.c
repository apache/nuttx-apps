/****************************************************************************
 * apps/examples/usbserial/host.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_USBSERIAL_INONLY) && \
    defined(CONFIG_EXAMPLES_USBSERIAL_OUTONLY)
#  error "Cannot define both CONFIG_EXAMPLES_USBSERIAL_INONLY and _OUTONLY"
#endif
#if defined(CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL) && \
    defined(CONFIG_EXAMPLES_USBSERIAL_ONLYBIG)
#  error "Cannot define both CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL and _ONLYBIG"
#endif

#if !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYBIG) && \
    !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL)
#  ifndef CONFIG_EXAMPLES_USBSERIAL_INONLY
#    define COUNTER_NEEDED 1
#  endif
#endif

#ifdef CONFIG_CDCACM
#  define DEFAULT_TTYDEV "/dev/ttyACM0"
#else
#  define DEFAULT_TTYDEV "/dev/ttyUSB0"
#endif
#define BUFFER_SIZE    1024

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *g_ttydev = DEFAULT_TTYDEV;

#ifndef CONFIG_EXAMPLES_USBSERIAL_ONLYBIG
static const char g_shortmsg[] = "Sure... You betcha!!\n";
#endif

#ifndef CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL
static const char g_longmsg[] =
  "I am proud to come to this city as the guest of your distinguished "
  "Mayor, who has symbolized throughout the world the fighting spirit of "
  "West Berlin. And I am proud to visit the Federal Republic with your "
  "distinguished Chancellor who for so many years has committed Germany "
  "to democracy and freedom and progress, and to come here in the company "
  "of my fellow American, General Clay, who has been in this city during "
  "its great moments of crisis and will come again if ever needed.\n"
  "Two thousand years ago the proudest boast was \"civis Romanus sum.\" "
  "Today, in the world of freedom, the proudest boast is "
  "\"Ich bin ein Berliner.\"\r\"I appreciate my interpreter translating "
  "my German!\nThere are many people in the world who really "
  "don't understand, or say they don't, what is the great issue between "
  "the free world and the Communist world. Let them come to Berlin. "
  "There are some who say that communism is the wave of the future. "
  "Let them come to Berlin. And there are some who say in Europe and "
  "elsewhere we can work with the Communists. Let them come to Berlin. "
  "And there are even a few who say that it is true that communism is "
  "an evil system, but it permits us to make economic progress. "
  "Lass' sie nach Berlin kommen. Let them come to Berlin.\n"
  "Freedom has many difficulties and democracy is not perfect, but we have "
  "never had to put a wall up to keep our people in, to prevent them from "
  "leaving us. I want to say, on behalf of my countrymen, who live many "
  "miles away on the other side of the Atlantic, who are far distant "
  "from you, that they take the greatest pride that they have been able "
  "to share with you, even from a distance, the story of the last 18 years. "
  "I know of no town, no city, that has been besieged for 18 years that "
  "still lives with the vitality and the force, and the hope and the "
  "determination of the city of West Berlin. While the wall is the most "
  "obvious and vivid demonstration of the failures of the Communist system, "
  "for all the world to see, we take no satisfaction in it, for it is, "
  "as your Mayor has said, an offense not only against history but an "
  "offense against humanity, separating families, dividing husbands and "
  "wives and brothers and sisters, and dividing a people who wish to be "
  "joined together.\nWhat is true of this city is true of Germany--real, "
  "lasting peace in Europe can never be assured as long as one German "
  "out of four is denied the elementary right of free men, and that is "
  "to make a free choice. In 18 years of peace and good faith, this "
  "generation of Germans has earned the right to be free, including the "
  "right to unite their families and their nation in lasting peace, "
  "with good will to all people. You live in a defended island of freedom, "
  "but your life is part of the main. So let me ask you as I close, to lift "
  "your eyes beyond the dangers of today, to the hopes of tomorrow, beyond "
  "the freedom merely of this city of Berlin, or your country of Germany, "
  "to the advance of freedom everywhere, beyond the wall to the day of "
  "peace with justice, beyond yourselves and ourselves to all mankind.\n"
  "Freedom is indivisible, and when one man is enslaved, all are not free. "
  "When all are free, then we can look forward to that day when this city "
  "will be joined as one and this country and this great Continent of "
  "Europe in a peaceful and hopeful globe. When that day finally comes, "
  "as it will, the people of West Berlin can take sober satisfaction "
  "in the fact that they were in the front lines for almost two decades.\n"
  "All free men, wherever they may live, are citizens of Berlin, and, "
  "therefore, as a free man, I take pride in the words "
  "\"Ich bin ein Berliner.\"\n"
  "President John F. Kennedy - June 26, 1963\n";
#endif

#ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
static char g_iobuffer[BUFFER_SIZE];
#endif

/****************************************************************************
 * show_usage
 ****************************************************************************/

static void show_usage(const char *progname, int exitcode)
{
  fprintf(stderr, "USAGE: %s [<ttydev>]\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * main
 ****************************************************************************/

int main(int argc, char **argv, char **envp)
{
  struct termios tty;
#ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
  ssize_t nbytes;
#endif
#ifdef COUNTER_NEEDED
  int count = 0;
#endif
  int fd;
  int ret;

  /* Handle input parameters */

  if (argc > 1)
    {
      if (argc > 2)
        {
          fprintf(stderr, "Too many arguments on command line\n");
          show_usage(argv[0], 1);
        }

      g_ttydev = argv[1];
    }

  /* Open the USB serial device for blocking read/write */

  do
    {
      printf("main: Opening USB serial driver\n");
      fd = open(g_ttydev, O_RDWR);
      if (fd < 0)
        {
          printf("main: ERROR: Failed to open %s: %s\n",
                 g_ttydev, strerror(errno));
          printf("main:        Assume not connected. Wait and try again.\n");
          printf("main:        (Control-C to terminate).\n");
          sleep(5);
        }
    }
  while (fd < 0);

  printf("main: Successfully opened the serial driver\n");

  /* Configure the serial port in raw mode (at least turn off echo) */

  ret = tcgetattr(fd, &tty);
  if (ret < 0)
    {
      printf("main: ERROR: Failed to get termios for %s: %s\n",
             g_ttydev, strerror(errno));
      close(fd);
      return 1;
    }

  tty.c_iflag &=
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tty.c_oflag &= ~OPOST;
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tty.c_cflag &= ~(CSIZE | PARENB);
  tty.c_cflag |= CS8;

  ret = tcsetattr(fd, TCSANOW, &tty);
  if (ret < 0)
    {
      printf("main: ERROR: Failed to set termios for %s: %s\n",
             g_ttydev, strerror(errno));
      close(fd);
      return 1;
    }

  /* Wait for and/or send messages -- forever */

  for (; ; )
    {
      /* Test IN messages (device-to-host) */

#ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
      printf("main: Reading from the serial driver\n");
      printf("main: ... (Control-C to terminate) ...\n");
      nbytes = read(fd, g_iobuffer, BUFFER_SIZE - 1);
      if (nbytes < 0)
        {
          printf("main: ERROR: Failed to read from %s: %s\n",
                 g_ttydev, strerror(errno));
          close(fd);
          return 2;
        }
      else if (nbytes == 0)
        {
          printf("main: End-of-file encountered\n");
          break;
        }

      g_iobuffer[nbytes] = '\0';
      printf("main: Received %ld bytes:\n", (long)nbytes);
      printf("      \"%s\"\n", g_iobuffer);
#else
      printf("main: Waiting...\n");
      sleep(5);
#endif /* CONFIG_EXAMPLES_USBSERIAL_OUTONLY */

      /* Test OUT messages (host-to-device) */

#ifndef CONFIG_EXAMPLES_USBSERIAL_INONLY
#if !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL) && \
    !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYBIG)
      count++;
      if (count < 5)
        {
          printf("main: Sending %d bytes..\n", sizeof(g_shortmsg));
          nbytes = write(fd, g_shortmsg, sizeof(g_shortmsg));
        }
      else
        {
          printf("main: Sending %d bytes..\n", sizeof(g_longmsg));
          nbytes = write(fd, g_longmsg, sizeof(g_longmsg));
          count = 0;
        }

#elif !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL)
      printf("main: Sending %d bytes..\n", sizeof(g_longmsg));
      nbytes = write(fd, g_longmsg, sizeof(g_longmsg));

#else /* !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYBIG) */
      printf("main: Sending %d bytes..\n", sizeof(g_shortmsg));
      nbytes = write(fd, g_shortmsg, sizeof(g_shortmsg));
#endif

      /* Test if write was successful */

      if (nbytes < 0)
        {
          printf("main: ERROR: Failed to write to %s: %s\n", g_ttydev,
                 strerror(errno));
          close(fd);
          return 2;
        }

      printf("main: %ld bytes sent\n", (long)nbytes);
#endif /* CONFIG_EXAMPLES_USBSERIAL_INONLY */
    }

  close(fd);
  return 0;
}
