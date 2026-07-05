/****************************************************************************
 * apps/examples/lely_master/master_main.c
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
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include <canutils/lely/config.h>

#include <lely/co/csdo.h>
#include <lely/co/dev.h>
#include <lely/co/nmt.h>
#include <lely/co/sdev.h>
#include <lely/co/sdo.h>
#include <lely/co/time.h>

#include "sdev.h"
#include "candev.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SLAVE_ID    0x02
#define OBJ_COUNTER 0x2100

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum master_phase_e
{
  PHASE_WAIT,   /* wait for the slave, then read its info via SDO */
  PHASE_SDO,    /* SDO request in flight */
  PHASE_RUN     /* network operational, streaming PDO data */
};

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
static bool g_sdo_done;

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

  return comaster_candev_send(msg);
}

/****************************************************************************
 * Name: nmt_start_remote
 ****************************************************************************/

static void nmt_start_remote(co_unsigned8_t id)
{
  struct can_msg msg;

  /* NMT command frame: COB-ID 0x000, {command specifier, node-ID} */

  memset(&msg, 0, sizeof(msg));
  msg.len     = 2;
  msg.data[0] = CO_NMT_CS_START;
  msg.data[1] = id;

  comaster_candev_send(&msg);
}

/****************************************************************************
 * Name: on_sdo_up
 ****************************************************************************/

static void on_sdo_up(co_csdo_t *sdo, co_unsigned16_t idx,
                      co_unsigned8_t subidx, co_unsigned32_t ac,
                      const void *ptr, size_t n, void *data)
{
  co_unsigned32_t val;

  UNUSED(sdo);
  UNUSED(idx);
  UNUSED(subidx);
  UNUSED(data);

  if (ac == 0 && n >= sizeof(co_unsigned32_t))
    {
      memcpy(&val, ptr, sizeof(val));
      printf("SDO: slave device type (0x1000) = 0x%08x\n", (unsigned)val);
    }
  else
    {
      printf("SDO: read failed, abort code 0x%08x\n", (unsigned)ac);
    }

  g_sdo_done = true;
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
  struct timespec  now;
  struct can_msg   msg;
  struct timespec  t0;
  co_unsigned32_t  last  = 0xffffffff;
  int              phase = PHASE_WAIT;
  int              ret;
  co_csdo_t       *csdo;
  co_unsigned32_t  v;

  memset(&g_net, 0, sizeof(g_net));
  g_sdo_done = false;
  setvbuf(stdout, NULL, _IONBF, 0);

  ret = comaster_candev_init();
  if (ret < 0)
    {
      printf("ERROR: comaster_candev_init failed %d\n", ret);
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

  g_net.dev = co_dev_create_from_sdev(&g_sdev_master);
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

  printf("lely CANopen master running (node 0x%02x)\n", g_sdev_master.id);

  clock_gettime(CLOCK_MONOTONIC, &t0);

  while (1)
    {
      clock_gettime(CLOCK_MONOTONIC, &now);
      can_net_set_time(g_net.net, &now);

      while (comaster_candev_recv(&msg) > 0)
        {
          can_net_recv(g_net.net, &msg);
        }

      switch (phase)
        {
          /* Give the slave time to boot, then read its device type with
           * a Client-SDO transfer (works while both nodes are pre-op).
           */

          case PHASE_WAIT:
            {
              if (elapsed_ms(&now, &t0) >= 500)
                {
                  csdo = co_nmt_get_csdo(g_net.nmt, 1);
                  if (csdo != NULL && co_csdo_up_req(
                        csdo, 0x1000, 0x00, &on_sdo_up, NULL) == 0)
                    {
                      phase = PHASE_SDO;
                    }
                  else
                    {
                      t0 = now;
                    }
                }

              break;
            }

          /* Once the SDO completes, bring the network to OPERATIONAL:
           * the master itself and the slave (NMT start).  PDO/SYNC then
           * flow.
           */

          case PHASE_SDO:
            {
              if (g_sdo_done)
                {
                  co_nmt_cs_ind(g_net.nmt, CO_NMT_CS_START);
                  nmt_start_remote(SLAVE_ID);
                  printf("NMT: master + slave 0x%02x OPERATIONAL\n",
                         SLAVE_ID);
                  phase = PHASE_RUN;
                }

              break;
            }

          /* The slave's synchronous TPDO lands in object 0x2100 via the
           * RPDO mapping; print it whenever it changes.
           */

          case PHASE_RUN:
            {
              v = co_dev_get_val_u32(g_net.dev, OBJ_COUNTER, 0x00);
              if (v != last)
                {
                  printf("PDO rx: counter = %u\n", (unsigned)v);
                  last = v;
                }

              break;
            }
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
