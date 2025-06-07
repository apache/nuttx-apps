/****************************************************************************
 * apps/system/syslogd/syslogd_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for asyslogditional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with the
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef CONFIG_LIBC_EXECFUNCS
#include <spawn.h>
#endif

#include <getopt.h>
#include <syslog.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* syslogd program version */

#define SYSLOGD_VERSION "0.0.0"

/* Minimum buffer size check */

#if CONFIG_SYSTEM_SYSLOGD_ENTRYSIZE < 480
#error "SYSTEM_SYSLOGD_ENTRYSIZE must be more than 480 to satisfy RFC 5424"
#endif

/* Maximum number of arguments that can be passed to syslogd */

#define MAX_ARGS 8

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: print_usage
 ****************************************************************************/

static void print_usage(void)
{
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  %s [-vdn]\n", CONFIG_SYSTEM_SYSLOGD_PROGNAME);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  int fd;
  int sock;
  int c;
  ssize_t bread;
  ssize_t bsent;
  ssize_t endpos;
  char *end;
  size_t bufpos = 0;
  struct sockaddr_in server;
  char buffer[CONFIG_SYSTEM_SYSLOGD_ENTRYSIZE];
  bool debugmode = false;
  bool skiplog = false;
#ifdef CONFIG_LIBC_EXECFUNCS
  pid_t pid;
  bool background = true;
  char *new_argv[MAX_ARGS + 1];
#endif

  /* Parse command line options */

  while ((c = getopt(argc, argv, ":vdn")) != -1)
    {
      switch (c)
        {
        case 'v':

          /* Print version and exit */

          printf("%s " SYSLOGD_VERSION " (NuttX)\n", argv[0]);
          return EXIT_SUCCESS;

        case 'd':

          /* Enable debug mode and stay in foreground */

          debugmode = true;
          printf("Enabling debug mode.\n");
#ifdef CONFIG_LIBC_EXECFUNCS
          background = false;
#endif
          break;

        case 'n':

          /* Stay in foreground */

#ifdef CONFIG_LIBC_EXECFUNCS
          background = false;
#endif
          break;

        case '?':
          print_usage();
          exit(EXIT_FAILURE);
          break;
        }
    }

    /* Run this program in the background as a spawned task if the background
     * option was selected.
     */

#ifdef CONFIG_LIBC_EXECFUNCS
  if (background)
    {
      /* Set up the arguments, which is identical to the original except with
       * an added `-n` flag to ensure that the new process does not 'respawn'
       */

      if (argc > MAX_ARGS)
        {
          fprintf(stderr,
                  "Cannot spawn syslogd daemon: arg count %d exceeds %d",
                  argc, MAX_ARGS);
          return EXIT_FAILURE;
        }

      new_argv[0] = argv[0]; /* Same program name */
      new_argv[1] = "-n";    /* Prevent daemon from spawning another child */
      memcpy(&new_argv[2], &argv[1], sizeof(char *) * (argc - 1));

      /* Spawn the child for backgrounding now */

      if (posix_spawn(&pid, argv[0], NULL, NULL, new_argv, NULL) != 0)
        {
          fprintf(stderr, "Failed to fork() to background process: %d\n",
                  errno);
          return EXIT_FAILURE;
        }
      else
        {
          /* We succeeded in spawning, exit now. */

          return EXIT_SUCCESS;
        }
    }
#endif /* CONFIG_LIBC_EXECFUNCS */

  /* Set up client connection information */

  server.sin_family = AF_INET;
  server.sin_port = htons(CONFIG_SYSTEM_SYSLOGD_PORT);
  server.sin_addr.s_addr = inet_addr(CONFIG_SYSTEM_SYSLOGD_ADDR);

  if (server.sin_addr.s_addr == INADDR_NONE)
    {
      fprintf(stderr, "Invalid address '%s'\n", CONFIG_SYSTEM_SYSLOGD_ADDR);
      return EXIT_FAILURE;
    }

  /* Create a UDP socket */

  if (debugmode)
    {
      printf("Creating UDP socket %s:%u\n", CONFIG_SYSTEM_SYSLOGD_ADDR,
             CONFIG_SYSTEM_SYSLOGD_PORT);
    }

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      fprintf(stderr, "Couldn't create UDP socket: %d\n", errno);
      return EXIT_FAILURE;
    }

  /* Open syslog stream */

  if (debugmode)
    {
      printf("Opening syslog device '%s' to read entries.\n",
             CONFIG_SYSLOG_DEVPATH);
    }

  fd = open(CONFIG_SYSLOG_DEVPATH, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "Could not open syslog stream: %d", errno);
      close(sock);
      return EXIT_FAILURE;
    }

  /* Transmit syslog messages forever */

  if (debugmode)
    {
      printf("Beginning to continuously transmit syslog entries.\n");
    }

  for (; ; )
    {
      /* Read as much data as possible into the remaining space in our buffer
       */

      bread = read(fd, &buffer[bufpos], sizeof(buffer) - bufpos);
      if (bread < 0)
        {
          fprintf(stderr, "Failed to read from syslog: %d", errno);
          close(fd);
          close(sock);
          return EXIT_FAILURE;
        }

      if (bread == 0 && bufpos == 0)
        {
          /* Stream is over, terminate the program. */

          if (debugmode)
            {
              printf("Syslog stream depleted, exiting...\n");
            }

          break; /* Successful exit */
        }

      /* Get the position of the '\n' character of the syslog entry,
       * signifying its end.
       */

      end = memchr(buffer, '\n', bufpos + bread);
      if (end == NULL)
        {
          endpos = -1;
        }
      else
        {
          endpos = end - buffer;
        }

      if (endpos < 0)
        {
          /* If we couldn't find a newline character in the buffer, it means
           * that the syslog entry doesn't end in our local buffer. We either
           * need to:
           *
           * 1) If `bread` is 0, acknowledge that there is no more data to be
           * read and our buffer will never contain a newline character. We
           * can exit successfully in this case.
           *
           * 2) Read more and try again if there's still room in our buffer
           *
           * 3) Acknowledge that the syslog entry is too long for our buffer
           * size, and skip it since we can't construct a UDP packet if
           * that's the case.
           */

          if (bread == 0)
            {
              break; /* Successful exit */
            }
          else if (bufpos + bread < sizeof(buffer))
            {
              /* Try to get more bytes in our buffer in case we read while
               * more bytes were coming.
               */

              bufpos += bread;
              continue;
            }

          /* If we are here, there's no room left in our buffer. We need to
           * skip this entry.
           */

          fprintf(stderr, "Couldn't find end of log in local buffer, "
                          "skipping entry until the next newline...\n");
          skiplog = true;
          bufpos = 0; /* Wipe all buffer contents */
          continue;
        }

      /* Print out entry if we are in debug mode and not skipping this line.
       * `endpos` + 1 to print newline too.
       */

      if (debugmode && !skiplog)
        {
          bsent = write(0, buffer, endpos + 1);
          if (bsent < 0)
            {
              fprintf(stderr, "Couldn't print syslog entry: %d\n", errno);
            }
        }

      /* Send entry over UDP (without newline) if we're not skipping this
       * line
       */

      if (!skiplog)
        {
          bsent = sendto(sock, buffer, endpos, 0,
                         (const struct sockaddr *)&server, sizeof(server));
          if (bsent < 0)
            {
              fprintf(stderr, "Couldn't send syslog over UDP: %d\n", errno);
            }
        }

      /* Take whatever bytes were leftover from our bulk read, and move them
       * to the front of the buffer. Mark the new starting point for the next
       * read so we don't overwrite them.
       *
       * endpos + 1 to overwrite vestigial newline character.
       */

      bufpos = (bufpos + bread) - (endpos + 1);

      if (bufpos > 0)
        {
          /* Copy from right after the newline character up until the end of
           * unread bytes
           */

          memcpy(buffer, &buffer[endpos + 1], bufpos);
        }

      /* If we got here while skipping a log, it means the endpos for the log
       * being skipped was found. Now we're done skipping the log.
       */

      if (skiplog)
        {
          skiplog = false;
        }
    }

  close(fd);
  close(sock);

  return EXIT_SUCCESS;
}
