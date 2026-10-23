/****************************************************************************
 * apps/examples/hidcombo/hidcombo_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>

#include <nuttx/input/keyboard.h>
#include <nuttx/input/mouse.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HIDCOMBO_NEVENTS  40     /* Default number of events to wait for */
#define HIDCOMBO_TIMEOUT  30000  /* Give up after 30 seconds without events */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR const char *hidcombo_keytype(uint32_t type)
{
  switch (type)
    {
      case KEYBOARD_PRESS:
        return "press";

      case KEYBOARD_RELEASE:
        return "release";

      case KEYBOARD_SPECPRESS:
        return "special press";

      case KEYBOARD_SPECREL:
        return "special release";

      default:
        return "unknown";
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hidcombo_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct keyboard_event_s key;
  struct mouse_report_s mouse;
  struct pollfd fds[2];
  int nevents = HIDCOMBO_NEVENTS;
  int nkeys = 0;
  int nmouse = 0;
  int ret;

  if (argc > 1)
    {
      nevents = atoi(argv[1]);
    }

  fds[0].fd = open(CONFIG_EXAMPLES_HIDCOMBO_KBDDEV, O_RDONLY | O_NONBLOCK);
  if (fds[0].fd < 0)
    {
      printf("ERROR: Failed to open %s: %d\n",
             CONFIG_EXAMPLES_HIDCOMBO_KBDDEV, errno);
      return EXIT_FAILURE;
    }

  fds[1].fd = open(CONFIG_EXAMPLES_HIDCOMBO_MOUSEDEV, O_RDONLY | O_NONBLOCK);
  if (fds[1].fd < 0)
    {
      printf("ERROR: Failed to open %s: %d\n",
             CONFIG_EXAMPLES_HIDCOMBO_MOUSEDEV, errno);
      close(fds[0].fd);
      return EXIT_FAILURE;
    }

  fds[0].events = POLLIN;
  fds[1].events = POLLIN;

  printf("hidcombo: waiting for %d events from %s and %s\n", nevents,
         CONFIG_EXAMPLES_HIDCOMBO_KBDDEV, CONFIG_EXAMPLES_HIDCOMBO_MOUSEDEV);

  while (nkeys + nmouse < nevents)
    {
      fds[0].revents = 0;
      fds[1].revents = 0;

      ret = poll(fds, 2, HIDCOMBO_TIMEOUT);
      if (ret < 0)
        {
          printf("ERROR: poll failed: %d\n", errno);
          break;
        }
      else if (ret == 0)
        {
          printf("hidcombo: no events for %d seconds\n",
                 HIDCOMBO_TIMEOUT / 1000);
          break;
        }

      /* Read all of the buffered key events */

      if ((fds[0].revents & POLLIN) != 0)
        {
          while (read(fds[0].fd, &key, sizeof(key)) == sizeof(key))
            {
              nkeys++;
              printf("KEY   %-15s code=0x%02lx '%c'\n",
                     hidcombo_keytype(key.type), (unsigned long)key.code,
                     key.type <= KEYBOARD_RELEASE &&
                     key.code >= 0x20 && key.code < 0x7f ?
                     (int)key.code : '.');
            }
        }

      /* The mouse driver keeps only the latest sample */

      if ((fds[1].revents & POLLIN) != 0 &&
          read(fds[1].fd, &mouse, sizeof(mouse)) == sizeof(mouse))
        {
          nmouse++;
          printf("MOUSE buttons=%02x x=%d y=%d\n",
                 mouse.buttons, mouse.x, mouse.y);
        }
    }

  printf("hidcombo: %d key events, %d mouse events\n", nkeys, nmouse);

  close(fds[1].fd);
  close(fds[0].fd);
  return EXIT_SUCCESS;
}
