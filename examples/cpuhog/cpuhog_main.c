/****************************************************************************
 * apps/examples/cpuhog/cpuhog_main.c
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
 * Example usage:
 *
 * NuttShell (NSH)
 * nsh> cpuhog > /dev/ttyS1 &
 * cpuhog [5:50]
 * nsh> cpuhog > /dev/ttyS2 &
 * cpuhog [7:50]
 * nsh> cpuhog &
 * cpuhog [8:50]
 * nsh> cpuhog 2
 * ps
 * PID   PRI SCHD TYPE   NP STATE    CPU    NAME
 *     0   0 FIFO TASK      READY      0.9% Idle Task()
 *     1 192 FIFO KTHREAD   WAITSIG    0.0% work()
 *     2 200 FIFO KTHREAD   WAITSIG    0.0% wdog()
 *     3 100 FIFO TASK      RUNNING    0.0% init()
 *     5  50 RR   TASK      WAITSEM   27.7% cpuhog(20009c70, 20009c80)
 *     7  50 RR   TASK      WAITSEM   30.3% cpuhog(20008c00, 20008c10)
 *     8  50 RR   TASK      READY     41.7% cpuhog()
 * nsh>
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <nuttx/clock.h>
#include <nuttx/arch.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CPUHOG_FIFO_FNAME "/dev/cpuhogfifo"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct state_s
{
  volatile bool initialized;
  sem_t sem;
  int count;
} g_state;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int id = -1;
  char buf[256];
  int fd = -1;

  if (!g_state.initialized)
    {
      sem_init(&g_state.sem, 0, 1);
      mkfifo(CPUHOG_FIFO_FNAME, 0666);
      g_state.count = 0;
      g_state.initialized = true;
      printf("cpuhog initialized\n");
    }

  while (1)
    {
      /* To test semaphore interaction (debugging system crashes...) */

      while (sem_wait(&g_state.sem) != 0)
        {
          DEBUGASSERT(errno == EINTR || errno == ECANCELED);
        }

      /* Burn some inside semlock */

      up_udelay(3000);
      sem_post(&g_state.sem);

      /* Unassigned */

      if (id == -1)
        {
          id = g_state.count++;
          printf("cpuhog %d", id);
          if (id == 0)
            {
              printf(": consumer\n");
              fd = open(CPUHOG_FIFO_FNAME, O_RDONLY);
            }
          else if (id == 1)
            {
              printf(": producer\n");
              fd = open(CPUHOG_FIFO_FNAME, O_WRONLY);
            }
          else
            {
              printf("\n");
            }
        }

      /* Consumer */

      else if (id == 0)
        {
          if (read(fd, buf, sizeof(buf)) > 0)
            {
              printf("-");
            }
          else
            {
              perror("fifo: ");
            }
        }

      /* Producer */

      else if (id == 1)
        {
          if (write(fd, buf, sizeof(buf)) > 0)
            {
              printf("+");
            }
          else
            {
              perror("fifo: ");
            }
        }

      /* Otherwise just a cpu burner */

      else
        {
          /* Burn some outside semlock */

          up_udelay(3000);
        }

      fflush(stdout);
    }

  return 0;
}
