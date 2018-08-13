/****************************************************************************
 * examples/ostest/tls.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/tls.h>

#include "ostest.h"

#ifdef CONFIG_TLS

#include <arch/tls.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct tls_info_s g_save_info;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void get_tls_info(FAR struct tls_info_s *info)
{
  memcpy(info, up_tls_info(), sizeof(struct tls_info_s));
}

static void put_tls_info(FAR const struct tls_info_s *info)
{
  memcpy(up_tls_info(), info, sizeof(struct tls_info_s));
}

static void set_tls_info(uintptr_t value)
{
  FAR struct tls_info_s *info = up_tls_info();
  int i;

  for (i = 0; i < CONFIG_TLS_NELEM; i++)
    {
      info->tl_elem[i] = value;
    }
}

static bool verify_tls_info(uintptr_t value)
{
  FAR struct tls_info_s *info = up_tls_info();
  bool fail = false;
  int i;

  for (i = 0; i < CONFIG_TLS_NELEM; i++)
    {
      if (info->tl_elem[i] != value)
        {
          printf("tls: ERROR Element %d: Set %lx / read %lx\n",
                 i, (unsigned long)value,
                 (unsigned long)info->tl_elem[i]);
          fail = true;
        }
    }

  return fail;
}

static void do_tls_test(uintptr_t value)
{
  set_tls_info(value);
  if (!verify_tls_info(value))
    {
      printf("tls: Successfully set %lx\n", (unsigned long)value);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void tls_test(void)
{
  get_tls_info(&g_save_info);
  do_tls_test(0);
  do_tls_test(0xffffffff);
  do_tls_test(0x55555555);
  do_tls_test(0xaaaaaaaa);
  put_tls_info(&g_save_info);
}

#endif /* CONFIG_TLS */
