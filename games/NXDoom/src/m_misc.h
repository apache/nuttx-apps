/****************************************************************************
 * apps/games/NXDoom/src/m_misc.h
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
 *   Miscellaneous.
 *
 ****************************************************************************/

#ifndef __M_MISC__
#define __M_MISC__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>

#include "doomtype.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* debugging code to check there are no loops in a linked list
 * disabled unless explicitly requested
 */

#ifdef DEBUG_LINKED_LISTS

#define LINKED_LIST_CHECK_NO_CYCLE(list_type, list, next_member)             \
  do                                                                         \
    {                                                                        \
      if (list != NULL)                                                      \
        {                                                                    \
          list_type *slow, *fast;                                            \
          slow = list;                                                       \
          fast = list->next_member;                                          \
          while (fast)                                                       \
            {                                                                \
              if (!fast->next_member)                                        \
                {                                                            \
                  break;                                                     \
                }                                                            \
              fast = fast->next_member->next_member;                         \
              slow = slow->next_member;                                      \
              if (slow == fast)                                              \
                {                                                            \
                  fprintf(stderr, "loop in linked list " #list " in %s:%d",  \
                          __FILE__, __LINE__);                               \
                  __builtin_trap();                                          \
                }                                                            \
            }                                                                \
        }                                                                    \
    }                                                                        \
  while (0)

#else /* DEBUG_LINKED_LISTS */

#define LINKED_LIST_CHECK_NO_CYCLE(list_type, list, next_member)             \
  do                                                                         \
    {                                                                        \
    }                                                                        \
  while (0)

#endif /* DEBUG_LINKED_LISTS */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

boolean m_write_file(const char *name, const void *source, int length);
int m_read_file(const char *name, byte **buffer);
void m_make_directory(const char *dir);
char *m_temp_file(const char *s);
boolean m_file_exists(const char *file);
char *m_file_case_exists(const char *file);
long m_file_length(FILE *handle);
boolean m_str_to_int(const char *str, int *result);
char *m_dir_name(const char *path);
const char *m_base_name(const char *path);
void m_extract_file_base(const char *path, char *dest);
void m_force_uppercase(char *text);
void m_force_lowercase(char *text);
char *m_string_duplicate(const char *orig);
boolean m_str_copy(char *dest, const char *src, size_t dest_size);
boolean m_string_concat(char *dest, const char *src, size_t dest_size);
char *m_string_replace(const char *haystack, const char *needle,
                       const char *replacement);
char *m_string_join(const char *s, ...);
boolean m_string_starts_with(const char *s, const char *prefix);
boolean m_string_ends_with(const char *s, const char *suffix);
void m_normalize_slashes(char *str);

#endif /* __M_MISC__ */
