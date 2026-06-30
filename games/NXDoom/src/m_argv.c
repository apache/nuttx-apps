/****************************************************************************
 * apps/games/NXDoom/src/m_argv.c
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "d_iwad.h"
#include "doomtype.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAXARGVS 100

/****************************************************************************
 * Public Data
 ****************************************************************************/

int myargc;
char **myargv;

char *exedir = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void load_response_file(int argv_index, const char *filename)
{
  FILE *handle;
  int size;
  char *infile;
  char *file;
  char **newargv;
  int newargc;
  int i;
  int k;

  /* Read the response file into memory */

  handle = fopen(filename, "rb");

  if (handle == NULL)
    {
      printf("\nNo such response file!");
      exit(1);
    }

  printf("Found response file %s!\n", filename);

  size = m_file_length(handle);

  /* Read in the entire file
   * Allocate one byte extra - this is in case there is an argument
   * at the end of the response file, in which case a '\0' will be
   * needed.
   */

  file = malloc(size + 1);

  i = 0;

  while (i < size)
    {
      k = fread(file + i, 1, size - i, handle);

      if (k < 0)
        {
          i_error("Failed to read full contents of '%s'", filename);
        }

      i += k;
    }

  fclose(handle);

  /* Create new arguments list array */

  newargv = malloc(sizeof(char *) * MAXARGVS);
  newargc = 0;
  memset(newargv, 0, sizeof(char *) * MAXARGVS);

  /* Copy all the arguments in the list up to the response file */

  if (argv_index >= MAXARGVS)
    {
      i_error("Too many arguments up to the response file!");
    }

  for (i = 0; i < argv_index; ++i)
    {
      newargv[i] = myargv[i];
      myargv[i] = NULL;
      ++newargc;
    }

  infile = file;
  k = 0;

  while (k < size)
    {
      /* Skip past space characters to the next argument */

      while (k < size && isspace(infile[k]))
        {
          ++k;
        }

      if (k >= size)
        {
          break;
        }

      /* If the next argument is enclosed in quote marks, treat
       * the contents as a single argument.  This allows long filenames
       * to be specified.
       */

      if (infile[k] == '\"')
        {
          char *argstart;

          /* Skip the first character(") */

          ++k;

          argstart = &infile[k];

          /* Read all characters between quotes */

          while (k < size && infile[k] != '\"' && infile[k] != '\n')
            {
              ++k;
            }

          if (k >= size || infile[k] == '\n')
            {
              i_error("Quotes unclosed in response file '%s'", filename);
            }

          /* Cut off the string at the closing quote */

          infile[k] = '\0';
          ++k;

          if (newargc >= MAXARGVS)
            {
              i_error("Too many arguments in the response file!");
            }

          newargv[newargc++] = m_string_duplicate(argstart);
        }
      else
        {
          char *argstart;

          /* Read in the next argument until a space is reached */

          argstart = &infile[k];

          while (k < size && !isspace(infile[k]))
            {
              ++k;
            }

          /* Cut off the end of the argument at the first space */

          infile[k] = '\0';
          ++k;

          if (newargc >= MAXARGVS)
            {
              i_error("Too many arguments in the response file!");
            }

          newargv[newargc++] = m_string_duplicate(argstart);
        }
    }

  /* Add arguments following the response file argument */

  if (newargc + myargc - (argv_index + 1) >= MAXARGVS)
    {
      i_error("Too many arguments following the response file!");
    }

  for (i = argv_index + 1; i < myargc; ++i)
    {
      newargv[newargc] = myargv[i];
      myargv[i] = NULL;
      ++newargc;
    }

  /* Free any old strings in myargv which were not moved to newargv */

  for (i = 0; i < myargc; ++i)
    {
      if (myargv[i] != NULL)
        {
          free(myargv[i]);
          myargv[i] = NULL;
        }
    }

  free(myargv);
  myargv = newargv;
  myargc = newargc;

  free(file);

#if 0
  /* Disabled - Vanilla Doom does not do this.
   * Display arguments
   */

  printf("%d command-line args:\n", myargc);

  for (k = 1; k < myargc; k++)
    {
      printf("'%s'\n", myargv[k]);
    }
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* m_check_parm
 * Checks for the given parameter in the program's command line arguments.
 * Returns the argument number (1 to argc-1) or 0 if not present
 */

int m_check_parm_with_args(const char *check, int num_args)
{
  int i;

  /* Check if myargv[i] has been set to NULL in load_response_file(),
   * which may call i_error(), which in turn calls m_parm_exists("-nogui").
   */

  for (i = 1; i < myargc - num_args && myargv[i]; i++)
    {
      if (!strcasecmp(check, myargv[i])) return i;
    }

  return 0;
}

/* m_parm_exists
 *
 * Returns true if the given parameter exists in the program's command
 * line arguments, false if not.
 */

boolean m_parm_exists(const char *check)
{
  return m_check_parm(check) != 0;
}

int m_check_parm(const char *check)
{
  return m_check_parm_with_args(check, 0);
}

/* Find a Response File */

void m_find_response_file(void)
{
  int i;

  for (i = 1; i < myargc; i++)
    {
      if (myargv[i][0] == '@')
        {
          load_response_file(i, myargv[i] + 1);
        }
    }

  for (; ; )
    {
      /* @arg <file>
       *
       * Load extra command-line arguments from the given response
       * file.  Arguments read from the file are inserted into the
       * command line, replacing this argument.  A response file can
       * also be loaded using the abbreviated syntax '@file.rsp'.
       */

      i = m_check_parm_with_args("-response", 1);
      if (i <= 0)
        {
          break;
        }

      /* Replace the -response argument so that the next time through
       * the loop we'll ignore it. Since some parameters stop reading when
       * an argument beginning with a '-' is encountered, we keep something
       * that starts with a '-'.
       */

      free(myargv[i]);
      myargv[i] = m_string_duplicate("-_");
      load_response_file(i + 1, myargv[i + 1]);
    }
}

/* Return the name of the executable used to start the program: */

const char *m_get_executable_name(void)
{
  return m_base_name(myargv[0]);
}

void m_set_exe_dir(void)
{
  char *dirname;

  dirname = m_dir_name(myargv[0]);
  exedir = m_string_join(dirname, DIR_SEPARATOR_S, NULL);
  free(dirname);
}
