/****************************************************************************
 * testing/irtest/cmdGeneral.cpp
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

#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>

#include "enum.hpp"
#include "cmd.hpp"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void wake(int)
{
  pthread_mutex_lock(&mutex);
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

static void print_cmd(const cmd *cmd)
{
  printf("%s(", cmd->name);
  for (int i = 0; cmd->args[i].name; i++)
    {
      printf(i ? ", %s" : "%s", cmd->args[i].name);
    }

  printf(")\n");
}

static int print_cmd(const char *name)
{
  const cmd *cmd = &__start_cmds;
  for (; cmd < &__stop_cmds; cmd++)
    {
      if (strcmp(name, cmd->name) == 0)
        {
          print_cmd(cmd);
          return 0;
        }
    }

  return -ENOENT;
}

static void print_all_cmds()
{
  const cmd *cmd = &__stop_cmds - 1;
  for (; cmd >= &__start_cmds; cmd--)
    {
      print_cmd(cmd);
    }
}

static void print_enum(const enum_type *e)
{
  printf("%s\n", e->type);
  for (int i = 0; e->value[i].name; i++)
    {
      printf(e->fmt, e->value[i].value);
      printf(" %s\n", e->value[i].name);
    }
}

static int print_enum(const char *type)
{
  const enum_type *e = &__start_enums;
  for (; e < &__stop_enums; e++)
    {
      if (strcmp(type, e->type) == 0)
        {
          print_enum(e);
          return 0;
        }
    }

  return -ENOENT;
}

static void print_all_enums()
{
  const enum_type *e = &__stop_enums - 1;
  for (; e >= &__start_enums; e--)
    {
      print_enum(e);
    }
}

static void print_cmd_and_enum(const cmd *cmd)
{
  print_cmd(cmd);
  for (int i = 0; cmd->args[i].type; i++)
    {
      print_enum(cmd->args[i].type);
    }
}

static int print_cmd_and_enum(const char *name)
{
  const cmd *cmd = &__start_cmds;
  for (; cmd < &__stop_cmds; cmd++)
    {
      if (strcmp(name, cmd->name) == 0)
        {
          print_cmd_and_enum(cmd);
          return 0;
        }
    }

  return -ENOENT;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

CMD0(quit)
{
  exit(0);
  return 0;
}

CMD1(sleep, float, seconds)
{
  int ret;

  pthread_mutex_lock(&mutex);
  signal(SIGINT, wake);
  if (seconds)
    {
      struct timeval now;
      struct timespec ts;

      gettimeofday(&now, NULL);
      ts.tv_sec = now.tv_sec + seconds;
      ts.tv_nsec = now.tv_usec * 1000;
      ret = pthread_cond_timedwait(&cond, &mutex, &ts);
    }
  else
    {
      ret = pthread_cond_wait(&cond, &mutex);
    }

  pthread_mutex_unlock(&mutex);
  signal(SIGINT, SIG_DFL);
  return ret == ETIMEDOUT ? 0 : ret;
}

CMD1(help, const char *, name)
{
  int r = 0;
  if (name != 0 && *name != 0)
    {
      r = print_cmd_and_enum(name);
    }
  else
    {
      print_all_enums();
      print_all_cmds();
    }

  return r;
}
