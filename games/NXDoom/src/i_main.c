/****************************************************************************
 * apps/games/NXDoom/src/i_main.c
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
 * DESCRIPTION:
 *  Main program, simply calls d_doom_main high level loop.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: d_doom_main()
 *
 * Description:
 *  Not a globally visible function, just included for source reference,
 *  calls all startup code, parses command line options.
 *
 ****************************************************************************/

void d_doom_main(void);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  /* Lets nxstore (or any other supervisor) ask this process to exit
   * cleanly via SIGTERM instead of the only other option being a forced
   * task_delete() from outside - see i_system.h/i_system.c for why that
   * matters on this board (a forced kill mid framebuffer/heap access was
   * observed to hang the whole system, not just this task).
   */

  i_install_quit_signal();

  /* save arguments */

  myargc = argc;
  myargv = malloc(argc * sizeof(char *));
  assert(myargv != NULL);

  for (int i = 0; i < argc; i++)
    {
      myargv[i] = m_string_duplicate(argv[i]);
    }

  /* Print the program version and exit. */

  if (m_parm_exists("-version") || m_parm_exists("--version"))
    {
      puts(PACKAGE_STRING);
      exit(0);
    }

  m_find_response_file();
  m_set_exe_dir();

  /* start doom */

  d_doom_main();

  return 0;
}
