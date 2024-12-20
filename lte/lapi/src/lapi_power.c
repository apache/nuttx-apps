/****************************************************************************
 * apps/lte/lapi/src/lapi_power.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <sys/param.h>
#include <nuttx/wireless/lte/lte_ioctl.h>

#include "lte/lte_api.h"
#include "lte/lapi.h"

#include "lapi_dbg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DAEMON_NAME     "alt1250"
#define DAEMON_PRI      100
#define DAEMON_STACK_SZ 2048
#define CMD_PREFIX      "-s"
#define ADDR_LEN        (strlen(CMD_PREFIX) + 9)  /* 32bit + '\0' */

#define RETRY_INTERVAL  100
#define RETRY_OVER      3

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int alt1250_main(int argc, FAR char *argv[]);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_lock = SEM_INITIALIZER(1);
static sem_t g_sync;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lapi_lock
 ****************************************************************************/

static inline void lapi_lock(FAR sem_t *lock)
{
  int ret;

  do
    {
      ret = sem_wait(lock);
    }
  while (ret == -EINTR);
}

/****************************************************************************
 * Name: lapi_unlock
 ****************************************************************************/

static inline void lapi_unlock(FAR sem_t *lock)
{
  sem_post(lock);
}

/****************************************************************************
 * Name: is_daemon_running
 ****************************************************************************/

static bool is_daemon_running(void)
{
  int sock;
  bool is_run = false;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      if (errno == ENETDOWN)
        {
          is_run = false;
        }
      else
        {
          is_run = true;
        }
    }
  else
    {
      close(sock);
      is_run = true;
    }

  return is_run;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lte_initialize
 ****************************************************************************/

int lte_initialize(void)
{
  int ret = 0;

  lapi_lock(&g_lock);

  if (!is_daemon_running())
    {
      FAR char *argv[2];
      char addr[ADDR_LEN];

      sem_init(&g_sync, 0, 0);

      /* address -> ascii */

      snprintf(addr, ADDR_LEN, "%s%08lx", CMD_PREFIX,
        (unsigned long)&g_sync);

      argv[0] = addr;
      argv[1] = NULL; /* termination */

      ret = task_create(DAEMON_NAME, DAEMON_PRI, DAEMON_STACK_SZ,
        alt1250_main, argv);
      if (ret < 0)
        {
          ret = -errno;
          lapi_printf("failed to create task:%d\n", errno);
        }
      else
        {
          ret = 0;
          sem_wait(&g_sync);
        }
    }
  else
    {
      ret = -EALREADY;
    }

  lapi_unlock(&g_lock);

  return ret;
}

/****************************************************************************
 * Name: lte_finalize
 ****************************************************************************/

int lte_finalize(void)
{
  int ret = 0;
  int count = 0;

  lapi_lock(&g_lock);

  while (count < RETRY_OVER)
    {
      ret = lapi_req(LTE_CMDID_FIN, NULL, 0, NULL, 0, NULL);
      if (ret >= 0)
        {
          sem_wait(&g_sync);
          sem_destroy(&g_sync);
          break;
        }

      usleep(RETRY_INTERVAL);
      count++;
    }

  if (ret == -ENETDOWN)
    {
      ret = -EALREADY;
    }

  lapi_unlock(&g_lock);

  return ret;
}

/****************************************************************************
 * Name: lte_set_report_restart
 ****************************************************************************/

int lte_set_report_restart(restart_report_cb_t callback)
{
  return lapi_req(LTE_CMDID_SETRESTART, NULL, 0, NULL, 0, callback);
}

/****************************************************************************
 * Name: lte_power_on
 ****************************************************************************/

int lte_power_on(void)
{
  return lapi_req(LTE_CMDID_POWERON, NULL, 0, NULL, 0, NULL);
}

/****************************************************************************
 * Name: lte_power_off
 ****************************************************************************/

int lte_power_off(void)
{
  int ret;
  int count = 0;

  while (count < RETRY_OVER)
    {
      ret = lapi_req(LTE_CMDID_POWEROFF, NULL, 0, NULL, 0, NULL);
      if ((ret >= 0) || (ret == -EALREADY))
        {
          break;
        }

      usleep(RETRY_INTERVAL);
      count++;
    }

  return ret;
}

/****************************************************************************
 * Name: lte_acquire_wakelock
 ****************************************************************************/

int lte_acquire_wakelock(void)
{
  return lapi_req(LTE_CMDID_TAKEWLOCK, NULL, 0, NULL, 0, NULL);
}

/****************************************************************************
 * Name: lte_release_wakelock
 ****************************************************************************/

int lte_release_wakelock(void)
{
  return lapi_req(LTE_CMDID_GIVEWLOCK, NULL, 0, NULL, 0, NULL);
}

/****************************************************************************
 * Name: lte_get_wakelock_count
 ****************************************************************************/

int lte_get_wakelock_count(void)
{
  return lapi_req(LTE_CMDID_COUNTWLOCK, NULL, 0, NULL, 0, NULL);
}

/****************************************************************************
 * Name: lte_set_context_save_cb
 ****************************************************************************/

int lte_set_context_save_cb(context_save_cb_t callback)
{
  return lapi_req(LTE_CMDID_SETCTXCB, NULL, 0, NULL, 0, callback);
}

/****************************************************************************
 * Name: lte_hibernation_resume
 ****************************************************************************/

int lte_hibernation_resume(FAR const uint8_t *res_ctx, int len)
{
  FAR void *inarg[] =
    {
      (FAR void *)res_ctx,
      &len
    };

  int dummy_arg; /* Dummy for blocking API call */

  if (res_ctx == NULL || len < 0)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_RESUME,
                  (FAR void *)inarg, nitems(inarg),
                  (FAR void *)&dummy_arg, 0, NULL);
}
