/****************************************************************************
 * system/note/note_main.c
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

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/sched_note.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_note_daemon_started;
static uint8_t g_note_buffer[CONFIG_SYSTEM_NOTE_BUFFERSIZE];

/* Names of task/thread states */

static FAR const char *g_statenames[] =
{
  "Invalid",
  "Waiting for Unlock",
  "Ready",
  "Running",
  "Inactive",
  "Waiting for Semaphore",
#ifndef CONFIG_DISABLE_SIGNALS
  "Waiting for Signal",
#endif
#ifndef CONFIG_DISABLE_MQUEUE
  "Waiting for MQ empty",
  "Waiting for MQ full"
#endif
};

#define NSTATES (sizeof(g_statenames)/sizeof(FAR const char *))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dump_notes
 ****************************************************************************/

static void dump_notes(size_t nread)
{
  FAR struct note_common_s *note;
  uint32_t systime;
  pid_t pid;
  off_t offset;

  offset = 0;
  while (offset < nread)
    {
      note    = (FAR struct note_common_s *)&g_note_buffer[offset];
      pid     =  (pid_t)note->nc_pid[0] +
                ((pid_t)note->nc_pid[1] << 8);
      systime = (uint32_t) note->nc_systime[0]        +
                (uint32_t)(note->nc_systime[1] << 8)  +
                (uint32_t)(note->nc_systime[2] << 16) +
                (uint32_t)(note->nc_systime[3] << 24);

      switch (note->nc_type)
        {
          case NOTE_START:
            {
              FAR struct note_start_s *note_start =
                (FAR struct note_start_s *)note;

              if (note->nc_length < sizeof(struct note_start_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note too small for start note: %d\n",
                         note->nc_length);
                  return;
                }

#ifdef CONFIG_SMP
#if CONFIG_TASK_NAME_SIZE > 0
              syslog(LOG_INFO, "%08lx: Task %u \"%s\" started, CPU%u, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     note_start->nst_name, (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
#else
              syslog(LOG_INFO, "%08lx: Task %u started, CPU%u, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
#endif
#else
#if CONFIG_TASK_NAME_SIZE > 0
              syslog(LOG_INFO, "%08lx: Task %u \"%s\" started, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     note_start->nst_name, (unsigned int)note->nc_priority);
#else
              syslog(LOG_INFO, "%08lx: Task %u started, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_priority);
#endif
#endif
            }
            break;

          case NOTE_STOP:
            {
              if (note->nc_length != sizeof(struct note_stop_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for stop note: %d\n",
                         note->nc_length);
                  return;
                }

#ifdef CONFIG_SMP
              syslog(LOG_INFO,
                     "%08lx: Task %u stopped, CPU%u, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu, (unsigned int)note->nc_priority);
#else
              syslog(LOG_INFO,
                     "%08lx: Task %u stopped, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_priority);
#endif
            }
            break;

          case NOTE_SUSPEND:
            {
              FAR struct note_suspend_s *note_suspend =
                (FAR struct note_suspend_s *)note;
              FAR const char *statename;

              if (note->nc_length != sizeof(struct note_suspend_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for suspend note: %d\n",
                         note->nc_length);
                  return;
                }

              if (note_suspend->nsu_state < NSTATES)
                {
                  statename = g_statenames[note_suspend->nsu_state];
                }
              else
                {
                  statename = "ERROR";
                }

#ifdef CONFIG_SMP
              syslog(LOG_INFO,
                     "%08lx: Task %u suspended, CPU%u, priority %u, state \"%s\"\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority, statename);
#else
              syslog(LOG_INFO,
                     "%08lx: Task %u suspended, priority %u, state \"%s\"\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_priority, statename);
#endif
            }
            break;

          case NOTE_RESUME:
            {
              if (note->nc_length != sizeof(struct note_resume_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for resume note: %d\n",
                         note->nc_length);
                  return;
                }

#ifdef CONFIG_SMP
              syslog(LOG_INFO,
                     "%08lx: Task %u resumed, CPU%u, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
#else
              syslog(LOG_INFO,
                     "%08lx: Task %u resumed, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_priority);
#endif
            }
            break;

#ifdef CONFIG_SMP
          case NOTE_CPU_START:
            {
              FAR struct note_cpu_start_s *note_start =
                (FAR struct note_cpu_start_s *)note;

              if (note->nc_length != sizeof(struct note_cpu_start_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for CPU start note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog(LOG_INFO,
                     "%08lx: Task %u CPU%u requests CPU%u to start, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note_start->ncs_target,
                     (unsigned int)note->nc_priority);
            }
            break;

          case NOTE_CPU_STARTED:
            {
              if (note->nc_length != sizeof(struct note_cpu_started_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for CPU started note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog(LOG_INFO,
                     "%08lx: Task %u CPU%u has started, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
            }
            break;

          case NOTE_CPU_PAUSE:
            {
              FAR struct note_cpu_pause_s *note_pause =
                (FAR struct note_cpu_pause_s *)note;

              if (note->nc_length != sizeof(struct note_cpu_pause_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for CPU pause note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog(LOG_INFO,
                     "%08lx: Task %u CPU%u requests CPU%u to pause, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note_pause->ncp_target,
                     (unsigned int)note->nc_priority);
            }
            break;

          case NOTE_CPU_PAUSED:
            {
              if (note->nc_length != sizeof(struct note_cpu_paused_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for CPU paused note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog(LOG_INFO,
                     "%08lx: Task %u CPU%u has paused, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
            }
            break;

          case NOTE_CPU_RESUME:
            {
              FAR struct note_cpu_resume_s *note_resume =
                (FAR struct note_cpu_resume_s *)note;

              if (note->nc_length != sizeof(struct note_cpu_resume_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for CPU resume note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog(LOG_INFO,
                     "%08lx: Task %u CPU%u requests CPU%u to resume, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note_resume->ncr_target,
                     (unsigned int)note->nc_priority);
            }
            break;

          case NOTE_CPU_RESUMED:
            {
              if (note->nc_length != sizeof(struct note_cpu_resumed_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for CPU resumed note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog(LOG_INFO,
                     "%08lx: Task %u CPU%u has resumed, priority %u\n",
                     (unsigned long)systime, (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
            }
            break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_PREEMPTION
          case NOTE_PREEMPT_LOCK:
          case NOTE_PREEMPT_UNLOCK:
            {
              FAR struct note_preempt_s *note_preempt =
                (FAR struct note_preempt_s *)note;
              uint16_t count;

              if (note->nc_length != sizeof(struct note_preempt_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for preemption note: %d\n",
                         note->nc_length);
                  return;
                }

              count = (uint16_t) note_preempt->npr_count[0] +
                      (uint16_t)(note_preempt->npr_count[1] << 8);

              if (note->nc_type == NOTE_PREEMPT_LOCK)
                {
#ifdef CONFIG_SMP
                  syslog(LOG_INFO,
                         "%08lx: Task %u locked, CPU%u, priority %u, count=%u\n",
                         (unsigned long)systime, (unsigned int)pid,
                         (unsigned int)note->nc_cpu,
                         (unsigned int)note->nc_priority, (unsigned int)count);
#else
                  syslog(LOG_INFO,
                         "%08lx: Task %u locked, priority %u, count=%u\n",
                         (unsigned long)systime, (unsigned int)pid,
                         (unsigned int)note->nc_priority, (unsigned int)count);
#endif
                }
              else
                {
#ifdef CONFIG_SMP
                  syslog(LOG_INFO,
                         "%08lx: Task %u unlocked, CPU%u, priority %u, count=%u\n",
                         (unsigned long)systime, (unsigned int)pid,
                         (unsigned int)note->nc_cpu,
                         (unsigned int)note->nc_priority, (unsigned int)count);
#else
                  syslog(LOG_INFO,
                         "%08lx: Task %u unlocked, priority %u, count=%u\n",
                         (unsigned long)systime, (unsigned int)pid,
                         (unsigned int)note->nc_priority, (unsigned int)count);
#endif
                }
            }
            break;

#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_CSECTION
          case NOTE_CSECTION_ENTER:
          case NOTE_CSECTION_LEAVE:
            {
#ifdef CONFIG_SMP
              FAR struct note_csection_s *note_csection =
                (FAR struct note_csection_s *)note;
              uint16_t count;
#endif

              if (note->nc_length != sizeof(struct note_csection_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for csection note: %d\n",
                         note->nc_length);
                  return;
                }

#ifdef CONFIG_SMP
              count = (uint16_t) note_csection->ncs_count[0] +
                      (uint16_t)(note_csection->ncs_count[1] << 8);

              if (note->nc_type == NOTE_CSECTION_ENTER)
                {
                  syslog(LOG_INFO,
                         "%08lx: Task %u enter csection, CPU%u, priority %u, count=%u\n",
                         (unsigned long)systime, (unsigned int)pid,
                         (unsigned int)note->nc_cpu,
                         (unsigned int)note->nc_priority, (unsigned int)count);
                }
              else
                {
                  syslog(LOG_INFO,
                         "%08lx: Task %u leave csection, CPU%u, priority %u, count=%u\n",
                         (unsigned long)systime, (unsigned int)pid,
                         (unsigned int)note->nc_cpu,
                         (unsigned int)note->nc_priority, (unsigned int)count);
                }
#else
              if (note->nc_type == NOTE_CSECTION_ENTER)
                {
                  syslog(LOG_INFO, "%08lx: Task %u enter csection, priority %u\n",
                       (unsigned long)systime, (unsigned int)pid,
                       (unsigned int)note->nc_priority);
                }
              else
                {
                  syslog(LOG_INFO, "%08lx: Task %u leave csection, priority %u\n",
                       (unsigned long)systime, (unsigned int)pid,
                       (unsigned int)note->nc_priority);
                }
#endif
            }
            break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_SPINLOCKS
          case NOTE_SPINLOCK_LOCK:
          case NOTE_SPINLOCK_LOCKED:
          case NOTE_SPINLOCK_UNLOCK:
          case NOTE_SPINLOCK_ABORT:
            {
              FAR struct note_spinlock_s *note_spinlock =
                (FAR struct note_spinlock_s *)note;

              if (note->nc_length != sizeof(struct note_spinlock_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for spinlock note: %d\n",
                         note->nc_length);
                  return;
                }

             switch (note->nc_type)
               {
#ifdef CONFIG_SMP
                case NOTE_SPINLOCK_LOCK:
                  {
                    syslog(LOG_INFO,
                           "%08lx: Task %u CPU%u wait for spinlock=%p value=%u priority %u\n",
                           (unsigned long)systime, (unsigned int)pid,
                           (unsigned int)note->nc_cpu,
                           note_spinlock->nsp_spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_LOCKED:
                  {
                    syslog(LOG_INFO,
                           "%08lx: Task %u CPU%u has spinlock=%p value=%u priority %u\n",
                           (unsigned long)systime, (unsigned int)pid,
                           (unsigned int)note->nc_cpu,
                           note_spinlock->nsp_spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_UNLOCK:
                  {
                    syslog(LOG_INFO,
                           "%08lx: Task %u CPU%u unlocking spinlock=%p value=%u priority %u\n",
                           (unsigned long)systime, (unsigned int)pid,
                           (unsigned int)note->nc_cpu,
                           note_spinlock->nsp_spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_ABORT:
                  {
                    syslog(LOG_INFO,
                           "%08lx: Task %u CPU%u abort wait on spinlock=%p value=%u priority %u\n",
                           (unsigned long)systime, (unsigned int)pid,
                           (unsigned int)note->nc_cpu,
                           note_spinlock->nsp_spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;
#else
                case NOTE_SPINLOCK_LOCK:
                  {
                    syslog(LOG_INFO,
                           "%08lx: Task %u wait for spinlock=%p value=%u priority %u\n",
                           (unsigned long)systime, (unsigned int)pid,
                           note_spinlock->nsp_spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_LOCKED:
                  {
                    syslog(LOG_INFO,
                           "%08lx: Task %u has spinlock=%p value=%u priority %u\n",
                           (unsigned long)systime, (unsigned int)pid,
                           note_spinlock->nsp_spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_UNLOCK:
                  {
                    syslog(LOG_INFO,
                           "%08lx: Task %u unlocking spinlock=%p value=%u priority %u\n",
                           (unsigned long)systime, (unsigned int)pid,
                           (unsigned int)note->nc_cpu,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_ABORT:
                  {
                    syslog(LOG_INFO,
                           "%08lx: Task %u abort wait on spinlock=%p value=%u priority %u\n",
                           (unsigned long)systime, (unsigned int)pid,
                           note_spinlock->nsp_spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;
#endif
               }
#endif

          default:
            syslog(LOG_INFO, "Unrecognized note type: %d\n", note->nc_type);
            return;
        }

      offset += note->nc_length;
    }
}

/****************************************************************************
 * Name: note_daemon
 ****************************************************************************/

static int note_daemon(int argc, char *argv[])
{
  ssize_t nread;
  int fd;

  /* Indicate that we are running */

  g_note_daemon_started = true;
  syslog(LOG_INFO, "note_daemon: Running\n");

  /* Open the note driver */

  syslog(LOG_INFO, "note_daemon: Opening /dev/note\n");
  fd = open("/dev/note", O_RDONLY);
  if (fd < 0)
    {
      int errcode = errno;
      syslog(LOG_INFO, "note_daemon: ERROR: Failed to open /dev/note: %d\n",
             errcode);
      goto errout;
    }

  /* Now loop forever, dumping note data to the display */

  for (; ; )
    {
      nread = read(fd, g_note_buffer, CONFIG_SYSTEM_NOTE_BUFFERSIZE);
      if (nread > 0)
        {
          dump_notes(nread);
        }

      usleep(CONFIG_SYSTEM_NOTE_DELAY * 1000L);
    }

  (void)close(fd);

errout:
  g_note_daemon_started = false;

  syslog(LOG_INFO, "note_daemon: Terminating\n");
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * note_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int note_main(int argc, FAR char *argv[])
#endif
{
  int ret;

  printf("note_main: Starting the note_daemon\n");
  if (g_note_daemon_started)
    {
      printf("note_main: note_daemon already running\n");
      return EXIT_SUCCESS;
    }

  ret = task_create("note_daemon", CONFIG_SYSTEM_NOTE_PRIORITY,
                    CONFIG_SYSTEM_NOTE_STACKSIZE, note_daemon,
                    NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("note_main: ERROR: Failed to start note_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  printf("note_main: note_daemon started\n");
  return EXIT_SUCCESS;
}
