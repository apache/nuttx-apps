/****************************************************************************
 * apps/games/NXDoom/src/i_glob.c
 *
 * SPDX-License-Identifier: GPLv2
 *
 * Copyright(C) 2018 Simon Howard
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
 *
 * File globbing API. This allows the contents of the filesystem
 * to be interrogated.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "i_glob.h"
#include "m_misc.h"

#include <dirent.h>

#include <dirent.h>
#include <sys/stat.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct glob_s
{
  char **globs;
  int num_globs;
  int flags;
  DIR *dir;
  char *directory;
  char *last_filename;

  /* These fields are only used when the GLOB_FLAG_SORTED flag is set:
   */

  char **filenames;
  int filenames_len;
  int next_index;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Only the fields d_name and (as an XSI extension) d_ino are specified
 * in POSIX.1.  Other than Linux, the d_type field is available mainly
 * only on BSD systems.  The remaining fields are available on many, but
 * not all systems.
 */

static boolean is_directory(char *dir, struct dirent *de)
{
#if defined(_DIRENT_HAVE_D_TYPE)
  if (de->d_type != DT_UNKNOWN && de->d_type != DT_LNK)
    {
      return de->d_type == DT_DIR;
    }
  else
#endif
    {
      char *filename;
      struct stat sb;
      int result;

      filename = m_string_join(dir, DIR_SEPARATOR_S, de->d_name, NULL);
      result = stat(filename, &sb);
      free(filename);

      if (result != 0)
        {
          return false;
        }

      return S_ISDIR(sb.st_mode);
    }
}

static void free_string_list(char **globs, int num_globs)
{
  int i;
  for (i = 0; i < num_globs; ++i)
    {
      free(globs[i]);
    }

  free(globs);
}

static boolean matches_glob(const char *name, const char *glob, int flags)
{
  int n;
  int g;

  while (*glob != '\0')
    {
      n = *name;
      g = *glob;

      if ((flags & GLOB_FLAG_NOCASE) != 0)
        {
          n = tolower(n);
          g = tolower(g);
        }

      if (g == '*')
        {
          /* To handle *-matching we skip past the * and recurse
           * to check each subsequent character in turn. If none
           * match then the whole match is a failure.
           */

          while (*name != '\0')
            {
              if (matches_glob(name, glob + 1, flags))
                {
                  return true;
                }

              ++name;
            }

          return glob[1] == '\0';
        }
      else if (g != '?' && n != g)
        {
          /* For normal characters the name must match the glob,
           * but for ? we don't care what the character is.
           */

          return false;
        }

      ++name;
      ++glob;
    }

  /* Match successful when glob and name end at the same time. */

  return *name == '\0';
}

static boolean matches_any_glob(const char *name, glob_t *glob)
{
  int i;

  for (i = 0; i < glob->num_globs; ++i)
    {
      if (matches_glob(name, glob->globs[i], glob->flags))
        {
          return true;
        }
    }

  return false;
}

static char *next_glob(glob_t *glob)
{
  struct dirent *de;

  do
    {
      de = readdir(glob->dir);
      if (de == NULL)
        {
          return NULL;
        }
    }
  while (is_directory(glob->directory, de) ||
         !matches_any_glob(de->d_name, glob));

  /* Return the fully-qualified path, not just the bare filename. */

  return m_string_join(glob->directory, DIR_SEPARATOR_S, de->d_name, NULL);
}

static void read_all_filenames(glob_t *glob)
{
  char *name;

  glob->filenames = NULL;
  glob->filenames_len = 0;
  glob->next_index = 0;

  for (; ; )
    {
      name = next_glob(glob);
      if (name == NULL)
        {
          break;
        }

      glob->filenames = realloc(glob->filenames,
                                (glob->filenames_len + 1) * sizeof(char *));
      glob->filenames[glob->filenames_len] = name;
      ++glob->filenames_len;
    }
}

static void sort_filenames(char **filenames, int len, int flags)
{
  char *pivot;
  char *tmp;
  int i;
  int left_len;
  int cmp;

  if (len <= 1)
    {
      return;
    }

  pivot = filenames[len - 1];
  left_len = 0;
  for (i = 0; i < len - 1; ++i)
    {
      if ((flags & GLOB_FLAG_NOCASE) != 0)
        {
          cmp = strcasecmp(filenames[i], pivot);
        }
      else
        {
          cmp = strcmp(filenames[i], pivot);
        }

      if (cmp < 0)
        {
          tmp = filenames[i];
          filenames[i] = filenames[left_len];
          filenames[left_len] = tmp;
          ++left_len;
        }
    }

  filenames[len - 1] = filenames[left_len];
  filenames[left_len] = pivot;

  sort_filenames(filenames, left_len, flags);
  sort_filenames(&filenames[left_len + 1], len - left_len - 1, flags);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

glob_t *i_start_multi_glob(const char *directory, int flags,
        const char *glob, ...)
{
  char **globs;
  int num_globs;
  glob_t *result;
  va_list args;

  globs = malloc(sizeof(char *));
  if (globs == NULL)
    {
      return NULL;
    }

  globs[0] = m_string_duplicate(glob);
  num_globs = 1;

  va_start(args, glob);
  for (; ; )
    {
      const char *arg = va_arg(args, const char *);
      char **new_globs;

      if (arg == NULL)
        {
          break;
        }

      new_globs = realloc(globs, sizeof(char *) * (num_globs + 1));
      if (new_globs == NULL)
        {
          free_string_list(globs, num_globs);
        }

      globs = new_globs;
      globs[num_globs] = m_string_duplicate(arg);
      ++num_globs;
    }

  va_end(args);

  result = malloc(sizeof(glob_t));
  if (result == NULL)
    {
      free_string_list(globs, num_globs);
      return NULL;
    }

  result->dir = opendir(directory);
  if (result->dir == NULL)
    {
      free_string_list(globs, num_globs);
      free(result);
      return NULL;
    }

  result->directory = m_string_duplicate(directory);
  result->globs = globs;
  result->num_globs = num_globs;
  result->flags = flags;
  result->last_filename = NULL;
  result->filenames = NULL;
  result->filenames_len = 0;
  result->next_index = -1;
  return result;
}

glob_t *i_start_glob(const char *directory, const char *glob, int flags)
{
  return i_start_multi_glob(directory, flags, glob, NULL);
}

void i_end_glob(glob_t *glob)
{
  if (glob == NULL)
    {
      return;
    }

  free_string_list(glob->globs, glob->num_globs);
  free_string_list(glob->filenames, glob->filenames_len);

  free(glob->directory);
  free(glob->last_filename);
  (void)closedir(glob->dir);
  free(glob);
}

const char *i_next_glob(glob_t *glob)
{
  const char *result;

  if (glob == NULL)
    {
      return NULL;
    }

  /* In unsorted mode we just return the filenames as we read
   * them back from the system API.
   */

  if ((glob->flags & GLOB_FLAG_SORTED) == 0)
    {
      free(glob->last_filename);
      glob->last_filename = next_glob(glob);
      return glob->last_filename;
    }

  /* In sorted mode we read the whole list of filenames into memory,
   * sort them and return them one at a time.
   */

  if (glob->next_index < 0)
    {
      read_all_filenames(glob);
      sort_filenames(glob->filenames, glob->filenames_len, glob->flags);
    }

  if (glob->next_index >= glob->filenames_len)
    {
      return NULL;
    }

  result = glob->filenames[glob->next_index];
  ++glob->next_index;
  return result;
}
