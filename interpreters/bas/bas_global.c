/****************************************************************************
 * apps/interpreters/bas/bas_global.c
 * Global variables and functions.
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
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bas_auto.h"
#include "bas.h"
#include "bas_error.h"
#include "bas_fs.h"
#include "bas_global.h"
#include "bas_var.h"

#include <nuttx/clock.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

#ifndef RAND_MAX
#  define RAND_MAX 32767
#endif

#define _(String) String

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int wildcardmatch(const char *a, const char *pattern)
{
  while (*pattern)
    {
      switch (*pattern)
        {
        case '*':
          {
            ++pattern;
            while (*a)
              if (wildcardmatch(a, pattern))
                {
                  return 1;
                }
              else
                {
                  ++a;
                }

            break;
          }

        case '?':
          {
            if (*a)
              {
                ++a;
                ++pattern;
              }
            else
              {
                return 0;
              }

            break;
          }

        default:
          if (*a == *pattern)
            {
              ++a;
              ++pattern;
            }
          else
            {
              return 0;
            }
        }
    }

  return (*pattern == '\0' && *a == '\0');
}

static long int intValue(struct Auto *stack, int l)
{
  struct Value value;
  struct Value *arg = Var_value(Auto_local(stack, l), 0, (int *)0, &value);
  assert(arg->type == V_INTEGER);
  return arg->u.integer;
}

static double realValue(struct Auto *stack, int l)
{
  struct Value value;
  struct Value *arg = Var_value(Auto_local(stack, l), 0, (int *)0, &value);
  assert(arg->type == V_REAL);
  return arg->u.real;
}

static struct String *stringValue(struct Auto *stack, int l)
{
  struct Value value;
  struct Value *arg = Var_value(Auto_local(stack, l), 0, (int *)0, &value);
  assert(arg->type == V_STRING);
  return &(arg->u.string);
}

static struct Value *bin(struct Value *v, unsigned long int value,
                         long int digits)
{
  char buf[sizeof(long int) * 8 + 1];
  char *s;

  Value_new_STRING(v);
  s = buf + sizeof(buf);
  *--s = '\0';
  if (digits == 0)
    {
      digits = 1;
    }

  while (digits || value)
    {
      *--s = value & 1 ? '1' : '0';
      if (digits)
        {
          --digits;
        }

      value >>= 1;
    }

  String_appendChars(&v->u.string, s);
  return v;
}

static struct Value *hex(struct Value *v, long int value, long int digits)
{
  char buf[sizeof(long int) * 2 + 1];

  sprintf(buf, "%0*lx", (int)digits, value);
  Value_new_STRING(v);
  String_appendChars(&v->u.string, buf);
  return v;
}

static struct Value *find(struct Value *v, struct String *pattern,
                          long int occurrence)
{
  struct String dirname, basename;
  char *slash;
  DIR *dir;
  struct dirent *ent;
  int currentdir;
  int found = 0;

  Value_new_STRING(v);
  String_new(&dirname);
  String_new(&basename);
  String_appendString(&dirname, pattern);
  while (dirname.length > 0 && dirname.character[dirname.length - 1] == '/')
    {
      String_delete(&dirname, dirname.length - 1, 1);
    }

  if ((slash = strrchr(dirname.character, '/')) == (char *)0)
    {
      String_appendString(&basename, &dirname);
      String_delete(&dirname, 0, dirname.length);
      String_appendChar(&dirname, '.');
      currentdir = 1;
    }
  else
    {
      String_appendChars(&basename, slash + 1);
      String_delete(&dirname, slash - dirname.character,
                    dirname.length - (slash - dirname.character));
      currentdir = 0;
    }

  if ((dir = opendir(dirname.character)) != (DIR *) 0)
    {
      while ((ent = readdir(dir)) != (struct dirent *)0)
        {
          if (wildcardmatch(ent->d_name, basename.character))
            {
              if (found == occurrence)
                {
                  if (currentdir)
                    {
                      String_appendChars(&v->u.string, ent->d_name);
                    }
                  else
                    {
                      String_appendPrintf(&v->u.string, "%s/%s",
                                          dirname.character, ent->d_name);
                    }

                  break;
                }

              ++found;
            }
        }

      closedir(dir);
    }

  String_destroy(&dirname);
  String_destroy(&basename);
  return v;
}

static struct Value *instr(struct Value *v, long int start, long int len,
                           struct String *haystack, struct String *needle)
{
  const char *haystackChars = haystack->character;
  size_t haystackLength = haystack->length;
  const char *needleChars = needle->character;
  size_t needleLength = needle->length;
  int found;

  --start;
  if (start < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("position"));
    }

  if (len < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  if (((size_t) start) >= haystackLength)
    {
      return Value_new_INTEGER(v, 0);
    }

  haystackChars += start;
  haystackLength -= start;
  if (haystackLength > len)
    {
      haystackLength = len;
    }

  found = 1 + start;
  while (needleLength <= haystackLength)
    {
      if (memcmp(haystackChars, needleChars, needleLength) == 0)
        {
          return Value_new_INTEGER(v, found);
        }

      ++haystackChars;
      --haystackLength;
      ++found;
    }

  return Value_new_INTEGER(v, 0);
}

static struct Value *string(struct Value *v, long int len, int c)
{
  if (len < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  if (c < 0 || c > 255)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("code"));
    }

  Value_new_STRING(v);
  String_size(&v->u.string, len);
  if (len)
    {
      memset(v->u.string.character, c, len);
    }

  return v;
}

static struct Value *mid(struct Value *v, struct String *s, long int position,
                         long int length)
{
  --position;
  if (position < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("position"));
    }

  if (length < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  if (((size_t) position) + length > s->length)
    {
      length = s->length - position;
      if (length < 0)
        {
          length = 0;
        }
    }

  Value_new_STRING(v);
  String_size(&v->u.string, length);
  if (length > 0)
    {
      memcpy(v->u.string.character, s->character + position, length);
    }

  return v;
}

static struct Value *inkey(struct Value *v, long int timeout, long int chn)
{
  int c;

  if ((c = FS_inkeyChar(chn, timeout * 10)) == -1)
    {
      if (FS_errmsg)
        {
          return Value_new_ERROR(v, IOERROR, FS_errmsg);
        }
      else
        {
          return Value_new_STRING(v);
        }
    }
  else
    {
      Value_new_STRING(v);
      String_appendChar(&v->u.string, c);
      return v;
    }
}

static struct Value *input(struct Value *v, long int len, long int chn)
{
  int ch = -1;

  if (len <= 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  Value_new_STRING(v);
  while (len-- && (ch = FS_getChar(chn)) != -1)
    {
      String_appendChar(&v->u.string, ch);
    }

  if (ch == -1)
    {
      Value_destroy(v);
      return Value_new_ERROR(v, IOERROR, FS_errmsg);
    }

  return v;
}

static struct Value *env(struct Value *v, long int n)
{
  int i;

  --n;
  if (n < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("variable number"));
    }

  for (i = 0; i < n && environ[i]; ++i);

  Value_new_STRING(v);
  if (i == n && environ[i])
    {
      String_appendChars(&v->u.string, environ[i]);
    }

  return v;
}

static struct Value *rnd(struct Value *v, long int x)
{
  if (x < 0)
    {
      srand(-x);
    }

  if (x == 0 || x == 1)
    {
      Value_new_REAL(v, rand() / (double)RAND_MAX);
    }
  else
    {
      Value_new_REAL(v, rand() % x + 1);
    }

  return v;
}

static struct Value *fn_abs(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, fabs(realValue(stack, 0)));
}

static struct Value *fn_asc(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);

  if (s->length == 0)
    {
      return Value_new_ERROR(v, UNDEFINED,
                             _("`asc' or `code' of empty string"));
    }

  return Value_new_INTEGER(v, s->character[0] & 0xff);
}

static struct Value *fn_atn(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, atan(realValue(stack, 0)));
}

static struct Value *fn_bini(struct Value *v, struct Auto *stack)
{
  return bin(v, intValue(stack, 0), 0);
}

static struct Value *fn_bind(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int n;

  n = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("number"));
    }

  return bin(v, n, 0);
}

static struct Value *fn_binii(struct Value *v, struct Auto *stack)
{
  return bin(v, intValue(stack, 0), intValue(stack, 1));
}

static struct Value *fn_bindi(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int n;

  n = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("number"));
    }

  return bin(v, n, intValue(stack, 1));
}

static struct Value *fn_binid(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int digits;

  digits = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("digits"));
    }

  return bin(v, intValue(stack, 0), digits);
}

static struct Value *fn_bindd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int n, digits;

  n = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("number"));
    }

  digits = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("digits"));
    }

  return bin(v, n, digits);
}

static struct Value *fn_chr(struct Value *v, struct Auto *stack)
{
  long int chr = intValue(stack, 0);

  if (chr < 0 || chr > 255)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("character code"));
    }

  Value_new_STRING(v);
  String_size(&v->u.string, 1);
  v->u.string.character[0] = chr;
  return v;
}

static struct Value *fn_cint(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, ceil(realValue(stack, 0)));
}

static struct Value *fn_cos(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, cos(realValue(stack, 0)));
}

static struct Value *fn_command(struct Value *v, struct Auto *stack)
{
  int i;

  Value_new_STRING(v);
  for (i = 0; i < g_bas_argc; ++i)
    {
      if (i)
        {
          String_appendChar(&v->u.string, ' ');
        }

      String_appendChars(&v->u.string, g_bas_argv[i]);
    }

  return v;
}

static struct Value *fn_commandi(struct Value *v, struct Auto *stack)
{
  int a;

  a = intValue(stack, 0);
  if (a < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("argument number"));
    }

  Value_new_STRING(v);
  if (a == 0)
    {
      if (g_bas_argv0 != (char *)0)
        {
          String_appendChars(&v->u.string, g_bas_argv0);
        }
    }
  else if (a <= g_bas_argc)
    {
      String_appendChars(&v->u.string, g_bas_argv[a - 1]);
    }

  return v;
}

static struct Value *fn_commandd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int a;

  a = Value_toi(realValue(stack, 0), &overflow);
  if (overflow || a < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("argument number"));
    }

  Value_new_STRING(v);
  if (a == 0)
    {
      if (g_bas_argv0 != (char *)0)
        {
          String_appendChars(&v->u.string, g_bas_argv0);
        }
    }
  else if (a <= g_bas_argc)
    {
      String_appendChars(&v->u.string, g_bas_argv[a - 1]);
    }

  return v;
}

static struct Value *fn_cvi(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);
  long int n = (s->length && s->character[s->length - 1] < 0) ? -1 : 0;
  int i;

  for (i = s->length - 1; i >= 0; --i)
    {
      n = (n << 8) | (s->character[i] & 0xff);
    }

  return Value_new_INTEGER(v, n);
}

static struct Value *fn_cvs(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);
  float n;

  if (s->length != sizeof(float))
    {
      return Value_new_ERROR(v, BADCONVERSION, _("number"));
    }

  memcpy(&n, s->character, sizeof(float));
  return Value_new_REAL(v, (double)n);
}

static struct Value *fn_cvd(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);
  double n;

  if (s->length != sizeof(double))
    {
      return Value_new_ERROR(v, BADCONVERSION, _("number"));
    }

  memcpy(&n, s->character, sizeof(double));
  return Value_new_REAL(v, n);
}

static struct Value *fn_date(struct Value *v, struct Auto *stack)
{
  time_t t;
  struct tm *now;

  Value_new_STRING(v);
  String_size(&v->u.string, 10);
  time(&t);
  now = localtime(&t);
  sprintf(v->u.string.character, "%02d-%02d-%04d", now->tm_mon + 1,
          now->tm_mday, now->tm_year + 1900);
  return v;
}

static struct Value *fn_dec(struct Value *v, struct Auto *stack)
{
  struct Value value, *arg;
  size_t using;

  Value_new_STRING(v);
  arg = Var_value(Auto_local(stack, 0), 0, (int *)0, &value);
  using = 0;
  Value_toStringUsing(arg, &v->u.string, stringValue(stack, 1), &using);
  return v;
}

static struct Value *fn_deg(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, realValue(stack, 0) * (180.0 / M_PI));
}

static struct Value *fn_det(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v,
                        stack->lastdet.type ==
                        V_NIL ? 0.0 : (stack->lastdet.type ==
                                       V_REAL ? stack->lastdet.u.
                                       real : stack->lastdet.u.integer));
}

static struct Value *fn_edit(struct Value *v, struct Auto *stack)
{
  int code;
  char *begin, *end, *rd, *wr;
  char quote;

  code = intValue(stack, 1);
  Value_new_STRING(v);
  String_appendString(&v->u.string, stringValue(stack, 0));
  begin = rd = wr = v->u.string.character;
  end = rd + v->u.string.length;

  /* 8 - Discard Leading Spaces and Tabs */

  if (code & 8)
    {
      while (rd < end && (*rd == ' ' || *rd == '\t'))
        {
          ++rd;
        }
    }

  while (rd < end)
    {
      /* 1 - Discard parity bit */

      if (code & 1)
        {
          *rd = *rd & 0x7f;
        }

      /* 2 - Discard all spaces and tabs */

      if ((code & 2) && (*rd == ' ' || *rd == '\t'))
        {
          ++rd;
          continue;
        }

      /* 4 - Discard all carriage returns, line feeds, form feeds, deletes,
       * escapes, and nulls */

      if ((code & 4) &&
          (*rd == '\r' || *rd == '\n' || *rd == '\f' || *rd == 127 || *rd == 27
           || *rd == '\0'))
        {
          ++rd;
          continue;
        }

      /* 16 - Convert Multiple Spaces and Tabs to one space */

      if ((code & 16) && ((*rd == ' ') || (*rd == '\t')))
        {
          *wr++ = ' ';
          while (rd < end && (*rd == ' ' || *rd == '\t'))
            {
              ++rd;
            }

          continue;
        }

      /* 32 - Convert lower to upper case */

      if ((code & 32) && islower((int)*rd))
        {
          *wr++ = toupper((int)*rd++);
          continue;
        }

      /* 64 - Convert brackets to parentheses */

      if (code & 64)
        {
          if (*rd == '[')
            {
              *wr++ = '(';
              ++rd;
              continue;
            }
          else if (*rd == ']')
            {
              *wr++ = ')';
              ++rd;
              continue;
            }
        }

      /* 256 - Suppress all editing for characters within quotation marks */

      if ((code & 256) && (*rd == '"' || *rd == '\''))
        {
          quote = *rd;
          *wr++ = *rd++;
          while (rd < end && *rd != quote)
            {
              *wr++ = *rd++;
            }

          if (rd < end)
            {
              *wr++ = *rd++;
              quote = '\0';
            }

          continue;
        }

      *wr++ = *rd++;
    }

  /* 128 - Discard Trailing Spaces and Tabs */

  if ((code & 128) && wr > begin)
    {
      while (wr > begin && (*(wr - 1) == '\0' || *(wr - 1) == '\t'))
        {
          --wr;
        }
    }

  String_size(&v->u.string, wr - begin);
  return v;
}

static struct Value *fn_environi(struct Value *v, struct Auto *stack)
{
  return env(v, intValue(stack, 0));
}

static struct Value *fn_environd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int n;

  n = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("number"));
    }

  return env(v, n);
}

static struct Value *fn_environs(struct Value *v, struct Auto *stack)
{
  char *var;

  Value_new_STRING(v);
  if ((var = stringValue(stack, 0)->character))
    {
      char *val = getenv(var);

      if (val)
        {
          String_appendChars(&v->u.string, val);
        }
    }

  return v;
}

static struct Value *fn_eof(struct Value *v, struct Auto *stack)
{
  int e = FS_eof(intValue(stack, 0));

  if (e == -1)
    {
      return Value_new_ERROR(v, IOERROR, FS_errmsg);
    }

  return Value_new_INTEGER(v, e ? -1 : 0);
}

static struct Value *fn_erl(struct Value *v, struct Auto *stack)
{
  return Value_new_INTEGER(v, stack->erl);
}

static struct Value *fn_err(struct Value *v, struct Auto *stack)
{
  return Value_new_INTEGER(v,
                           stack->err.type ==
                           V_NIL ? 0 : stack->err.u.error.code);
}

static struct Value *fn_exp(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, exp(realValue(stack, 0)));
}

static struct Value *fn_false(struct Value *v, struct Auto *stack)
{
  return Value_new_INTEGER(v, 0);
}

static struct Value *fn_find(struct Value *v, struct Auto *stack)
{
  return find(v, stringValue(stack, 0), 0);
}

static struct Value *fn_findi(struct Value *v, struct Auto *stack)
{
  return find(v, stringValue(stack, 0), intValue(stack, 1));
}

static struct Value *fn_findd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int n;

  n = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("number"));
    }

  return find(v, stringValue(stack, 0), n);
}

static struct Value *fn_fix(struct Value *v, struct Auto *stack)
{
  double x = realValue(stack, 0);
  return Value_new_REAL(v, x < 0.0 ? ceil(x) : floor(x));
}

static struct Value *fn_frac(struct Value *v, struct Auto *stack)
{
  double x = realValue(stack, 0);
  return Value_new_REAL(v, x < 0.0 ? x - ceil(x) : x - floor(x));
}

static struct Value *fn_freefile(struct Value *v, struct Auto *stack)
{
  return Value_new_INTEGER(v, FS_freechn());
}

static struct Value *fn_hexi(struct Value *v, struct Auto *stack)
{
  char buf[sizeof(long int) * 2 + 1];

  sprintf(buf, "%lx", intValue(stack, 0));
  Value_new_STRING(v);
  String_appendChars(&v->u.string, buf);
  return v;
}

static struct Value *fn_hexd(struct Value *v, struct Auto *stack)
{
  char buf[sizeof(long int) * 2 + 1];
  int overflow;
  long int n;

  n = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("number"));
    }

  sprintf(buf, "%lx", n);
  Value_new_STRING(v);
  String_appendChars(&v->u.string, buf);
  return v;
}

static struct Value *fn_hexii(struct Value *v, struct Auto *stack)
{
  return hex(v, intValue(stack, 0), intValue(stack, 1));
}

static struct Value *fn_hexdi(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int n;

  n = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("number"));
    }
  return hex(v, n, intValue(stack, 1));
}

static struct Value *fn_hexid(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int digits;

  digits = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("digits"));
    }

  return hex(v, intValue(stack, 0), digits);
}

static struct Value *fn_hexdd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int n, digits;

  n = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("number"));
    }

  digits = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("digits"));
    }

  return hex(v, n, digits);
}

static struct Value *fn_int(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, floor(realValue(stack, 0)));
}

static struct Value *fn_intp(struct Value *v, struct Auto *stack)
{
  long int l;

  errno = 0;
  l = lrint(floor(realValue(stack, 0)));
  if (errno == EDOM)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("number"));
    }

  return Value_new_INTEGER(v, l);
}

static struct Value *fn_inp(struct Value *v, struct Auto *stack)
{
  int r = FS_portInput(intValue(stack, 0));

  if (r == -1)
    {
      return Value_new_ERROR(v, IOERROR, FS_errmsg);
    }
  else
    {
      return Value_new_INTEGER(v, r);
    }
}

static struct Value *fn_input1(struct Value *v, struct Auto *stack)
{
  return input(v, intValue(stack, 0), STDCHANNEL);
}

static struct Value *fn_input2(struct Value *v, struct Auto *stack)
{
  return input(v, intValue(stack, 0), intValue(stack, 1));
}

static struct Value *fn_inkey(struct Value *v, struct Auto *stack)
{
  return inkey(v, 0, STDCHANNEL);
}

static struct Value *fn_inkeyi(struct Value *v, struct Auto *stack)
{
  return inkey(v, intValue(stack, 0), STDCHANNEL);
}

static struct Value *fn_inkeyd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int t;

  t = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("time"));
    }

  return inkey(v, t, STDCHANNEL);
}

static struct Value *fn_inkeyii(struct Value *v, struct Auto *stack)
{
  return inkey(v, intValue(stack, 0), intValue(stack, 1));
}

static struct Value *fn_inkeyid(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int chn;

  chn = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("channel"));
    }

  return inkey(v, intValue(stack, 0), chn);
}

static struct Value *fn_inkeydi(struct Value *v, struct Auto *stack)
{
  return inkey(v, realValue(stack, 0), intValue(stack, 1));
}

static struct Value *fn_inkeydd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int t, chn;

  t = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("time"));
    }

  chn = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("channel"));
    }

  return inkey(v, t, chn);
}

static struct Value *fn_instr2(struct Value *v, struct Auto *stack)
{
  struct String *haystack = stringValue(stack, 0);

  return instr(v, 1, haystack->length, haystack, stringValue(stack, 1));
}

static struct Value *fn_instr3iss(struct Value *v, struct Auto *stack)
{
  struct String *haystack = stringValue(stack, 1);

  return instr(v, intValue(stack, 0), haystack->length, haystack,
               stringValue(stack, 2));
}

static struct Value *fn_instr3ssi(struct Value *v, struct Auto *stack)
{
  struct String *haystack = stringValue(stack, 0);

  return instr(v, intValue(stack, 2), haystack->length, haystack,
               stringValue(stack, 1));
}

static struct Value *fn_instr3dss(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int start;
  struct String *haystack;

  start = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("start"));
    }

  haystack = stringValue(stack, 1);
  return instr(v, start, haystack->length, haystack, stringValue(stack, 2));
}

static struct Value *fn_instr3ssd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int start;
  struct String *haystack;

  start = Value_toi(realValue(stack, 2), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("start"));
    }

  haystack = stringValue(stack, 0);
  return instr(v, start, haystack->length, haystack, stringValue(stack, 1));
}

static struct Value *fn_instr4ii(struct Value *v, struct Auto *stack)
{
  return instr(v, intValue(stack, 2), intValue(stack, 3), stringValue(stack, 0),
               stringValue(stack, 1));
}

static struct Value *fn_instr4id(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int len;

  len = Value_toi(realValue(stack, 3), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  return instr(v, intValue(stack, 2), len, stringValue(stack, 0),
               stringValue(stack, 1));
}

static struct Value *fn_instr4di(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int start;

  start = Value_toi(realValue(stack, 2), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("start"));
    }

  return instr(v, start, intValue(stack, 3), stringValue(stack, 0),
               stringValue(stack, 1));
}

static struct Value *fn_instr4dd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int start, len;

  start = Value_toi(realValue(stack, 2), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("start"));
    }

  len = Value_toi(realValue(stack, 3), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  return instr(v, start, len, stringValue(stack, 0), stringValue(stack, 1));
}

static struct Value *fn_lcase(struct Value *v, struct Auto *stack)
{
  Value_new_STRING(v);
  String_appendString(&v->u.string, stringValue(stack, 0));
  String_lcase(&v->u.string);
  return v;
}

static struct Value *fn_len(struct Value *v, struct Auto *stack)
{
  return Value_new_INTEGER(v, stringValue(stack, 0)->length);
}

static struct Value *fn_left(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);
  long int len = intValue(stack, 1);
  int left = ((size_t) len) < s->length ? len : s->length;

  if (left < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  Value_new_STRING(v);
  String_size(&v->u.string, left);
  if (left)
    {
      memcpy(v->u.string.character, s->character, left);
    }

  return v;
}

static struct Value *fn_loc(struct Value *v, struct Auto *stack)
{
  long int l = FS_loc(intValue(stack, 0));

  if (l == -1)
    {
      return Value_new_ERROR(v, IOERROR, FS_errmsg);
    }

  return Value_new_INTEGER(v, l);
}

static struct Value *fn_lof(struct Value *v, struct Auto *stack)
{
  long int l = FS_lof(intValue(stack, 0));

  if (l == -1)
    {
      return Value_new_ERROR(v, IOERROR, FS_errmsg);
    }

  return Value_new_INTEGER(v, l);
}

static struct Value *fn_log(struct Value *v, struct Auto *stack)
{
  if (realValue(stack, 0) <= 0.0)
    {
      Value_new_ERROR(v, UNDEFINED, _("Logarithm of negative value"));
    }
  else
    {
      Value_new_REAL(v, log(realValue(stack, 0)));
    }

  return v;
}

static struct Value *fn_log10(struct Value *v, struct Auto *stack)
{
  if (realValue(stack, 0) <= 0.0)
    {
      Value_new_ERROR(v, UNDEFINED, _("Logarithm of negative value"));
    }
  else
    {
      Value_new_REAL(v, log10(realValue(stack, 0)));
    }

  return v;
}

static struct Value *fn_log2(struct Value *v, struct Auto *stack)
{
  if (realValue(stack, 0) <= 0.0)
    {
      Value_new_ERROR(v, UNDEFINED, _("Logarithm of negative value"));
    }
  else
    {
      Value_new_REAL(v, log2(realValue(stack, 0)));
    }

  return v;
}

static struct Value *fn_ltrim(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);
  int len = s->length;
  int spaces;

  for (spaces = 0; spaces < len && s->character[spaces] == ' '; ++spaces);
  Value_new_STRING(v);
  String_size(&v->u.string, len - spaces);
  if (len - spaces)
    {
      memcpy(v->u.string.character, s->character + spaces, len - spaces);
    }

  return v;
}

static struct Value *fn_match(struct Value *v, struct Auto *stack)
{
  struct String *needle = stringValue(stack, 0);
  const char *needleChars = needle->character;
  const char *needleEnd = needle->character + needle->length;
  struct String *haystack = stringValue(stack, 1);
  const char *haystackChars = haystack->character;
  size_t haystackLength = haystack->length;
  long int start = intValue(stack, 2);
  long int found;
  const char *n, *h;

  if (start < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("position"));
    }

  if (((size_t) start) >= haystackLength)
    {
      return Value_new_INTEGER(v, 0);
    }

  haystackChars += start;
  haystackLength -= start;
  found = 1 + start;
  while (haystackLength)
    {
      for (n = needleChars, h = haystackChars;
           n < needleEnd && h < (haystackChars + haystackLength); ++n, ++h)
        {
          if (*n == '\\')
            {
              if (++n < needleEnd && *n != *h)
                {
                  break;
                }
            }
          else if (*n == '!')
            {
              if (!isalpha((int)*h))
                {
                  break;
                }
            }
          else if (*n == '#')
            {
              if (!isdigit((int)*h))
                {
                  break;
                }
            }
          else if (*n != '?' && *n != *h)
            {
              break;
            }
        }

      if (n == needleEnd)
        {
          return Value_new_INTEGER(v, found);
        }

      ++haystackChars;
      --haystackLength;
      ++found;
    }

  return Value_new_INTEGER(v, 0);
}

static struct Value *fn_maxii(struct Value *v, struct Auto *stack)
{
  long int x, y;

  x = intValue(stack, 0);
  y = intValue(stack, 1);
  return Value_new_INTEGER(v, x > y ? x : y);
}

static struct Value *fn_maxdi(struct Value *v, struct Auto *stack)
{
  double x;
  long int y;

  x = realValue(stack, 0);
  y = intValue(stack, 1);
  return Value_new_REAL(v, x > y ? x : y);
}

static struct Value *fn_maxid(struct Value *v, struct Auto *stack)
{
  long int x;
  double y;

  x = intValue(stack, 0);
  y = realValue(stack, 1);
  return Value_new_REAL(v, x > y ? x : y);
}

static struct Value *fn_maxdd(struct Value *v, struct Auto *stack)
{
  double x, y;

  x = realValue(stack, 0);
  y = realValue(stack, 1);
  return Value_new_REAL(v, x > y ? x : y);
}

static struct Value *fn_mid2i(struct Value *v, struct Auto *stack)
{
  return mid(v, stringValue(stack, 0), intValue(stack, 1),
             stringValue(stack, 0)->length);
}

static struct Value *fn_mid2d(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int start;

  start = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("start"));
    }

  return mid(v, stringValue(stack, 0), start, stringValue(stack, 0)->length);
}

static struct Value *fn_mid3ii(struct Value *v, struct Auto *stack)
{
  return mid(v, stringValue(stack, 0), intValue(stack, 1), intValue(stack, 2));
}

static struct Value *fn_mid3id(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int len;

  len = Value_toi(realValue(stack, 2), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  return mid(v, stringValue(stack, 0), intValue(stack, 1), len);
}

static struct Value *fn_mid3di(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int start;

  start = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("start"));
    }

  return mid(v, stringValue(stack, 0), start, intValue(stack, 2));
}

static struct Value *fn_mid3dd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int start, len;

  start = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("start"));
    }

  len = Value_toi(realValue(stack, 2), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  return mid(v, stringValue(stack, 0), start, len);
}

static struct Value *fn_minii(struct Value *v, struct Auto *stack)
{
  long int x, y;

  x = intValue(stack, 0);
  y = intValue(stack, 1);
  return Value_new_INTEGER(v, x < y ? x : y);
}

static struct Value *fn_mindi(struct Value *v, struct Auto *stack)
{
  double x;
  long int y;

  x = realValue(stack, 0);
  y = intValue(stack, 1);
  return Value_new_REAL(v, x < y ? x : y);
}

static struct Value *fn_minid(struct Value *v, struct Auto *stack)
{
  long int x;
  double y;

  x = intValue(stack, 0);
  y = realValue(stack, 1);
  return Value_new_REAL(v, x < y ? x : y);
}

static struct Value *fn_mindd(struct Value *v, struct Auto *stack)
{
  double x, y;

  x = realValue(stack, 0);
  y = realValue(stack, 1);
  return Value_new_REAL(v, x < y ? x : y);
}

static struct Value *fn_mki(struct Value *v, struct Auto *stack)
{
  long int x = intValue(stack, 0);
  size_t i;

  Value_new_STRING(v);
  String_size(&v->u.string, sizeof(long int));
  for (i = 0; i < sizeof(long int); ++i, x >>= 8)
    {
      v->u.string.character[i] = (x & 0xff);
    }

  return v;
}

static struct Value *fn_mks(struct Value *v, struct Auto *stack)
{
  float x = realValue(stack, 0);

  Value_new_STRING(v);
  String_size(&v->u.string, sizeof(float));
  memcpy(v->u.string.character, &x, sizeof(float));
  return v;
}

static struct Value *fn_mkd(struct Value *v, struct Auto *stack)
{
  double x = realValue(stack, 0);

  Value_new_STRING(v);
  String_size(&v->u.string, sizeof(double));
  memcpy(v->u.string.character, &x, sizeof(double));
  return v;
}

static struct Value *fn_oct(struct Value *v, struct Auto *stack)
{
  char buf[sizeof(long int) * 3 + 1];

  sprintf(buf, "%lo", intValue(stack, 0));
  Value_new_STRING(v);
  String_appendChars(&v->u.string, buf);
  return v;
}

static struct Value *fn_pi(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, M_PI);
}

static struct Value *fn_peek(struct Value *v, struct Auto *stack)
{
  int r = FS_memInput(intValue(stack, 0));

  if (r == -1)
    {
      return Value_new_ERROR(v, IOERROR, FS_errmsg);
    }
  else
    {
      return Value_new_INTEGER(v, r);
    }
}

static struct Value *fn_pos(struct Value *v, struct Auto *stack)
{
  return Value_new_INTEGER(v, FS_charpos(STDCHANNEL) + 1);
}

static struct Value *fn_rad(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, (realValue(stack, 0) * M_PI) / 180.0);
}

static struct Value *fn_right(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);
  int len = s->length;
  int right = intValue(stack, 1) < len ? intValue(stack, 1) : len;
  if (right < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  Value_new_STRING(v);
  String_size(&v->u.string, right);
  if (right)
    {
      memcpy(v->u.string.character, s->character + len - right, right);
    }

  return v;
}

static struct Value *fn_rnd(struct Value *v, struct Auto *stack)
{
  return rnd(v, 0);
}

static struct Value *fn_rndi(struct Value *v, struct Auto *stack)
{
  return rnd(v, intValue(stack, 0));
}

static struct Value *fn_rndd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int limit;

  limit = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("limit"));
    }

  return rnd(v, limit);
}

static struct Value *fn_rtrim(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);
  int len = s->length;
  int lastSpace;

  for (lastSpace = len; lastSpace > 0 && s->character[lastSpace - 1] == ' ';
       --lastSpace);

  Value_new_STRING(v);
  String_size(&v->u.string, lastSpace);
  if (lastSpace)
    {
      memcpy(v->u.string.character, s->character, lastSpace);
    }

  return v;
}

static struct Value *fn_sgn(struct Value *v, struct Auto *stack)
{
  double x = realValue(stack, 0);
  return Value_new_INTEGER(v, x < 0.0 ? -1 : (x == 0.0 ? 0 : 1));
}

static struct Value *fn_sin(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, sin(realValue(stack, 0)));
}

static struct Value *fn_space(struct Value *v, struct Auto *stack)
{
  long int len = intValue(stack, 0);

  if (len < 0)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  Value_new_STRING(v);
  String_size(&v->u.string, len);
  if (len)
    {
      memset(v->u.string.character, ' ', len);
    }

  return v;
}

static struct Value *fn_sqr(struct Value *v, struct Auto *stack)
{
  if (realValue(stack, 0) < 0.0)
    {
      Value_new_ERROR(v, OUTOFRANGE, _("Square root argument"));
    }
  else
    {
      Value_new_REAL(v, sqrt(realValue(stack, 0)));
    }

  return v;
}

static struct Value *fn_str(struct Value *v, struct Auto *stack)
{
  struct Value value, *arg;
  struct String s;

  arg = Var_value(Auto_local(stack, 0), 0, (int *)0, &value);
  assert(arg->type != V_ERROR);
  String_new(&s);
  Value_toString(arg, &s, ' ', -1, 0, 0, 0, 0, -1, 0, 0);
  v->type = V_STRING;
  v->u.string = s;
  return v;
}

static struct Value *fn_stringii(struct Value *v, struct Auto *stack)
{
  return string(v, intValue(stack, 0), intValue(stack, 1));
}

static struct Value *fn_stringid(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int chr;

  chr = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("character code"));
    }

  return string(v, intValue(stack, 0), chr);
}

static struct Value *fn_stringdi(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int len;

  len = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  return string(v, len, intValue(stack, 1));
}

static struct Value *fn_stringdd(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int len, chr;

  len = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  chr = Value_toi(realValue(stack, 1), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("character code"));
    }

  return string(v, len, chr);
}

static struct Value *fn_stringis(struct Value *v, struct Auto *stack)
{
  if (stringValue(stack, 1)->length == 0)
    {
      return Value_new_ERROR(v, UNDEFINED, _("`string$' of empty string"));
    }

  return string(v, intValue(stack, 0), stringValue(stack, 1)->character[0]);
}

static struct Value *fn_stringds(struct Value *v, struct Auto *stack)
{
  int overflow;
  long int len;

  len = Value_toi(realValue(stack, 0), &overflow);
  if (overflow)
    {
      return Value_new_ERROR(v, OUTOFRANGE, _("length"));
    }

  if (stringValue(stack, 1)->length == 0)
    {
      return Value_new_ERROR(v, UNDEFINED, _("`string$' of empty string"));
    }

  return string(v, len, stringValue(stack, 1)->character[0]);
}

static struct Value *fn_strip(struct Value *v, struct Auto *stack)
{
  size_t i;

  Value_new_STRING(v);
  String_appendString(&v->u.string, stringValue(stack, 0));
  for (i = 0; i < v->u.string.length; ++i)
    {
      v->u.string.character[i] &= 0x7f;
    }

  return v;
}

static struct Value *fn_tan(struct Value *v, struct Auto *stack)
{
  return Value_new_REAL(v, tan(realValue(stack, 0)));
}

static struct Value *fn_timei(struct Value *v, struct Auto *stack)
{
  return Value_new_INTEGER(v, (unsigned long)(clock() / (CLK_TCK / 100.0)));
}

static struct Value *fn_times(struct Value *v, struct Auto *stack)
{
  time_t t;
  struct tm *now;

  Value_new_STRING(v);
  String_size(&v->u.string, 8);
  time(&t);
  now = localtime(&t);
  sprintf(v->u.string.character, "%02d:%02d:%02d", now->tm_hour, now->tm_min,
          now->tm_sec);
  return v;
}

static struct Value *fn_timer(struct Value *v, struct Auto *stack)
{
  time_t t;
  struct tm *l;

  time(&t);
  l = localtime(&t);
  return Value_new_REAL(v, l->tm_hour * 3600 + l->tm_min * 60 + l->tm_sec);
}

static struct Value *fn_tl(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);

  Value_new_STRING(v);
  if (s->length)
    {
      int tail = s->length - 1;

      String_size(&v->u.string, tail);
      if (s->length)
        {
          memcpy(v->u.string.character, s->character + 1, tail);
        }
    }
  return v;
}

static struct Value *fn_true(struct Value *v, struct Auto *stack)
{
  return Value_new_INTEGER(v, -1);
}

static struct Value *fn_ucase(struct Value *v, struct Auto *stack)
{
  Value_new_STRING(v);
  String_appendString(&v->u.string, stringValue(stack, 0));
  String_ucase(&v->u.string);
  return v;
}

static struct Value *fn_val(struct Value *v, struct Auto *stack)
{
  struct String *s = stringValue(stack, 0);
  char *end;
  long int i;
  int overflow;

  if (s->character == (char *)0)
    {
      return Value_new_REAL(v, 0.0);
    }

  i = Value_vali(s->character, &end, &overflow);
  if (*end == '\0')
    {
      return Value_new_INTEGER(v, i);
    }
  else
    {
      return Value_new_REAL(v, Value_vald(s->character, (char **)0, &overflow));
    }
}

static unsigned int hash(const char *s)
{
  unsigned int h = 0;

  while (*s)
    {
      h = h * 256 + tolower(*s);
      ++s;
    }

  return h % GLOBAL_HASHSIZE;
}

static void builtin(struct Global *this, const char *ident, enum ValueType type,
                    struct Value *(*func) (struct Value * value,
                                           struct Auto * stack), int argLength,
                    ...)
{
  struct Symbol **r;
  struct Symbol *s, **sptr;
  int i;
  va_list ap;

  for (r = &this->table[hash(ident)];
       *r != (struct Symbol *)0 && cistrcmp((*r)->name, ident);
       r = &((*r)->next));

  if (*r == (struct Symbol *)0)
    {
      *r = malloc(sizeof(struct Symbol));
      (*r)->name = strcpy(malloc(strlen(ident) + 1), ident);
      (*r)->next = (struct Symbol *)0;
      s = (*r);
    }
  else
    {
      for (sptr = &((*r)->u.sub.u.bltin.next); *sptr;
           sptr = &((*sptr)->u.sub.u.bltin.next));

      *sptr = s = malloc(sizeof(struct Symbol));
    }

  s->u.sub.u.bltin.next = (struct Symbol *)0;
  s->type = BUILTINFUNCTION;
  s->u.sub.argLength = argLength;
  s->u.sub.argTypes =
    argLength ? malloc(sizeof(enum ValueType) *
                       argLength) : (enum ValueType *)0;
  s->u.sub.retType = type;
  va_start(ap, argLength);
  for (i = 0; i < argLength; ++i)
    {
      s->u.sub.argTypes[i] = (enum ValueType)va_arg(ap, int);
    }

  va_end(ap);
  s->u.sub.u.bltin.call = func;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct Global *Global_new(struct Global *this)
{
  builtin(this, "abs", V_REAL, fn_abs, 1, (int)V_REAL);
  builtin(this, "asc", V_INTEGER, fn_asc, 1, (int)V_STRING);
  builtin(this, "atn", V_REAL, fn_atn, 1, (int)V_REAL);
  builtin(this, "bin$", V_STRING, fn_bini, 1, (int)V_INTEGER);
  builtin(this, "bin$", V_STRING, fn_bind, 1, (int)V_REAL);
  builtin(this, "bin$", V_STRING, fn_binii, 2, (int)V_INTEGER, (int)V_INTEGER);
  builtin(this, "bin$", V_STRING, fn_bindi, 2, (int)V_REAL, (int)V_INTEGER);
  builtin(this, "bin$", V_STRING, fn_binid, 2, (int)V_INTEGER, (int)V_REAL);
  builtin(this, "bin$", V_STRING, fn_bindd, 2, (int)V_REAL, (int)V_REAL);
  builtin(this, "chr$", V_STRING, fn_chr, 1, (int)V_INTEGER);
  builtin(this, "cint", V_REAL, fn_cint, 1, (int)V_REAL);
  builtin(this, "code", V_INTEGER, fn_asc, 1, (int)V_STRING);
  builtin(this, "command$", V_STRING, fn_command, 0);
  builtin(this, "command$", V_STRING, fn_commandi, 1, (int)V_INTEGER);
  builtin(this, "command$", V_STRING, fn_commandd, 1, (int)V_REAL);
  builtin(this, "cos", V_REAL, fn_cos, 1, (int)V_REAL);
  builtin(this, "cvi", V_INTEGER, fn_cvi, 1, (int)V_STRING);
  builtin(this, "cvs", V_REAL, fn_cvs, 1, (int)V_STRING);
  builtin(this, "cvd", V_REAL, fn_cvd, 1, (int)V_STRING);
  builtin(this, "date$", V_STRING, fn_date, 0);
  builtin(this, "dec$", V_STRING, fn_dec, 2, (int)V_REAL, (int)V_STRING);
  builtin(this, "dec$", V_STRING, fn_dec, 2, (int)V_INTEGER, (int)V_STRING);
  builtin(this, "dec$", V_STRING, fn_dec, 2, (int)V_STRING, (int)V_STRING);
  builtin(this, "deg", V_REAL, fn_deg, 1, (int)V_REAL);
  builtin(this, "det", V_REAL, fn_det, 0);
  builtin(this, "edit$", V_STRING, fn_edit, 2, (int)V_STRING, (int)V_INTEGER);
  builtin(this, "environ$", V_STRING, fn_environi, 1, (int)V_INTEGER);
  builtin(this, "environ$", V_STRING, fn_environd, 1, (int)V_REAL);
  builtin(this, "environ$", V_STRING, fn_environs, 1, (int)V_STRING);
  builtin(this, "eof", V_INTEGER, fn_eof, 1, (int)V_INTEGER);
  builtin(this, "erl", V_INTEGER, fn_erl, 0);
  builtin(this, "err", V_INTEGER, fn_err, 0);
  builtin(this, "exp", V_REAL, fn_exp, 1, (int)V_REAL);
  builtin(this, "false", V_INTEGER, fn_false, 0);
  builtin(this, "find$", V_STRING, fn_find, 1, (int)V_STRING);
  builtin(this, "find$", V_STRING, fn_findi, 2, (int)V_STRING, (int)V_INTEGER);
  builtin(this, "find$", V_STRING, fn_findd, 2, (int)V_STRING, (int)V_REAL);
  builtin(this, "fix", V_REAL, fn_fix, 1, (int)V_REAL);
  builtin(this, "frac", V_REAL, fn_frac, 1, (int)V_REAL);
  builtin(this, "freefile", V_INTEGER, fn_freefile, 0);
  builtin(this, "fp", V_REAL, fn_frac, 1, (int)V_REAL);
  builtin(this, "hex$", V_STRING, fn_hexi, 1, (int)V_INTEGER);
  builtin(this, "hex$", V_STRING, fn_hexd, 1, (int)V_REAL);
  builtin(this, "hex$", V_STRING, fn_hexii, 2, (int)V_INTEGER, (int)V_INTEGER);
  builtin(this, "hex$", V_STRING, fn_hexdi, 2, (int)V_REAL, (int)V_INTEGER);
  builtin(this, "hex$", V_STRING, fn_hexid, 2, (int)V_INTEGER, (int)V_REAL);
  builtin(this, "hex$", V_STRING, fn_hexdd, 2, (int)V_REAL, (int)V_REAL);
  builtin(this, "inkey$", V_STRING, fn_inkey, 0);
  builtin(this, "inkey$", V_STRING, fn_inkeyi, 1, (int)V_INTEGER);
  builtin(this, "inkey$", V_STRING, fn_inkeyd, 1, (int)V_REAL);
  builtin(this, "inkey$", V_STRING, fn_inkeyii, 2, (int)V_INTEGER, (int)V_INTEGER);
  builtin(this, "inkey$", V_STRING, fn_inkeyid, 2, (int)V_INTEGER, (int)V_REAL);
  builtin(this, "inkey$", V_STRING, fn_inkeydi, 2, (int)V_REAL, (int)V_INTEGER);
  builtin(this, "inkey$", V_STRING, fn_inkeydd, 2, (int)V_REAL, (int)V_REAL);
  builtin(this, "inp", V_INTEGER, fn_inp, 1, (int)V_INTEGER);
  builtin(this, "input$", V_STRING, fn_input1, 1, (int)V_INTEGER);
  builtin(this, "input$", V_STRING, fn_input2, 2, (int)V_INTEGER, (int)V_INTEGER);
  builtin(this, "instr", V_INTEGER, fn_instr2, 2, (int)V_STRING, (int)V_STRING);
  builtin(this, "instr", V_INTEGER, fn_instr3iss, 3, (int)V_INTEGER, (int)V_STRING,
          V_STRING);
  builtin(this, "instr", V_INTEGER, fn_instr3ssi, 3, (int)V_STRING, (int)V_STRING,
          V_INTEGER);
  builtin(this, "instr", V_INTEGER, fn_instr3dss, 3, (int)V_REAL, (int)V_STRING,
          V_STRING);
  builtin(this, "instr", V_INTEGER, fn_instr3ssd, 3, (int)V_STRING, (int)V_STRING,
          V_REAL);
  builtin(this, "instr", V_INTEGER, fn_instr4ii, 4, (int)V_STRING, (int)V_STRING,
          (int)V_INTEGER, (int)V_INTEGER);
  builtin(this, "instr", V_INTEGER, fn_instr4id, 4, (int)V_STRING, (int)V_STRING,
          (int)V_INTEGER, (int)V_REAL);
  builtin(this, "instr", V_INTEGER, fn_instr4di, 4, (int)V_STRING, (int)V_STRING,
          (int)V_REAL, (int)V_INTEGER);
  builtin(this, "instr", V_INTEGER, fn_instr4dd, 4, (int)V_STRING, (int)V_STRING,
          (int)V_REAL, (int)V_REAL);
  builtin(this, "int", V_REAL, fn_int, 1, (int)V_REAL);
  builtin(this, "int%", V_INTEGER, fn_intp, 1, (int)V_REAL);
  builtin(this, "ip", V_REAL, fn_fix, 1, (int)V_REAL);
  builtin(this, "lcase$", V_STRING, fn_lcase, 1, (int)V_STRING);
  builtin(this, "lower$", V_STRING, fn_lcase, 1, (int)V_STRING);
  builtin(this, "left$", V_STRING, fn_left, 2, (int)V_STRING, (int)V_INTEGER);
  builtin(this, "len", V_INTEGER, fn_len, 1, (int)V_STRING);
  builtin(this, "loc", V_INTEGER, fn_loc, 1, (int)V_INTEGER);
  builtin(this, "lof", V_INTEGER, fn_lof, 1, (int)V_INTEGER);
  builtin(this, "log", V_REAL, fn_log, 1, (int)V_REAL);
  builtin(this, "log10", V_REAL, fn_log10, 1, (int)V_REAL);
  builtin(this, "log2", V_REAL, fn_log2, 1, (int)V_REAL);
  builtin(this, "ltrim$", V_STRING, fn_ltrim, 1, (int)V_STRING);
  builtin(this, "match", V_INTEGER, fn_match, 3, (int)V_STRING, (int)V_STRING,
          (int)V_INTEGER);
  builtin(this, "max", V_INTEGER, fn_maxii, 2, (int)V_INTEGER, (int)V_INTEGER);
  builtin(this, "max", V_REAL, fn_maxdi, 2, (int)V_REAL, (int)V_INTEGER);
  builtin(this, "max", V_REAL, fn_maxid, 2, (int)V_INTEGER, (int)V_REAL);
  builtin(this, "max", V_REAL, fn_maxdd, 2, (int)V_REAL, (int)V_REAL);
  builtin(this, "mid$", V_STRING, fn_mid2i, 2, (int)V_STRING, (int)V_INTEGER);
  builtin(this, "mid$", V_STRING, fn_mid2d, 2, (int)V_STRING, (int)V_REAL);
  builtin(this, "mid$", V_STRING, fn_mid3ii, 3, (int)V_STRING, (int)V_INTEGER,
          V_INTEGER);
  builtin(this, "mid$", V_STRING, fn_mid3id, 3, (int)V_STRING, (int)V_INTEGER, (int)V_REAL);
  builtin(this, "mid$", V_STRING, fn_mid3di, 3, (int)V_STRING, (int)V_REAL, (int)V_INTEGER);
  builtin(this, "mid$", V_STRING, fn_mid3dd, 3, (int)V_STRING, (int)V_REAL, (int)V_REAL);
  builtin(this, "min", V_INTEGER, fn_minii, 2, (int)V_INTEGER, (int)V_INTEGER);
  builtin(this, "min", V_REAL, fn_mindi, 2, (int)V_REAL, (int)V_INTEGER);
  builtin(this, "min", V_REAL, fn_minid, 2, (int)V_INTEGER, (int)V_REAL);
  builtin(this, "min", V_REAL, fn_mindd, 2, (int)V_REAL, (int)V_REAL);
  builtin(this, "mki$", V_STRING, fn_mki, 1, (int)V_INTEGER);
  builtin(this, "mks$", V_STRING, fn_mks, 1, (int)V_REAL);
  builtin(this, "mkd$", V_STRING, fn_mkd, 1, (int)V_REAL);
  builtin(this, "oct$", V_STRING, fn_oct, 1, (int)V_INTEGER);
  builtin(this, "peek", V_INTEGER, fn_peek, 1, (int)V_INTEGER);
  builtin(this, "pi", V_REAL, fn_pi, 0);
  builtin(this, "pos", V_INTEGER, fn_pos, 1, (int)V_INTEGER);
  builtin(this, "pos", V_INTEGER, fn_pos, 1, (int)V_REAL);
  builtin(this, "pos", V_INTEGER, fn_instr3ssi, 3, (int)V_STRING, (int)V_STRING,
          (int)V_INTEGER);
  builtin(this, "pos", V_INTEGER, fn_instr3ssd, 3, (int)V_STRING, (int)V_STRING,
          (int)V_REAL);
  builtin(this, "rad", V_REAL, fn_rad, 1, (int)V_REAL);
  builtin(this, "right$", V_STRING, fn_right, 2, (int)V_STRING, (int)V_INTEGER);
  builtin(this, "rnd", V_INTEGER, fn_rnd, 0);
  builtin(this, "rnd", V_INTEGER, fn_rndd, 1, (int)V_REAL);
  builtin(this, "rnd", V_INTEGER, fn_rndi, 1, (int)V_INTEGER);
  builtin(this, "rtrim$", V_STRING, fn_rtrim, 1, (int)V_STRING);
  builtin(this, "seg$", V_STRING, fn_mid3ii, 3, (int)V_STRING, (int)V_INTEGER,
          (int)V_INTEGER);
  builtin(this, "seg$", V_STRING, fn_mid3id, 3, (int)V_STRING, (int)V_INTEGER,
          (int)V_REAL);
  builtin(this, "seg$", V_STRING, fn_mid3di, 3, (int)V_STRING, (int)V_REAL,
          (int)V_INTEGER);
  builtin(this, "seg$", V_STRING, fn_mid3dd, 3, (int)V_STRING, (int)V_REAL,
          (int)V_REAL);
  builtin(this, "sgn", V_INTEGER, fn_sgn, 1, (int)V_REAL);
  builtin(this, "sin", V_REAL, fn_sin, 1, (int)V_REAL);
  builtin(this, "space$", V_STRING, fn_space, 1, (int)V_INTEGER);
  builtin(this, "sqr", V_REAL, fn_sqr, 1, (int)V_REAL);
  builtin(this, "str$", V_STRING, fn_str, 1, (int)V_REAL);
  builtin(this, "str$", V_STRING, fn_str, 1, (int)V_INTEGER);
  builtin(this, "string$", V_STRING, fn_stringii, 2, (int)V_INTEGER, (int)V_INTEGER);
  builtin(this, "string$", V_STRING, fn_stringid, 2, (int)V_INTEGER, (int)V_REAL);
  builtin(this, "string$", V_STRING, fn_stringdi, 2, (int)V_REAL, (int)V_INTEGER);
  builtin(this, "string$", V_STRING, fn_stringdd, 2, (int)V_REAL, (int)V_REAL);
  builtin(this, "string$", V_STRING, fn_stringis, 2, (int)V_INTEGER, (int)V_STRING);
  builtin(this, "string$", V_STRING, fn_stringds, 2, (int)V_REAL, (int)V_STRING);
  builtin(this, "strip$", V_STRING, fn_strip, 1, (int)V_STRING);
  builtin(this, "tan", V_REAL, fn_tan, 1, (int)V_REAL);
  builtin(this, "time", V_INTEGER, fn_timei, 0);
  builtin(this, "time$", V_STRING, fn_times, 0);
  builtin(this, "timer", V_REAL, fn_timer, 0);
  builtin(this, "tl$", V_STRING, fn_tl, 1, (int)V_STRING);
  builtin(this, "true", V_INTEGER, fn_true, 0);
  builtin(this, "ucase$", V_STRING, fn_ucase, 1, (int)V_STRING);
  builtin(this, "upper$", V_STRING, fn_ucase, 1, (int)V_STRING);
  builtin(this, "val", V_REAL, fn_val, 1, (int)V_STRING);
  return this;
}

int Global_find(struct Global *this, struct Identifier *ident, int oparen)
{
  struct Symbol **r;

  for (r = &this->table[hash(ident->name)];
       *r != (struct Symbol *)0 &&
       ((((*r)->type == GLOBALVAR && oparen) ||
         ((*r)->type == GLOBALARRAY && !oparen)) ||
        cistrcmp((*r)->name, ident->name)); r = &((*r)->next));

  if (*r == (struct Symbol *)0)
    {
      return 0;
    }

  ident->sym = (*r);
  return 1;
}

int Global_variable(struct Global *this, struct Identifier *ident,
                    enum ValueType type, enum SymbolType symbolType,
                    int redeclare)
{
  struct Symbol **r;

  for (r = &this->table[hash(ident->name)];
       *r != (struct Symbol *)0 && ((*r)->type != symbolType ||
                                    cistrcmp((*r)->name, ident->name));
       r = &((*r)->next));

  if (*r == (struct Symbol *)0)
    {
      *r = malloc(sizeof(struct Symbol));
      (*r)->name = strcpy(malloc(strlen(ident->name) + 1), ident->name);
      (*r)->next = (struct Symbol *)0;
      (*r)->type = symbolType;
      Var_new(&((*r)->u.var), type, 0, (unsigned int *)0, 0);
    }
  else if (redeclare)
    {
      Var_retype(&((*r)->u.var), type);
    }

  switch ((*r)->type)
    {
    case GLOBALVAR:
    case GLOBALARRAY:
      {
        ident->sym = (*r);
        break;
      }

    case BUILTINFUNCTION:
      {
        return 0;
      }

    case USERFUNCTION:
      {
        return 0;
      }

    default:
      assert(0);
    }

  return 1;
}

int Global_function(struct Global *this, struct Identifier *ident,
                    enum ValueType type, struct Pc *deffn, struct Pc *begin,
                    int argLength, enum ValueType *argTypes)
{
  struct Symbol **r;

  for (r = &this->table[hash(ident->name)];
       *r != (struct Symbol *)0 && cistrcmp((*r)->name, ident->name);
       r = &((*r)->next));

  if (*r != (struct Symbol *)0)
    {
      return 0;
    }

  *r = malloc(sizeof(struct Symbol));
  (*r)->name = strcpy(malloc(strlen(ident->name) + 1), ident->name);
  (*r)->next = (struct Symbol *)0;
  (*r)->type = USERFUNCTION;
  (*r)->u.sub.u.def.scope.start = *deffn;
  (*r)->u.sub.u.def.scope.begin = *begin;
  (*r)->u.sub.argLength = argLength;
  (*r)->u.sub.argTypes = argTypes;
  (*r)->u.sub.retType = type;
  (*r)->u.sub.u.def.localLength = 0;
  (*r)->u.sub.u.def.localTypes = (enum ValueType *)0;
  ident->sym = (*r);
  return 1;
}

void Global_endfunction(struct Global *this, struct Identifier *ident,
                        struct Pc *end)
{
  struct Symbol **r;

  for (r = &this->table[hash(ident->name)];
       *r != (struct Symbol *)0 && cistrcmp((*r)->name, ident->name);
       r = &((*r)->next));

  assert(*r != (struct Symbol *)0);
  (*r)->u.sub.u.def.scope.end = *end;
}

void Global_clear(struct Global *this)
{
  int i;

  for (i = 0; i < GLOBAL_HASHSIZE; ++i)
    {
      struct Symbol *v;

      for (v = this->table[i]; v; v = v->next)
        {
          if (v->type == GLOBALVAR || v->type == GLOBALARRAY)
            {
              Var_clear(&(v->u.var));
            }
        }
    }
}

void Global_clearFunctions(struct Global *this)
{
  int i;

  for (i = 0; i < GLOBAL_HASHSIZE; ++i)
    {
      struct Symbol **v = &this->table[i], *w;
      struct Symbol *sym;

      while (*v)
        {
          sym = *v;
          w = sym->next;
          if (sym->type == USERFUNCTION)
            {
              if (sym->u.sub.u.def.localTypes)
                {
                  free(sym->u.sub.u.def.localTypes);
                }

              if (sym->u.sub.argTypes)
                {
                  free(sym->u.sub.argTypes);
                }

              free(sym->name);
              free(sym);
              *v = w;
            }
          else
            {
              v = &sym->next;
            }
        }
    }
}

void Global_destroy(struct Global *this)
{
  int i;

  for (i = 0; i < GLOBAL_HASHSIZE; ++i)
    {
      struct Symbol *v = this->table[i], *w;
      struct Symbol *sym;

      while (v)
        {
          sym = v;
          w = v->next;
          switch (sym->type)
            {
            case GLOBALVAR:
            case GLOBALARRAY:
              Var_destroy(&(sym->u.var));
              break;

            case USERFUNCTION:
              {
                if (sym->u.sub.u.def.localTypes)
                  {
                    free(sym->u.sub.u.def.localTypes);
                  }

                if (sym->u.sub.argTypes)
                  {
                    free(sym->u.sub.argTypes);
                  }

                break;
              }

            case BUILTINFUNCTION:
              {
                if (sym->u.sub.argTypes)
                  {
                    free(sym->u.sub.argTypes);
                  }

                if (sym->u.sub.u.bltin.next)
                  {
                    sym = sym->u.sub.u.bltin.next;
                    while (sym)
                      {
                        struct Symbol *n;

                        if (sym->u.sub.argTypes)
                          {
                            free(sym->u.sub.argTypes);
                          }

                        n = sym->u.sub.u.bltin.next;
                        free(sym);
                        sym = n;
                      }
                  }

                break;
              }

            default:
              assert(0);
            }

          free(v->name);
          free(v);
          v = w;
        }

      this->table[i] = (struct Symbol *)0;
    }
}
