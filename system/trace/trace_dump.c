/****************************************************************************
 * apps/system/trace/trace_dump.c
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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/sched_note.h>
#include <nuttx/note/noteram_driver.h>

#include "trace.h"

#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
#  ifdef CONFIG_LIB_SYSCALL
#    include <syscall.h>
#  else
#    define CONFIG_LIB_SYSCALL
#    include <syscall.h>
#    undef CONFIG_LIB_SYSCALL
#  endif
#endif

#ifdef CONFIG_SMP
#  define NCPUS CONFIG_SMP_NCPUS
#else
#  define NCPUS 1
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Renumber idle task PIDs
 *  In NuttX, PID number less than NCPUS are idle tasks.
 *  In Linux, there is only one idle task of PID 0.
 */

#define get_pid(pid)  ((pid) < NCPUS ? 0 : (pid))

#define get_task_state(s) ((s) == 0 ? 'X' : \
                          ((s) <= LAST_READY_TO_RUN_STATE ? 'R' : 'S'))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The structure to hold the context data of trace dump */

struct trace_dump_cpu_context_s
{
  int intr_nest;          /* Interrupt nest level */
  bool pendingswitch;     /* sched_switch pending flag */
  int current_state;      /* Task state of the current line */
  pid_t current_pid;      /* Task PID of the current line */
  pid_t next_pid;         /* Task PID of the next line */
};

struct trace_dump_task_context_s
{
  FAR struct trace_dump_task_context_s *next;
  pid_t pid;                              /* Task PID */
  int syscall_nest;                       /* Syscall nest level */
  char name[CONFIG_TASK_NAME_SIZE + 1];   /* Task name (with NUL terminator) */
};

struct trace_dump_context_s
{
  struct trace_dump_cpu_context_s cpu[NCPUS];
  FAR struct trace_dump_task_context_s *task;
  int notefd;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: note_ioctl
 ****************************************************************************/

static void note_ioctl(int cmd, unsigned long arg)
{
  int notefd;

  notefd = open("/dev/note", O_RDONLY);
  if (notefd < 0)
    {
      fprintf(stderr,
             "trace: cannot open /dev/note\n");
      return;
    }

  ioctl(notefd, cmd, arg);
  close(notefd);
}

/****************************************************************************
 * Name: trace_dump_init_context
 ****************************************************************************/

static void trace_dump_init_context(FAR struct trace_dump_context_s *ctx,
                                   int fd)
{
  int cpu;

  /* Initialize the trace dump context */

  ctx->notefd = fd;

  for (cpu = 0; cpu < NCPUS; cpu++)
    {
      ctx->cpu[cpu].intr_nest = 0;
      ctx->cpu[cpu].pendingswitch = false;
      ctx->cpu[cpu].current_state = TSTATE_TASK_RUNNING;
      ctx->cpu[cpu].current_pid = -1;
      ctx->cpu[cpu].next_pid = -1;
    }

  ctx->task = NULL;
}

/****************************************************************************
 * Name: trace_dump_fini_context
 ****************************************************************************/

static void trace_dump_fini_context(FAR struct trace_dump_context_s *ctx)
{
  FAR struct trace_dump_task_context_s *tctx;
  FAR struct trace_dump_task_context_s *ntctx;

  /* Finalize the trace dump context */

  tctx = ctx->task;
  ctx->task = NULL;
  while (tctx != NULL)
    {
      ntctx = tctx->next;
      free(tctx);
      tctx = ntctx;
    }
}

/****************************************************************************
 * Name: copy_task_name
 ****************************************************************************/

#if CONFIG_TASK_NAME_SIZE > 0
static void copy_task_name(FAR char *dst, FAR const char *src)
{
  char c;
  int i;

  /* Replace space to underline
   * Text trace data format cannot use a space as a task name.
   */

  for (i = 0; i < CONFIG_TASK_NAME_SIZE; i++)
    {
      c = *src++;
      if (c == '\0')
        {
          break;
        }

      *dst++ = (c == ' ') ? '_' : c;
    }

  *dst = '\0';
}
#endif

/****************************************************************************
 * Name: get_task_context
 ****************************************************************************/

FAR static struct trace_dump_task_context_s *get_task_context(pid_t pid,
                                      FAR struct trace_dump_context_s *ctx)
{
  FAR struct trace_dump_task_context_s **tctxp;
  tctxp = &ctx->task;
  while (*tctxp != NULL)
    {
      if ((*tctxp)->pid == pid)
        {
          return *tctxp;
        }

      tctxp = &((*tctxp)->next);
    }

  /* Create new trace dump task context */

  *tctxp = (FAR struct trace_dump_task_context_s *)
           malloc(sizeof(struct trace_dump_task_context_s));
  if (*tctxp != NULL)
    {
      (*tctxp)->next = NULL;
      (*tctxp)->pid = pid;
      (*tctxp)->syscall_nest = 0;
      (*tctxp)->name[0] = '\0';

#if CONFIG_DRIVER_NOTERAM_TASKNAME_BUFSIZE > 0
        {
          struct noteram_get_taskname_s tnm;
          int res;

          tnm.pid = pid;
          res = ioctl(ctx->notefd, NOTERAM_GETTASKNAME, (unsigned long)&tnm);
          if (res == 0)
            {
              copy_task_name((*tctxp)->name, tnm.taskname);
            }
        }
#endif
    }

  return *tctxp;
}

/****************************************************************************
 * Name: get_task_name
 ****************************************************************************/

static const char *get_task_name(pid_t pid,
                                 FAR struct trace_dump_context_s *ctx)
{
  FAR struct trace_dump_task_context_s *tctx;

  tctx = get_task_context(pid, ctx);
  if (tctx != NULL && tctx->name[0] != '\0')
    {
      return tctx->name;
    }

  return "<noname>";
}

/****************************************************************************
 * Name: trace_dump_header
 ****************************************************************************/

static void trace_dump_header(FAR FILE *out,
                              FAR struct note_common_s *note,
                              FAR struct trace_dump_context_s *ctx)
{
  pid_t pid;
#ifdef CONFIG_SCHED_INSTRUMENTATION_HIRES
  uint32_t nsec = note->nc_systime_nsec[0] +
                  (note->nc_systime_nsec[1] << 8) +
                  (note->nc_systime_nsec[2] << 16) +
                  (note->nc_systime_nsec[3] << 24);
  uint32_t sec = note->nc_systime_sec[0] +
                 (note->nc_systime_sec[1] << 8) +
                 (note->nc_systime_sec[2] << 16) +
                 (note->nc_systime_sec[3] << 24);
#else
  uint32_t systime = note->nc_systime[0] +
                     (note->nc_systime[1] << 8) +
                     (note->nc_systime[2] << 16) +
                     (note->nc_systime[3] << 24);
#endif
#ifdef CONFIG_SMP
  int cpu = note->nc_cpu;
#else
  int cpu = 0;
#endif

  pid = ctx->cpu[cpu].current_pid;

  fprintf(out, "%8s-%-3u [%d] %3" PRIu32 ".%09" PRIu32 ": ",
          get_task_name(pid, ctx), get_pid(pid), cpu,
#ifdef CONFIG_SCHED_INSTRUMENTATION_HIRES
          sec, nsec
#else
          systime / (1000 * 1000 / CONFIG_USEC_PER_TICK),
          (systime % (1000 * 1000 / CONFIG_USEC_PER_TICK))
            * CONFIG_USEC_PER_TICK * 1000
#endif
         );
}

/****************************************************************************
 * Name: trace_dump_sched_switch
 ****************************************************************************/

static void trace_dump_sched_switch(FAR FILE *out,
                                    FAR struct note_common_s *note,
                                    FAR struct trace_dump_context_s *ctx)
{
  FAR struct trace_dump_cpu_context_s *cctx;
  pid_t current_pid;
  pid_t next_pid;
#ifdef CONFIG_SMP
  int cpu = note->nc_cpu;
#else
  int cpu = 0;
#endif

  cctx = &ctx->cpu[cpu];
  current_pid = cctx->current_pid;
  next_pid = cctx->next_pid;

  fprintf(out, "sched_switch: "
               "prev_comm=%s prev_pid=%u prev_state=%c ==> "
               "next_comm=%s next_pid=%u\n",
          get_task_name(current_pid, ctx), get_pid(current_pid),
          get_task_state(cctx->current_state),
          get_task_name(next_pid, ctx), get_pid(next_pid));

  cctx->current_pid = cctx->next_pid;
  cctx->pendingswitch = false;
}

/****************************************************************************
 * Name: trace_dump_one
 ****************************************************************************/

static int trace_dump_one(FAR FILE *out,
                          FAR uint8_t *p,
                          FAR struct trace_dump_context_s *ctx)
{
  FAR struct note_common_s *note = (FAR struct note_common_s *)p;
  FAR struct trace_dump_cpu_context_s *cctx;
  pid_t pid;
#ifdef CONFIG_SMP
  int cpu = note->nc_cpu;
#else
  int cpu = 0;
#endif

  cctx = &ctx->cpu[cpu];
  pid = note->nc_pid[0] + (note->nc_pid[1] << 8);

  if (cctx->current_pid < 0)
    {
      cctx->current_pid = pid;
    }

  /* Output one note */

  switch (note->nc_type)
    {
      case NOTE_START:
        {
#if CONFIG_TASK_NAME_SIZE > 0
          FAR struct note_start_s *nst = (FAR struct note_start_s *)p;
          FAR struct trace_dump_task_context_s *tctx;

          tctx = get_task_context(pid, ctx);
          if (tctx != NULL)
            {
              copy_task_name(tctx->name, nst->nst_name);
            }
#endif

          trace_dump_header(out, note, ctx);
          fprintf(out, "sched_wakeup_new: comm=%s pid=%d target_cpu=%d\n",
                  get_task_name(pid, ctx), get_pid(pid), cpu);
        }
        break;

      case NOTE_STOP:
        {
          /* This note informs the task to be stopped.
           * Change current task state for the succeeding NOTE_RESUME.
           */

          cctx->current_state = 0;
        }
        break;

      case NOTE_SUSPEND:
        {
          FAR struct note_suspend_s *nsu = (FAR struct note_suspend_s *)p;

          /* This note informs the task to be suspended.
           * Preserve the information for the succeeding NOTE_RESUME.
           */

          cctx->current_state = nsu->nsu_state;
        }
        break;

      case NOTE_RESUME:
        {
          /* This note informs the task to be resumed.
           * The task switch timing depends on the running context.
           */

          cctx->next_pid = pid;

          if (cctx->intr_nest == 0)
            {
              /* If not in the interrupt context, the task switch is
               * executed immediately.
               */

              trace_dump_header(out, note, ctx);
              trace_dump_sched_switch(out, note, ctx);
            }
          else
            {
              /* If in the interrupt context, the task switch is postponed
               * until leaving the interrupt handler.
               */

              trace_dump_header(out, note, ctx);
              fprintf(out, "sched_waking: comm=%s pid=%d target_cpu=%d\n",
                      get_task_name(cctx->next_pid, ctx),
                      get_pid(cctx->next_pid), cpu);
              cctx->pendingswitch = true;
            }
        }
        break;

#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
      case NOTE_SYSCALL_ENTER:
        {
          FAR struct note_syscall_enter_s *nsc;
          FAR struct trace_dump_task_context_s *tctx;
          int i;
          int j;
          uintptr_t arg;

          /* Exclude the case of syscall issued by an interrupt handler and
           * nested syscalls to correct tracecompass display.
           */

          if (cctx->intr_nest > 0)
            {
              break;
            }

          tctx = get_task_context(pid, ctx);
          if (tctx == NULL)
            {
              break;
            }

          tctx->syscall_nest++;
          if (tctx->syscall_nest > 1)
            {
              break;
            }

          nsc = (FAR struct note_syscall_enter_s *)p;
          if (nsc->nsc_nr < CONFIG_SYS_RESERVED ||
              nsc->nsc_nr >= SYS_maxsyscall)
            {
              break;
            }

          trace_dump_header(out, note, ctx);
          fprintf(out, "sys_%s(",
                  g_funcnames[nsc->nsc_nr - CONFIG_SYS_RESERVED]);

          for (i = j = 0; i < nsc->nsc_argc; i++)
            {
              arg = (uintptr_t)nsc->nsc_args[j++];
              arg |= (uintptr_t)nsc->nsc_args[j++] << 8;
#if UINTPTR_MAX > UINT16_MAX
              arg |= (uintptr_t)nsc->nsc_args[j++] << 16;
              arg |= (uintptr_t)nsc->nsc_args[j++] << 24;
#if UINTPTR_MAX > UINT32_MAX
              arg |= (uintptr_t)nsc->nsc_args[j++] << 32;
              arg |= (uintptr_t)nsc->nsc_args[j++] << 40;
              arg |= (uintptr_t)nsc->nsc_args[j++] << 48;
              arg |= (uintptr_t)nsc->nsc_args[j++] << 56;
#endif
#endif
              if (i == 0)
                {
                  fprintf(out, "arg%d: 0x%" PRIxPTR, i, arg);
                }
              else
                {
                  fprintf(out, ", arg%d: 0x%" PRIxPTR, i, arg);
                }
            }

          fprintf(out, ")\n");
        }
        break;

      case NOTE_SYSCALL_LEAVE:
        {
          FAR struct note_syscall_leave_s *nsc;
          FAR struct trace_dump_task_context_s *tctx;
          uintptr_t result;

          /* Exclude the case of syscall issued by an interrupt handler and
           * nested syscalls to correct tracecompass display.
           */

          if (cctx->intr_nest > 0)
            {
              break;
            }

          tctx = get_task_context(pid, ctx);
          if (tctx == NULL)
            {
              break;
            }

          tctx->syscall_nest--;
          if (tctx->syscall_nest > 0)
            {
              break;
            }

          tctx->syscall_nest = 0;

          nsc = (FAR struct note_syscall_leave_s *)p;
          if (nsc->nsc_nr < CONFIG_SYS_RESERVED ||
              nsc->nsc_nr >= SYS_maxsyscall)
            {
              break;
            }

          trace_dump_header(out, note, ctx);

          result =    (uintptr_t)nsc->nsc_result[0]
                   + ((uintptr_t)nsc->nsc_result[1] << 8)
#if UINTPTR_MAX > UINT16_MAX
                   + ((uintptr_t)nsc->nsc_result[2] << 16)
                   + ((uintptr_t)nsc->nsc_result[3] << 24)
#if UINTPTR_MAX > UINT32_MAX
                   + ((uintptr_t)nsc->nsc_result[4] << 32)
                   + ((uintptr_t)nsc->nsc_result[5] << 40)
                   + ((uintptr_t)nsc->nsc_result[6] << 48)
                   + ((uintptr_t)nsc->nsc_result[7] << 56)
#endif
#endif
          ;

          fprintf(out, "sys_%s -> 0x%" PRIxPTR "\n",
                  g_funcnames[nsc->nsc_nr - CONFIG_SYS_RESERVED],
                  result);
        }
        break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER
      case NOTE_IRQ_ENTER:
        {
          FAR struct note_irqhandler_s *nih;

          nih = (FAR struct note_irqhandler_s *)p;
          trace_dump_header(out, note, ctx);
          fprintf(out, "irq_handler_entry: irq=%u\n",
                  nih->nih_irq);
          cctx->intr_nest++;
        }
        break;

      case NOTE_IRQ_LEAVE:
        {
          FAR struct note_irqhandler_s *nih;

          nih = (FAR struct note_irqhandler_s *)p;
          trace_dump_header(out, note, ctx);
          fprintf(out, "irq_handler_exit: irq=%u\n",
                  nih->nih_irq);
          cctx->intr_nest--;

          if (cctx->intr_nest <= 0)
            {
              cctx->intr_nest = 0;
              if (cctx->pendingswitch)
                {
                  /* If the pending task switch exists, it is executed here */

                  trace_dump_header(out, note, ctx);
                  trace_dump_sched_switch(out, note, ctx);
                }
            }
        }
        break;
#endif

      default:
        break;
    }

  /* Return the length of the processed note */

  return note->nc_length;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trace_dump
 *
 * Description:
 *   Read notes and dump trace results.
 *
 ****************************************************************************/

int trace_dump(FAR FILE *out)
{
  struct trace_dump_context_s ctx;
  uint8_t tracedata[UCHAR_MAX];
  FAR uint8_t *p;
  int size;
  int ret;
  int fd;

  /* Open note for read */

  fd = open("/dev/note", O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr,
              "trace: cannot open /dev/note\n");
      return ERROR;
    }

  trace_dump_init_context(&ctx, fd);

  /* Read and output all notes */

  while (1)
    {
      ret = read(fd, tracedata, sizeof tracedata);
      if (ret <= 0)
        {
          break;
        }

      p = tracedata;
      do
        {
          size = trace_dump_one(out, p, &ctx);
          p += size;
          ret -= size;
        }
      while (ret > 0);
    }

  trace_dump_fini_context(&ctx);

  /* Close note */

  close(fd);

  return ret;
}

/****************************************************************************
 * Name: trace_dump_clear
 *
 * Description:
 *   Clear all contents of the buffer
 *
 ****************************************************************************/

void trace_dump_clear(void)
{
  note_ioctl(NOTERAM_CLEAR, 0);
}

/****************************************************************************
 * Name: trace_dump_get_overwrite
 *
 * Description:
 *   Get overwrite mode
 *
 ****************************************************************************/

bool trace_dump_get_overwrite(void)
{
  unsigned int mode = 0;

  note_ioctl(NOTERAM_GETMODE, (unsigned long)&mode);

  return mode == NOTERAM_MODE_OVERWRITE_ENABLE;
}

/****************************************************************************
 * Name: trace_dump_set_overwrite
 *
 * Description:
 *   Set overwrite mode
 *
 ****************************************************************************/

void trace_dump_set_overwrite(bool enable)
{
  unsigned int mode;

  mode = enable ? NOTERAM_MODE_OVERWRITE_ENABLE :
                  NOTERAM_MODE_OVERWRITE_DISABLE;

  note_ioctl(NOTERAM_SETMODE, (unsigned long)&mode);
}
