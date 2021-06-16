/****************************************************************************
 * apps/graphics/pdcurs34/nuttx/pdcthread.c
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

#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>

#include <nuttx/kmalloc.h>
#include "pdcnuttx.h"
#include "graphics/curses.h"

/* We are including a private header here because we want the scheduler
 * PIDHASH macros and don't want to copy them manually, just in case they
 * change in the future.  Not sure this is a good design practice, but
 * at least it will track any changes to the PIDHASH macro.
 */

#include "../sched/sched/sched.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_pdc_initialized = false;

#ifdef CONFIG_PDCURSES_MULTITHREAD_HASH
static FAR struct pdc_context_s *g_pdc_ctx_per_pid[CONFIG_MAX_TASKS];
#else
static sem_t g_pdc_thread_sem;

static FAR struct pdc_context_s *g_pdc_last_ctx = NULL;
static FAR struct pdc_context_s *g_pdc_ctx_head = NULL;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_ctx_initialize
 *
 * Description:
 *   This function initializes the variables used for managing PDC contexts.
 *
 ****************************************************************************/

static void PDC_ctx_initialize(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD_HASH
  int x;

  /* Initialize all task context pointers to NULL */

  for (x = 0; x < CONFIG_MAX_TASKS; x++)
    {
      g_pdc_ctx_per_pid[x] = NULL;
    }

#else
  /* Initialize the semaphore */

  sem_init(&g_pdc_thread_sem, 0, 1);
#endif

  g_pdc_initialized = true;
}

/****************************************************************************
 * Name: PDC_ctx_new
 *
 * Description:
 *   Allocate and initialize a new pdc_context_s structure.
 *
 ****************************************************************************/

static FAR struct pdc_context_s *PDC_ctx_new(void)
{
  FAR struct pdc_context_s *ctx;
  int pid;

  /* Allocate a new structure */

  ctx = (FAR struct pdc_context_s *)kmm_zalloc(sizeof(struct pdc_context_s));
  if (ctx == NULL)
    {
      return NULL;
    }

  /* Initialize the fields */

  c_gindex       = 1;
  TABSIZE        = 8;
  COLOR_PAIRS    = PDC_COLOR_PAIRS;
  ctx->panel_ctx = pdc_alloc_panel_ctx();
  ctx->term_ctx  = pdc_alloc_term_ctx();

  /* Get our PID */

  pid = getpid();

#ifdef CONFIG_PDCURSES_MULTITHREAD_HASH

  pid = PIDHASH(pid);
  g_pdc_ctx_per_pid[pid] = ctx;

#else

  /* Add this context to the linked list */

  ctx->pid = pid;
  if (g_pdc_ctx_head == NULL)
    {
      /* We are the first in the list */

      g_pdc_ctx_head = ctx;
    }
  else
    {
      /* Put ourselves as the first entry */

      g_pdc_ctx_head->prev = ctx;
      ctx->next = g_pdc_ctx_head;
      g_pdc_ctx_head = ctx;
    }
#endif /* CONFIG_PDCURSES_MULTITHREAD_HASH */

  return ctx;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_ctx
 *
 * Description:
 *   Added pdcurses interface called from many functions to eliminate
 *   global and static function variable usage.  This function returns a
 *   task specific context / struct pointer to those variables instead,
 *   allowing the pdcurses routines to be used by multiple tasks in a
 *   FLAT build.
 *
 ****************************************************************************/

FAR struct pdc_context_s * PDC_ctx(void)
{
  FAR struct pdc_context_s *ctx;
  int pid;

  /* Ensure we are initialized */

  if (!g_pdc_initialized)
    {
      PDC_ctx_initialize();
    }

  /* Get our PID */

  pid = getpid();

#ifdef CONFIG_PDCURSES_MULTITHREAD_HASH

  pid = PIDHASH(pid);
  ctx = g_pdc_ctx_per_pid[pid];

  if (!ctx)
    {
      /* We must allocate and initialize a new context */

      ctx = PDC_ctx_new();
      g_pdc_ctx_per_pid[pid] = ctx;
    }

#else

  /* Take the semaphore */

  sem_wait(&g_pdc_thread_sem);

  /* Test if the last context was ours */

  if (g_pdc_last_ctx && g_pdc_last_ctx->pid == pid)
    {
      /* We found the context */

      ctx = g_pdc_last_ctx;
    }
  else
    {
      /* Search the list */

      ctx = g_pdc_ctx_head;
      while (ctx)
        {
          /* Test for match */

          if (ctx->pid == pid)
            {
              break;
            }

          /* Advance to next item in list */

          ctx = ctx->next;
        }

      /* Test if context found */

      if (ctx == NULL)
        {
          /* We must allocate and initialize a new context */

          ctx = PDC_ctx_new();
        }
    }

  /* Mark ourselves as the last context used and Post the semaphore */

  g_pdc_last_ctx = ctx;
  sem_post(&g_pdc_thread_sem);
#endif   /* CONFIG_PDCURSES_MULTITHREAD_HASH */

  return ctx;
}

/****************************************************************************
 * Name: PDC_ctx_free
 *
 * Description:
 *   Free the PDC_ctx context associated with the current PID (and remove
 *   it from the linked list).
 *
 ****************************************************************************/

void PDC_ctx_free(void)
{
  FAR struct pdc_context_s *ctx;

#ifdef CONFIG_PDCURSES_MULTITHREAD_HASH
  int pid;

  /* Get a unique hash key from the PID */

  pid = PIDHASH(getpid());
  ctx = g_pdc_ctx_per_pid[pid];

  /* Free the context memory */

  if (ctx != NULL)
    {
      free(ctx->panel_ctx);
      free(ctx->term_ctx);
      free(ctx);
      g_pdc_ctx_per_pid[pid] = NULL;
    }

#else

  /* Get a pointer to the context */

  ctx = PDC_ctx();
  if (ctx == NULL)
    {
      return;
    }

  /* Take the semaphore */

  sem_wait(&g_pdc_thread_sem);

  /* Remove ourselves from the linked list */

  if (ctx == g_pdc_ctx_head)
    {
      /* Make our next the new head */

      g_pdc_ctx_head = ctx->next;

      /* Make the head prev pointer NULL */

      if (g_pdc_ctx_head)
        {
          g_pdc_ctx_head->prev = NULL;
        }
    }
  else
    {
      /* Make our next point to our prev */

      if (ctx->next)
        {
          ctx->next->prev = ctx->prev;
        }

      /* Make our prev point to our next */

      if (ctx->prev)
        {
          ctx->prev->next = ctx->next;
        }
    }

  /* Release the semaphore */

  sem_post(&g_pdc_thread_sem);

  /* Free the memory */

  free(ctx->panel_ctx);
  free(ctx->term_ctx);
  free(ctx);
#endif /* CONFIG_PDCURSES_MULTITHREAD_HASH */
}
