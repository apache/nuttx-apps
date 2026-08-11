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

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <spawn.h>
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

/* Named targets.
 *
 * A bare address says nothing about what should happen when it is touched,
 * so every target carries the outcome it expects.  A build that refuses
 * everything is as broken as one that permits everything, and only the
 * "self" case can tell the two apart.
 *
 * The addresses come from Kconfig because a user process cannot see kernel
 * symbols;  that is the boundary under test.  A board supplies them in its
 * defconfig.  A target with no address is reported as unavailable rather
 * than silently skipped.
 */

#define SANDBOX_KERNEL_ADDR    CONFIG_EXAMPLES_SANDBOX_KERNEL_ADDR
#define SANDBOX_PERIPH_ADDR    CONFIG_EXAMPLES_SANDBOX_PERIPH_ADDR
#define SANDBOX_UNMAPPED_ADDR  CONFIG_EXAMPLES_SANDBOX_UNMAPPED_ADDR

/* A protected build knows where the kernel ends without being told:  the
 * kernel blob is placed below the user blob and the boundary is exactly
 * CONFIG_NUTTX_USERSPACE.  A kernel build has no such address -- each
 * process is loaded into its own address environment -- so there the
 * Kconfig value is the only source.
 */

#if SANDBOX_KERNEL_ADDR == 0 && defined(CONFIG_NUTTX_USERSPACE)
#  undef  SANDBOX_KERNEL_ADDR
#  define SANDBOX_KERNEL_ADDR  ((uintptr_t)CONFIG_NUTTX_USERSPACE - 16)
#endif

#define CANARY_INTERVAL_US     10000
#define SETTLE_US              200000

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum sandbox_expect_e
{
  EXPECT_FAULT = 0,   /* The access must be refused */
  EXPECT_OK           /* The access must be allowed */
};

struct sandbox_target_s
{
  FAR const char        *name;
  uintptr_t              addr;
  enum sandbox_expect_e  expect;
  FAR const char        *what;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Touched by the "self" target.  It belongs to this process, so refusing it
 * means the boundary is drawn in the wrong place.
 */

static volatile uint32_t g_own_word = 0x600df00d;

/* Bumped by the canary thread.  It is the evidence that the system kept
 * running while the offender was killed:  a kernel that panicked stops
 * printing, and one that merely wedged leaves this standing still.
 */

static volatile unsigned long g_canary;
static volatile bool          g_canary_stop;

/* Sampled by the canary thread while the offender is alive.  Taking these
 * here lets waitpid() stay blocking, so the exit status is the real one.
 */

static volatile pid_t         g_watch_pid;
static volatile unsigned long g_peak_mem;
static volatile int           g_peak_fds;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pool_used
 *
 * Description:
 *   The bytes in use, read from /proc/meminfo.  The page pool is reported
 *   when there is one, because that is where the memory of a process comes
 *   from;  otherwise the kernel heap is.
 *
 *   This is what makes the leak check mean something.  A count that does not
 *   move while the offender is alive would measure nothing, and "the same
 *   before and after" would say nothing about whether the memory came back.
 *
 * Returned Value:
 *   The bytes in use, or 0 if /proc/meminfo cannot be read.
 *
 ****************************************************************************/

static unsigned long pool_used(void)
{
  char buf[512];
  FAR char *line;
  FAR char *save;
  unsigned long kmem = 0;
  unsigned long page = 0;
  int fd;
  int n;

  fd = open("/proc/meminfo", O_RDONLY);
  if (fd < 0)
    {
      return 0;
    }

  n = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if (n <= 0)
    {
      return 0;
    }

  buf[n] = '\0';

  for (line = strtok_r(buf, "\n", &save); line != NULL;
       line = strtok_r(NULL, "\n", &save))
    {
      FAR char *p = line;
      FAR char *end;
      unsigned long used;

      /* Every line is "<total> <used> <free> ... <name>".  Read the two
       * numbers with strtoul() rather than a scanset, which is not in every
       * sscanf().
       */

      while (*p == ' ')
        {
          p++;
        }

      strtoul(p, &end, 10);
      if (end == p)
        {
          continue;                     /* The header line. */
        }

      p = end;
      while (*p == ' ')
        {
          p++;
        }

      used = strtoul(p, &end, 10);
      if (end == p)
        {
          continue;
        }

      if (strstr(line, "Page") != NULL)
        {
          page = used;
        }
      else if (strstr(line, "Kmem") != NULL || strstr(line, "Umem") != NULL)
        {
          kmem = used;
        }
    }

  return page != 0 ? page : kmem;
}

/****************************************************************************
 * Name: count_fds
 *
 * Description:
 *   The descriptors a process holds, read from /proc/<pid>/group/fd.  The
 *   first line of that file is a header and is not counted.
 *
 * Returned Value:
 *   The number of open descriptors, or -1 when the process is gone, which is
 *   the evidence that its group was destroyed and not merely emptied.
 *
 ****************************************************************************/

static int count_fds(pid_t pid)
{
  char  path[32];
  char  buf[512];
  int   count = 0;
  int   fd;
  int   n;
  int   i;

  snprintf(path, sizeof(path), "/proc/%d/group/fd", (int)pid);

  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      return -1;
    }

  n = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if (n <= 0)
    {
      return 0;
    }

  buf[n] = '\0';

  /* Count the lines that start with a digit, which are the descriptors.  The
   * header starts with "FD".
   */

  for (i = 0; i < n; i++)
    {
      if ((i == 0 || buf[i - 1] == '\n') && buf[i] >= '0' && buf[i] <= '9')
        {
          count++;
        }
    }

  return count;
}

static FAR void *canary_thread(FAR void *arg)
{
  while (!g_canary_stop)
    {
      g_canary++;

      if (g_watch_pid > 0)
        {
          unsigned long mem = pool_used();
          int           fds = count_fds(g_watch_pid);

          if (mem > g_peak_mem)
            {
              g_peak_mem = mem;
            }

          if (fds > g_peak_fds)
            {
              g_peak_fds = fds;
            }
        }

      usleep(CANARY_INTERVAL_US);
    }

  return NULL;
}

/****************************************************************************
 * Name: resolve
 *
 * Description:
 *   Turn a target name, or a literal address, into an address and the
 *   outcome that address must produce.
 *
 * Returned Value:
 *   OK on success, ERROR if the name is unknown or has no address on this
 *   configuration.
 *
 ****************************************************************************/

static int resolve(FAR const char *name, FAR struct sandbox_target_s *t)
{
  t->name = name;

  if (strcmp(name, "self") == 0)
    {
      t->addr   = (uintptr_t)&g_own_word;
      t->expect = EXPECT_OK;
      t->what   = "this process's own data";
      return OK;
    }

  if (strcmp(name, "kernel") == 0)
    {
      t->addr   = SANDBOX_KERNEL_ADDR;
      t->expect = EXPECT_FAULT;
      t->what   = "kernel memory";
    }
  else if (strcmp(name, "periph") == 0)
    {
      t->addr   = SANDBOX_PERIPH_ADDR;
      t->expect = EXPECT_FAULT;
      t->what   = "a peripheral register";
    }
  else if (strcmp(name, "unmapped") == 0)
    {
      t->addr   = SANDBOX_UNMAPPED_ADDR;
      t->expect = EXPECT_FAULT;
      t->what   = "an address with no mapping";
    }
  else if (name[0] == '0' && (name[1] == 'x' || name[1] == 'X'))
    {
      t->addr   = (uintptr_t)strtoul(name, NULL, 0);
      t->expect = EXPECT_FAULT;
      t->what   = "a literal address";
    }
  else
    {
      printf("sandbox: unknown target \"%s\"\n", name);
      return ERROR;
    }

  if (t->addr == 0)
    {
      printf("sandbox: target \"%s\" has no address here.\n", name);
      printf("sandbox: set CONFIG_EXAMPLES_SANDBOX_%s_ADDR, or pass an "
             "address.\n",
             strcmp(name, "kernel") == 0   ? "KERNEL" :
             strcmp(name, "periph") == 0   ? "PERIPH" : "UNMAPPED");
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: touch
 *
 * Description:
 *   Make the access.  Where it is refused this does not return.
 *
 ****************************************************************************/

static void touch(uintptr_t addr, bool store)
{
  FAR volatile uint32_t *p = (FAR volatile uint32_t *)addr;

  printf("sandbox:   attempting %s of %p\n", store ? "write" : "read",
         (FAR void *)addr);
  fflush(stdout);

  if (store)
    {
      *p = 0xdeadbeef;
    }
  else
    {
      uint32_t v = *p;

      printf("sandbox:   the read completed and returned %08lx\n",
             (unsigned long)v);
      fflush(stdout);
      return;
    }

  printf("sandbox:   the write completed\n");
  fflush(stdout);
}

/****************************************************************************
 * Name: run_case
 *
 * Description:
 *   Spawn this program again as a separate process, in "escape" mode, and
 *   watch what happens to it and to everything else.
 *
 *   The offender has to be a process and not a task.  A kernel build does
 *   not give user code task_create();  making a process is the only way a
 *   user program can put the bad access somewhere it can be killed.
 *
 ****************************************************************************/

static int run_case(FAR const char *progname,
                    FAR const struct sandbox_target_s *t, bool store)
{
  FAR char *argv[5];
  char      addrbuf[24];
  unsigned long canary_before;
  unsigned long canary_after;
  unsigned long mem_before;
  unsigned long mem_during = 0;
  unsigned long mem_after;
  int       fd_during = -1;
  pthread_t canary;
  pid_t     pid;
  int       status = 0;
  int       fails  = 0;
  int       ret;

  printf("\nsandbox: target %s -- %s at %p, expecting %s\n",
         t->name, t->what, (FAR void *)t->addr,
         t->expect == EXPECT_OK ? "success" : "a fault");

  g_canary      = 0;
  g_canary_stop = false;
  g_watch_pid   = 0;
  g_peak_mem    = 0;
  g_peak_fds    = -1;

  if (pthread_create(&canary, NULL, canary_thread, NULL) != 0)
    {
      printf("sandbox: FAIL - could not start the canary\n");
      return 1;
    }

  usleep(SETTLE_US / 2);
  canary_before = g_canary;
  mem_before    = pool_used();

  snprintf(addrbuf, sizeof(addrbuf), "0x%lx", (unsigned long)t->addr);
  argv[0] = (FAR char *)progname;
  argv[1] = (FAR char *)"escape";
  argv[2] = store ? (FAR char *)"w" : (FAR char *)"r";
  argv[3] = addrbuf;
  argv[4] = NULL;

  ret = posix_spawn(&pid, progname, NULL, NULL, argv, NULL);
  if (ret != 0)
    {
      printf("sandbox: FAIL - could not spawn the offender (%d)\n", ret);
      g_canary_stop = true;
      pthread_join(canary, NULL);
      return 1;
    }

  /* Watch while the offender lives.  The count has to be seen to rise here,
   * or "the same before and after" says nothing at all.  If the system
   * panics instead of containing the fault, nothing below prints, which is
   * itself the result.
   */

  g_watch_pid = pid;

  if (waitpid(pid, &status, 0) < 0)
    {
      printf("sandbox: FAIL - could not wait for the offender\n");
      fails++;
    }

  g_watch_pid = 0;
  mem_during  = g_peak_mem;
  fd_during   = g_peak_fds;

  printf("sandbox: the offender exited with status %d\n", status);

  usleep(SETTLE_US);
  canary_after = g_canary;
  mem_after    = pool_used();
  g_canary_stop = true;
  pthread_join(canary, NULL);

  printf("sandbox: --- %s ---\n", t->name);

  if (t->expect == EXPECT_OK)
    {
      if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
          printf("sandbox: PASS - the allowed access completed\n");
        }
      else
        {
          printf("sandbox: FAIL - an owned access was refused\n");
          fails++;
        }
    }
  else if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
      printf("sandbox: FAIL - NOT CONTAINED, the access was permitted\n");
      fails++;
    }
  else
    {
      printf("sandbox: PASS - the offending process was terminated\n");
    }

  printf("sandbox: PASS - this process survived\n");

  if (canary_after > canary_before)
    {
      printf("sandbox: PASS - another thread ran (%lu -> %lu)\n",
             canary_before, canary_after);
    }
  else
    {
      printf("sandbox: FAIL - another thread stopped (%lu -> %lu)\n",
             canary_before, canary_after);
      fails++;
    }

  /* Memory: before, during, after. */

  printf("sandbox: memory %lu -> %lu -> %lu\n",
         mem_before, mem_during, mem_after);

  if (mem_during <= mem_before)
    {
      printf("sandbox: FAIL - the memory of the offender was never seen\n");
      fails++;
    }
  else if (mem_after > mem_before)
    {
      printf("sandbox: FAIL - %lu bytes were not given back\n",
             mem_after - mem_before);
      fails++;
    }
  else
    {
      printf("sandbox: PASS - %lu bytes taken and given back\n",
             mem_during - mem_before);
    }

  /* Descriptors: open while it lived, and the group gone after. */

  if (fd_during <= 0)
    {
      printf("sandbox: FAIL - the descriptors of the offender were never "
             "seen\n");
      fails++;
    }
  else if (count_fds(pid) >= 0)
    {
      printf("sandbox: FAIL - %d descriptor(s) are still open\n",
             count_fds(pid));
      fails++;
    }
  else
    {
      printf("sandbox: PASS - %d descriptor(s) open, none after\n",
             fd_during);
    }

  return fails;
}

static void usage(FAR const char *progname)
{
  printf("Usage: %s [r|w] [target ...]\n", progname);
  printf("       %s escape <r|w> <addr>\n", progname);
  printf("\n");
  printf("Targets:\n");
  printf("  self      this process's own data      must succeed\n");
  printf("  kernel    kernel memory                must fault\n");
  printf("  periph    a peripheral register        must fault\n");
  printf("  unmapped  an address with no mapping   must fault\n");
  printf("  0x...     a literal address            must fault\n");
  printf("\n");
  printf("With no target, self, kernel, periph and unmapped are all run.\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *defaults[] =
  {
    "self", "kernel", "periph", "unmapped"
  };

  struct sandbox_target_s t;
  FAR const char *progname = argv[0];
  char  raw[256];
  int   rawfd;
  int   rawn;
  bool  store = false;
  int   fails = 0;
  int   ran   = 0;
  int   i;

  if (argc > 1 && strcmp(argv[1], "-h") == 0)
    {
      usage(progname);
      return 0;
    }

  /* "escape" is how this program re-enters itself as the offender.  It makes
   * the access in this process and, where the access is refused, never gets
   * to the line below.
   */

  if (argc > 3 && strcmp(argv[1], "escape") == 0)
    {
      /* Take resources the kernel has to reclaim, and hold them across the
       * access.  A process that dies owning nothing says nothing about
       * whether killing it leaks:  the heap block is touched so its pages
       * are really committed, and the descriptor is left open on purpose.
       */

      FAR void *mem = malloc(CONFIG_EXAMPLES_SANDBOX_ALLOC);
      int fd = open("/system/bin/sandbox", O_RDONLY);

      if (mem != NULL)
        {
          memset(mem, 0xa5, CONFIG_EXAMPLES_SANDBOX_ALLOC);
        }

      printf("sandbox:   holding %d bytes at %p and fd %d\n",
             CONFIG_EXAMPLES_SANDBOX_ALLOC, mem, fd);
      fflush(stdout);

      touch((uintptr_t)strtoul(argv[3], NULL, 0), argv[2][0] == 'w');

      /* Only an allowed access arrives here.  Leave both outstanding, so
       * that the normal exit path is measured the same way as the kill.
       */

      return 0;
    }

  /* Show where the numbers below come from. */

  rawfd = open("/proc/meminfo", O_RDONLY);
  if (rawfd < 0)
    {
      printf("sandbox: /proc/meminfo cannot be opened (%d)\n", errno);
    }
  else
    {
      rawn = read(rawfd, raw, sizeof(raw) - 1);
      close(rawfd);

      if (rawn > 0)
        {
          raw[rawn] = '\0';
          printf("sandbox: /proc/meminfo reads\n%s", raw);
        }
      else
        {
          printf("sandbox: /proc/meminfo read gave %d\n", rawn);
        }
    }

  i = 1;
  if (argc > 1 && (argv[1][0] == 'r' || argv[1][0] == 'w') &&
      argv[1][1] == '\0')
    {
      store = (argv[1][0] == 'w');
      i++;
    }

  if (i >= argc)
    {
      int n;

      for (n = 0; n < (int)(sizeof(defaults) / sizeof(defaults[0])); n++)
        {
          if (resolve(defaults[n], &t) == OK)
            {
              fails += run_case(progname, &t, store);
              ran++;
            }
        }
    }
  else
    {
      for (; i < argc; i++)
        {
          if (resolve(argv[i], &t) == OK)
            {
              fails += run_case(progname, &t, store);
              ran++;
            }
        }
    }

  printf("\n");
  if (ran == 0)
    {
      printf("sandbox: no target could be resolved, nothing was tested\n");
      return 1;
    }

  if (fails == 0)
    {
      printf("sandbox: CONTAINED - %d target(s), every check passed\n", ran);
    }
  else
    {
      printf("sandbox: NOT CONTAINED - %d of %d target(s) failed\n",
             fails, ran);
    }

  return fails;
}
