/****************************************************************************
 * interpreters/micropython/py_readline.c
 *
 * This file was part of the Micro Python project, http://micropython.org/
 * and has been integrated into Nuttx by Dave Marples (dave@marples.net)
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "py/nlr.h"
#include "py/parsehelper.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/pfenv.h"

#include "py_readline.h"
#include "pyexec.h"
#include "genhdr/py-version.h"

#define EXEC_FLAG_PRINT_EOF (1)
#define EXEC_FLAG_ALLOW_DEBUGGING (2)
#define EXEC_FLAG_IS_REPL (4)

/****************************************************************************
 * Public Data
 ****************************************************************************/

pyexec_mode_kind_t pyexec_mode_kind = PYEXEC_MODE_FRIENDLY_REPL;
STATIC bool repl_display_debugging_info = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* parses, compiles and executes the code in the lexer
 * frees the lexer before returning
 * EXEC_FLAG_PRINT_EOF prints 2 EOF chars: 1 after normal output, 1 after
 *   exception output
 * EXEC_FLAG_ALLOW_DEBUGGING allows debugging info to be printed after
 *   executing the code
 * EXEC_FLAG_IS_REPL is used for REPL inputs (flag passed on to mp_compile)
 */

STATIC int parse_compile_execute(mp_lexer_t * lex,
                                 mp_parse_input_kind_t input_kind,
                                 int exec_flags)
{
  int ret = 0;

  mp_parse_error_kind_t parse_error_kind;
  mp_parse_node_t pn = mp_parse(lex, input_kind, &parse_error_kind);
  qstr source_name = lex->source_name;

  /* check for parse error */

  if (pn == MP_PARSE_NODE_NULL)
    {
      if (exec_flags & EXEC_FLAG_PRINT_EOF)
        {
          fprintf(stdout, "\x04");
        }

      mp_parse_show_exception(lex, parse_error_kind);
      mp_lexer_free(lex);
      goto finish;
    }

  mp_lexer_free(lex);

  mp_obj_t module_fun =
    mp_compile(pn, source_name, MP_EMIT_OPT_NONE,
               exec_flags & EXEC_FLAG_IS_REPL);

  /* check for compile error */

  if (mp_obj_is_exception_instance(module_fun))
    {
      if (exec_flags & EXEC_FLAG_PRINT_EOF)
        {
          fprintf(stdout, "\x04");
        }

      mp_obj_print_exception(printf_wrapper, NULL, module_fun);
      goto finish;
    }

  /* execute code */

  nlr_buf_t nlr;

  struct timespec start;
  clock_gettime(CLOCK_REALTIME, &start);

  if (nlr_push(&nlr) == 0)
    {
      // mp_hal_set_interrupt_char(CHAR_CTRL_C); /* allow ctrl-C to interrupt us */
      mp_call_function_0(module_fun);
      // mp_hal_set_interrupt_char(-1); /* disable interrupt */

      nlr_pop();
      ret = 1;
      if (exec_flags & EXEC_FLAG_PRINT_EOF)
        {
          fprintf(stdout, "\x04");
        }
    }
  else
    {
      /* uncaught exception */

      // mp_hal_set_interrupt_char(-1); /* disable interrupt */

      /* print EOF after normal output */

      if (exec_flags & EXEC_FLAG_PRINT_EOF)
        {
          fprintf(stdout, "\x04");
        }

      /* check for SystemExit */

      if (mp_obj_is_subclass_fast
          (mp_obj_get_type((mp_obj_t) nlr.ret_val), &mp_type_SystemExit))
        {
          /* at the moment, the value of SystemExit is unused */

          ret = PYEXEC_FORCED_EXIT;
        }
      else
        {
          mp_obj_print_exception(printf_wrapper, NULL, (mp_obj_t) nlr.ret_val);
          ret = 0;
        }
    }

  /* display debugging info if wanted */

  if ((exec_flags & EXEC_FLAG_ALLOW_DEBUGGING) && repl_display_debugging_info)
    {
      struct timespec endTime;
      clock_gettime(CLOCK_REALTIME, &endTime);

      mp_uint_t ticks = ((endTime.tv_sec - start.tv_sec) * 1000) +
        ((((endTime.tv_nsec / 1000000) + 1000) -
          (start.tv_nsec / 1000000)) % 1000);

      printf("took " UINT_FMT " ms\n", ticks);

      /* qstr info */

      {
        mp_uint_t n_pool, n_qstr, n_str_data_bytes, n_total_bytes;
        qstr_pool_info(&n_pool, &n_qstr, &n_str_data_bytes, &n_total_bytes);
        printf("qstr:\n  n_pool=" UINT_FMT "\n  n_qstr=" UINT_FMT
               "\n  n_str_data_bytes=" UINT_FMT "\n  n_total_bytes=" UINT_FMT
               "\n", n_pool, n_qstr, n_str_data_bytes, n_total_bytes);
      }
    }

finish:
  if (exec_flags & EXEC_FLAG_PRINT_EOF)
    {
      fprintf(stdout, "\x04");
    }

  return ret;
}

int pyexec_raw_repl(void)
{
  vstr_t line;
  vstr_init(&line, 32);

raw_repl_reset:
  fprintf(stdout, "raw REPL; CTRL-B to exit\r\n");

  for (;;)
    {
      vstr_reset(&line);
      fputc('>', stdout);
      fflush(stdout);

      for (;;)
        {
          char c = getc(stdin);
          if (c == CHAR_CTRL_A)
            {
              /* reset raw REPL */

              goto raw_repl_reset;
            }
          else if (c == CHAR_CTRL_B)
            {
              /* change to friendly REPL */

              fprintf(stdout, "\r\n");
              vstr_clear(&line);
              pyexec_mode_kind = PYEXEC_MODE_FRIENDLY_REPL;
              return 0;
            }
          else if (c == CHAR_CTRL_C)
            {
              /* clear line */

              vstr_reset(&line);
            }
          else if (c == CHAR_CTRL_D)
            {
              /* input finished */

              break;
            }
          else if (c <= 127)
            {
              /* let through any other ASCII character */

              vstr_add_char(&line, c);
              fputc(c, stdout);
            }
          fflush(stdout);
        }

      /* indicate reception of command */

      fprintf(stdout, "OK");

      if (line.len == 0)
        {
          /* exit for a soft reset */

          fprintf(stdout, "\r\n");
          vstr_clear(&line);
          return PYEXEC_FORCED_EXIT;
        }

      mp_lexer_t *lex =
        mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, line.buf, line.len, 0);
      if (lex == NULL)
        {
          printf("\x04MemoryError\n\x04");
        }
      else
        {
          int ret =
            parse_compile_execute(lex, MP_PARSE_FILE_INPUT,
                                  EXEC_FLAG_PRINT_EOF);
          if (ret & PYEXEC_FORCED_EXIT)
            {
              return ret;
            }
        }
      fflush(stdout);
    }
}

int pyexec_friendly_repl(void)
{
  vstr_t line;
  vstr_init(&line, 32);

friendly_repl_reset:
  fprintf(stdout,
          "Micro Python " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE
          "; NuttX with " CONFIG_ARCH_FAMILY " " CONFIG_ARCH_CHIP "\r\n");
  fprintf(stdout, "Type \"help()\" for more information.\r\n");

  for (;;)
    {
    input_restart:
      vstr_reset(&line);
      int ret = py_readline(&line, ">>> ");

      if (ret == CHAR_CTRL_A)
        {
          /* change to raw REPL */

          fprintf(stdout, "\r\n");
          vstr_clear(&line);
          pyexec_mode_kind = PYEXEC_MODE_RAW_REPL;
          return 0;
        }
      else if (ret == CHAR_CTRL_B)
        {
          /* reset friendly REPL */

          fprintf(stdout, "\r\n");
          goto friendly_repl_reset;
        }
      else if (ret == CHAR_CTRL_C)
        {
          /* break */

          fprintf(stdout, "\r\n");
          continue;
        }
      else if (ret == CHAR_CTRL_D)
        {
          /* exit for a soft reset */

          fprintf(stdout, "\r\n");
          vstr_clear(&line);
          return PYEXEC_FORCED_EXIT;
        }
      else if (vstr_len(&line) == 0)
        {
          continue;
        }

      while (mp_repl_continue_with_input(vstr_str(&line)))
        {
          vstr_add_char(&line, '\n');
          int ret = py_readline(&line, "... ");
          if (ret == CHAR_CTRL_C)
            {
              /* cancel everything */

              fprintf(stdout, "\r\n");
              goto input_restart;
            }
          else if (ret == CHAR_CTRL_D)
            {
              /* stop entering compound statement */

              break;
            }
        }

      mp_lexer_t *lex =
        mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, vstr_str(&line),
                                  vstr_len(&line), 0);
      if (lex == NULL)
        {
          printf("MemoryError\n");
        }
      else
        {
          int ret =
            parse_compile_execute(lex, MP_PARSE_SINGLE_INPUT,
                                  EXEC_FLAG_ALLOW_DEBUGGING |
                                  EXEC_FLAG_IS_REPL);
          if (ret & PYEXEC_FORCED_EXIT)
            {
              return ret;
            }
        }
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

int pyexec_file(const char *filename)
{
  mp_lexer_t *lex = mp_lexer_new_from_file(filename);

  if (lex == NULL)
    {
      printf("could not open file '%s' for reading\n", filename);
      return false;
    }

  return parse_compile_execute(lex, MP_PARSE_FILE_INPUT, 0);
}

mp_obj_t pyb_set_repl_info(mp_obj_t o_value)
{
  repl_display_debugging_info = mp_obj_get_int(o_value);
  return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(pyb_set_repl_info_obj, pyb_set_repl_info);
