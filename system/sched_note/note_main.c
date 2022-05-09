/************************************************************************************
 * apps/system/sched_note/note_main.c
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
 ************************************************************************************/

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/sched_note.h>

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

#ifdef CONFIG_SCHED_INSTRUMENTATION_HIRES
#  define syslog_time(priority, fmt, ...) \
            syslog(priority, "%08lx.%08lx: " fmt, \
                   (unsigned long)systime_sec, (unsigned long)systime_nsec, \
                   __VA_ARGS__)
#else
#  define syslog_time(priority, fmt, ...) \
            syslog(priority, "%08lx: " fmt, \
                   (unsigned long)systime, \
                   __VA_ARGS__)
#endif

/************************************************************************************
 * Private Data
 ************************************************************************************/

static bool g_note_daemon_started;
static uint8_t g_note_buffer[CONFIG_SYSTEM_NOTE_BUFFERSIZE];

/* Names of task/thread states */

#ifdef CONFIG_SCHED_INSTRUMENTATION_SWITCH
static FAR const char *g_statenames[] =
{
  "Invalid",
  "Waiting for Unlock",
  "Ready",
  "Running",
  "Inactive",
  "Waiting for Semaphore",
  "Waiting for Signal",
#ifndef CONFIG_DISABLE_MQUEUE
  "Waiting for MQ empty",
  "Waiting for MQ full"
#endif
};

#define NSTATES (sizeof(g_statenames)/sizeof(FAR const char *))
#endif

/************************************************************************************
 * Private Functions
 ************************************************************************************/

/************************************************************************************
 * Name: trace_dump_unflatten
 ************************************************************************************/

static void trace_dump_unflatten(FAR void *dst,
                                 FAR uint8_t *src, size_t len)
{
#ifdef CONFIG_ENDIAN_BIG
  FAR uint8_t *end = (FAR uint8_t *)dst + len - 1;
  while (len-- > 0)
    {
      *end-- = *src++;
    }
#else
  memcpy(dst, src, len);
#endif
}

/************************************************************************************
 * Name: dump_notes
 ************************************************************************************/

static void dump_notes(size_t nread)
{
  FAR struct note_common_s *note;
#ifdef CONFIG_SCHED_INSTRUMENTATION_HIRES
  uint32_t systime_sec;
  uint32_t systime_nsec;
#else
  uint32_t systime;
#endif
  pid_t pid;
  off_t offset;

  offset = 0;
  while (offset < nread)
    {
      note    = (FAR struct note_common_s *)&g_note_buffer[offset];
      trace_dump_unflatten(&pid, note->nc_pid, sizeof(pid));
#ifdef CONFIG_SCHED_INSTRUMENTATION_HIRES
      trace_dump_unflatten(&systime_nsec,
                           note->nc_systime_nsec, sizeof(systime_nsec));
      trace_dump_unflatten(&systime_sec,
                           note->nc_systime_sec, sizeof(systime_sec));
#else
      trace_dump_unflatten(&systime, note->nc_systime, sizeof(systime));
#endif

      switch (note->nc_type)
        {
          case NOTE_START:
            {
              FAR struct note_start_s *note_start =
                (FAR struct note_start_s *)note;

              if (note->nc_length < sizeof(struct note_start_s))
                {
                  syslog(LOG_ERR,
                         "Note too small for \"Start\" note: %d\n",
                         note->nc_length);
                  return;
                }

#ifdef CONFIG_SMP
#if CONFIG_TASK_NAME_SIZE > 0
              syslog_time(LOG_INFO,
                     "Task %u \"%s\" started, CPU%u, priority %u\n",
                     (unsigned int)pid,
                     note_start->nst_name, (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
#else
              syslog_time(LOG_INFO,
                     "Task %u started, CPU%u, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
#endif
#else
#if CONFIG_TASK_NAME_SIZE > 0
              syslog_time(LOG_INFO,
                     "Task %u \"%s\" started, priority %u\n",
                     (unsigned int)pid,
                     note_start->nst_name, (unsigned int)note->nc_priority);
#else
              syslog_time(LOG_INFO,
                     "Task %u started, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_priority);
#endif
#endif
            }
            break;

          case NOTE_STOP:
            {
              if (note->nc_length != sizeof(struct note_stop_s))
                {
                  syslog(LOG_ERR,
                         "Size incorrect for \"Stop\" note: %d\n",
                         note->nc_length);
                  return;
                }

#ifdef CONFIG_SMP
              syslog_time(LOG_INFO,
                     "Task %u stopped, CPU%u, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_cpu, (unsigned int)note->nc_priority);
#else
              syslog_time(LOG_INFO,
                     "Task %u stopped, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_priority);
#endif
            }
            break;

#ifdef CONFIG_SCHED_INSTRUMENTATION_SWITCH
          case NOTE_SUSPEND:
            {
              FAR struct note_suspend_s *note_suspend =
                (FAR struct note_suspend_s *)note;
              FAR const char *statename;

              if (note->nc_length != sizeof(struct note_suspend_s))
                {
                  syslog(LOG_ERR,
                         "Size incorrect for \"Suspend\" note: %d\n",
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
              syslog_time(LOG_INFO,
                     "Task %u suspended, CPU%u, priority %u, state \"%s\"\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority, statename);
#else
              syslog_time(LOG_INFO,
                     "Task %u suspended, priority %u, state \"%s\"\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_priority, statename);
#endif
            }
            break;

          case NOTE_RESUME:
            {
              if (note->nc_length != sizeof(struct note_resume_s))
                {
                  syslog(LOG_ERR,
                         "Size incorrect for \"Resume\" note: %d\n",
                         note->nc_length);
                  return;
                }

#ifdef CONFIG_SMP
              syslog_time(LOG_INFO,
                     "Task %u resumed, CPU%u, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
#else
              syslog_time(LOG_INFO,
                     "Task %u resumed, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_priority);
#endif
            }
            break;
#endif

#ifdef CONFIG_SMP
          case NOTE_CPU_START:
            {
              FAR struct note_cpu_start_s *note_start =
                (FAR struct note_cpu_start_s *)note;

              if (note->nc_length != sizeof(struct note_cpu_start_s))
                {
                  syslog(LOG_ERR,
                         "Size incorrect for \"CPU Start\" note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog_time(LOG_INFO,
                     "Task %u CPU%u requests CPU%u to start, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note_start->ncs_target,
                     (unsigned int)note->nc_priority);
            }
            break;

          case NOTE_CPU_STARTED:
            {
              if (note->nc_length != sizeof(struct note_cpu_started_s))
                {
                  syslog(LOG_ERR,
                         "Size incorrect for \"CPU started\" note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog_time(LOG_INFO,
                     "Task %u CPU%u has started, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
            }
            break;

#ifdef CONFIG_SCHED_INSTRUMENTATION_SWITCH
          case NOTE_CPU_PAUSE:
            {
              FAR struct note_cpu_pause_s *note_pause =
                (FAR struct note_cpu_pause_s *)note;

              if (note->nc_length != sizeof(struct note_cpu_pause_s))
                {
                  syslog(LOG_ERR,
                         "Size incorrect for \"CPU pause\" note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog_time(LOG_INFO,
                     "Task %u CPU%u requests CPU%u to pause, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note_pause->ncp_target,
                     (unsigned int)note->nc_priority);
            }
            break;

          case NOTE_CPU_PAUSED:
            {
              if (note->nc_length != sizeof(struct note_cpu_paused_s))
                {
                  syslog(LOG_ERR,
                         "Size incorrect for \"CPU paused\" note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog_time(LOG_INFO,
                     "Task %u CPU%u has paused, priority %u\n",
                     (unsigned int)pid,
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
                  syslog(LOG_ERR,
                         "Size incorrect for \"CPU resume\" note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog_time(LOG_INFO,
                     "Task %u CPU%u requests CPU%u to resume, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note_resume->ncr_target,
                     (unsigned int)note->nc_priority);
            }
            break;

          case NOTE_CPU_RESUMED:
            {
              if (note->nc_length != sizeof(struct note_cpu_resumed_s))
                {
                  syslog(LOG_ERR,
                         "Size incorrect for \"CPU resumed\" note: %d\n",
                         note->nc_length);
                  return;
                }

              syslog_time(LOG_INFO,
                     "Task %u CPU%u has resumed, priority %u\n",
                     (unsigned int)pid,
                     (unsigned int)note->nc_cpu,
                     (unsigned int)note->nc_priority);
            }
            break;
#endif
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
                  syslog(LOG_ERR,
                         "Size incorrect for \"Preemption\" note: %d\n",
                         note->nc_length);
                  return;
                }

              trace_dump_unflatten(&count, note_preempt->npr_count, sizeof(count));

              if (note->nc_type == NOTE_PREEMPT_LOCK)
                {
#ifdef CONFIG_SMP
                  syslog_time(LOG_INFO,
                         "Task %u locked, CPU%u, priority %u, count=%u\n",
                         (unsigned int)pid,
                         (unsigned int)note->nc_cpu,
                         (unsigned int)note->nc_priority, (unsigned int)count);
#else
                  syslog_time(LOG_INFO,
                         "Task %u locked, priority %u, count=%u\n",
                         (unsigned int)pid,
                         (unsigned int)note->nc_priority, (unsigned int)count);
#endif
                }
              else
                {
#ifdef CONFIG_SMP
                  syslog_time(LOG_INFO,
                         "Task %u unlocked, CPU%u, priority %u, count=%u\n",
                         (unsigned int)pid,
                         (unsigned int)note->nc_cpu,
                         (unsigned int)note->nc_priority, (unsigned int)count);
#else
                  syslog_time(LOG_INFO,
                         "Task %u unlocked, priority %u, count=%u\n",
                         (unsigned int)pid,
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
                  syslog(LOG_ERR,
                         "Size incorrect for \"csection\" note: %d\n",
                         note->nc_length);
                  return;
                }

#ifdef CONFIG_SMP
              trace_dump_unflatten(&count, note_csection->ncs_count, sizeof(count));

              if (note->nc_type == NOTE_CSECTION_ENTER)
                {
                  syslog_time(LOG_INFO,
                         "Task %u enter csection, CPU%u, priority %u, "
                         "count=%u\n",
                         (unsigned int)pid,
                         (unsigned int)note->nc_cpu,
                         (unsigned int)note->nc_priority, (unsigned int)count);
                }
              else
                {
                  syslog_time(LOG_INFO,
                         "Task %u leave csection, CPU%u, priority %u, "
                         "count=%u\n",
                         (unsigned int)pid,
                         (unsigned int)note->nc_cpu,
                         (unsigned int)note->nc_priority, (unsigned int)count);
                }
#else
              if (note->nc_type == NOTE_CSECTION_ENTER)
                {
                  syslog_time(LOG_INFO, "Task %u enter csection, priority %u\n",
                       (unsigned int)pid,
                       (unsigned int)note->nc_priority);
                }
              else
                {
                  syslog_time(LOG_INFO, "Task %u leave csection, priority %u\n",
                       (unsigned int)pid,
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
              FAR void *spinlock;

              if (note->nc_length != sizeof(struct note_spinlock_s))
                {
                  syslog(LOG_ERR,
                         "Size incorrect for \"Spinlock\" note: %d\n",
                         note->nc_length);
                  return;
                }

              trace_dump_unflatten(&spinlock,
                                   note_spinlock->nsp_spinlock,
                                   sizeof(spinlock));

             switch (note->nc_type)
               {
#ifdef CONFIG_SMP
                case NOTE_SPINLOCK_LOCK:
                  {
                    syslog_time(LOG_INFO,
                           "Task %u CPU%u wait for spinlock=%p value=%u "
                           "priority %u\n",
                           (unsigned int)pid,
                           (unsigned int)note->nc_cpu,
                           spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_LOCKED:
                  {
                    syslog_time(LOG_INFO,
                           "Task %u CPU%u has spinlock=%p value=%u "
                           "priority %u\n",
                           (unsigned int)pid,
                           (unsigned int)note->nc_cpu,
                           spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_UNLOCK:
                  {
                    syslog_time(LOG_INFO,
                           "Task %u CPU%u unlocking spinlock=%p value=%u "
                           "priority %u\n",
                           (unsigned int)pid,
                           (unsigned int)note->nc_cpu,
                           spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_ABORT:
                  {
                    syslog_time(LOG_INFO,
                           "Task %u CPU%u abort wait on spinlock=%p value=%u "
                           "priority %u\n",
                           (unsigned int)pid,
                           (unsigned int)note->nc_cpu,
                           spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;
#else
                case NOTE_SPINLOCK_LOCK:
                  {
                    syslog_time(LOG_INFO,
                           "Task %u wait for spinlock=%p value=%u "
                           "priority %u\n",
                           (unsigned int)pid,
                           spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_LOCKED:
                  {
                    syslog_time(LOG_INFO,
                           "Task %u has spinlock=%p value=%u priority %u\n",
                           (unsigned int)pid,
                           spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_UNLOCK:
                  {
                    syslog_time(LOG_INFO,
                           "Task %u unlocking spinlock=%p value=%u "
                           "priority %u\n",
                           (unsigned int)pid,
                           spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;

                case NOTE_SPINLOCK_ABORT:
                  {
                    syslog_time(LOG_INFO,
                           "Task %u abort wait on spinlock=%p value=%u "
                           "priority %u\n",
                           (unsigned int)pid,
                           spinlock,
                           (unsigned int)note_spinlock->nsp_value,
                           (unsigned int)note->nc_priority);
                  }
                  break;
#endif
               }
              break;
            }
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
                case NOTE_SYSCALL_ENTER:
                  {
                    FAR struct note_syscall_enter_s *note_sysenter =
                      (FAR struct note_syscall_enter_s *)note;

                    if (note->nc_length < SIZEOF_NOTE_SYSCALL_ENTER(0))
                      {
                        syslog(LOG_ERR,
                               "Size incorrect for \"SYSCALL enter\" note: %d\n",
                               note->nc_length);
                        return;
                      }

                    syslog_time(LOG_INFO,
                           "Task %u Enter SYSCALL %d\n",
                           (unsigned int)pid,
                           note_sysenter->nsc_nr);
                  }
                  break;

                case NOTE_SYSCALL_LEAVE:
                  {
                    FAR struct note_syscall_leave_s *note_sysleave =
                      (FAR struct note_syscall_leave_s *)note;
                    uintptr_t result;

                    if (note->nc_length != sizeof(struct note_syscall_leave_s))
                      {
                        syslog(LOG_ERR,
                               "Size incorrect for \"SYSCALL leave\" note: %d\n",
                               note->nc_length);
                        return;
                      }

                    trace_dump_unflatten(&result,
                                         note_sysleave->nsc_result,
                                         sizeof(result));

                    syslog_time(LOG_INFO,
                           "Task %u Leave SYSCALL %d: %" PRIdPTR "\n",
                           (unsigned int)pid,
                           note_sysleave->nsc_nr, result);
                  }
                  break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER
                case NOTE_IRQ_ENTER:
                case NOTE_IRQ_LEAVE:
                  {
                    FAR struct note_irqhandler_s *note_irq =
                      (FAR struct note_irqhandler_s *)note;

                    if (note->nc_length != sizeof(struct note_irqhandler_s))
                      {
                        syslog(LOG_ERR,
                               "Size incorrect for \"IRQ\" note: %d\n",
                               note->nc_length);
                        return;
                      }

                    syslog_time(LOG_INFO,
                           "Task %u %s IRQ %d\n",
                           (unsigned int)pid,
                           note->nc_type == NOTE_IRQ_ENTER ? "Enter" : "Leave",
                           note_irq->nih_irq);
                  }
                  break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_DUMP
                case NOTE_DUMP_STRING:
                  {
                    FAR struct note_string_s *note_string =
                      (FAR struct note_string_s *)note;

                    if (note->nc_length < sizeof(struct note_string_s))
                      {
                        syslog(LOG_INFO,
                               "ERROR: note too small for string note: %d\n",
                               note->nc_length);
                        return;
                      }

                    syslog_time(LOG_INFO,
                           "Task %u priority %u, string:%s\n",
                           (unsigned int)pid,
                           (unsigned int)note->nc_priority,
                           note_string->nst_data);
                  }
                  break;

                case NOTE_DUMP_BINARY:
                  {
                    FAR struct note_binary_s *note_binary =
                      (FAR struct note_binary_s *)note;
                    uintptr_t ip;
                    char out[1280];
                    int count;
                    int ret = 0;
                    int i;

                    count = note->nc_length - sizeof(struct note_binary_s) + 1;

                    if (count < 0)
                      {
                        syslog(LOG_INFO,
                               "ERROR: note too small for binary note: %d\n",
                               note->nc_length);
                        return;
                      }

                    for (i = 0; i < count; i++)
                      {
                        ret += sprintf(&out[ret], " 0x%x", note_binary->nbi_data[i]);
                      }

                    trace_dump_unflatten(&ip, note_binary->nbi_ip,
                                         sizeof(ip));

                    syslog_time(LOG_INFO,
                           "Task %u priority %u, ip=0x%" PRIdPTR
                            " event=%u count=%u%s\n",
                           (unsigned int)pid,
                           (unsigned int)note->nc_priority,
                           note_binary->nbi_ip,
                           note_binary->nbi_event,
                           count,
                           out);
                  }
                  break;
#endif

          default:
            syslog(LOG_INFO, "Unrecognized note type: %d\n", note->nc_type);
            return;
        }

      offset += note->nc_length;
    }
}

/************************************************************************************
 * Name: note_daemon
 ************************************************************************************/

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
      syslog(LOG_ERR, "note_daemon: ERROR: Failed to open /dev/note: %d\n",
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

  close(fd);

errout:
  g_note_daemon_started = false;

  syslog(LOG_INFO, "note_daemon: Terminating\n");
  return EXIT_FAILURE;
}

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: note_main
 ************************************************************************************/

int main(int argc, FAR char *argv[])
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
