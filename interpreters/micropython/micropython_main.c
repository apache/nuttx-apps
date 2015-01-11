/****************************************************************************
 * interpreters/micropython/micropython_main.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
 *            Dave Marples <dave@marples.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <debug.h>
#include <stdio.h>
#include <math.h>

#include "mpconfig.h"
#include "nlr.h"
#include "misc.h"
#include "qstr.h"
#include "lexer.h"
#include "parse.h"
#include "obj.h"
#include "parsehelper.h"
#include "compile.h"
#include "runtime0.h"
#include "runtime.h"
#include "repl.h"
#include "pfenv.h"
#include "pyexec.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FORCE_EVAL(x) do {                        \
        if (sizeof(x) == sizeof(float)) {         \
                volatile float __x;               \
                __x = (x);                        \
                (void)__x;                        \
        } else if (sizeof(x) == sizeof(double)) { \
                volatile double __x;              \
                __x = (x);                        \
                (void)__x;                        \
        } else {                                  \
                volatile long double __x;         \
                __x = (x);                        \
                (void)__x;                        \
        }                                         \
} while(0);

/****************************************************************************
 * Private Data
****************************************************************************/

/****************************************************************************
 * Private Function
****************************************************************************/

void do_str(FAR const char *src)
{
  FAR mp_lexer_t *lex =
    mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
  if (lex == NULL)
    {
      return;
    }

  mp_parse_error_kind_t parse_error_kind;
  mp_parse_node_t pn = mp_parse(lex, MP_PARSE_SINGLE_INPUT, &parse_error_kind);

  if (pn == MP_PARSE_NODE_NULL)
    {
      /* parse error */

      mp_parse_show_exception(lex, parse_error_kind);
      mp_lexer_free(lex);
      return;
    }

  /* parse okay */

  qstr source_name = lex->source_name;
  mp_lexer_free(lex);
  mp_obj_t module_fun = mp_compile(pn, source_name, MP_EMIT_OPT_NONE, true);

  if (mp_obj_is_exception_instance(module_fun))
    {
      /* compile error */

      mp_obj_print_exception(printf_wrapper, NULL, module_fun);
      return;
    }

  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0)
    {
      mp_call_function_0(module_fun);
      nlr_pop();
    }
  else
    {
      /* uncaught exception */

      mp_obj_print_exception(printf_wrapper, NULL, (mp_obj_t) nlr.ret_val);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

float nanf(FAR const char *tagp)
{
  (void)tagp;
  return 0;
}

float copysignf(float x, float y)
{
  if (y < 0)
    {
      return -fabsf(x);
    }

  return fabsf(x);
}

float truncf(float x)
{
  union
    {
      float f;
      uint32_t i;
    } u =
  {
  x};
  int e = (int)(u.i >> 23 & 0xff) - 0x7f + 9;
  uint32_t m;

  if (e >= 23 + 9)
    {
      return x;
    }

  if (e < 9)
    {
      e = 1;
    }

  m = -1U >> e;
  if ((u.i & m) == 0)
    {
      return x;
    }

  FORCE_EVAL(x + 0x1p120f);
  u.i &= ~m;
  return u.f;
}

/****************************************************************************
 * mp_import_stat
 ****************************************************************************/

mp_import_stat_t mp_import_stat(FAR const char *path)
{
  return MP_IMPORT_STAT_NO_EXIST;
}

/****************************************************************************
 * mp_lexer_new_from_file
 ****************************************************************************/

mp_lexer_t *mp_lexer_new_from_file(FAR const char *filename)
{
  return NULL;
}

/****************************************************************************
 * mp_builtin_open
 ****************************************************************************/

mp_obj_t mp_builtin_open(uint n_args, const mp_obj_t * args, mp_map_t * kwargs)
{
  return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

/****************************************************************************
 * nlr_jump_fail
 ****************************************************************************/

void nlr_jump_fail(void *val)
{
  fprintf(stderr, "FATAL: uncaught exception %p\n", val);
  exit(-1);
}

/****************************************************************************
 * micropython_main
 ****************************************************************************/

#ifdef CONFIG_INTERPRETERS_MICROPYTHON
int micropython_main(int argc, char *argv[])
{
  mp_init();
  for (;;)
    {
      if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL)
        {
          if (pyexec_raw_repl() != 0)
            {
              break;
            }
        }
      else
        {
          if (pyexec_friendly_repl() != 0)
            {
              break;
            }
        }
    }

  mp_deinit();
  return 0;
}
#endif
