/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/src/misc.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <stdarg.h>
#include <stdio.h>
#include <sys/random.h>
#include <syslog.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>

#include "port.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* <zephyr/logging/log.h> replaced the syslog LOG_ERR priority */

#define SYSLOG_ERR     3
#define SYSLOG_WARNING 4
#define SYSLOG_INFO    6
#define SYSLOG_DEBUG   7

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const int g_syslog_prio[] =
{
  SYSLOG_ERR, SYSLOG_ERR, SYSLOG_WARNING, SYSLOG_INFO, SYSLOG_DEBUG
};

static const char g_level_tag[] = "?EWID";

static pthread_once_t g_init_once = PTHREAD_ONCE_INIT;

K_THREAD_STACK_DEFINE(g_sys_work_q_stack,
                      CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);

/* Provided by the linker for the "zblue_init" section, see <zephyr/init.h> */

extern const struct z_zblue_init __start_zblue_init[];
extern const struct z_zblue_init __stop_zblue_init[];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int init_cmp(const void *a, const void *b)
{
  const struct z_zblue_init *x = *(const struct z_zblue_init **)a;
  const struct z_zblue_init *y = *(const struct z_zblue_init **)b;

  if (x->level != y->level)
    {
      return x->level - y->level;
    }

  if (x->prio != y->prio)
    {
      return x->prio - y->prio;
    }

  /* qsort() is not stable: keep the link order of equal entries */

  return x < y ? -1 : x > y;
}

static void kernel_init(void)
{
  const struct k_work_queue_config cfg =
  {
    .name = "sysworkq",
  };

  const struct z_zblue_init **list;
  size_t n = __stop_zblue_init - __start_zblue_init;
  size_t i;
  int ret;

  k_work_queue_start(&k_sys_work_q, g_sys_work_q_stack,
                     K_THREAD_STACK_SIZEOF(g_sys_work_q_stack),
                     CONFIG_SYSTEM_WORKQUEUE_PRIORITY, &cfg);

  /* Run the SYS_INIT() entries in Zephyr order: by level, then priority */

  list = malloc(n * sizeof(*list));
  __ASSERT_NO_MSG(n == 0 || list != NULL);
  for (i = 0; i < n; i++)
    {
      list[i] = &__start_zblue_init[i];
    }

  qsort(list, n, sizeof(*list), init_cmp);
  for (i = 0; i < n; i++)
    {
      ret = list[i]->init_fn();
      if (ret < 0)
        {
          syslog(SYSLOG_ERR, "zblue: init %p failed: %d\n",
                 list[i]->init_fn, ret);
        }
    }

  free(list);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Runs what the Zephyr kernel runs at boot: the system work queue and the
 * SYS_INIT() functions.  bt_enable() checks that the HCI device is ready
 * before anything else, which is the first point zblue gets control.
 */

void z_zblue_kernel_init(void)
{
  pthread_once(&g_init_once, kernel_init);
}

bool z_impl_device_is_ready(const struct device *dev)
{
  z_zblue_kernel_init();
  return dev != NULL;
}

/* Assertions: __ASSERT() failures end the program, as in Zephyr */

void zassert_vprint(const char *fmt, va_list ap)
{
  vsyslog(SYSLOG_ERR, fmt, ap);
}

void zassert_fail(const char *cond, const char *file, unsigned int line,
                  const char *fmt, ...)
{
  char msg[128];
  va_list ap;

  msg[0] = '\0';
  if (fmt != NULL)
    {
      va_start(ap, fmt);
      vsnprintf(msg, sizeof(msg), fmt, ap);
      va_end(ap);
    }

  syslog(SYSLOG_ERR, "zblue: assertion failed: %s at %s:%u %s\n",
         cond != NULL ? cond : "?", file != NULL ? file : "?", line, msg);
  abort();
}

/* Logging */

void z_zblue_log(int level, const char *module, const char *fmt, ...)
{
  char buf[192];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  level = CLAMP(level, 0, 4);
  syslog(g_syslog_prio[level], "%c: %s: %s\n", g_level_tag[level], module,
         buf);
}

void z_zblue_hexdump(int level, const char *module, const void *data,
                     size_t length, const char *str)
{
  const uint8_t *p = data;
  char line[16 * 3 + 1];
  size_t i;
  size_t n;

  z_zblue_log(level, module, "%s", str);
  for (i = 0; i < length; i += 16)
    {
      for (n = 0; n < 16 && i + n < length; n++)
        {
          snprintf(&line[n * 3], 4, "%02x ", p[i + n]);
        }

      z_zblue_log(level, module, "  %s", line);
    }
}

void printk(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintk(fmt, ap);
  va_end(ap);
}

void vprintk(const char *fmt, va_list ap)
{
  vsyslog(SYSLOG_INFO, fmt, ap);
}

int snprintk(char *str, size_t size, const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = vsnprintf(str, size, fmt, ap);
  va_end(ap);
  return ret;
}

int vsnprintk(char *str, size_t size, const char *fmt, va_list ap)
{
  return vsnprintf(str, size, fmt, ap);
}

/* Random numbers */

void z_impl_sys_rand_get(void *dst, size_t len)
{
  arc4random_buf(dst, len);
}

int z_impl_sys_csrand_get(void *dst, size_t len)
{
  return getrandom(dst, len, 0) == (ssize_t)len ? 0 : -EIO;
}
