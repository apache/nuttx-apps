/****************************************************************************
 * apps/games/NXDoom/src/deh_main.c
 *
 * SPDX-License-Identifier: GPLv2
 *
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
 * Main dehacked code
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
#include "i_glob.h"
#include "i_system.h"
#include "m_argv.h"
#include "w_wad.h"

#include "deh_defs.h"
#include "deh_io.h"
#include "deh_main.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static boolean deh_initialized = false;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* If true, we can parse [STRINGS] sections in BEX format. */

boolean deh_allow_extended_strings = false;

/* If true, we can do long string replacements. */

boolean deh_allow_long_strings = false;

/* If true, we can do cheat replacements longer than the originals. */

boolean deh_allow_long_cheats = false;

/* If false, dehacked cheat replacements are ignored. */

boolean deh_apply_cheats = true;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Called on startup to call the Init functions */

static void initialize_sections(void)
{
  unsigned int i;

  for (i = 0; deh_section_types[i] != NULL; ++i)
    {
      if (deh_section_types[i]->init != NULL)
        {
          deh_section_types[i]->init();
        }
    }
}

static void deh_init(void)
{
  /* @category mod
   *
   * Ignore cheats in dehacked files.
   */

  if (m_check_parm("-nocheats") > 0)
    {
      deh_apply_cheats = false;
    }

  /* Call init functions for all the section definitions. */

  initialize_sections();

  deh_initialized = true;
}

/* Given a section name, get the section structure which corresponds */

static deh_section_t *get_section_by_name(char *name)
{
  unsigned int i;

  /* we explicitly do not recognize [STRINGS] sections at all
   * if extended strings are not allowed
   */

  if (!deh_allow_extended_strings && !strncasecmp("[STRINGS]", name, 9))
    {
      return NULL;
    }

  for (i = 0; deh_section_types[i] != NULL; ++i)
    {
      if (!strcasecmp(deh_section_types[i]->name, name))
        {
          return deh_section_types[i];
        }
    }

  return NULL;
}

/* Is the string passed just whitespace? */

static boolean is_whitespace(char *s)
{
  for (; *s; ++s)
    {
      if (!isspace(*s)) return false;
    }

  return true;
}

/* Strip whitespace from the start and end of a string */

static char *clean_string(char *s)
{
  char *strending;

  /* Leading whitespace */

  while (*s && isspace(*s))
    ++s;

  /* Trailing whitespace */

  strending = s + strlen(s) - 1;

  while (strlen(s) > 0 && isspace(*strending))
    {
      *strending = '\0';
      --strending;
    }

  return s;
}

static boolean check_signatures(deh_context_t *context)
{
  size_t i;
  char *line;

  /* Read the first line */

  line = deh_read_line(context, false);

  if (line == NULL)
    {
      return false;
    }

  /* Check all signatures to see if one matches */

  for (i = 0; deh_signatures[i] != NULL; ++i)
    {
      if (!strcmp(deh_signatures[i], line))
        {
          return true;
        }
    }

  return false;
}

/* Parses a comment string in a dehacked file. */

static void deh_parse_comment(char *comment)
{
  /* Welcome, to the super-secret Chocolate Doom-specific Dehacked
   * overrides function.
   *
   * Putting these magic comments into your Dehacked lumps will
   * allow you to go beyond the normal limits of Vanilla Dehacked.
   * Because of this, these comments are deliberately undocumented,
   * and if you're using them you should be aware that your mod
   * is not compatible with Vanilla Doom and you're probably a
   * very naughty person.
   *

   * Allow comments containing this special value to allow string
   * replacements longer than those permitted by DOS dehacked.
   * This allows us to use a dehacked patch for doing string
   * replacements for emulating Chex Quest.
   *
   * If you use this, your dehacked patch may not work in Vanilla
   * Doom.
   */

  if (strstr(comment, "*allow-long-strings*") != NULL)
    {
      deh_allow_long_strings = true;
    }

  /* Allow magic comments to allow longer cheat replacements than
   * those permitted by DOS dehacked.  This is also for Chex
   * Quest.
   */

  if (strstr(comment, "*allow-long-cheats*") != NULL)
    {
      deh_allow_long_cheats = true;
    }

  /* Allow magic comments to allow parsing [STRINGS] section
   * that are usually only found in BEX format files. This allows
   * for substitution of map and episode names when loading
   * Freedoom/FreeDM IWADs.
   */

  if (strstr(comment, "*allow-extended-strings*") != NULL)
    {
      deh_allow_extended_strings = true;
    }
}

/* Parses a dehacked file by reading from the context */

static void deh_parse_context(deh_context_t *context)
{
  deh_section_t *current_section = NULL;
  char section_name[20];
  void *tag = NULL;
  boolean extended;
  char *line;

  /* Read the header and check it matches the signature */

  if (!check_signatures(context))
    {
      deh_error(context, "This is not a valid dehacked patch file!");
    }

  /* Read the file */

  while (!deh_had_error(context))
    {
      /* Read the next line. We only allow the special extended parsing
       * for the BEX [STRINGS] section.
       */

      extended = current_section != NULL &&
                 !strcasecmp(current_section->name, "[STRINGS]");
      line = deh_read_line(context, extended);

      /* end of file? */

      if (line == NULL)
        {
          return;
        }

      while (line[0] != '\0' && isspace(line[0]))
        ++line;

      if (line[0] == '#')
        {
          /* comment */

          deh_parse_comment(line);
          continue;
        }

      if (is_whitespace(line))
        {
          if (current_section != NULL)
            {
              /* end of section */

              if (current_section->end != NULL)
                {
                  current_section->end(context, tag);
                }

              current_section = NULL;
            }
        }
      else
        {
          if (current_section != NULL)
            {
              /* parse this line */

              current_section->line_parser(context, line, tag);
            }
          else
            {
              /* possibly the start of a new section */

              sscanf(line, "%19s", section_name);

              current_section = get_section_by_name(section_name);

              if (current_section != NULL)
                {
                  tag = current_section->start(context, line);
                }
              else
                {
                  /* printf("unknown section name %s\n", section_name); */
                }
            }
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void deh_checksum(sha1_digest_t digest)
{
  SHA1_CTX sha1_context;
  unsigned int i;

  sha1init(&sha1_context);

  for (i = 0; deh_section_types[i] != NULL; ++i)
    {
      if (deh_section_types[i]->sha1_hash != NULL)
        {
          deh_section_types[i]->sha1_hash(&sha1_context);
        }
    }

  sha1final(digest, &sha1_context);
}

/* This pattern is used a lot of times in different sections,
 * an assignment is essentially just a statement of the form:
 *
 * Variable Name = Value
 *
 * The variable name can include spaces or any other characters.
 * The string is split on the '=', essentially.
 *
 * Returns true if read correctly
 */

boolean deh_parse_assignment(char *line, char **variable_name, char **value)
{
  char *p;

  /* find the equals */

  p = strchr(line, '=');

  if (p == NULL)
    {
      return false;
    }

  /* variable name at the start
   * turn the '=' into a \0 to terminate the string here
   */

  *p = '\0';
  *variable_name = clean_string(line);

  /* value immediately follows the '=' */

  *value = clean_string(p + 1);

  return true;
}

/* Parses a dehacked file */

int deh_loadfile(const char *filename)
{
  deh_context_t *context;
  boolean had_error;

  if (!deh_initialized)
    {
      deh_init();
    }

  /* Before parsing a new file, reset special override flags to false.
   * Magic comments should only apply to the file in which they were
   * defined, and shouldn't carry over to subsequent files as well.
   */

  deh_allow_long_strings = false;
  deh_allow_long_cheats = false;
  deh_allow_extended_strings = false;

  printf(" loading %s\n", filename);

  context = deh_open_file(filename);

  if (context == NULL)
    {
      fprintf(stderr, "deh_loadfile: Unable to open %s\n", filename);
      return 0;
    }

  deh_parse_context(context);

  had_error = deh_had_error(context);

  deh_close_file(context);

  if (had_error)
    {
      i_error("Error parsing dehacked file");
    }

  return 1;
}

/* Load all dehacked patches from the given directory. */

void deh_auto_load_patches(const char *path)
{
  const char *filename;
  glob_t *glob;

  glob = i_start_multi_glob(path, GLOB_FLAG_NOCASE | GLOB_FLAG_SORTED,
                            "*.deh", "*.hhe", "*.seh", NULL);
  for (; ; )
    {
      filename = i_next_glob(glob);
      if (filename == NULL)
        {
          break;
        }

      printf(" [autoload]");
      deh_loadfile(filename);
    }

  i_end_glob(glob);
}

/* Load dehacked file from WAD lump.
 * If allow_long is set, allow long strings and cheats just for this lump.
 */

int deh_load_lump(int lumpnum, boolean allow_long, boolean allow_error)
{
  deh_context_t *context;
  boolean had_error;

  if (!deh_initialized)
    {
      deh_init();
    }

  /* Reset all special flags to defaults. */

  deh_allow_long_strings = allow_long;
  deh_allow_long_cheats = allow_long;
  deh_allow_extended_strings = false;

  context = deh_open_lump(lumpnum);

  if (context == NULL)
    {
      fprintf(stderr, "deh_loadfile: Unable to open lump %i\n", lumpnum);
      return 0;
    }

  deh_parse_context(context);

  had_error = deh_had_error(context);

  deh_close_file(context);

  /* If there was an error while parsing, abort with an error, but allow
   * errors to just be ignored if allow_error=true.
   */

  if (!allow_error && had_error)
    {
      i_error("Error parsing dehacked lump");
    }

  return 1;
}

int deh_load_lump_by_name(const char *name, boolean allow_long,
                       boolean allow_error)
{
  int lumpnum;

  lumpnum = w_check_num_for_name(name);

  if (lumpnum == -1)
    {
      fprintf(stderr, "deh_load_lump_by_name: '%s' lump not found\n", name);
      return 0;
    }

  return deh_load_lump(lumpnum, allow_long, allow_error);
}

/* Check the command line for -deh argument, and others. */

void deh_parse_command_line(void)
{
  char *filename;
  int p;

  /* @arg <files>
   * @category mod
   *
   * Load the given dehacked patch(es)
   */

  p = m_check_parm("-deh");

  if (p > 0)
    {
      ++p;

      while (p < myargc && myargv[p][0] != '-')
        {
          filename = d_try_find_wad_by_name(myargv[p]);
          deh_loadfile(filename);
          free(filename);
          ++p;
        }
    }
}
