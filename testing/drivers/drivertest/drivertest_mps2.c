/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_mps2.c
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
#include <nuttx/serial/uart_cmsdk.h>

#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_TEST_IRQPRIO_TTHREAD  8
#define CONFIG_TEST_IRQPRIO_LOOP_CNT 5000
#define CONFIG_TEST_IRQPRIO_LOG      0

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

#if CONFIG_TEST_IRQPRIO_LOG
# define TAG_BEGIN(v) \
do { \
  if ((v)->begin) \
    { \
      up_puts((v)->begin); \
    } \
} while(0)

# define TAG_END(v) \
do { \
  if ((v)->end) \
    { \
      up_puts((v)->end); \
    } \
} while(0)
#else
#define TAG_BEGIN(v)
#define TAG_END(v)
#endif

static_assert(NVIC_SYSH_PRIORITY_DEFAULT == 0x80, "prio");

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct mps2_an500_uart_s
{
  volatile uint32_t *ctrl;
  volatile uint32_t *tx;
  volatile uint32_t *clear;
  int irq;
  int before;
  sem_t *sem;
  struct mps2_an500_uart_s *trigger;
  int after;
#if CONFIG_TEST_IRQPRIO_LOG
  const char *begin;
  const char *end;
#endif
} mps2_an500_uart_t;

typedef struct mps2_an500_timer_s
{
  volatile uint32_t *reload;
  volatile uint32_t *ctrl;
  volatile uint32_t *clear;
  int irq;
  int before;
  sem_t *sem;
  int after;
#if CONFIG_TEST_IRQPRIO_LOG
  const char *begin;
  const char *end;
#endif
} mps2_an500_timer_t;

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int uart_irq_tx_handle(int irq, void *context, void *arg);
static int timer_irq_handle(int irq, void *context, void *arg);
static uint32_t uart_random_test(mps2_an500_uart_t *uart);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static mps2_an500_uart_t uarts[5];
static mps2_an500_timer_t timer[2];
static sem_t test_sem = SEM_INITIALIZER(0);

static const int armv7m_gpio_base = 16;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void mps_uart_init(void)
{
  static const uint32_t uartbase[] =
    {
      0x40004000, 0x40005000, 0x40006000, 0x40007000, 0x40009000
    };

  /* static const int an500_uart_tx_irq_offset = 1; */

  static const int uarttxirq[] =
    {
      1, 3, 5, 19, 21
    };

  static const int uarttxirq_prio[] =
    {
      0x80, 0x90, 0xb0, 0xc0, 0xd0,
    };

#if CONFIG_TEST_IRQPRIO_LOG
  static const char *begin_tag[] =
    {
      "0", "1", "2", "3", "4"
    };

  static const char *end_tag[] =
    {
      " ", "A", "B", "C", "D"
    };
#endif

  mps2_an500_uart_t *prev = NULL;

  for (int i = 0; i < 5; i++)
    {
      mps2_an500_uart_t *u = &uarts[i];

      u->ctrl  = MPS2_ADDR2REG_PTR(uartbase[i], UART_CTRL_OFFSET);
      u->tx    = MPS2_ADDR2REG_PTR(uartbase[i], UART_THR_OFFSET);
      u->clear = MPS2_ADDR2REG_PTR(uartbase[i], UART_INTSTS_OFFSET);
      u->irq   = MPS2_IRQ_FROMBASE(armv7m_gpio_base, uarttxirq[i]);

      if (i > 0)
        {
          irq_attach(u->irq, uart_irq_tx_handle, u);
          up_enable_irq(u->irq);
          *u->ctrl = UART_CTRL_TX_ENABLE | UART_CTRL_TX_INT_ENABLE;
          u->trigger = prev;
          prev       = u;
#if CONFIG_TEST_IRQPRIO_LOG
          u->begin = begin_tag[i];
          u->end   = end_tag[i];
#endif
        }

      up_prioritize_irq(u->irq, uarttxirq_prio[i]);
    }
}

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
      0x80, 0xa0
    };

#if CONFIG_TEST_IRQPRIO_LOG
  static const char *begin_tag[] =
    {
      "t", "u"
    };

  static const char *end_tag[] =
    {
      "T", "U"
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

      up_prioritize_irq(t->irq, timer_irq_prio[i]);

#if CONFIG_TEST_IRQPRIO_LOG
      t->begin = begin_tag[i];
      t->end   = end_tag[i];
#endif
    }
}

static int uart_irq_tx_handle(int irq, void *context, void *arg)
{
  mps2_an500_uart_t *u = arg;
  *u->clear            = UART_INTSTATUS_TX;

  TAG_BEGIN(u);

  up_udelay(u->before);

  if (u->sem != NULL)
    {
      sem_post(u->sem);
    }

  if (u->trigger != NULL)
    {
      uart_random_test(u->trigger);
    }

  up_udelay(u->after);

  TAG_END(u);

  return 0;
}

static int timer_irq_handle(int irq, void *context, void *arg)
{
  mps2_an500_timer_t *t = arg;
  *t->clear = 1;

  TAG_BEGIN(t);

  up_udelay(t->before);

  if (t->sem != NULL)
    {
      sem_post(t->sem);
    }

  up_udelay(t->after);

  TAG_END(t);

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

static void uart_simple_test(mps2_an500_uart_t *uart)
{
  uart->before = 0;
  uart->sem      = NULL;
  uart->after  = 0;
  *uart->tx      = 0;
}

static uint32_t uart_random_test(mps2_an500_uart_t *uart)
{
  /* 31bits random
   * | thread usleep | after udelay | before udelay | uart_sel  | use sem |
   * | 30...19 (12b) | 18..11 (8b)  | 10..3 (8b)    | 2..1 (2b) | 0  (1b) |
   */

  uint32_t r   = random();
  int use_sem  = (r >> 0) & 0x1;
  int u_before = (r >> 3) & (0xff);
  int u_after  = (r >> 12) & (0xff);

  mps2_an500_uart_t *u;

  if (uart != NULL)
    {
      u = uart;
      if (!use_sem)
        {
          return r;
        }
    }
  else
    {
      int uart_select = (r >> 1) & 0x3;
      u               = &uarts[uart_select + 1];
    }

  if (use_sem)
    {
      u->sem = &test_sem;
    }
  else
    {
      u->sem = NULL;
    }

  u->before = u_before;
  u->after  = u_after;
  *u->tx      = 0;

  if (!up_interrupt_context())
    {
      int u_thread = (r >> 19) & (0xfff);
      usleep(u_thread);
    }

  return r;
}

static void *test_irq_awaker_thread_entry(void *arg)
{
  struct timespec ts;
  int cnt = 0;
  int ret;
  while (true)
    {
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec++;
      ret = sem_timedwait(&test_sem, &ts);
      if (ret != OK)
        {
          break;
        }
      else
        {
          cnt++;
          if (cnt % 10000 == 0)
            {
              printf("%d, recv:%d\n", gettid(), cnt);
            }
#if CONFIG_TEST_IRQPRIO_LOG
          else
            {
              printf(".");
            }
#endif
        }
    }

  printf("timeoutquit %d\n", cnt);
  return 0;
}

static void *test_irq_prio_thread_entry(void *arg)
{
  int i;
  for (i = 0; i < CONFIG_TEST_IRQPRIO_LOOP_CNT; i++)
    {
      uart_random_test(NULL);
    }

  return NULL;
}

static void drivertest_mps2(void **argv)
{
  pid_t tid[CONFIG_TEST_IRQPRIO_TTHREAD + 1];
  pthread_attr_t attr;
  int i;

  printf("init done\n");

  for (i = 1; i < 5; i++)
    {
      uart_simple_test(&uarts[i]);
    }

  printf("simple_test done\n");
  timer[0].before = 1;
  timer[0].after = 1;
  timer_begin_test(&timer[0], 1000);
  timer[1].before = 10;
  timer[1].after = 10;
  timer_begin_test(&timer[1], 100 * 1000);

  pthread_attr_init(&attr);

  attr.priority = 1;
  for (i = 0; i < CONFIG_TEST_IRQPRIO_TTHREAD; i++)
    {
      attr.priority++;
      pthread_create(&tid[i], &attr, test_irq_prio_thread_entry, NULL);
    }

  attr.priority = 255;
  pthread_create(&tid[CONFIG_TEST_IRQPRIO_TTHREAD],
                 &attr,
                 test_irq_awaker_thread_entry,
                 NULL);
  printf("thread init done\n");

  for (i = 0; i < CONFIG_TEST_IRQPRIO_TTHREAD; i++)
    {
      pthread_join(tid[i], NULL);
    }

  printf("uart join done\n");
  timer_end_test(&timer[0]);
  timer_end_test(&timer[1]);
  printf("timer end done\n");
  pthread_join(tid[CONFIG_TEST_IRQPRIO_TTHREAD], NULL);
  printf("sem thread join done\n");
}

static int setup(void **argv)
{
  mps_uart_init();
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
        drivertest_mps2, setup, teardown, NULL),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
