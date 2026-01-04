/****************************************************************************
 * apps/examples/smf/hsm_psicc2_thread.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Glenn Andrews
 * State Machine example copyright (c) Miro Samek
 *
 * Implementation of the statechart in Figure 2.11 of
 * Practical UML Statecharts in C/C++, 2nd Edition by Miro Samek
 * https://www.state-machine.com/psicc2
 * Used with permission of the author.
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

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <system/smf.h>

#include "hsm_psicc2_thread.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct s_object
{
  struct smf_ctx ctx; /* MUST be first (matches SMF_CTX(obj)) */
  struct hsm_psicc2_event event;
  int foo;
};

enum demo_states
{
  STATE_INITIAL,
  STATE_S,
  STATE_S1,
  STATE_S2,
  STATE_S11,
  STATE_S21,
  STATE_S211
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct s_object s_obj;
static bool g_started;
static const struct smf_state demo_states[];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * STATE_INITIAL
 ****************************************************************************/

static void initial_entry(void *o)
{
  struct s_object *obj = (struct s_object *)o;

  printf("[psicc2] %s\n", __func__);
  obj->foo = false;
}

static enum smf_state_result initial_run(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
  return SMF_EVENT_PROPAGATE;
}

static void initial_exit(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

/****************************************************************************
 * STATE_S
 ****************************************************************************/

static void s_entry(void *o)
{
  (void)o;

  printf("[psicc2] %s", __func__);
}

static enum smf_state_result s_run(void *o)
{
  struct s_object *obj = (struct s_object *)o;

  printf("[psicc2] %s\n", __func__);

  switch (obj->event.event_id)
    {
      case EVENT_E:
        printf("[psicc2] %s received EVENT_E", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S11]);
        break;

      case EVENT_I:
        if (obj->foo)
          {
            printf("[psicc2] %s received EVENT_I and set foo false\n",
                   __func__);
            obj->foo = false;
          }
        else
          {
            printf("[psicc2] %s received EVENT_I and did nothing\n",
                   __func__);
          }

        return SMF_EVENT_HANDLED;

      case EVENT_TERMINATE:
        printf("[psicc2] %s received EVENT_TERMINATE. Terminating\n",
               __func__);
        smf_set_terminate(SMF_CTX(obj), -1);
        break;

      default:
        break;
    }

  return SMF_EVENT_PROPAGATE;
}

static void s_exit(void *o)
{
  (void)o;

  printf("%s", __func__);
}

/****************************************************************************
 * STATE_S1
 ****************************************************************************/

static void s1_entry(void *o)
{
  (void)o;

  printf("%s", __func__);
}

static enum smf_state_result s1_run(void *o)
{
  struct s_object *obj = (struct s_object *)o;

  printf("[psicc2] %s\n", __func__);

  switch (obj->event.event_id)
    {
      case EVENT_A:
        printf("[psicc2] %s received EVENT_A\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
        break;

      case EVENT_B:
        printf("[psicc2] %s received EVENT_B\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S11]);
        break;

      case EVENT_C:
        printf("[psicc2] %s received EVENT_C\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S2]);
        break;

      case EVENT_D:
        if (!obj->foo)
          {
            printf("[psicc2] %s received EVENT_D and acted on it\n",
                   __func__);
            obj->foo = true;
            smf_set_state(SMF_CTX(obj), &demo_states[STATE_S]);
          }
        else
          {
            printf("[psicc2] %s received EVENT_D and ignored it\n",
                   __func__);
          }
        break;

      case EVENT_F:
        printf("[psicc2] %s received EVENT_F\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S211]);
        break;

      case EVENT_I:
        printf("[psicc2] %s received EVENT_I\n", __func__);
        return SMF_EVENT_HANDLED;

      default:
        break;
    }

  return SMF_EVENT_PROPAGATE;
}

static void s1_exit(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

/****************************************************************************
 * STATE_S11
 ****************************************************************************/

static void s11_entry(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

static enum smf_state_result s11_run(void *o)
{
  struct s_object *obj = (struct s_object *)o;

  printf("[psicc2] %s\n", __func__);

  switch (obj->event.event_id)
    {
      case EVENT_D:
        if (obj->foo)
          {
            printf("[psicc2] %s received EVENT_D and acted upon it\n",
                   __func__);
            obj->foo = false;
            smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
          }
        else
          {
            printf("[psicc2] %s received EVENT_D and ignored it\n",
                   __func__);
          }
        break;

      case EVENT_G:
        printf("[psicc2] %s received EVENT_G\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S21]);
        break;

      case EVENT_H:
        printf("[psicc2] %s received EVENT_H\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S]);
        break;

      default:
        break;
    }

  return SMF_EVENT_PROPAGATE;
}

static void s11_exit(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

/****************************************************************************
 * STATE_S2
 ****************************************************************************/

static void s2_entry(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

static enum smf_state_result s2_run(void *o)
{
  struct s_object *obj = (struct s_object *)o;

  printf("[psicc2] %s\n", __func__);

  switch (obj->event.event_id)
    {
      case EVENT_C:
        printf("[psicc2] %s received EVENT_C\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
        break;

      case EVENT_F:
        printf("[psicc2] %s received EVENT_F\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S11]);
        break;

      case EVENT_I:
        if (!obj->foo)
          {
            printf("[psicc2] %s received EVENT_I and set foo true\n",
                   __func__);
            obj->foo = true;
            return SMF_EVENT_HANDLED;
          }
        else
          {
            printf("[psicc2] %s received EVENT_I and did nothing\n",
                   __func__);
          }
        break;

      default:
        break;
    }

  return SMF_EVENT_PROPAGATE;
}

static void s2_exit(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

/****************************************************************************
 * STATE_S21
 ****************************************************************************/

static void s21_entry(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

static enum smf_state_result s21_run(void *o)
{
  struct s_object *obj = (struct s_object *)o;

  printf("[psicc2] %s\n", __func__);

  switch (obj->event.event_id)
    {
      case EVENT_A:
        printf("[psicc2] %s received EVENT_A\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S21]);
        break;

      case EVENT_B:
        printf("[psicc2] %s received EVENT_B\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S211]);
        break;

      case EVENT_G:
        printf("[psicc2] %s received EVENT_G\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
        break;

      default:
        break;
    }

  return SMF_EVENT_PROPAGATE;
}

static void s21_exit(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

/****************************************************************************
 * STATE_S211
 ****************************************************************************/

static void s211_entry(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

static enum smf_state_result s211_run(void *o)
{
  struct s_object *obj = (struct s_object *)o;

  printf("[psicc2] %s\n", __func__);

  switch (obj->event.event_id)
    {
      case EVENT_D:
        printf("[psicc2] %s received EVENT_D\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S21]);
        break;

      case EVENT_H:
        printf("[psicc2] %s received EVENT_H\n", __func__);
        smf_set_state(SMF_CTX(obj), &demo_states[STATE_S]);
        break;

      default:
        break;
    }

  return SMF_EVENT_PROPAGATE;
}

static void s211_exit(void *o)
{
  (void)o;

  printf("[psicc2] %s\n", __func__);
}

/****************************************************************************
 * State Table
 ****************************************************************************/

static const struct smf_state demo_states[] =
{
  [STATE_INITIAL] = SMF_CREATE_STATE(
                    initial_entry,
                    initial_run,
                    initial_exit,
                    NULL,
                     &demo_states[STATE_S2]
                    ),
  [STATE_S]       = SMF_CREATE_STATE(
                    s_entry,
                    s_run,
                    s_exit,
                    &demo_states[STATE_INITIAL],
                    &demo_states[STATE_S11]
                  ),
  [STATE_S1]      = SMF_CREATE_STATE(
                    s1_entry,
                    s1_run,
                    s1_exit,
                    &demo_states[STATE_S],
                    &demo_states[STATE_S11]
                  ),
  [STATE_S2]      = SMF_CREATE_STATE(
                    s2_entry,
                    s2_run,
                    s2_exit,
                    &demo_states[STATE_S],
                    &demo_states[STATE_S211]
                  ),
  [STATE_S11]     = SMF_CREATE_STATE(
                    s11_entry,
                    s11_run,
                    s11_exit,
                    &demo_states[STATE_S1],
                    NULL
                  ),
  [STATE_S21]     = SMF_CREATE_STATE(
                    s21_entry,
                    s21_run,
                    s21_exit,
                    &demo_states[STATE_S2],
                    &demo_states[STATE_S211]
                  ),
  [STATE_S211]    = SMF_CREATE_STATE(
                    s211_entry,
                    s211_run,
                    s211_exit,
                    &demo_states[STATE_S21],
                    NULL
                  )
};

/****************************************************************************
 * Name: hsm_psicc2_thread_main
 ****************************************************************************/

static int hsm_psicc2_thread_main(int argc, char **argv)
{
  struct mq_attr attr;
  mqd_t mq;
  ssize_t n;
  int rc;

  (void)argc;
  (void)argv;

  memset(&attr, 0, sizeof(attr));
  attr.mq_maxmsg = CONFIG_EXAMPLES_SMF_EVENT_QUEUE_SIZE;
  attr.mq_msgsize = sizeof(struct hsm_psicc2_event);

  mq = mq_open(CONFIG_EXAMPLES_SMF_MQ_NAME, O_CREAT | O_RDONLY, 0666, &attr);
  if (mq == (mqd_t)-1)
    {
      printf("psicc2][ERR] mq_open(%s) failed: %d\n",
             CONFIG_EXAMPLES_SMF_MQ_NAME, errno);
      return -1;
    }

  printf("[psicc2] State Machine thread started\n");

  memset(&s_obj, 0, sizeof(s_obj));
  smf_set_initial(SMF_CTX(&s_obj), &demo_states[STATE_INITIAL]);

  while (true)
    {
      n = mq_receive(mq, (char *)&s_obj.event, sizeof(s_obj.event), NULL);
      if (n < 0)
        {
          printf("psicc2][ERR] mq_receive failed: %d\n", errno);
          continue;
        }

      rc = smf_run_state(SMF_CTX(&s_obj));
      if (rc != 0)
        {
          printf("[psicc2] SMF terminated (rc=%d). Exiting thread\n", rc);
          break;
        }
    }

  mq_close(mq);
  mq_unlink(CONFIG_EXAMPLES_SMF_MQ_NAME);

  g_started = false;
  memset(&s_obj, 0, sizeof(s_obj));

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hsm_psicc2_thread_start
 ****************************************************************************/

int hsm_psicc2_thread_start(void)
{
  int pid;

  if (g_started)
    {
      return 0;
    }

  pid = task_create("psicc2_thread",
                    CONFIG_EXAMPLES_SMF_PRIORITY,
                    CONFIG_EXAMPLES_SMF_STACKSIZE,
                    hsm_psicc2_thread_main,
                    NULL);
  if (pid < 0)
    {
      printf("psicc2][ERR] task_create failed: %d", errno);
      return -1;
    }

  g_started = true;
  return 0;
}

/****************************************************************************
 * Name: hsm_psicc2_post_event
 ****************************************************************************/

int hsm_psicc2_post_event(uint32_t event_id)
{
  struct hsm_psicc2_event ev;
  mqd_t mq;
  int rc;

  ev.event_id = event_id;

  mq = mq_open(CONFIG_EXAMPLES_SMF_MQ_NAME, O_WRONLY | O_NONBLOCK);
  if (mq == (mqd_t)-1)
    {
      printf("psicc2][ERR] mq_open(O_WRONLY) failed: %d "
             "(did you run 'hsm_psicc2 start'?)\n",
             errno);
      return -1;
    }

  rc = mq_send(mq, (const char *)&ev, sizeof(ev), 0);
  if (rc < 0)
    {
      printf("psicc2][ERR] mq_send failed: %d\n", errno);
      mq_close(mq);
      return -1;
    }

  mq_close(mq);
  return 0;
}
