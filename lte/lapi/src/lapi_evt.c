/****************************************************************************
 * apps/lte/lapi/src/lapi_evt.c
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
#include <strings.h>
#include <mqueue.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <nuttx/wireless/lte/lte_ioctl.h>

#include "lte/lte_api.h"
#include "lte/lapi.h"

#include "lapi_dbg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define EVENT_MAX 64

/****************************************************************************
 * Private Data
 ****************************************************************************/

static mqd_t g_mqd;
static struct lte_evtctx_out_s g_evtctx;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lapi_evtinit
 ****************************************************************************/

int lapi_evtinit(FAR const char *mqname)
{
  int ret = OK;
  struct mq_attr attr =
    {
      0
    };

  struct lte_evtctx_in_s in;
  FAR void *inarg[] =
    {
      &in
    };

  FAR void *outarg[] =
    {
      &g_evtctx
    };

  attr.mq_maxmsg = EVENT_MAX;
  attr.mq_msgsize = sizeof(uint64_t);

  g_mqd = mq_open(mqname, O_CREAT | O_RDONLY | O_EXCL, 0666, &attr);
  if (g_mqd == (mqd_t)-1)
    {
      lapi_printf("failed to open mq(%s): %d\n", mqname, errno);
      return -errno;
    }

  in.mqname = mqname;
  ret = lapi_req(LTE_CMDID_SETEVTCTX, (FAR void *)inarg, nitems(inarg),
    (FAR void *)outarg, nitems(outarg), NULL);
  if (ret < 0)
    {
      lapi_printf("failed to lapi request: %d\n", ret);
      mq_close(g_mqd);
      mq_unlink(mqname);
    }

  return ret;
}

/****************************************************************************
 * Name: lapi_evtdestoy
 ****************************************************************************/

void lapi_evtdestoy(void)
{
  mq_close(g_mqd);
}

/****************************************************************************
 * Name: lapi_evtyield
 ****************************************************************************/

int lapi_evtyield(int timeout_ms)
{
  int ret;
  ssize_t sz;
  uint64_t buf;
  struct timespec ts;
  bool is_exit = false;

  if (timeout_ms >= 0)
    {
      if (clock_gettime(CLOCK_REALTIME, &ts) != OK)
        {
          return -errno;
        }

      ts.tv_sec += timeout_ms / 1000;
      timeout_ms -= timeout_ms / 1000;
      ts.tv_nsec += timeout_ms * 1000 * 1000;
    }

  while (!is_exit)
    {
      if (timeout_ms >= 0)
        {
          sz = mq_timedreceive(g_mqd, (FAR void *)&buf, sizeof(buf), NULL,
            &ts);
          if (sz < 0)
            {
              ret = -errno;
              if (errno != ETIMEDOUT)
                {
                  lapi_printf("failed to mq_receive: %d\n", errno);
                }

              is_exit = true;
              continue;
            }
        }
      else
        {
          sz = mq_receive(g_mqd, (FAR void *)&buf, sizeof(buf), NULL);
          if (sz < 0)
            {
              ret = -errno;
              lapi_printf("failed to mq_receive: %d\n", errno);
              is_exit = true;
              continue;
            }
        }

      if (buf == 0ULL)
        {
          ret = OK;
          is_exit = true;
        }
      else
        {
          g_evtctx.handle(buf);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: lapi_evtsend
 ****************************************************************************/

int lapi_evtsend(uint64_t evtbitmap)
{
  int ret;

  ret = mq_send(g_mqd, (FAR const char *)&evtbitmap, sizeof(evtbitmap), 0);
  if (ret < 0)
    {
      ret = -errno;
      lapi_printf("failed to send mq: %d\n", errno);
    }

  return ret;
}
