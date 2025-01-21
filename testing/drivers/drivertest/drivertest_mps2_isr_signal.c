/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_mps2_isr_signal.c
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
#include <signal.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/semaphore.h>
#include <nuttx/serial/uart_cmsdk.h>

#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_TEST_ISR_SIGNAL_US    10000

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
  pid_t pid;
  int irq;
} mps2_an500_timer_t;

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int timer_irq_handle(int irq, void *context, void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int last_sig = 0;

static mps2_an500_timer_t timer[2];

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

#ifdef CONFIG_ARCH_IRQPRIO
  static const int timer_irq_prio[] =
    {
      0x80, 0xa0
    };
#endif

  for (int i = 0; i < 2; i++)
    {
      mps2_an500_timer_t *t = &timer[i];

      t->reload  = MPS2_ADDR2REG_PTR(timerbase[i], MPS_TIMER_RELOAD_OFFSET);
      t->ctrl  = MPS2_ADDR2REG_PTR(timerbase[i], MPS_TIMER_CTRL_OFFSET);
      t->clear = MPS2_ADDR2REG_PTR(timerbase[i], MPS_TIMER_CLEAR_OFFSET);
      t->irq   = MPS2_IRQ_FROMBASE(armv7m_gpio_base, timerirq[i]);

      irq_attach(t->irq, timer_irq_handle, t);
      up_enable_irq(t->irq);

#ifdef CONFIG_ARCH_IRQPRIO
      up_prioritize_irq(t->irq, timer_irq_prio[i]);
#endif
    }
}

static int timer_irq_handle(int irq, void *context, void *arg)
{
  mps2_an500_timer_t *t = arg;
  *t->clear = 1;
  *t->ctrl = 0;

  tgkill(t->pid, t->pid, SIGINT);
  return 0;
}

static void timer_begin_test(mps2_an500_timer_t *t, uint32_t reload_us)
{
  uint32_t reload = reload_us * 25;
  *t->reload = reload;

  *t->ctrl = MPS_TIMER_CTRL_IE | MPS_TIMER_CTRL_ENABLE;
}

static void timer_end_test(mps2_an500_timer_t *t)
{
  *t->ctrl = 0;
  *t->clear = 1;
}

static void test_sa_handler(int signo)
{
  last_sig = signo;
}

static void test_irq_signal(void **argv)
{
  mps2_an500_timer_t *t = &timer[1];
  struct timespec ts;

  signal(SIGINT, test_sa_handler);

  clock_gettime(CLOCK_MONOTONIC, &ts);

  t->pid = getpid();
  timer_begin_test(t, CONFIG_TEST_ISR_SIGNAL_US);

  /* Runing thread interrupt by exception_direct causing signal miss */

  while (true)
    {
      struct timespec now;
      up_udelay(1000);
      clock_gettime(CLOCK_MONOTONIC, &now);
      if (now.tv_sec - ts.tv_sec > 1 || last_sig != 0)
        {
          break;
        }
    }

  assert_int_equal(last_sig, SIGINT);
}

static int setup(void **argv)
{
  mps_timer_init();
  return 0;
}

static int teardown(void **argv)
{
  timer_end_test(&timer[0]);
  timer_end_test(&timer[1]);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_prestate_setup_teardown(
        test_irq_signal, setup, teardown, NULL),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
