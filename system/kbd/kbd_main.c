/****************************************************************************
 * apps/system/kbd/kbd_main.c
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

/* Dump what a keyboard reports.
 *
 * Every keyboard registered with keyboard_register() is read the same way,
 * whatever the hardware behind it is:  a USB HID keyboard, a matrix, the
 * simulator, virtio.  So this works with all of them, and it is the place
 * to look when bringing up a new one.
 *
 * The device delivers struct keyboard_event_s events unless the kernel was
 * built with INPUT_KEYBOARD_BYTESTREAM, in which case it delivers the byte
 * stream that the keyboard codec defines.  This follows that setting rather
 * than having a switch of its own.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/input/keyboard.h>

#ifdef CONFIG_INPUT_KEYBOARD_BYTESTREAM
#  include <nuttx/streams.h>
#  include <nuttx/input/kbd_codec.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define KBD_READ_SIZE 64

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: kbd_typename
 ****************************************************************************/

static FAR const char *kbd_typename(uint32_t type)
{
  switch (type)
    {
      case KEYBOARD_PRESS:
        return "press";

      case KEYBOARD_RELEASE:
        return "release";

      case KEYBOARD_SPECPRESS:
        return "specpress";

      case KEYBOARD_SPECREL:
        return "specrel";

      default:
        return "unknown";
    }
}

/****************************************************************************
 * Name: kbd_show
 *
 * Description:
 *   Print one key.  A normal key carries a character, a special key carries
 *   a value from enum kbd_keycode_e, and the type is what says which.
 *
 ****************************************************************************/

static void kbd_show(uint32_t code, uint32_t type)
{
  if (type == KEYBOARD_PRESS || type == KEYBOARD_RELEASE)
    {
      printf("%-9s code %3" PRIu32 " '%c'\n", kbd_typename(type), code,
             isprint(code) ? (int)code : '.');
    }
  else
    {
      printf("%-9s keycode %" PRIu32 "\n", kbd_typename(type), code);
    }

  fflush(stdout);
}

/****************************************************************************
 * Name: kbd_dump
 *
 * Description:
 *   Print everything in one read.
 *
 * Returned Value:
 *   The number of keys printed.
 *
 ****************************************************************************/

#ifdef CONFIG_INPUT_KEYBOARD_BYTESTREAM
static size_t kbd_dump(FAR char *buffer, size_t nbytes)
{
  struct lib_meminstream_s stream;
  struct kbd_getstate_s state;
  uint8_t ch;
  size_t nkeys = 0;
  int ret;

  lib_meminstream(&stream, buffer, nbytes);
  memset(&state, 0, sizeof(state));

  for (; ; )
    {
      ret = kbd_decode(&stream.common, &state, &ch);
      if (ret == KBD_ERROR)
        {
          break;
        }

      kbd_show(ch, ret);
      nkeys++;
    }

  return nkeys;
}
#else
static size_t kbd_dump(FAR char *buffer, size_t nbytes)
{
  FAR struct keyboard_event_s *key = (FAR struct keyboard_event_s *)buffer;
  size_t nkeys = 0;

  if ((nbytes % sizeof(struct keyboard_event_s)) != 0)
    {
      fprintf(stderr, "kbd: short read of %zu bytes, ignoring\n", nbytes);
      return 0;
    }

  while (nbytes >= sizeof(struct keyboard_event_s))
    {
      kbd_show(key->code, key->type);

      nbytes -= sizeof(struct keyboard_event_s);
      key++;
      nkeys++;
    }

  return nkeys;
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
  FAR const char *devpath = CONFIG_SYSTEM_KBD_DEVPATH;
  char buffer[KBD_READ_SIZE];
  size_t nkeys = 0;
  size_t limit = 0;
  ssize_t nbytes;
  int fd;

  if (argc > 1)
    {
      devpath = argv[1];
    }

  if (argc > 2)
    {
      limit = strtoul(argv[2], NULL, 10);
    }

  /* Wait for the device.  A USB keyboard appears when it is plugged in, so
   * this is the normal way to start rather than an error path.
   */

  for (; ; )
    {
      fd = open(devpath, O_RDONLY);
      if (fd >= 0)
        {
          break;
        }

      printf("kbd: waiting for %s: %d\n", devpath, errno);
      fflush(stdout);
      sleep(3);
    }

  printf("kbd: reading %s, %s\n", devpath,
#ifdef CONFIG_INPUT_KEYBOARD_BYTESTREAM
         "byte stream"
#else
         "keyboard events"
#endif
        );
  fflush(stdout);

  for (; ; )
    {
      nbytes = read(fd, buffer, sizeof(buffer));
      if (nbytes < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          fprintf(stderr, "kbd: read %s failed: %d\n", devpath, errno);
          break;
        }
      else if (nbytes == 0)
        {
          /* The keyboard is gone */

          break;
        }

      nkeys += kbd_dump(buffer, nbytes);

      if (limit > 0 && nkeys >= limit)
        {
          break;
        }
    }

  close(fd);
  printf("kbd: %zu keys\n", nkeys);
  return EXIT_SUCCESS;
}
