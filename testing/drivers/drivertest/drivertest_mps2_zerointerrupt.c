/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_mps2_zerointerrupt.c
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

#include <nuttx/nuttx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/semaphore.h>

#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_TEST_IRQPRIO_TTHREAD  8
#define CONFIG_TEST_IRQPRIO_LOOP_CNT 5000

#ifndef NVIC_IRQ_PENDSV
#  define NVIC_IRQ_PENDSV 14
#endif

#define MPS2_ADDR2REG_PTR(base, off) (uint32_t*)((uint32_t*)(base) + (off))
#define MPS2_IRQ_FROMBASE(base, off) ((base) + (off))

/* https://developer.arm.com/documentation/101104/0200/programmers-model/
 * base-element/cmsdk-timer
 */

#define MPS_TIMER_CTRL_OFFSET   0
#define MPS_TIMER_VALUE_OFFSET  1
#define MPS_TIMER_RELOAD_OFFSET 2
#define MPS_TIMER_CLEAR_OFFSET  3

#define MPS_TIMER_CTRL_ENABLE   (1<<0)
#define MPS_TIMER_CTRL_IE       (1<<3)

static_assert(NVIC_SYSH_PRIORITY_DEFAULT == 0x80, "prio");

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct mps2_an500_timer_s
{
  volatile uint32_t *reload;
  volatile uint32_t *ctrl;
  volatile uint32_t *clear;
  int irq;
  int before;
  sem_t *sem;
  int after;
} mps2_an500_timer_t;

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int pendsv_irq_handle(int irq, void *context, void *arg);
static int timer_irq_handle(int irq, void *context, void *arg);
static int g_tag = 0;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static mps2_an500_timer_t g_timer;
static sem_t test_sem = SEM_INITIALIZER(0);

static const int armv7m_gpio_base = 16;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void mps_timer_init(void)
{
  static const uint32_t timerbase[] =
    {
      0x40000000, 0x40001000
    };

  static const int timerirq[] =
    {
      8, 9
    };

  static const int timer_irq_prio[] =
    {
      0x60, 0x70
    };

  mps2_an500_timer_t *t = &g_timer;

  t->reload  = MPS2_ADDR2REG_PTR(timerbase[0], MPS_TIMER_RELOAD_OFFSET);
  t->ctrl  = MPS2_ADDR2REG_PTR(timerbase[0], MPS_TIMER_CTRL_OFFSET);
  t->clear = MPS2_ADDR2REG_PTR(timerbase[0], MPS_TIMER_CLEAR_OFFSET);
  t->irq   = MPS2_IRQ_FROMBASE(armv7m_gpio_base, timerirq[0]);

  irq_attach(t->irq, timer_irq_handle, t);
  up_enable_irq(t->irq);

  up_prioritize_irq(t->irq, timer_irq_prio[0]);

  irq_attach(NVIC_IRQ_PENDSV, pendsv_irq_handle, NULL);
}

static int pendsv_irq_handle(int irq, void *context, void *arg)
{
  mps2_an500_timer_t *t = &g_timer;

  if (t->sem != NULL)
    {
      sem_post(t->sem);
    }

  up_udelay(t->after);

  DEBUGASSERT(g_tag == 0);

  return 0;
}

static int timer_irq_handle(int irq, void *context, void *arg)
{
  mps2_an500_timer_t *t = arg;
  *t->clear = 1;

  up_udelay(t->before);

  up_trigger_irq(NVIC_IRQ_PENDSV, 0);

  return 0;
}

static void timer_begin_test(mps2_an500_timer_t *t, uint32_t reload_us)
{
  uint32_t reload = reload_us * 25;
  *t->reload = reload;
  t->sem     = &test_sem;

  *t->ctrl = MPS_TIMER_CTRL_IE | MPS_TIMER_CTRL_ENABLE;
}

static void timer_end_test(mps2_an500_timer_t *t)
{
  *t->ctrl = 0;
  *t->clear = 1;
}

static void *test_irq_awaker_thread_entry(void *arg)
{
  struct timespec ts;
  int cnt = 0;
  int ret;

  while (cnt++ < CONFIG_TEST_IRQPRIO_LOOP_CNT)
    {
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec++;
      ret = sem_timedwait(&test_sem, &ts);
      if (ret != OK)
        {
          break;
        }

      if (cnt % 1000 == 0)
        {
          printf("cnt %d\n", cnt);
        }
    }

  printf("timeoutquit %d\n", cnt);
  return 0;
}

static void drivertest_mps2_zerointerrupt(void **argv)
{
  pid_t tid;
  irqstate_t flags;
  pthread_attr_t attr;
  int cnt = 0;

  printf("init done\n");

  printf("simple_test done\n");
  g_timer.before = 1;
  g_timer.after = 1;
  timer_begin_test(&g_timer, 1000);

  pthread_attr_init(&attr);

  attr.priority = 255;
  pthread_create(&tid,
                 &attr,
                 test_irq_awaker_thread_entry,
                 NULL);
  printf("thread init done\n");

  while (cnt++ < CONFIG_TEST_IRQPRIO_LOOP_CNT)
    {
      flags = enter_critical_section();
      g_tag++;
      up_udelay(100);
      g_tag--;
      leave_critical_section(flags);
    }

  pthread_join(tid, NULL);

  timer_end_test(&g_timer);
  printf("timer end done\n");
  pthread_join(tid, NULL);
  printf("sem thread join done\n");
}

static int setup(void **argv)
{
  mps_timer_init();
  return 0;
}

static int teardown(void **argv)
{
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_prestate_setup_teardown(
        drivertest_mps2_zerointerrupt, setup, teardown, NULL),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
