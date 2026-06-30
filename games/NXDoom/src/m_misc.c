/****************************************************************************
 * apps/games/NXDoom/src/m_misc.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 1993-2008 Raven Software
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
 *  Miscellaneous.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include "doomtype.h"

#include "i_system.h"
#include "m_misc.h"
#include "z_zone.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void m_make_directory(const char *path)
{
  mkdir(path, 0755);
}

boolean m_file_exists(const char *filename)
{
  FILE *fstream;

  fstream = fopen(filename, "r");

  if (fstream != NULL)
    {
      fclose(fstream);
      return true;
    }
  else
    {
      /* If we can't open because the file is a directory, the
       * "file" exists at least!
       */

      return errno == EISDIR;
    }
}

/* Check if a file exists by probing for common case variation of its
 * filename. Returns a newly allocated string that the caller is responsible
 * for freeing.
 */

char *m_file_case_exists(const char *path)
{
  char *path_dup;
  char *filename;
  char *ext;

  path_dup = m_string_duplicate(path);

  /* 0: actual path */

  if (m_file_exists(path_dup))
    {
      return path_dup;
    }

  filename = strrchr(path_dup, DIR_SEPARATOR);
  if (filename != NULL)
    {
      filename++;
    }
  else
    {
      filename = path_dup;
    }

  /* 1: lowercase filename, e.g. doom2.wad */

  m_force_lowercase(filename);

  if (m_file_exists(path_dup))
    {
      return path_dup;
    }

  /* 2: uppercase filename, e.g. DOOM2.WAD */

  m_force_uppercase(filename);

  if (m_file_exists(path_dup))
    {
      return path_dup;
    }

  /* 3. uppercase basename with lowercase extension, e.g. DOOM2.wad */

  ext = strrchr(path_dup, '.');
  if (ext != NULL && ext > filename)
    {
      m_force_lowercase(ext + 1);

      if (m_file_exists(path_dup))
        {
          return path_dup;
        }
    }

  /* 4. lowercase filename with uppercase first letter, e.g. Doom2.wad */

  if (strlen(filename) > 1)
    {
      m_force_lowercase(filename + 1);

      if (m_file_exists(path_dup))
        {
          return path_dup;
        }
    }

  /* 5. no luck */

  free(path_dup);
  return NULL;
}

long m_file_length(FILE *handle)
{
  long savedpos;
  long length;

  /* save the current position in the file */

  savedpos = ftell(handle);

  /* jump to the end and find the length */

  fseek(handle, 0, SEEK_END);
  length = ftell(handle);

  /* go back to the old location */

  fseek(handle, savedpos, SEEK_SET);

  return length;
}

boolean m_write_file(const char *name, const void *source, int length)
{
  FILE *handle;
  int count;

  handle = fopen(name, "wb");

  if (handle == NULL) return false;

  count = fwrite(source, 1, length, handle);
  fclose(handle);

  if (count < length) return false;

  return true;
}

int m_read_file(const char *name, byte **buffer)
{
  FILE *handle;
  int count;
  int length;
  byte *buf;

  handle = fopen(name, "rb");
  if (handle == NULL) i_error("Couldn't read file %s", name);

  /* find the size of the file by seeking to the end and
   * reading the current position
   */

  length = m_file_length(handle);

  buf = z_malloc(length + 1, PU_STATIC, NULL);
  count = fread(buf, 1, length, handle);
  fclose(handle);

  if (count < length) i_error("Couldn't read file %s", name);

  buf[length] = '\0';
  *buffer = buf;
  return length;
}

/* Returns the path to a temporary file of the given name, stored
 * inside the system temporary directory.
 *
 * The returned value must be freed with z_free after use.
 */

char *m_temp_file(const char *s)
{
  const char *tempdir;

  /* Check the $TMPDIR environment variable to find the location. */

  tempdir = getenv("TMPDIR");

  if (tempdir == NULL)
    {
      tempdir = "/tmp";
    }

  return m_string_join(tempdir, DIR_SEPARATOR_S, s, NULL);
}

boolean m_str_to_int(const char *str, int *result)
{
  return sscanf(str, " 0x%x", (unsigned int *)result) == 1 ||
         sscanf(str, " 0X%x", (unsigned int *)result) == 1 ||
         sscanf(str, " 0%o", (unsigned int *)result) == 1 ||
         sscanf(str, " %d", result) == 1;
}

/* Returns the directory portion of the given path, without the trailing
 * slash separator character. If no directory is described in the path,
 * the string "." is returned. In either case, the result is newly allocated
 * and must be freed by the caller after use.
 */

char *m_dir_name(const char *path)
{
  char *result;
  const char *pf;
  const char *pb;

  pf = strrchr(path, '/');
  pb = NULL;

  if (pf == NULL && pb == NULL)
    {
      return m_string_duplicate(".");
    }
  else
    {
      const char *p = (pb > pf) ? pb : pf;
      result = m_string_duplicate(path);
      result[p - path] = '\0';
      return result;
    }
}

/* Returns the base filename described by the given path (without the
 * directory name). The result points inside path and nothing new is
 * allocated.
 */

const char *m_base_name(const char *path)
{
  const char *pf;
  const char *pb;

  pf = strrchr(path, '/');
  pb = NULL;

  if (pf == NULL && pb == NULL)
    {
      return path;
    }
  else
    {
      const char *p = (pb > pf) ? pb : pf;
      return p + 1;
    }
}

void m_extract_file_base(const char *path, char *dest)
{
  const char *src;
  const char *filename;
  int length;

  src = path + strlen(path) - 1;

  /* back up until a \ or the start */

  while (src != path && *(src - 1) != DIR_SEPARATOR)
    {
      src--;
    }

  filename = src;

  /* Copy up to eight characters
   * Note: Vanilla Doom exits with an error if a filename is specified
   * with a base of more than eight characters.  To remove the 8.3
   * filename limit, instead we simply truncate the name.
   */

  length = 0;
  memset(dest, 0, 8);

  while (*src != '\0' && *src != '.')
    {
      if (length >= 8)
        {
          printf("Warning: Truncated '%s' lump name to '%.8s'.\n", filename,
                 dest);
          break;
        }

      dest[length++] = toupper((int)*src++);
    }
}

/****************************************************************************
 * Name: m_force_uppercase
 *
 * Description:
 *  (PROC) Change string to uppercase.
 *
 ****************************************************************************/

void m_force_uppercase(char *text)
{
  char *p;

  for (p = text; *p != '\0'; ++p)
    {
      *p = toupper(*p);
    }
}

/****************************************************************************
 * Name: m_force_lowercase
 *
 * Description:
 *  (PROC) Change string to lowercase.
 *
 ****************************************************************************/

void m_force_lowercase(char *text)
{
  char *p;

  for (p = text; *p != '\0'; ++p)
    {
      *p = tolower(*p);
    }
}

/****************************************************************************
 * Name: m_string_duplicate
 *
 * Description:
 *  Safe version of strdup() that checks the string was successfully
 *  allocated.
 *
 ****************************************************************************/

char *m_string_duplicate(const char *orig)
{
  char *result;

  result = strdup(orig);

  if (result == NULL)
    {
      i_error("Failed to duplicate string (length %zu)\n", strlen(orig));
    }

  return result;
}

char *m_string_replace(const char *haystack, const char *needle,
                       const char *replacement)
{
  char *result;
  char *dst;
  const char *p;
  size_t needle_len = strlen(needle);
  size_t result_len;
  size_t dst_len;

  /* Iterate through occurrences of 'needle' and calculate the size of
   * the new string.
   */

  result_len = strlen(haystack) + 1;
  p = haystack;

  for (; ; )
    {
      p = strstr(p, needle);
      if (p == NULL)
        {
          break;
        }

      p += needle_len;
      result_len += strlen(replacement) - needle_len;
    }

  /* Construct new string. */

  result = malloc(result_len);
  if (result == NULL)
    {
      i_error("m_string_replace: Failed to allocate new string");
      return NULL;
    }

  dst = result;
  dst_len = result_len;
  p = haystack;

  while (*p != '\0')
    {
      if (!strncmp(p, needle, needle_len))
        {
          m_str_copy(dst, replacement, dst_len);
          p += needle_len;
          dst += strlen(replacement);
          dst_len -= strlen(replacement);
        }
      else
        {
          *dst = *p;
          ++dst;
          --dst_len;
          ++p;
        }
    }

  *dst = '\0';

  return result;
}

/* Safe string copy function that works like OpenBSD's strlcpy().
 * Returns true if the string was not truncated.
 */

boolean m_str_copy(char *dest, const char *src, size_t dest_size)
{
  size_t len;

  if (dest_size >= 1)
    {
      dest[dest_size - 1] = '\0';
      strncpy(dest, src, dest_size - 1);
    }
  else
    {
      return false;
    }

  len = strlen(dest);
  return src[len] == '\0';
}

/* Safe string concat function that works like OpenBSD's strlcat().
 * Returns true if string not truncated.
 */

boolean m_string_concat(char *dest, const char *src, size_t dest_size)
{
  size_t offset;

  offset = strlen(dest);
  if (offset > dest_size)
    {
      offset = dest_size;
    }

  return m_str_copy(dest + offset, src, dest_size - offset);
}

/* Returns true if 's' begins with the specified prefix. */

boolean m_string_starts_with(const char *s, const char *prefix)
{
  return strlen(s) >= strlen(prefix) &&
         strncmp(s, prefix, strlen(prefix)) == 0;
}

/* Returns true if 's' ends with the specified suffix. */

boolean m_string_ends_with(const char *s, const char *suffix)
{
  return strlen(s) >= strlen(suffix) &&
         strcmp(s + strlen(s) - strlen(suffix), suffix) == 0;
}

/* Return a newly-malloced string with all the strings given as arguments
 * concatenated together.
 */

char *m_string_join(const char *s, ...)
{
  char *result;
  const char *v;
  va_list args;
  size_t result_len;

  result_len = strlen(s) + 1;

  va_start(args, s);

  for (; ; )
    {
      v = va_arg(args, const char *);
      if (v == NULL)
        {
          break;
        }

      result_len += strlen(v);
    }

  va_end(args);

  result = malloc(result_len);

  if (result == NULL)
    {
      i_error("m_string_join: Failed to allocate new string.");
      return NULL;
    }

  m_str_copy(result, s, result_len);

  va_start(args, s);

  for (; ; )
    {
      v = va_arg(args, const char *);
      if (v == NULL)
        {
          break;
        }

      m_string_concat(result, v, result_len);
    }

  va_end(args);

  return result;
}

/****************************************************************************
 * Name: m_normalize_slashes
 *
 * Description:
 *   Remove trailing slashes, translate backslashes to slashes. The string to
 *   normalize is passed and returned in str.
 *
 *   killough 11/98: rewritten
 *
 *   [STRIFE] - haleyjd 20110210: Borrowed from Eternity and adapted to
 *   respect the DIR_SEPARATOR define used by Choco Doom. This routine
 *   originated in BOOM.
 *
 ****************************************************************************/

void m_normalize_slashes(char *str)
{
  char *p;

  /* Convert all slashes/backslashes to DIR_SEPARATOR */

  for (p = str; *p; p++)
    {
      if ((*p == '/' || *p == '\\') && *p != DIR_SEPARATOR)
        {
          *p = DIR_SEPARATOR;
        }
    }

  /* Remove trailing slashes */

  while (p > str && *--p == DIR_SEPARATOR)
    {
      *p = 0;
    }

  /* Collapse multiple slashes */

  for (p = str; (*str++ = *p); )
    {
      if (*p++ == DIR_SEPARATOR)
        {
          while (*p == DIR_SEPARATOR)
            {
              p++;
            }
        }
    }
}
