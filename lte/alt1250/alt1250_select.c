/****************************************************************************
 * apps/lte/alt1250/alt1250_select.c
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

#include <stdint.h>
#include <string.h>

#include <nuttx/modem/alt1250.h>

#include "alt1250_dbg.h"
#include "alt1250_daemon.h"
#include "alt1250_socket.h"
#include "alt1250_usockif.h"
#include "alt1250_container.h"
#include "alt1250_devif.h"
#include "alt1250_evt.h"
#include "alt1250_usrsock_hdlr.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SELECT_CONTAINER_MAX    (2)
#define SELECT_MODE_NONBLOCK    (0)
#define SELECT_MODE_BLOCK       (1)
#define SELECT_MODE_BLOCKCANCEL (2)
#define READSET_BIT             (1 << 0)
#define WRITESET_BIT            (1 << 1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct select_params_s
{
  int32_t ret;
  int32_t err;
  int32_t id;
  altcom_fd_set readset;
  altcom_fd_set writeset;
  altcom_fd_set exceptset;
};

/****************************************************************************
 * Private Function Prototype
 ****************************************************************************/

static struct alt_container_s select_container_obj[SELECT_CONTAINER_MAX];
static FAR struct alt_container_s *g_current_container;
static FAR void *g_selectargs[SELECT_CONTAINER_MAX][6];
static struct select_params_s g_select_params[SELECT_CONTAINER_MAX];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: send_select_command
 ****************************************************************************/

static int send_select_command(FAR struct alt1250_s *dev,
                               int32_t mode, int32_t id, int32_t maxfds,
                               FAR altcom_fd_set *readset,
                               FAR altcom_fd_set *writeset,
                               FAR altcom_fd_set *exceptset)
{
  FAR void *in[7];
  uint16_t used_setbit = 0;
  int32_t usock_result;
  struct alt_container_s container =
  {
    0
  };

  if (readset)
    {
      used_setbit |= READSET_BIT;
    }

  if (writeset)
    {
      used_setbit |= WRITESET_BIT;
    }

  /* These members are referenced only when sending a command and
   * not when receiving a response, so local variables are used.
   */

  in[0] = &mode;
  in[1] = &id;
  in[2] = &maxfds;
  in[3] = &used_setbit;
  in[4] = readset;
  in[5] = writeset;
  in[6] = exceptset;

  set_container_ids(&container, 0, LTE_CMDID_SELECT);
  set_container_argument(&container, in, nitems(in));

  return altdevice_send_command(dev, dev->altfd, &container, &usock_result);
}

/****************************************************************************
 * Name: select_cancel
 ****************************************************************************/

static void select_cancel(FAR struct alt1250_s *dev)
{
  int32_t dummy_maxfds = 0;
  altcom_fd_set dummy_readset;
  altcom_fd_set dummy_writeset;
  altcom_fd_set dummy_exceptset;

  if (dev->sid != -1)
    {
      send_select_command(dev, SELECT_MODE_BLOCKCANCEL, dev->sid,
                          dummy_maxfds, &dummy_readset, &dummy_writeset,
                          &dummy_exceptset);
      dev->sid = -1;
    }
}

/****************************************************************************
 * name: select_start
 ****************************************************************************/

static void select_start(FAR struct alt1250_s *dev)
{
  int32_t maxfds = -1;
  altcom_fd_set readset;
  altcom_fd_set writeset;
  altcom_fd_set exceptset;
  FAR struct usock_s *usock;
  int i;

  dev->sid = (++dev->scnt & 0x7fffffff);

  ALTCOM_FD_ZERO(&readset);
  ALTCOM_FD_ZERO(&writeset);
  ALTCOM_FD_ZERO(&exceptset);

  for (i = 0; i < SOCKET_COUNT; i++)
    {
      usock = usocket_search(dev, i);
      if (usock && IS_STATE_SELECTABLE(usock))
        {
          if (IS_STATE_READABLE(usock))
            {
              ALTCOM_FD_SET(USOCKET_ALTSOCKID(usock), &readset);
            }

          if (IS_STATE_WRITABLE(usock))
            {
              ALTCOM_FD_SET(USOCKET_ALTSOCKID(usock), &writeset);
            }

          ALTCOM_FD_SET(USOCKET_ALTSOCKID(usock), &exceptset);

          maxfds = ((maxfds > USOCKET_ALTSOCKID(usock)) ? maxfds :
            USOCKET_ALTSOCKID(usock));
        }
    }

  if (maxfds != -1)
    {
      send_select_command(dev, SELECT_MODE_BLOCK, dev->sid,
                          maxfds + 1, &readset, &writeset, &exceptset);
    }
}

/****************************************************************************
 * name: recv_selectcontainer
 ****************************************************************************/

static FAR struct alt_container_s *recv_selectcontainer(
  FAR struct alt1250_s *dev)
{
  g_current_container = altdevice_exchange_selcontainer(dev->altfd,
                                                        g_current_container);
  return g_current_container;
}

/****************************************************************************
 * name: operate_dataavail
 ****************************************************************************/

static void operate_dataavail(FAR struct alt1250_s *dev,
                              FAR struct usock_s *usock,
                              FAR altcom_fd_set *readset,
                              FAR altcom_fd_set *writeset,
                              FAR altcom_fd_set *exceptset)
{
  if (IS_STATE_SELECTABLE(usock))
    {
      if (ALTCOM_FD_ISSET(USOCKET_ALTSOCKID(usock), exceptset))
        {
          dbg_alt1250("exceptset is set. usockid: %d\n",
                      USOCKET_USOCKID(usock));

          USOCKET_SET_STATE(usock, SOCKET_STATE_ABORTED);
        }

      if (ALTCOM_FD_ISSET(USOCKET_ALTSOCKID(usock), readset))
        {
          dbg_alt1250("readset is set. usockid: %d\n",
                      USOCKET_USOCKID(usock));

          USOCKET_CLR_SELECTABLE(usock, SELECT_READABLE);
          usockif_sendrxready(dev->usockfd, USOCKET_USOCKID(usock));
        }

      if (ALTCOM_FD_ISSET(USOCKET_ALTSOCKID(usock), writeset))
        {
          dbg_alt1250("writeset is set. usockid: %d\n",
                      USOCKET_USOCKID(usock));

          if (USOCKET_STATE(usock) == SOCKET_STATE_WAITCONN)
            {
              if (nextstep_check_connectres(dev, usock) == REP_NO_ACK_WOFREE)
                {
                  USOCKET_CLR_SELECTABLE(usock, SELECT_WRITABLE);
                }
            }
          else
            {
              USOCKET_CLR_SELECTABLE(usock, SELECT_WRITABLE);
              usockif_sendtxready(dev->usockfd, USOCKET_USOCKID(usock));
            }
        }
    }
}

/****************************************************************************
 * Name: handle_selectevt
 ****************************************************************************/

static void handle_selectevt(FAR struct alt1250_s *dev,
                             int32_t altcom_resp, int32_t modem_errno,
                             int32_t selectreq_id,
                             FAR altcom_fd_set *readset,
                             FAR altcom_fd_set *writeset,
                             FAR altcom_fd_set *exceptset)
{
  int i;
  FAR struct usock_s *usock;

  dbg_alt1250("select reply. ret=%ld modem_errno=%ld\n",
              altcom_resp, modem_errno);

  if (selectreq_id == dev->sid)
    {
      altcom_resp = COMBINE_ERRCODE(altcom_resp, modem_errno);
      if (altcom_resp >= 0)
        {
          for (i = 0; i < SOCKET_COUNT; i++)
            {
              usock = usocket_search(dev, i);
              operate_dataavail(dev, usock, readset, writeset, exceptset);
            }

          usocket_commitstate(dev);
        }
    }

  /* For debug */

  else
    {
      dbg_alt1250("Select event come wish in no selected. sel_id = %ld\n",
                  selectreq_id);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: perform_select_event
 ****************************************************************************/

uint64_t perform_select_event(FAR struct alt1250_s *dev, uint64_t bitmap)
{
  uint64_t bit = 0ULL;

  if (alt1250_checkcmdid(LTE_CMDID_SELECT, bitmap, &bit))
    {
      FAR struct alt_container_s *selectcontainer
          = recv_selectcontainer(dev);

      /* container->outparam[0]: return code
       * container->outparam[1]: error code
       * container->outparam[2]: select id
       * container->outparam[3]: readset
       * container->outparam[4]: writeset
       * container->outparam[5]: exceptset
       */

      handle_selectevt(dev,
                       *((FAR int32_t *)selectcontainer->outparam[0]),
                       *((FAR int32_t *)selectcontainer->outparam[1]),
                       *((FAR int32_t *)selectcontainer->outparam[2]),
                       (FAR altcom_fd_set *)(selectcontainer->outparam[3]),
                       (FAR altcom_fd_set *)(selectcontainer->outparam[4]),
                       (FAR altcom_fd_set *)(selectcontainer->outparam[5]));
    }

  return bit;
}

/****************************************************************************
 * name: init_selectcontainer
 ****************************************************************************/

void init_selectcontainer(FAR struct alt1250_s *dev)
{
  int i;

  memset(select_container_obj, 0, sizeof(select_container_obj));

  for (i = 0; i < SELECT_CONTAINER_MAX; i++)
    {
      g_selectargs[i][0] = &g_select_params[i].ret;
      g_selectargs[i][1] = &g_select_params[i].err;
      g_selectargs[i][2] = &g_select_params[i].id;
      g_selectargs[i][3] = &g_select_params[i].readset;
      g_selectargs[i][4] = &g_select_params[i].writeset;
      g_selectargs[i][5] = &g_select_params[i].exceptset;

      select_container_obj[i].outparam = g_selectargs[i];
      select_container_obj[i].outparamlen = nitems(g_selectargs[i]);
    }

  altdevice_exchange_selcontainer(dev->altfd, &select_container_obj[0]);
  g_current_container = &select_container_obj[1];
}

/****************************************************************************
 * name: restart_select
 ****************************************************************************/

void restart_select(FAR struct alt1250_s *dev)
{
  select_cancel(dev);
  select_start(dev);
}
