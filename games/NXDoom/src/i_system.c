/****************************************************************************
 * apps/games/NXDoom/src/i_system.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

#include "deh_str.h"
#include "doomtype.h"
#include "i_joystick.h"
#include "i_sound.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"

#include "i_system.h"

#include "w_wad.h"
#include "z_zone.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFAULT_RAM 16 /* MiB */
#define MIN_RAM 4      /* MiB */

#define DOS_MEM_DUMP_SIZE 10

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct atexit_listentry_s atexit_listentry_t;

struct atexit_listentry_s
{
  atexit_func_t func;
  boolean run_on_error;
  atexit_listentry_t *next;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static atexit_listentry_t *exit_funcs = NULL;

static boolean already_quitting = false;

/* Read Access Violation emulation.
 *
 * From PrBoom+, by entryway.
 */

/* C:\>debug
 * -d 0:0
 *
 * DOS 6.22:
 * 0000:0000  (57 92 19 00) F4 06 70 00-(16 00)
 * DOS 7.1:
 * 0000:0000  (9E 0F C9 00) 65 04 70 00-(16 00)
 * Win98:
 * 0000:0000  (9E 0F C9 00) 65 04 70 00-(16 00)
 * DOSBox under XP:
 * 0000:0000  (00 00 00 F1) ?? ?? ?? 00-(07 00)
 */

static const unsigned char mem_dump_dos622[DOS_MEM_DUMP_SIZE] =
{
  0x57, 0x92, 0x19, 0x00, 0xf4, 0x06, 0x70, 0x00, 0x16, 0x00,
};

static const unsigned char mem_dump_win98[DOS_MEM_DUMP_SIZE] =
{
  0x9e, 0x0f, 0xc9, 0x00, 0x65, 0x04, 0x70, 0x00, 0x16, 0x00,
};

static const unsigned char mem_dump_dosbox[DOS_MEM_DUMP_SIZE] =
{
  0x00, 0x00, 0x00, 0xf1, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
};

static unsigned char mem_dump_custom[DOS_MEM_DUMP_SIZE];

static const unsigned char *dos_mem_dump = mem_dump_dos622;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Zone memory auto-allocation function that allocates the zone size
 * by trying progressively smaller zone sizes until one is found that
 * works.
 */

static byte *auto_alloc_memory(int *size, int default_ram, int min_ram)
{
  byte *zonemem;

  /* Allocate the zone memory.  This loop tries progressively smaller
   * zone sizes until a size is found that can be allocated.
   * If we used the -mb command line parameter, only the parameter
   * provided is accepted.
   */

  zonemem = NULL;

  while (zonemem == NULL)
    {
      /* We need a reasonable minimum amount of RAM to start. */

      if (default_ram < min_ram)
        {
          i_error("Unable to allocate %i MiB of RAM for zone", default_ram);
        }

      /* Try to allocate the zone memory. */

      *size = default_ram * 1024 * 1024;

      zonemem = malloc(*size);

      /* Failed to allocate?  Reduce zone size until we reach a size
       * that is acceptable.
       */

      if (zonemem == NULL)
        {
          default_ram -= 1;
        }
    }

  return zonemem;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void i_at_exit(atexit_func_t func, boolean run_on_error)
{
  atexit_listentry_t *entry;

  entry = malloc(sizeof(*entry));

  entry->func = func;
  entry->run_on_error = run_on_error;
  entry->next = exit_funcs;
  exit_funcs = entry;
}

/* Tactile feedback function, probably used for the Logitech Cyberman */

void i_tactile(int on, int off, int total)
{
}

byte *i_zone_base(int *size)
{
  byte *zonemem;
  int min_ram;
  int default_ram;
  int p;

  /* @category obscure
   * @arg <mb>
   *
   * Specify the heap size, in MiB.
   */

  p = m_check_parm_with_args("-mb", 1);

  if (p > 0)
    {
      default_ram = atoi(myargv[p + 1]);
      min_ram = default_ram;
    }
  else
    {
      /* Because of the 8-byte pointer size in a 64-bit build, the default
       * heap size (16 MiB) is insufficient compared to a 32-bit build. For
       * example, the Alien Vendetta avm62402.lmp demo completes successfully
       * on a 32-bit build, but terminates with an out of memory error on a
       * 64-bit build. Therefore, to maintain consistency with a 32-bit
       * build, the heap size should be increased.
       */

      if (sizeof(void *) == 8)
        {
          default_ram = DEFAULT_RAM * 2;
        }
      else
        {
          default_ram = DEFAULT_RAM;
        }

      min_ram = MIN_RAM;
    }

  zonemem = auto_alloc_memory(size, default_ram, min_ram);

  printf("zone memory: %p, %x allocated for zone\n", zonemem, *size);

  return zonemem;
}

void i_print_banner(const char *msg)
{
  int i;
  int spaces = 35 - (strlen(msg) / 2);

  for (i = 0; i < spaces; ++i)
    putchar(' ');

  puts(msg);
}

void i_print_divider(void)
{
  int i;

  for (i = 0; i < 75; ++i)
    {
      putchar('=');
    }

  putchar('\n');
}

void i_print_startup_banner(const char *gamedescription)
{
  i_print_divider();
  i_print_banner(gamedescription);
  i_print_divider();

  printf(
      " " PACKAGE_NAME
      " is free software, covered by the GNU General Public\n"
      " License.  There is NO warranty; not even for MERCHANTABILITY or "
      "FITNESS\n"
      " FOR A PARTICULAR PURPOSE. You are welcome to change and distribute\n"
      " copies under certain conditions. See the source for more "
      "information.\n");

  i_print_divider();
}

/* i_console_stdout
 *
 * Returns true if stdout is a real console, false if it is a file
 */

boolean i_console_stdout(void)
{
  return isatty(fileno(stdout));
}

/* i_init */

#if 0
void i_init(void)
{
  i_check_is_screensaver();
  i_init_timer();
  i_init_joystick();
}

void i_bind_variables(void)
{
  i_bind_video_variables();
  i_bind_joystick_variables();
  i_bind_sound_variables();
}
#endif

/* i_quit */

void i_quit(void)
{
  atexit_listentry_t *entry;

  /* Run through all exit functions */

  entry = exit_funcs;

  while (entry != NULL)
    {
      entry->func();
      entry = entry->next;
    }

  exit(0);
}

void i_error(const char *error, ...)
{
  char msgbuf[512];
  va_list argptr;
  atexit_listentry_t *entry;
  boolean exit_gui_popup;

  if (already_quitting)
    {
      fprintf(stderr, "Warning: recursive call to i_error detected.\n");
      exit(-1);
    }
  else
    {
      already_quitting = true;
    }

  /* Message first. */

  va_start(argptr, error);

  vfprintf(stderr, error, argptr);
  fprintf(stderr, "\n\n");
  va_end(argptr);
  fflush(stderr);

  /* Write a copy of the message into buffer. */

  va_start(argptr, error);
  memset(msgbuf, 0, sizeof(msgbuf));
  vsnprintf(msgbuf, sizeof(msgbuf), error, argptr);
  va_end(argptr);

  /* Shutdown. Here might be other errors. */

  entry = exit_funcs;

  while (entry != NULL)
    {
      if (entry->run_on_error)
        {
          entry->func();
        }

      entry = entry->next;
    }

  /* @category obscure
   *
   * If specified, don't show a GUI window for error messages when the
   * game exits with an error.
   */

  exit_gui_popup = !m_parm_exists("-nogui");

  /* Pop up a GUI dialog box to show the error message, if the
   * game was not run from the console (and the user will
   * therefore be unable to otherwise see the message).
   */

  if (exit_gui_popup && !i_console_stdout())
    {
    }

  /* abort(); */

  exit(-1);
}

void *i_realloc(void *ptr, size_t size)
{
  void *new_ptr;

  new_ptr = realloc(ptr, size);

  if (size != 0 && new_ptr == NULL)
    {
      i_error("i_realloc: failed on reallocation of %zu bytes", size);
    }

  return new_ptr;
}

boolean i_get_memory_value(unsigned int offset, void *value, int size)
{
  static boolean firsttime = true;

  if (firsttime)
    {
      int p;
      int i;
      int val;

      firsttime = false;
      i = 0;

      /* @category compat
       * @arg <version>
       *
       * Specify DOS version to emulate for NULL pointer dereference
       * emulation.  Supported versions are: dos622, dos71, dosbox.
       * The default is to emulate DOS 7.1 (Windows 98).
       */

      p = m_check_parm_with_args("-setmem", 1);

      if (p > 0)
        {
          if (!strcasecmp(myargv[p + 1], "dos622"))
            {
              dos_mem_dump = mem_dump_dos622;
            }

          if (!strcasecmp(myargv[p + 1], "dos71"))
            {
              dos_mem_dump = mem_dump_win98;
            }
          else if (!strcasecmp(myargv[p + 1], "dosbox"))
            {
              dos_mem_dump = mem_dump_dosbox;
            }
          else
            {
              for (i = 0; i < DOS_MEM_DUMP_SIZE; ++i)
                {
                  ++p;

                  if (p >= myargc || myargv[p][0] == '-')
                    {
                      break;
                    }

                  m_str_to_int(myargv[p], &val);
                  mem_dump_custom[i++] = (unsigned char)val;
                }

              dos_mem_dump = mem_dump_custom;
            }
        }
    }

  switch (size)
    {
    case 1:
      *((unsigned char *)value) = dos_mem_dump[offset];
      return true;
    case 2:
      *((unsigned short *)value) =
          dos_mem_dump[offset] | (dos_mem_dump[offset + 1] << 8);
      return true;
    case 4:
      *((unsigned int *)value) =
          dos_mem_dump[offset] | (dos_mem_dump[offset + 1] << 8) |
          (dos_mem_dump[offset + 2] << 16) |
          (dos_mem_dump[offset + 3] << 24);
      return true;
    }

  return false;
}
