/****************************************************************************
 * examples/cpuhog/cpuhog_main.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
 *            Bob Doiron
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

int cpuhog_main(int argc, char *argv[])
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

  while(1)
    {
      /* To test semaphore interaction (debugging system crashes...) */

      while (sem_wait(&g_state.sem) != 0)
        {
          ASSERT(errno == EINTR);
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
