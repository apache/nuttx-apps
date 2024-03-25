/****************************************************************************
 * apps/interpreters/wamr/wamr_custom_init.c
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>
#include <sys/param.h>
#include <sys/types.h>
#include <iconv.h>

#include "wasm_export.h"
#include "wasm_native.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_aligned_memory_map_sem = SEM_INITIALIZER(1);
static uintptr_t g_aligned_memory_map[CONFIG_INTERPRETERS_WAMR_LIBC_NUTTX_POSIXMEMALIGN_MAP_SIZE][2] = {{0, 0}};

static bool
add_to_aligned_map(uintptr_t mapped, uintptr_t raw)
{
  int i;
  bool ret = false;

  sem_wait(&g_aligned_memory_map_sem);

  for (i = 0; i < CONFIG_INTERPRETERS_WAMR_LIBC_NUTTX_POSIXMEMALIGN_MAP_SIZE; i++)
    {
      DEBUGASSERT(g_aligned_memory_map[i][0] != mapped);

      if (g_aligned_memory_map[i][0] == 0 && g_aligned_memory_map[i][1] == 0)
      {
        g_aligned_memory_map[i][0] = mapped;
        g_aligned_memory_map[i][1] = raw;
        ret = true;
        break;
      }
    }

  sem_post(&g_aligned_memory_map_sem);
  return ret;
}

static uintptr_t
remove_from_aligned_map(uintptr_t mapped)
{
  int i;
  uintptr_t ret = 0;

  if (mapped == 0)
    {
      return ret;
    }

  sem_wait(&g_aligned_memory_map_sem);

  for (i = 0; i < CONFIG_INTERPRETERS_WAMR_LIBC_NUTTX_POSIXMEMALIGN_MAP_SIZE; i++)
    {
      if (g_aligned_memory_map[i][0] == mapped)
        {
          g_aligned_memory_map[i][0] = 0;
          ret = g_aligned_memory_map[i][1];
          g_aligned_memory_map[i][1] = 0;
          break;
        }
    }

  sem_post(&g_aligned_memory_map_sem);
  return ret;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void
va_list_string2conv(wasm_exec_env_t exec_env, const char *format,
                      va_list ap, bool to_native)
{
  wasm_module_inst_t module_inst = get_module_inst(exec_env);
  char *pos = *((char **)&ap);

  if (pos == NULL)
    {
      return;
    }

  int long_ctr = 0;
  int might = 0;

  while (*format)
    {
      if (!might)
        {
          if (*format == '%')
            {
              might = 1;
              long_ctr = 0;
            }
        }
      else
        {
          switch (*format)
            {
              case '.':
              case '+':
              case '-':
              case ' ':
              case '#':
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                  goto still_might;

              case 't':
              case 'z':
                  long_ctr = 1;
                  goto still_might;

              case 'j':
                  long_ctr = 2;
                  goto still_might;

              case 'l':
                  long_ctr++;
              case 'h':
                  goto still_might;

              case 'o':
              case 'd':
              case 'i':
              case 'u':
              case 'p':
              case 'x':
              case 'X':
              case 'c':
                {
                  if (long_ctr < 2)
                    {
                      pos += sizeof(int32_t);
                    }
                  else
                    {
                      pos += sizeof(int64_t);
                    }
                  break;
                }

              case 'e':
              case 'E':
              case 'g':
              case 'G':
              case 'f':
              case 'F':
                {
                  pos += sizeof(double);
                  break;
                }

              case 's':
                {
                  if (to_native)
                    {
                      *(uintptr_t *)pos =
                          (uintptr_t)addr_app_to_native(*(uintptr_t *)pos);
                    }
                  else
                    {
                      *(uintptr_t *)pos =
                          (uintptr_t)addr_native_to_app(*(uintptr_t *)pos);
                    }
                  pos += sizeof(uintptr_t);
                  break;
                }

              default:
                  break;
          }

        might = 0;
      }

  still_might:
      ++format;
  }
}


#define va_list_string2native(exec_env, format, ap) \
  va_list_string2conv(exec_env, format, ap, true)

#define va_list_string2app(exec_env, format, ap) \
  va_list_string2conv(exec_env, format, ap, false)

static void
scanf_begin(wasm_module_inst_t module_inst, va_list ap)
{
  uintptr_t *apv = *(uintptr_t **)&ap;
  if (apv == NULL)
    {
      return;
    }
  while (*apv != 0)
    {
      *apv = (uintptr_t)addr_app_to_native(*apv);
      apv++;
    }
}

static void
scanf_end(wasm_module_inst_t module_inst, va_list ap)
{
  uintptr_t *apv = *(uintptr_t **)&ap;
  if (apv == NULL)
    {
      return;
    }
  while (*apv != 0)
    {
      *apv = (uintptr_t)addr_native_to_app((void *)*apv);
      apv++;
    }
}

static pthread_mutex_t g_compare_mutex = PTHREAD_MUTEX_INITIALIZER;
static wasm_exec_env_t g_compare_env;
static void           *g_compare_func;

static int
compare_proxy(const void *a, const void *b)
{
  wasm_module_inst_t module_inst = get_module_inst(g_compare_env);
  uint32_t argv[2];

  argv[0] = addr_native_to_app((void *)a);
  argv[1] = addr_native_to_app((void *)b);

  return wasm_runtime_call_indirect(g_compare_env,
           (uint32_t)addr_native_to_app(g_compare_func), 2, argv) ?
             argv[0] : 0;
}

#ifndef GLUE_FUNCTION_qsort
#define GLUE_FUNCTION_qsort
void glue_qsort(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2,
                uintptr_t parm3, uintptr_t parm4)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  pthread_mutex_lock(&g_compare_mutex);
  g_compare_env = env;
  g_compare_func = parm4;
  qsort((FAR void *)parm1, (size_t)parm2,
        (size_t)parm3, compare_proxy);
  pthread_mutex_unlock(&g_compare_mutex);
}

#endif /* GLUE_FUNCTION_qsort */

#ifndef GLUE_FUNCTION_bsearch
#define GLUE_FUNCTION_bsearch
uintptr_t glue_bsearch(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2,
                       uintptr_t parm3, uintptr_t parm4, uintptr_t parm5)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  uintptr_t ret;

  pthread_mutex_lock(&g_compare_mutex);
  g_compare_env = env;
  g_compare_func = parm5;
  ret = bsearch((FAR const void *)parm1,
                (FAR const void *)parm2,
                (size_t)parm3, (size_t)parm4, compare_proxy);
  pthread_mutex_unlock(&g_compare_mutex);
  ret = addr_native_to_app((void *)ret);
  return ret;
}

#endif /* GLUE_FUNCTION_bsearch */

static void glue_msghdr_begin(wasm_module_inst_t module_inst,
                              FAR struct msghdr *hdr)
{
  int i;

  hdr->msg_iov = addr_app_to_native((uintptr_t)hdr->msg_iov);

  for (i = 0; i < hdr->msg_iovlen; i++)
    {
      hdr->msg_iov[i].iov_base =
        addr_app_to_native((uintptr_t)hdr->msg_iov[i].iov_base);
    }

  if (hdr->msg_name != NULL && hdr->msg_namelen > 0)
    {
      hdr->msg_name = addr_app_to_native(hdr->msg_name);
    }

  if (hdr->msg_control != NULL && hdr->msg_controllen > 0)
    {
      hdr->msg_control = addr_app_to_native((uintptr_t)hdr->msg_control);
    }
}

static void glue_msghdr_end(wasm_module_inst_t module_inst,
                            FAR struct msghdr *hdr)
{
  int i;

  for (i = 0; i < hdr->msg_iovlen; i++)
    {
      hdr->msg_iov[i].iov_base =
        addr_native_to_app((uintptr_t)hdr->msg_iov[i].iov_base);
    }

  hdr->msg_iov = addr_native_to_app((uintptr_t)hdr->msg_iov);

  if (hdr->msg_name != NULL && hdr->msg_namelen > 0)
    {
      hdr->msg_name = addr_native_to_app(hdr->msg_name);
    }

  if (hdr->msg_control != NULL && hdr->msg_controllen > 0)
    {
      hdr->msg_control = addr_native_to_app((uintptr_t)hdr->msg_control);
    }
}

#ifndef GLUE_FUNCTION_sendmsg
#define GLUE_FUNCTION_sendmsg
uintptr_t glue_sendmsg(wasm_exec_env_t env, uintptr_t parm1,
                       uintptr_t parm2, uintptr_t parm3)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  FAR struct msghdr *hdr =
    (FAR struct msghdr *)(uintptr_t)parm2;

  glue_msghdr_begin(module_inst, hdr);
  uintptr_t ret = sendmsg((int)parm1,
                          (FAR struct msghdr *)(parm2),
                          (int)parm3);
  glue_msghdr_end(module_inst, hdr);

  return ret;
}

#endif /* GLUE_FUNCTION_sendmsg */

#ifndef GLUE_FUNCTION_recvmsg
#define GLUE_FUNCTION_recvmsg
uintptr_t glue_recvmsg(wasm_exec_env_t env, uintptr_t parm1,
                       uintptr_t parm2, uintptr_t parm3)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  FAR struct msghdr *hdr =
    (FAR struct msghdr *)((uintptr_t)parm2);

  glue_msghdr_begin(module_inst, hdr);
  uintptr_t ret = recvmsg((int)parm1,
                          (FAR struct msghdr *)(parm2),
                          (int)parm3);
  glue_msghdr_end(module_inst, hdr);

  return ret;
}

#endif /* GLUE_FUNCTION_recvmsg */

#ifndef GLUE_FUNCTION_strsep
#define GLUE_FUNCTION_strsep
uintptr_t glue_strsep(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  FAR char **stringp = parm1;

  if (*stringp != NULL)
    {
      *stringp = addr_app_to_native(*stringp);
    }

  return addr_native_to_app((uintptr_t)strsep(
    (FAR char **)addr_app_to_native(parm1),
    (FAR const char *)addr_app_to_native(parm2)));
}

#endif /* GLUE_FUNCTION_strsep */

#ifndef GLUE_FUNCTION_scandir
#define GLUE_FUNCTION_scandir
uintptr_t glue_scandir(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2,
                       uintptr_t parm3, uintptr_t parm4)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  int ret = 0;
  int i = 0;

  ret = scandir((FAR const char *)parm1,
                 (FAR struct dirent ***)parm2,
                 (FAR void *)parm3, alphasort);

  for (i = 0; i < ret; i++)
    {
      (*(uintptr_t **)parm2)[i] = addr_native_to_app((*(uintptr_t **)parm2)[i]);
    }

  *(uintptr_t *)parm2 = addr_native_to_app(*(uintptr_t *)parm2);

  return ret;
}

#endif /* GLUE_FUNCTION_scandir */

#ifndef GLUE_FUNCTION_daemon
#define GLUE_FUNCTION_daemon
uintptr_t glue_daemon(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2)
{
  return 0;
}

#endif /* GLUE_FUNCTION_daemon */

#ifndef GLUE_FUNCTION_posix_memalign
#define GLUE_FUNCTION_posix_memalign
uintptr_t glue_posix_memalign(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2, uintptr_t parm3)
{
  uintptr_t rawptr;
  wasm_module_inst_t module_inst = get_module_inst(env);

  /* Extra size for align */

  parm3 += parm2;
  int ret = posix_memalign((FAR void **)parm1, (size_t)parm2, (size_t)parm3);

  /* If the memory allocation is failed, return NULL */

  if (ret != OK)
    {
      return NULL;
    }

  /* Add the original pointer to the map */

  rawptr = *(uintptr_t *)parm1;
  *(void **)parm1 = addr_native_to_app((uintptr_t)*(void **)parm1);
  *(uintptr_t *)parm1 = (*(uintptr_t*)parm1 + parm2 - 1) & ~(parm2 - 1);

  if (add_to_aligned_map(*(uintptr_t *)parm1, rawptr))
    {
      return ret;
    }
  else
    {
      free(rawptr);
      return NULL;
    }
}
#endif /* GLUE_FUNCTION_posix_memalign */

#ifndef GLUE_FUNCTION_free
#define GLUE_FUNCTION_free
void glue_free(wasm_exec_env_t env, uintptr_t parm1)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  uintptr_t ret = NULL;
  uintptr_t app_addr = NULL;

  /* Try to pop the original pointer from the map */
  if (parm1 != (uintptr_t)NULL)
    {
      app_addr = addr_native_to_app(parm1);
      ret = remove_from_aligned_map(app_addr);
    }

  if (ret)
    {
      parm1 = ret;
    }
  free((FAR void *)parm1);
}

#endif /* GLUE_FUNCTION_free */

#ifndef GLUE_FUNCTION_vasprintf
#define GLUE_FUNCTION_vasprintf
uintptr_t glue_vasprintf(wasm_exec_env_t env, uintptr_t parm1, uintptr_t format, va_list ap)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  uintptr_t ret;
  va_list_string2native(env, format, ap);
  ret = vasprintf((FAR char **)parm1, (FAR const IPTR char *)format, ap);
  *(uintptr_t *)parm1 = addr_native_to_app(*(uintptr_t *)parm1);
  return ret;
}
#endif /* GLUE_FUNCTION_vasprintf */

#ifndef GLUE_FUNCTION_strtol
#define GLUE_FUNCTION_strtol
uintptr_t glue_strtol(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2, uintptr_t parm3)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  uintptr_t ret;
  *(uintptr_t *)parm2 = addr_app_to_native(*(uintptr_t *)parm2);
  ret = strtol((FAR const char *)parm1, (FAR char **)parm2, (int)parm3);
  *(uintptr_t *)parm2 = addr_native_to_app(*(uintptr_t *)parm2);
  return ret;
}

#endif /* GLUE_FUNCTION_strtol */

#if defined(CONFIG_HAVE_LONG_LONG)
#ifndef GLUE_FUNCTION_strtoll
#define GLUE_FUNCTION_strtoll
uintptr_t glue_strtoll(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2, uintptr_t parm3)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  uintptr_t ret;
  *(uintptr_t *)parm2 = addr_app_to_native(*(uintptr_t *)parm2);
  ret = strtoll((FAR const char *)parm1, (FAR char **)parm2, (int)parm3);
  *(uintptr_t *)parm2 = addr_native_to_app(*(uintptr_t *)parm2);
  return ret;
}
#endif /* GLUE_FUNCTION_strtoll */
#endif

#ifndef GLUE_FUNCTION_strtoul
#define GLUE_FUNCTION_strtoul
uintptr_t glue_strtoul(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2, uintptr_t parm3)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  uintptr_t ret;
  *(uintptr_t *)parm2 = addr_app_to_native(*(uintptr_t *)parm2);
  ret = strtoul((FAR const char *)parm1, (FAR char **)parm2, (int)parm3);
  *(uintptr_t *)parm2 = addr_native_to_app(*(uintptr_t *)parm2);
  return ret;
}

#endif /* GLUE_FUNCTION_strtoul */

#ifndef GLUE_FUNCTION_strtoull
#define GLUE_FUNCTION_strtoull
uintptr_t glue_strtoull(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2, uintptr_t parm3)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  uintptr_t ret;
  *(uintptr_t *)parm2 = addr_app_to_native(*(uintptr_t *)parm2);
  ret = strtoull((FAR const char *)parm1, (FAR char **)parm2, (int)parm3);
  *(uintptr_t *)parm2 = addr_native_to_app(*(uintptr_t *)parm2);
  return ret;
}

#endif /* GLUE_FUNCTION_strtoull */

#if defined(CONFIG_LIBC_LOCALE)

#ifndef GLUE_FUNCTION_iconv
#define GLUE_FUNCTION_iconv
uintptr_t glue_iconv(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2, uintptr_t parm3, uintptr_t parm4, uintptr_t parm5)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  uintptr_t ret;
  *(uintptr_t *)parm2 = addr_app_to_native(*(uintptr_t *)parm2);
  *(uintptr_t *)parm4 = addr_app_to_native(*(uintptr_t *)parm4);
  ret = iconv((iconv_t)parm1, (FAR char **)parm2, (FAR size_t *)parm3, (FAR char **)parm4, (FAR size_t *)parm5);
  *(uintptr_t *)parm2 = addr_native_to_app(*(uintptr_t *)parm2);
  *(uintptr_t *)parm4 = addr_native_to_app(*(uintptr_t *)parm4);
  return ret;
}

#endif /* GLUE_FUNCTION_iconv */
#endif /* defined(CONFIG_LIBC_LOCALE) */

#ifndef GLUE_FUNCTION_versionsort
#define GLUE_FUNCTION_versionsort
uintptr_t glue_versionsort(wasm_exec_env_t env, uintptr_t parm1, uintptr_t parm2)
{
  wasm_module_inst_t module_inst = get_module_inst(env);
  uintptr_t ret;
  *(uintptr_t *)parm1 = addr_app_to_native(*(uintptr_t *)parm1);
  *(uintptr_t *)parm2 = addr_app_to_native(*(uintptr_t *)parm2);
  ret = versionsort((FAR const struct dirent **)parm1, (FAR const struct dirent **)parm2);
  *(uintptr_t *)parm1 = addr_native_to_app(*(uintptr_t *)parm1);
  *(uintptr_t *)parm2 = addr_native_to_app(*(uintptr_t *)parm2);
  return ret;
}

#endif /* GLUE_FUNCTION_versionsort */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "libc_glue.c"
#include "libm_glue.c"
#include "syscall_glue.c"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool
wamr_custom_init(RuntimeInitArgs *init_args)
{
  bool ret = wasm_runtime_full_init(init_args);

  if (!ret)
    {
      return ret;
    }

  /* Add extra init hook here */

  ret = wasm_native_register_natives("env", g_syscall_native_symbols,
                                      nitems(g_syscall_native_symbols));
  if (ret == true)
    {
      ret = wasm_native_register_natives("env", g_libc_native_symbols,
                                          nitems(g_libc_native_symbols));
      if (ret == true)
        {
          ret = wasm_native_register_natives("env", g_libm_native_symbols,
                                              nitems(g_libm_native_symbols));
        }
    }

  return ret;
}
