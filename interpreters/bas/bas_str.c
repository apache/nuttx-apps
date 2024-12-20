/****************************************************************************
 * apps/interpreters/bas/bas_str.c
 *
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 1999-2014 Michael Haardt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bas_str.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int cistrcmp(const char *s, const char *r)
{
  assert(s != (char *)0);
  assert(r != (char *)0);
  while (*s && tolower(*s) == tolower(*r))
    {
      ++s;
      ++r;
    }

  return ((tolower(*s) - tolower(*r)));
}

struct String *String_new(struct String *self)
{
  assert(self != (struct String *)0);
  self->length = 0;
  self->character = (char *)0;
  self->field = (struct StringField *)0;
  return self;
}

void String_destroy(struct String *self)
{
  assert(self != (struct String *)0);
  if (self->field)
    {
      String_leaveField(self);
    }

  if (self->length)
    {
      free(self->character);
    }
}

int String_joinField(struct String *self, struct StringField *field,
                     char *character, size_t length)
{
  struct String **n;

  assert(self != (struct String *)0);
  if (self->field)
    {
      String_leaveField(self);
    }

  self->field = field;
  if ((n =
       (struct String **)realloc(field->refStrings,
                                 sizeof(struct String *) * (field->refCount +
                                                            1))) ==
      (struct String **)0)
    {
      return -1;
    }

  field->refStrings = n;
  field->refStrings[field->refCount] = self;
  ++field->refCount;
  if (self->length)
    {
      free(self->character);
    }

  self->character = character;
  self->length = length;
  return 0;
}

void String_leaveField(struct String *self)
{
  struct StringField *field;
  int i;
  struct String **ref;

  assert(self != (struct String *)0);
  field = self->field;
  assert(field != (struct StringField *)0);
  for (i = 0, ref = field->refStrings; i < field->refCount; ++i, ++ref)
    {
      if (*ref == self)
        {
          int further = --field->refCount - i;

          if (further)
            {
              memmove(ref, ref + 1, further * sizeof(struct String **));
            }

          self->character = (char *)0;
          self->length = 0;
          self->field = (struct StringField *)0;
          return;
        }
    }

  assert(0);
}

struct String *String_clone(struct String *self, const struct String *original)
{
  assert(self != (struct String *)0);
  String_new(self);
  String_appendString(self, original);
  return self;
}

int String_size(struct String *self, size_t length)
{
  char *n;

  assert(self != (struct String *)0);
  if (self->field)
    {
      String_leaveField(self);
    }

  if (length)
    {
      if (length > self->length)
        {
          if ((n = realloc(self->character, length + 1)) == (char *)0)
            {
              return -1;
            }

          self->character = n;
        }

      self->character[length] = '\0';
    }
  else
    {
      if (self->length)
        {
          free(self->character);
        }

      self->character = (char *)0;
    }

  self->length = length;
  return 0;
}

int String_appendString(struct String *self, const struct String *app)
{
  size_t oldlength = self->length;

  if (self->field)
    {
      String_leaveField(self);
    }

  if (app->length == 0)
    {
      return 0;
    }

  if (String_size(self, self->length + app->length) == -1)
    {
      return -1;
    }

  memcpy(self->character + oldlength, app->character, app->length);
  return 0;
}

int String_appendChar(struct String *self, char ch)
{
  size_t oldlength = self->length;

  if (self->field)
    {
      String_leaveField(self);
    }

  if (String_size(self, self->length + 1) == -1)
    {
      return -1;
    }

  self->character[oldlength] = ch;
  return 0;
}

int String_appendChars(struct String *self, const char *ch)
{
  size_t oldlength = self->length;
  size_t chlen = strlen(ch);

  if (self->field)
    {
      String_leaveField(self);
    }

  if (String_size(self, self->length + chlen) == -1)
    {
      return -1;
    }

  memcpy(self->character + oldlength, ch, chlen);
  return 0;
}

int String_appendPrintf(struct String *self, const char *fmt, ...)
{
  char buf[1024];
  size_t l, j;
  va_list ap;

  if (self->field)
    {
      String_leaveField(self);
    }

  va_start(ap, fmt);
  l = vsprintf(buf, fmt, ap);
  va_end(ap);
  j = self->length;
  if (String_size(self, j + l) == -1)
    {
      return -1;
    }

  memcpy(self->character + j, buf, l);
  return 0;
}

int String_insertChar(struct String *self, size_t where, char ch)
{
  size_t oldlength = self->length;

  if (self->field)
    {
      String_leaveField(self);
    }

  assert(where < oldlength);
  if (String_size(self, self->length + 1) == -1)
    {
      return -1;
    }

  memmove(self->character + where + 1, self->character + where,
          oldlength - where);
  self->character[where] = ch;
  return 0;
}

int String_delete(struct String *self, size_t where, size_t len)
{
  size_t oldlength = self->length;

  if (self->field)
    {
      String_leaveField(self);
    }

  assert(where < oldlength);
  assert(len > 0);
  if ((where + len) < oldlength)
    {
      memmove(self->character + where, self->character + where + len,
              oldlength - where - len);
    }

  self->character[self->length -= len] = '\0';
  return 0;
}

void String_ucase(struct String *self)
{
  size_t i;

  for (i = 0; i < self->length; ++i)
    {
      self->character[i] = toupper(self->character[i]);
    }
}

void String_lcase(struct String *self)
{
  size_t i;

  for (i = 0; i < self->length; ++i)
    {
      self->character[i] = tolower(self->character[i]);
    }
}

int String_cmp(const struct String *self, const struct String *s)
{
  size_t pos;
  int res;
  const char *thisch, *sch;

  for (pos = 0, thisch = self->character, sch = s->character;
       pos < self->length && pos < s->length; ++pos, ++thisch, ++sch)
    {
      if ((res = (*thisch - *sch)))
        {
          return res;
        }
    }

  return (self->length - s->length);
}

void String_lset(struct String *self, const struct String *s)
{
  size_t copy;

  copy = (self->length < s->length ? self->length : s->length);
  if (copy)
    {
      memcpy(self->character, s->character, copy);
    }

  if (copy < self->length)
    {
      memset(self->character + copy, ' ', self->length - copy);
    }
}

void String_rset(struct String *self, const struct String *s)
{
  size_t copy;

  copy = (self->length < s->length ? self->length : s->length);
  if (copy)
    {
      memcpy(self->character + self->length - copy, s->character, copy);
    }

  if (copy < self->length)
    {
      memset(self->character, ' ', self->length - copy);
    }
}

void String_set(struct String *self, size_t pos, const struct String *s,
                size_t length)
{
  if (self->length >= pos)
    {
      if (self->length < (pos + length))
        {
          length = self->length - pos;
        }

      if (length)
        {
          memcpy(self->character + pos, s->character, length);
        }
    }
}

struct StringField *StringField_new(struct StringField *self)
{
  self->refStrings = (struct String **)0;
  self->refCount = 0;
  return self;
}

void StringField_destroy(struct StringField *self)
{
  int i;

  for (i = self->refCount; i > 0;)
    {
      String_leaveField(self->refStrings[--i]);
    }

  self->refCount = -1;
  free(self->refStrings);
}
