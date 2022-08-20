/****************************************************************************
 * apps/interpreters/bas/bas_str.h
 *
 *   Copyright (c) 1999-2014 Michael Haardt
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
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef __APPS_EXAMPLES_BAS_BAS_STR_H
#define __APPS_EXAMPLES_BAS_BAS_STR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct String
{
  size_t length;
  char *character;
  struct StringField *field;
};

struct StringField
{
  struct String **refStrings;
  int refCount;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int cistrcmp(const char *s, const char *r);

struct String *String_new(struct String *this);
void String_destroy(struct String *this);
int String_joinField(struct String *this, struct StringField *field,
                     char *character, size_t length);
void String_leaveField(struct String *this);
struct String *String_clone(struct String *this, const struct String *clon);
int String_appendString(struct String *this, const struct String *app);
int String_appendChar(struct String *this, char ch);
int String_appendChars(struct String *this, const char *ch);
int String_appendPrintf(struct String *this, const char *fmt, ...)
    printflike(2, 3);
int String_insertChar(struct String *this, size_t where, char ch);
int String_delete(struct String *this, size_t where, size_t len);
void String_ucase(struct String *this);
void String_lcase(struct String *this);
int String_size(struct String *this, size_t length);
int String_cmp(const struct String *this, const struct String *s);
void String_lset(struct String *this, const struct String *s);
void String_rset(struct String *this, const struct String *s);
void String_set(struct String *this, size_t pos, const struct String *s,
                size_t length);

struct StringField *StringField_new(struct StringField *this);
void StringField_destroy(struct StringField *this);

#endif /* __APPS_EXAMPLES_BAS_BAS_STR_H */
