/****************************************************************************
 * apps/interpreters/quickjs/qjsmini.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <malloc.h>

#include <quickjs.h>
#include <cutils.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MALLOC_OVERHEAD 8

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct trace_malloc_data
{
  uint8_t *base;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: help
 ****************************************************************************/

static void qjs_help(void)
{
  printf("QuickJS version " CONFIG_VERSION "\n"
         "usage: "
         "qjs"
         " [options] [file [args]]\n"
         "-h  --help         list options\n"
         "-e  --eval EXPR    evaluate EXPR\n"
         "-T  --trace        trace memory allocation\n"
         "-d  --dump         dump the memory usage stats\n"
         "    --memory-limit n       limit the memory usage to 'n' bytes\n"
         "    --stack-size n         limit the stack size to 'n' bytes\n"
         "    --unhandled-rejection  dump unhandled promise rejections\n"
         "-q  --quit         just instantiate the interpreter and quit\n");
  exit(1);
}

/****************************************************************************
 * Name: js_trace_malloc_ptr_offset
 ****************************************************************************/

static inline unsigned long long
js_trace_malloc_ptr_offset(uint8_t *ptr,
                           struct trace_malloc_data *dp)
{
  return ptr - dp->base;
}

/****************************************************************************
 * Name: js_trace_malloc_usable_size
 ****************************************************************************/

static inline size_t js_trace_malloc_usable_size(void *ptr)
{
  return malloc_usable_size(ptr);
}

/****************************************************************************
 * Name: js_trace_malloc_printf
 ****************************************************************************/

static void __attribute__((format(printf, 2, 3)))
js_trace_malloc_printf(JSMallocState *s, const char *fmt, ...)
{
  va_list ap;
  int c;

  va_start(ap, fmt);
  while ((c = *fmt++) != '\0')
    {
      if (c == '%')
        {
          /* only handle %p and %zd */

          if (*fmt == 'p')
            {
              uint8_t *ptr = va_arg(ap, void *);
              if (ptr == NULL)
                {
                  printf("NULL");
                }
              else
                {
                  printf("H%+06lld.%zd",
                        js_trace_malloc_ptr_offset(ptr, s->opaque),
                        js_trace_malloc_usable_size(ptr));
                }

              fmt++;
              continue;
            }

          if (fmt[0] == 'z' && fmt[1] == 'd')
            {
              size_t sz = va_arg(ap, size_t);
              printf("%zd", sz);
              fmt += 2;
              continue;
            }
        }

      putc(c, stdout);
    }

  va_end(ap);
}

/****************************************************************************
 * Name: js_trace_malloc_init
 ****************************************************************************/

static void js_trace_malloc_init(struct trace_malloc_data *s)
{
  free(s->base = malloc(8));
}

/****************************************************************************
 * Name: js_trace_malloc
 ****************************************************************************/

static void *js_trace_malloc(JSMallocState *s, size_t size)
{
  void *ptr;

  /* Do not allocate zero bytes: behavior is platform dependent */

  assert(size != 0);

  if (unlikely(s->malloc_size + size > s->malloc_limit))
      return NULL;
  ptr = malloc(size);
  js_trace_malloc_printf(s, "A %zd -> %p\n", size, ptr);
  if (ptr)
    {
      s->malloc_count++;
      s->malloc_size += js_trace_malloc_usable_size(ptr) + MALLOC_OVERHEAD;
    }

  return ptr;
}

/****************************************************************************
 * Name: js_trace_free
 ****************************************************************************/

static void js_trace_free(JSMallocState *s, void *ptr)
{
  if (!ptr)
    return;

  js_trace_malloc_printf(s, "F %p\n", ptr);
  s->malloc_count--;
  s->malloc_size -= js_trace_malloc_usable_size(ptr) + MALLOC_OVERHEAD;
  free(ptr);
}

/****************************************************************************
 * Name: js_trace_realloc
 ****************************************************************************/

static void *js_trace_realloc(JSMallocState *s, void *ptr, size_t size)
{
  size_t old_size;

  if (!ptr)
    {
      if (size == 0)
          return NULL;
      return js_trace_malloc(s, size);
    }

  old_size = js_trace_malloc_usable_size(ptr);
  if (size == 0)
    {
      js_trace_malloc_printf(s, "R %zd %p\n", size, ptr);
      s->malloc_count--;
      s->malloc_size -= old_size + MALLOC_OVERHEAD;
      free(ptr);
      return NULL;
    }

  if (s->malloc_size + size - old_size > s->malloc_limit)
      return NULL;

  js_trace_malloc_printf(s, "R %zd %p", size, ptr);

  ptr = realloc(ptr, size);
  js_trace_malloc_printf(s, " -> %p\n", ptr);
  if (ptr)
    {
      s->malloc_size += js_trace_malloc_usable_size(ptr) - old_size;
    }

  return ptr;
}

/****************************************************************************
 * Name: js_dump_obj
 ****************************************************************************/

static void js_dump_obj(JSContext *ctx, FILE *f, JSValueConst val)
{
  const char *str;

  str = JS_ToCString(ctx, val);
  if (str)
    {
      fprintf(f, "%s\n", str);
      JS_FreeCString(ctx, str);
    }
  else
    {
      fprintf(f, "[exception]\n");
    }
}

/****************************************************************************
 * Name: js_std_dump_error1
 ****************************************************************************/

static void js_std_dump_error1(JSContext *ctx, JSValueConst exception_val)
{
  JSValue val;
  BOOL is_error;

  is_error = JS_IsError(ctx, exception_val);
  js_dump_obj(ctx, stderr, exception_val);
  if (is_error)
    {
      val = JS_GetPropertyStr(ctx, exception_val, "stack");
      if (!JS_IsUndefined(val))
        {
          js_dump_obj(ctx, stderr, val);
        }

      JS_FreeValue(ctx, val);
    }
}

/****************************************************************************
 * Name: js_std_dump_error
 ****************************************************************************/

static void js_std_dump_error(JSContext *ctx)
{
  JSValue exception_val;

  exception_val = JS_GetException(ctx);
  js_std_dump_error1(ctx, exception_val);
  JS_FreeValue(ctx, exception_val);
}

/****************************************************************************
 * Name: js_eval_buf
 ****************************************************************************/

static int js_eval_buf(JSContext *ctx, const void *buf, int buf_len,
                       const char *filename, int eval_flags)
{
  JSValue val;
  int ret;

  if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE)
    {
      val = JS_Eval(ctx, buf, buf_len, filename,
                    eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
      if (!JS_IsException(val))
        {
          val = JS_EvalFunction(ctx, val);
        }
    }
  else
    {
      val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
    }

  if (JS_IsException(val))
    {
      js_std_dump_error(ctx);
      ret = -1;
    }
  else
    {
      ret = 0;
    }

  JS_FreeValue(ctx, val);
  return ret;
}

/****************************************************************************
 * Name: js_load_file
 ****************************************************************************/

uint8_t *js_load_file(JSContext *ctx, size_t *pbuf_len, const char *filename)
{
  FILE *f;
  uint8_t *buf;
  size_t buf_len;
  long lret;

  f = fopen(filename, "rb");
  if (!f)
      return NULL;
  if (fseek(f, 0, SEEK_END) < 0)
      goto fail;
  lret = ftell(f);
  if (lret < 0)
      goto fail;

  if (lret == LONG_MAX)
    {
      errno = EISDIR;
      goto fail;
    }

  buf_len = lret;
  if (fseek(f, 0, SEEK_SET) < 0)
      goto fail;
  if (ctx)
      buf = js_malloc(ctx, buf_len + 1);
  else
      buf = malloc(buf_len + 1);
  if (!buf)
      goto fail;
  if (fread(buf, 1, buf_len, f) != buf_len)
    {
      errno = EIO;
      if (ctx)
        js_free(ctx, buf);
      else
        free(buf);
    fail:
      fclose(f);
      return NULL;
    }

  buf[buf_len] = '\0';
  fclose(f);
  *pbuf_len = buf_len;
  return buf;
}

/****************************************************************************
 * Name: js_eval_file
 ****************************************************************************/

static int js_eval_file(JSContext *ctx, const char *filename, int module)
{
  uint8_t *buf;
  int ret;
  int eval_flags;
  size_t buf_len;

  buf = js_load_file(ctx, &buf_len, filename);
  if (!buf)
    {
      perror(filename);
      exit(1);
    }

  if (module < 0)
    {
      module = (has_suffix(filename, ".mjs") ||
                JS_DetectModule((const char *)buf, buf_len));
    }

  if (module)
    eval_flags = JS_EVAL_TYPE_MODULE;
  else
    eval_flags = JS_EVAL_TYPE_GLOBAL;
  ret = js_eval_buf(ctx, buf, buf_len, filename, eval_flags);
  js_free(ctx, buf);
  return ret;
}

static JSValue js_print(JSContext *ctx, JSValueConst this_val,
                        int argc, JSValueConst *argv)
{
  int i;
  const char *str;
  size_t len;

  for (i = 0; i < argc; i++)
    {
      if (i != 0)
        putchar(' ');
      str = JS_ToCStringLen(ctx, &len, argv[i]);
      if (!str)
          return JS_EXCEPTION;
      fwrite(str, 1, len, stdout);
      JS_FreeCString(ctx, str);
    }

  putchar('\n');
  return JS_UNDEFINED;
}

void js_std_add_helpers(JSContext *ctx)
{
  JSValue global_obj, console;
  global_obj = JS_GetGlobalObject(ctx);

  console = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, console, "log",
                    JS_NewCFunction(ctx, js_print, "log", 1));
  JS_SetPropertyStr(ctx, global_obj, "console", console);

  JS_FreeValue(ctx, global_obj);
}

/****************************************************************************
 * Public Functions Prototypes
 ****************************************************************************/

int js_ext_init(JSContext *ctx);
int js_ext_destroy(JSContext *ctx);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * qjs_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  JSRuntime *rt = NULL;
  JSContext *ctx = NULL;
  struct trace_malloc_data trace_data =
  {
    NULL
  };

  static const JSMallocFunctions trace_mf =
  {
      js_trace_malloc,
      js_trace_free,
      js_trace_realloc,
      malloc_usable_size,
  };

  int argi;
  char *expr = NULL;
  int dump_memory = 0;
  int trace_memory = 0;
  int empty_run = 0;
  size_t memory_limit = 0;
  size_t stack_size = 0;

  argi = 1;
  while (argi < argc && *argv[argi] == '-')
    {
      char *arg = argv[argi] + 1;
      const char *longopt = "";

      if (!*arg)
        break;
      argi++;
      if (*arg == '-')
        {
          longopt = arg + 1;
          arg += strlen(arg);
          if (!*longopt)
            break;
        }

      for (; *arg || *longopt; longopt = "")
        {
          char opt = *arg;
          if (opt)
            {
              arg++;
            }

          if (opt == 'h' || opt == '?' || !strcmp(longopt, "help"))
            {
              qjs_help();
              continue;
            }

          if (opt == 'e' || !strcmp(longopt, "eval"))
            {
              if (*arg)
                {
                  expr = arg;
                  break;
                }

              if (argi < argc)
                {
                  expr = argv[argi++];
                  break;
                }

              fprintf(stderr, "qjs: missing expression for -e\n");
              exit(2);
            }

          if (opt == 'd' || !strcmp(longopt, "dump"))
            {
              dump_memory++;
              continue;
            }

          if (opt == 'T' || !strcmp(longopt, "trace"))
            {
              trace_memory++;
              continue;
            }

          if (opt == 'q' || !strcmp(longopt, "quit"))
            {
              empty_run++;
              continue;
            }

          if (!strcmp(longopt, "memory-limit"))
            {
              if (argi >= argc)
                {
                  fprintf(stderr, "expecting memory limit");
                  exit(1);
                }

              memory_limit = (size_t)strtod(argv[argi++], NULL);
              continue;
            }

          if (!strcmp(longopt, "stack-size"))
            {
              if (argi >= argc)
                {
                  fprintf(stderr, "expecting stack size");
                  exit(1);
                }

              stack_size = (size_t)strtod(argv[argi++], NULL);
              continue;
            }

          if (opt)
            {
              fprintf(stderr, "qjs: unknown option '-%c'\n", opt);
            }
          else
            {
              fprintf(stderr, "qjs: unknown option '--%s'\n", longopt);
            }

          qjs_help();
        }
    }

  if (trace_memory)
    {
      js_trace_malloc_init(&trace_data);
      rt = JS_NewRuntime2(&trace_mf, &trace_data);
    }
  else
    {
      rt = JS_NewRuntime();
    }

  if (!rt)
    {
      fprintf(stderr, "qjs: cannot allocate JS runtime\n");
      goto fail;
    }

  if (memory_limit != 0)
    JS_SetMemoryLimit(rt, memory_limit);
  if (stack_size != 0)
    JS_SetMaxStackSize(rt, stack_size);

  ctx = JS_NewContext(rt);

  if (!ctx)
    {
      fprintf(stderr, "qjs: cannot allocate JS context\n");
      goto fail;
    }

  js_std_add_helpers(ctx);

#ifdef CONFIG_INTERPRETERS_QUICKJS_EXT_HOOK
  if (OK != js_ext_init(ctx))
    {
      fprintf(stderr, "qjs: external context init failed\n");
      goto fail;
    }
#endif

  if (!empty_run)
    {
      if (expr)
        {
          if (js_eval_buf(ctx, expr, strlen(expr), "<cmdline>", 0))
            {
              goto fail;
            }
        }
      else if (argi < argc)
        {
          const char *filename;
          filename = argv[argi];
          if (js_eval_file(ctx, filename, 1))
            {
              goto fail;
            }
        }
    }

  if (dump_memory)
    {
      JSMemoryUsage stats;
      JS_ComputeMemoryUsage(rt, &stats);
      JS_DumpMemoryUsage(stdout, &stats, rt);
    }

fail:
  if (ctx)
    {
#ifdef CONFIG_INTERPRETERS_QUICKJS_EXT_HOOK
      js_ext_destroy(ctx);
#endif
      JS_FreeContext(ctx);
    }

  if (rt)
    JS_FreeRuntime(rt);

  return 0;
}
