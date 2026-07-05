/****************************************************************************
 * apps/examples/lely_slave/slave_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <canutils/lely/config.h>

#include <lely/co/dev.h>
#include <lely/co/nmt.h>
#include <lely/co/sdev.h>
#include <lely/co/sdo.h>
#include <lely/co/time.h>

#include "sdev.h"
#include "candev.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct co_net_s
{
  can_net_t *net;
  co_dev_t  *dev;
  co_nmt_t  *nmt;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct co_net_s g_net;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: elapsed_ms
 ****************************************************************************/

static long elapsed_ms(const struct timespec *a, const struct timespec *b)
{
  return (a->tv_sec - b->tv_sec) * 1000 +
         (a->tv_nsec - b->tv_nsec) / 1000000;
}

/****************************************************************************
 * Name: on_can_send
 ****************************************************************************/

static int on_can_send(const struct can_msg *msg, void *data)
{
  UNUSED(data);

  return coslave_candev_send(msg);
}

/****************************************************************************
 * Name: on_time
 ****************************************************************************/

static void on_time(co_time_t *time, const struct timespec *tp, void *data)
{
  UNUSED(time);
  UNUSED(data);

  /* Update the wall clock */

  clock_settime(CLOCK_REALTIME, tp);
}

/****************************************************************************
 * Name: on_nmt_cs
 ****************************************************************************/

static void on_nmt_cs(co_nmt_t *nmt, co_unsigned8_t cs, void *data)
{
  UNUSED(data);

  switch (cs)
    {
      case CO_NMT_CS_START:
      case CO_NMT_CS_ENTER_PREOP:
        {
          co_time_set_ind(co_nmt_get_time(nmt), &on_time, NULL);
          break;
        }

      case CO_NMT_CS_RESET_NODE:
        {
          exit(0);
          break;
        }

      case CO_NMT_CS_STOP:
      case CO_NMT_CS_RESET_COMM:
      default:
        {
          break;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct timespec now;
  struct timespec t_last;
  struct can_msg  msg;
  co_unsigned32_t counter = 0;
  int             ret;

  memset(&g_net, 0, sizeof(g_net));
  setvbuf(stdout, NULL, _IONBF, 0);

  ret = coslave_candev_init();
  if (ret < 0)
    {
      printf("ERROR: coslave_candev_init failed %d\n", ret);
      goto errout;
    }

  g_net.net = can_net_create();
  if (!g_net.net)
    {
      printf("ERROR: can_net_create failed\n");
      goto errout;
    }

  can_net_set_send_func(g_net.net, &on_can_send, NULL);

  clock_gettime(CLOCK_MONOTONIC, &now);
  can_net_set_time(g_net.net, &now);

  /* Create dev from struct */

  g_net.dev = co_dev_create_from_sdev(&g_sdev_slave);
  if (!g_net.dev)
    {
      printf("ERROR: co_dev_create_from_sdev failed\n");
      goto errout;
    }

  g_net.nmt = co_nmt_create(g_net.net, g_net.dev);
  if (!g_net.nmt)
    {
      printf("ERROR: co_nmt_create failed\n");
      goto errout;
    }

  /* Start the NMT service by resetting the node */

  co_nmt_cs_ind(g_net.nmt, CO_NMT_CS_RESET_NODE);

  /* Set the NMT indication function after the initial reset */

  co_nmt_set_cs_ind(g_net.nmt, &on_nmt_cs, NULL);

  printf("lely CANopen slave running (node 0x%02x)\n", g_sdev_slave.id);

  clock_gettime(CLOCK_MONOTONIC, &t_last);

  while (1)
    {
      clock_gettime(CLOCK_MONOTONIC, &now);
      can_net_set_time(g_net.net, &now);

      while (coslave_candev_recv(&msg) > 0)
        {
          can_net_recv(g_net.net, &msg);
        }

      /* Update the counter mapped into TPDO1 (0x2100) every 100 ms.
       * When the master has brought us to OPERATIONAL, the synchronous TPDO
       * sends the current value on each SYNC.
       */

      if (elapsed_ms(&now, &t_last) >= 100)
        {
          t_last = now;
          counter++;
          co_dev_set_val_u32(g_net.dev, 0x2100, 0x00, counter);
        }

      usleep(1000);
    }

errout:
  if (g_net.nmt)
    {
      co_nmt_destroy(g_net.nmt);
    }

  if (g_net.dev)
    {
      co_dev_destroy(g_net.dev);
    }

  if (g_net.net)
    {
      can_net_destroy(g_net.net);
    }

  return 0;
}
