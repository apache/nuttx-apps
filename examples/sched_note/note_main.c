/****************************************************************************
 * examples/note/note_main.c
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
static uint8_t g_note_buffer[CONFIG_EXAMPLES_NOTE_BUFFERSIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dump_notes
 ****************************************************************************/

static void dump_notes(size_t nread)
{
  FAR struct note_common_s *note;
  unsigned int notelen;
  uint32_t systime;
  pid_t pid;
  off_t offset;

  offset = 0;
  while (offset < nread)
    {
      note    = (FAR struct note_common_s *)&g_note_buffer[offset];
      notelen = note->nc_length;
      systime = (uint32_t) note->nc_systime[0]        +
                (uint32_t)(note->nc_systime[1] << 8)  +
                (uint32_t)(note->nc_systime[1] << 16) +
                (uint32_t)(note->nc_systime[1] << 24);

      switch (note->nc_type)
        {
          case NOTE_START:
            {
              FAR struct note_start_s *note_start =
                (FAR struct note_start_s *)note;

              if (notelen < sizeof(struct note_start_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note too small for start note: %d\n",
                         notelen);
                  return;
                }

              pid = (pid_t) note_start->nst_pid[0] +
                    (pid_t)(note_start->nst_pid[1] << 8);

#if CONFIG_TASK_NAME_SIZE > 0
              syslog(LOG_INFO, "%08lx: Task %d \"%s\" started\n",
                     (unsigned long)systime, (int)pid,
                     note_start->nst_name);
#else
              syslog(LOG_INFO, "%08lx: Task %d started\n",
                     (unsigned long)systime, (int)pid);
#endif
            }
            break;

          case NOTE_STOP:
            {
              FAR struct note_stop_s *note_stop =
                (FAR struct note_stop_s *)note;

              if (notelen != sizeof(struct note_stop_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for stop note: %d\n",
                         notelen);
                  return;
                }

              pid = (pid_t) note_stop->nsp_pid[0] +
                    (pid_t)(note_stop->nsp_pid[1] << 8);

              syslog(LOG_INFO, "%08lx: Task %d stopped\n",
                     (unsigned long)systime, (int)pid);
            }
            break;

          case NOTE_SWITCH:
            {
              FAR struct note_switch_s *note_switch =
                (FAR struct note_switch_s *)note;
              pid_t pidin;
              pid_t pidout;

              if (notelen != sizeof(struct note_switch_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for switch note: %d\n",
                         notelen);
                  return;
                }

              pidout = (pid_t) note_switch->nsw_pidout[0] +
                       (pid_t)(note_switch->nsw_pidout[1] << 8);
              pidin  = (pid_t) note_switch->nsw_pidin[0] +
                       (pid_t)(note_switch->nsw_pidin[1] << 8);

              syslog(LOG_INFO, "%08lx: Task switch %d->%d\n",
                     (unsigned long)systime, (int)pidout, (int)pidin);
            }
            break;

#ifdef CONFIG_SCHED_INSTRUMENTATION_PREEMPTION
          case NOTE_PREEMPT_LOCK:
          case NOTE_PREEMPT_UNLOCK:
            {
              FAR struct note_preempt_s *note_preempt =
                (FAR struct note_preempt_s *)note;
              uint16_t count;

              if (notelen != sizeof(struct note_preempt_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for preemption note: %d\n",
                         notelen);
                  return;
                }

              pid   = (pid_t) note_preempt->npr_pid[0] +
                      (pid_t)(note_preempt->npr_pid[1] << 8);
              count = (uint16_t) note_preempt->npr_count[0] +
                      (uint16_t)(note_preempt->npr_count[1] << 8);

              if (note->nc_type == NOTE_PREEMPT_LOCK)
                {
                  syslog(LOG_INFO, "%08lx: Task %d locked, count=%u\n",
                       (unsigned long)systime, (int)pid,
                       (unsigned int)count);
                }
              else
                {
                  syslog(LOG_INFO, "%08lx: Task %d unlocked, count=%u\n",
                       (unsigned long)systime, (int)pid,
                       (unsigned int)count);
                }
            }
            break;

#endif
#ifdef CONFIG_SCHED_INSTRUMENTATION_CSECTION
          case NOTE_CSECTION_ENTER:
          case NOTE_CSECTION_LEAVE:
            {
              FAR struct note_csection_s *note_csection =
                (FAR struct note_csection_s *)note;
#ifdef CONFIG_SMP
              uint16_t count;
#endif

              if (notelen != sizeof(struct note_csection_s))
                {
                  syslog(LOG_INFO,
                         "ERROR: note size incorrect for csection note: %d\n",
                         notelen);
                  return;
                }

              pid   = (pid_t) note_csection->ncs_pid[0] +
                      (pid_t)(note_csection->ncs_pid[1] << 8);
#ifdef CONFIG_SMP
              count = (uint16_t) note_csection->ncs_count[0] +
                      (uint16_t)(note_csection->ncs_count[1] << 8);

              if (note->nc_type == NOTE_CSECTION_ENTER)
                {
                  syslog(LOG_INFO, "%08lx: Task %d enter csection, count=%u\n",
                       (unsigned long)systime, (int)pid,
                       (unsigned int)count);
                }
              else
                {
                  syslog(LOG_INFO, "%08lx: Task %d leave csection, count=%u\n",
                       (unsigned long)systime, (int)pid,
                       (unsigned int)count);
                }
#else
              if (note->nc_type == NOTE_CSECTION_ENTER)
                {
                  syslog(LOG_INFO, "%08lx: Task %d enter csection\n",
                       (unsigned long)systime, (int)pid);
                }
              else
                {
                  syslog(LOG_INFO, "%08lx: Task %d leave csection\n",
                       (unsigned long)systime, (int)pid);
                }
#endif
            }
            break;

#endif
          default:
            syslog(LOG_INFO, "Unrecognized note type: %d\n", note->nc_type);
            return;
        }

      offset += notelen;
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
      nread = read(fd, g_note_buffer, CONFIG_EXAMPLES_NOTE_BUFFERSIZE);
      if (nread > 0)
        {
          dump_notes(nread);
        }

      usleep(CONFIG_EXAMPLES_NOTE_DELAY * 1000L);
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

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int note_main(int argc, FAR char *argv[])
#endif
{
  FAR char *ledargv[2];
  int ret;

  printf("note_main: Starting the note_daemon\n");
  if (g_note_daemon_started)
    {
      printf("note_main: note_daemon already running\n");
      return EXIT_SUCCESS;
    }

  ledargv[0] = "note_daemon";
  ledargv[1] = NULL;

  ret = task_create("note_daemon", CONFIG_EXAMPLES_NOTE_PRIORITY,
                    CONFIG_EXAMPLES_NOTE_STACKSIZE, note_daemon,
                    (FAR char * const *)ledargv);
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
