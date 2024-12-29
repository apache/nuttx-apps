/****************************************************************************
 * apps/system/sched_note/note_main.c
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

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <nuttx/sched_note.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_note_daemon_started;
static uint8_t g_note_buffer[CONFIG_SYSTEM_NOTE_BUFFERSIZE];

/****************************************************************************
 * Name: note_daemon
 ****************************************************************************/

static int note_daemon(int argc, char *argv[])
{
  ssize_t nread;
  int fd;

  /* Indicate that we are running */

  g_note_daemon_started = true;
  syslog(LOG_INFO, "note_daemon: Running\n");

  /* Open the note driver */

  syslog(LOG_INFO, "note_daemon: Opening /dev/note/ram\n");
  fd = open("/dev/note/ram", O_RDONLY);
  if (fd < 0)
    {
      int errcode = errno;
      syslog(LOG_ERR, "note_daemon: ERROR: Failed to open /dev/note/ram: "
            "%d\n",
             errcode);
      goto errout;
    }

  /* Now loop forever, dumping note data to the display */

  for (; ; )
    {
      nread = read(fd, g_note_buffer, CONFIG_SYSTEM_NOTE_BUFFERSIZE);
      if (nread > 0)
        {
          syslog(LOG_INFO, "%.*s", (int)nread, g_note_buffer);
        }

      usleep(CONFIG_SYSTEM_NOTE_DELAY * 1000L);
    }

  close(fd);

errout:
  g_note_daemon_started = false;

  syslog(LOG_INFO, "note_daemon: Terminating\n");
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: note_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  printf("note_main: Starting the note_daemon\n");
  if (g_note_daemon_started)
    {
      printf("note_main: note_daemon already running\n");
      return EXIT_SUCCESS;
    }

  ret = task_create("note_daemon", CONFIG_SYSTEM_NOTE_PRIORITY,
                    CONFIG_SYSTEM_NOTE_STACKSIZE, note_daemon,
                    NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("note_main: ERROR: Failed to start note_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  printf("note_main: note_daemon started\n");
  return EXIT_SUCCESS;
}
