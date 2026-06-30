/****************************************************************************
 * apps/games/NXDoom/src/deh_io.c
 *
 * SPDX-License-Identifer: GPLv2
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
 *
 * Dehacked I/O code (does all reads from dehacked files)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#include "deh_defs.h"
#include "deh_io.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  DEH_INPUT_FILE,
  DEH_INPUT_LUMP
} deh_input_type_t;

struct deh_context_s
{
  deh_input_type_t type;
  char *filename;

  /* If the input comes from a memory buffer, pointer to the memory
   * buffer.
   */

  unsigned char *input_buffer;
  size_t input_buffer_len;
  unsigned int input_buffer_pos;
  int lumpnum;

  /* If the input comes from a file, the file stream for reading
   * data.
   */

  FILE *stream;

  /* Current line number that we have reached: */

  int linenum;

  /* Used by deh_read_line: */

  boolean last_was_newline;
  char *readbuffer;
  int readbuffer_size;

  /* Error handling. */

  boolean had_error;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static deh_context_t *deh_new_context(void)
{
  deh_context_t *context;

  context = z_malloc(sizeof(*context), PU_STATIC, NULL);

  /* Initial read buffer size of 128 bytes */

  context->readbuffer_size = 128;
  context->readbuffer = z_malloc(context->readbuffer_size, PU_STATIC, NULL);
  context->linenum = 0;
  context->last_was_newline = true;

  context->had_error = false;

  return context;
}

/* Increase the read buffer size */

static void increase_read_buffer(deh_context_t *context)
{
  char *newbuffer;
  int newbuffer_size;

  newbuffer_size = context->readbuffer_size * 2;
  newbuffer = z_malloc(newbuffer_size, PU_STATIC, NULL);

  memcpy(newbuffer, context->readbuffer, context->readbuffer_size);

  z_free(context->readbuffer);

  context->readbuffer = newbuffer;
  context->readbuffer_size = newbuffer_size;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Open a dehacked file for reading
 * Returns NULL if open failed
 */

deh_context_t *deh_open_file(const char *filename)
{
  FILE *fstream;
  deh_context_t *context;

  fstream = fopen(filename, "r");

  if (fstream == NULL) return NULL;

  context = deh_new_context();

  context->type = DEH_INPUT_FILE;
  context->stream = fstream;
  context->filename = m_string_duplicate(filename);

  return context;
}

/* Open a WAD lump for reading. */

deh_context_t *deh_open_lump(int lumpnum)
{
  deh_context_t *context;
  void *lump;

  lump = w_cache_lump_num(lumpnum, PU_STATIC);

  context = deh_new_context();

  context->type = DEH_INPUT_LUMP;
  context->lumpnum = lumpnum;
  context->input_buffer = lump;
  context->input_buffer_len = w_lump_length(lumpnum);
  context->input_buffer_pos = 0;

  context->filename = malloc(9);
  m_str_copy(context->filename, lumpinfo[lumpnum]->name, 9);

  return context;
}

/* Close dehacked file */

void deh_close_file(deh_context_t *context)
{
  if (context->type == DEH_INPUT_FILE)
    {
      fclose(context->stream);
    }
  else if (context->type == DEH_INPUT_LUMP)
    {
      w_release_lump_num(context->lumpnum);
    }

  free(context->filename);
  z_free(context->readbuffer);
  z_free(context);
}

int deh_get_char_file(deh_context_t *context)
{
  if (feof(context->stream))
    {
      /* end of file */

      return -1;
    }

  return fgetc(context->stream);
}

int deh_get_char_lump(deh_context_t *context)
{
  int result;

  if (context->input_buffer_pos >= context->input_buffer_len)
    {
      return -1;
    }

  result = context->input_buffer[context->input_buffer_pos];
  ++context->input_buffer_pos;

  return result;
}

/* Reads a single character from a dehacked file */

int deh_get_char(deh_context_t *context)
{
  int result = 0;
  boolean last_was_cr = false;

  /* Track the current line number */

  if (context->last_was_newline)
    {
      ++context->linenum;
    }

  /* Read characters, converting CRLF to LF */

  do
    {
      switch (context->type)
        {
        case DEH_INPUT_FILE:
          result = deh_get_char_file(context);
          break;

        case DEH_INPUT_LUMP:
          result = deh_get_char_lump(context);
          break;
        }

      /* Handle \r characters not paired with \n */

      if (last_was_cr && result != '\n')
        {
          switch (context->type)
            {
            case DEH_INPUT_FILE:
              ungetc(result, context->stream);
              break;

            case DEH_INPUT_LUMP:
              --context->input_buffer_pos;
              break;
            }

          return '\r';
        }

      last_was_cr = result == '\r';
    }
  while (last_was_cr);

  context->last_was_newline = result == '\n';

  return result;
}

/* Read a whole line */

char *deh_read_line(deh_context_t *context, boolean extended)
{
  int c;
  int pos;
  boolean escaped = false;

  for (pos = 0; ; )
    {
      c = deh_get_char(context);

      if (c < 0 && pos == 0)
        {
          /* end of file */

          return NULL;
        }

      /* cope with lines of any length: increase the buffer size */

      if (pos >= context->readbuffer_size)
        {
          increase_read_buffer(context);
        }

      /* extended string support */

      if (extended && c == '\\')
        {
          c = deh_get_char(context);

          /* "\n" in the middle of a string indicates an internal linefeed */

          if (c == 'n')
            {
              context->readbuffer[pos] = '\n';
              ++pos;
              continue;
            }

          /* values to be assigned may be split onto multiple lines by ending
           * each line that is to be continued with a backslash
           */

          if (c == '\n')
            {
              escaped = true;
              continue;
            }
        }

      /* blanks before the backslash are included in the string
       * but indentation after the linefeed is not
       */

      if (escaped && c >= 0 && isspace(c) && c != '\n')
        {
          continue;
        }
      else
        {
          escaped = false;
        }

      if (c == '\n' || c < 0)
        {
          /* end of line: a full line has been read */

          context->readbuffer[pos] = '\0';
          break;
        }
      else if (c != '\0')
        {
          /* normal character; don't allow NUL characters to be
           * added.
           */

          context->readbuffer[pos] = (char)c;
          ++pos;
        }
    }

  return context->readbuffer;
}

void deh_warning(deh_context_t *context, const char *msg, ...)
{
  va_list args;

  va_start(args, msg);

  fprintf(stderr, "%s:%i: warning: ", context->filename, context->linenum);
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\n");

  va_end(args);
}

void deh_error(deh_context_t *context, const char *msg, ...)
{
  va_list args;

  va_start(args, msg);

  fprintf(stderr, "%s:%i: ", context->filename, context->linenum);
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\n");

  va_end(args);

  context->had_error = true;
}

boolean deh_had_error(deh_context_t *context)
{
  return context->had_error;
}
