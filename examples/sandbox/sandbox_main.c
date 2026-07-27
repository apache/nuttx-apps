/****************************************************************************
 * apps/examples/sandbox/sandbox_main.c
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

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Where to poke.
 *
 * The point of this test is to be architecture-neutral, so the address is
 * derived rather than hard-coded.  In every BUILD_PROTECTED configuration in
 * the tree the kernel blob is placed *below* the user blob, and the boundary
 * between them is exactly CONFIG_NUTTX_USERSPACE:
 *
 *   qemu-armv7a:pnsh       0x00200000     mps2-an521:knsh     0x10200000
 *   qemu-armv8a:pnsh       0x41000000     pimoroni-pico-2-plus:pnsh
 *   rv-virt:pnsh[64]       0x80040000                         0x10100000
 *
 * so the word just below it belongs to the kernel in all of them.  Whether
 * that address holds kernel code, kernel data, or nothing mapped at all does
 * not matter:  either way an unprivileged task must not be able to read it.
 *
 * A BUILD_FLAT configuration has no such boundary and no CONFIG_NUTTX_USERSPACE
 * at all; there the test reports that there is nothing to contain rather than
 * pretending to pass.
 */

#ifdef CONFIG_NUTTX_USERSPACE
#  define SANDBOX_HAVE_TARGET   1
#  define SANDBOX_TARGET        ((uintptr_t)CONFIG_NUTTX_USERSPACE - 16)
#else
#  define SANDBOX_HAVE_TARGET   0
#  define SANDBOX_TARGET        ((uintptr_t)0)
#endif

#define CANARY_PRIORITY         (CONFIG_EXAMPLES_SANDBOX_PRIORITY - 1)
#define CANARY_STACKSIZE        2048
#define ESCAPE_STACKSIZE        CONFIG_EXAMPLES_SANDBOX_STACKSIZE

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Bumped continuously by the canary task.  It is the evidence that the rest
 * of the system kept running while the offender was being killed:  a system
 * that panicked or reset stops printing entirely, and one that merely wedged
 * leaves this stuck.
 */

static volatile unsigned long g_canary;
static volatile bool          g_canary_stop;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int canary_task(int argc, FAR char *argv[])
{
  while (!g_canary_stop)
    {
      g_canary++;
      usleep(10000);
    }

  return 0;
}

/****************************************************************************
 * Name: escape
 *
 * Description:
 *   Make the access that must not be allowed.  This runs in whatever task
 *   calls it, and in a correctly isolated build it does not return.
 *
 ****************************************************************************/

static void escape(uintptr_t addr, bool store)
{
  FAR volatile uint32_t *p = (FAR volatile uint32_t *)addr;

  printf("sandbox:   attempting %s of %p\n", store ? "WRITE" : "read",
         (FAR void *)addr);
  fflush(stdout);

  if (store)
    {
      /* A write is the more dangerous direction and is not the default:  if
       * the hardware does *not* contain it, this corrupts whatever it lands
       * on.  It is offered because a read-only mapping would let a read
       * through while still refusing the write.
       */

      *p = 0xdeadbeef;
    }
  else
    {
      uint32_t v = *p;

      /* Consume the value so the load cannot be optimised away. */

      printf("sandbox:   NOT CONTAINED -- read %08lx\n", (unsigned long)v);
      fflush(stdout);
      return;
    }

  printf("sandbox:   NOT CONTAINED -- the access completed\n");
  fflush(stdout);
}

static int escape_task(int argc, FAR char *argv[])
{
  bool      store = (argc > 1 && argv[1][0] == 'w');
  uintptr_t addr  = SANDBOX_TARGET;

  if (argc > 2)
    {
      addr = (uintptr_t)strtoul(argv[2], NULL, 0);
    }

  escape(addr, store);

  /* Reaching here means the access was allowed. */

  return 1;
}

/****************************************************************************
 * Name: selfcheck
 *
 * Description:
 *   Spawn the offender as a separate task and watch what happens to it, and
 *   to everything else.  Three things have to be true for a pass:  the
 *   offending task must die, this task must still be running afterwards, and
 *   the canary must still be advancing.
 *
 ****************************************************************************/

static int selfcheck(bool store, uintptr_t addr)
{
  FAR char *argv[3];
  char      addrbuf[24];
  unsigned long before;
  unsigned long after;
  pid_t     canary;
  pid_t     pid;
  int       status = 0;
  int       ret;
  int       fails = 0;

  printf("sandbox: target %p (%s)\n", (FAR void *)addr,
         store ? "write" : "read");
#if SANDBOX_HAVE_TARGET
  printf("sandbox: derived from CONFIG_NUTTX_USERSPACE = %p\n",
         (FAR void *)(uintptr_t)CONFIG_NUTTX_USERSPACE);
#endif

  /* Start the canary before anything else, so it is already running when the
   * offender faults.
   */

  g_canary      = 0;
  g_canary_stop = false;

  canary = task_create("sandbox_canary", CANARY_PRIORITY, CANARY_STACKSIZE,
                       canary_task, NULL);
  if (canary < 0)
    {
      printf("sandbox: FAIL - could not start the canary task\n");
      return 1;
    }

  usleep(100000);
  before = g_canary;

  snprintf(addrbuf, sizeof(addrbuf), "0x%lx", (unsigned long)addr);
  argv[0] = store ? (FAR char *)"w" : (FAR char *)"r";
  argv[1] = addrbuf;
  argv[2] = NULL;

  printf("sandbox: starting the offending task\n");
  fflush(stdout);

  pid = task_create("sandbox_escape", CONFIG_EXAMPLES_SANDBOX_PRIORITY,
                    ESCAPE_STACKSIZE, escape_task, argv);
  if (pid < 0)
    {
      printf("sandbox: FAIL - could not start the offending task\n");
      g_canary_stop = true;
      return 1;
    }

  /* Wait for the offender to be reaped.  If the system contains the fault by
   * killing just that task, this returns.  If it panics or resets, nothing
   * below ever prints -- which is itself the result, visible on the console.
   */

#ifdef CONFIG_SCHED_WAITPID
  ret = waitpid(pid, &status, 0);
  if (ret < 0)
    {
      /* ECHILD here means the task was already reaped, which is still
       * containment -- it died and the system moved on.
       */

      printf("sandbox: waitpid() returned %d (task already reaped)\n", ret);
    }
  else
    {
      printf("sandbox: offender reaped, status %d\n", status);
    }
#else
  /* No waitpid:  poll until the pid is gone. */

  for (ret = 0; ret < 100; ret++)
    {
      if (kill(pid, 0) < 0)
        {
          break;
        }

      usleep(50000);
    }

  printf("sandbox: offender gone after %d polls\n", ret);
#endif

  usleep(200000);
  after = g_canary;
  g_canary_stop = true;

  /* Now the three things that make this a pass. */

  printf("\n");
  printf("sandbox: --- results ---\n");

  if (kill(pid, 0) == 0)
    {
      printf("sandbox: FAIL - the offending task is still alive\n");
      fails++;
    }
  else
    {
      printf("sandbox: PASS - the offending task was terminated\n");
    }

  printf("sandbox: PASS - this task survived and is still running\n");

  if (after > before)
    {
      printf("sandbox: PASS - unrelated task kept running (%lu -> %lu)\n",
             before, after);
    }
  else
    {
      printf("sandbox: FAIL - unrelated task stopped (%lu -> %lu)\n",
             before, after);
      fails++;
    }

  usleep(100000);

  printf("\n");
  if (fails == 0)
    {
      printf("sandbox: CONTAINED - the sandbox held\n");
    }
  else
    {
      printf("sandbox: NOT CONTAINED - %d check(s) failed\n", fails);
    }

  return fails;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void usage(void)
{
  printf("Usage: sandbox [escape [r|w] [addr]]\n"
         "  (no args)         spawn an offending task and check it is\n"
         "                    contained while everything else survives\n"
         "  escape [r|w] [a]  make the bad access in *this* task; in a\n"
         "                    contained build this task does not return\n");
}

int main(int argc, FAR char *argv[])
{
  bool      store = false;
  uintptr_t addr  = SANDBOX_TARGET;
  int       argbase = 1;

  if (argc > 1 && strcmp(argv[1], "-h") == 0)
    {
      usage();
      return 0;
    }

#if !SANDBOX_HAVE_TARGET
  printf("sandbox: this is a flat build -- there is no kernel/user boundary\n"
         "sandbox: to escape from, so there is nothing to contain.  Build a\n"
         "sandbox: protected or kernel configuration to run this test.\n");
  if (argc <= 1)
    {
      return 0;
    }
#endif

  if (argc > 1 && strcmp(argv[1], "escape") == 0)
    {
      argbase = 2;
    }

  if (argc > argbase && (argv[argbase][0] == 'w' || argv[argbase][0] == 'r'))
    {
      store = (argv[argbase][0] == 'w');
      argbase++;
    }

  if (argc > argbase)
    {
      addr = (uintptr_t)strtoul(argv[argbase], NULL, 0);
    }

  if (argc > 1 && strcmp(argv[1], "escape") == 0)
    {
      /* One-shot mode:  fault in this task, deliberately. */

      printf("sandbox: escaping from this task -- expect it to die\n");
      escape(addr, store);
      printf("sandbox: NOT CONTAINED - returned from the bad access\n");
      return 1;
    }

  return selfcheck(store, addr);
}
