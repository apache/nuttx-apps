/****************************************************************************
 * apps/system/adb/shell_service.c
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

#include <stdlib.h>

#include "adb.h"
#include "shell_service.h"
#include "shell_pipe.h"

/****************************************************************************
 * Private types
 ****************************************************************************/

typedef struct ash_service_s
{
  adb_service_t service;
  shell_pipe_t pipe;
  adb_client_t *client;
} ash_service_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void exec_on_data_available(shell_pipe_t * pipe, apacket * p);

static int shell_ack(adb_service_t *service, apacket *p);
static int shell_write(adb_service_t *service, apacket *p);
static void shell_close(struct adb_service_s *service);
static void shell_kick(adb_service_t *service);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void exec_on_data_available(shell_pipe_t * pipe, apacket * p)
{
  ash_service_t *service = container_of(pipe, ash_service_t, pipe);

  if (p->msg.data_length <= 0)
    {
      /* Got EOF */

      adb_service_close(service->client, &service->service, p);
      return;
    }

  p->write_len = p->msg.data_length;
  p->msg.arg0 = service->service.id;
  p->msg.arg1 = service->service.peer_id;
  adb_send_data_frame(service->client, p);
}

static int shell_write(adb_service_t *service, apacket *p)
{
  int ret;
  ash_service_t *svc = container_of(service, ash_service_t, service);
  UNUSED(svc);

  if (p->msg.data_length <= 0)
    {
      return -1;
    }

  ret = shell_pipe_write(&svc->pipe, p->data, p->msg.data_length);

  if (ret < 0)
    {
      /* Shell process terminated, close service */

      return -1;
    }

  assert(ret == p->msg.data_length);
  return 0;
}

static int shell_ack(adb_service_t *service, apacket *p)
{
  UNUSED(service);
  UNUSED(p);
  return 0;
}

static void shell_kick(adb_service_t *service)
{
  int ret;
  ash_service_t *svc = container_of(service, ash_service_t, service);
  ret = shell_pipe_start(&svc->pipe, exec_on_data_available);

  /* TODO handle return code */

  assert(ret == 0);
}

static void shell_on_close(shell_pipe_t *pipe)
{
  ash_service_t *svc = container_of(pipe, ash_service_t, pipe);
  free(svc);
}

static void shell_close(adb_service_t *service)
{
  ash_service_t *svc = container_of(service, ash_service_t, service);

  /* FIXME missing logic here if shell process is still running */

  shell_pipe_destroy(&svc->pipe, shell_on_close);
}

static const adb_service_ops_t shell_ops =
{
  .on_write_frame = shell_write,
  .on_ack_frame   = shell_ack,
  .on_kick        = shell_kick,
  .close          = shell_close
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

adb_service_t * shell_service(adb_client_t *client, const char *params)
{
  UNUSED(params);
  UNUSED(client);

  int ret;
  char **argv;
  const char *target;
  ash_service_t *service =
      (ash_service_t *)malloc(sizeof(ash_service_t));

  if (service == NULL)
    {
      return NULL;
    }

  service->client = client;
  service->service.ops = &shell_ops;

  ret = shell_pipe_setup(client, &service->pipe);

  /* TODO check return code */

  assert(ret == 0);

  /* Check parameters after "shell:" */

  target = &params[6];

  if (target[0] != 0)
    {
      /* Build argv: <nsh -c "command">
       * argv[0] => "-c"
       * argv[1] => command
       * argv[2] => NULL
       *
       * malloc content:
       * - x3 argv pointers
       * - 3 characters: "-c\0"
       * - space for command string
       */

      argv = malloc(sizeof(char *) * 3 + 3 + (strlen(target)+1));

      argv[0] = (char *)&argv[3];
      argv[1] = &((char *)&argv[3])[3];
      argv[2] = NULL;
      strcpy(argv[0], "-c");
      strcpy(argv[1], target);
    }
  else
    {
      argv = NULL;
    }

  ret = shell_pipe_exec(argv, &service->pipe,
                        exec_on_data_available);

  free(argv);

  if (ret)
    {
      adb_log("failed to setup shell pipe %d\n", ret);
      free(service);
      return NULL;
    }

  return &service->service;
}
