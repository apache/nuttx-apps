/****************************************************************************
 * apps/lte/alt1250/alt1250_socket.c
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

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>

#include "alt1250_dbg.h"
#include "alt1250_daemon.h"
#include "alt1250_usockif.h"
#include "alt1250_socket.h"
#include "alt1250_select.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usocket_search
 ****************************************************************************/

FAR struct usock_s *usocket_search(FAR struct alt1250_s *dev, int usockid)
{
  FAR struct usock_s *ret = NULL;

  dbg_alt1250("%s usockid: %d\n", __func__, usockid);

  if (usockid >= 0 && usockid < nitems(dev->sockets))
    {
      ret = &dev->sockets[usockid];
    }

  return ret;
}

/****************************************************************************
 * name: usocket_alloc
 ****************************************************************************/

FAR struct usock_s *usocket_alloc(FAR struct alt1250_s *dev, int16_t type)
{
  int i;
  int base;
  int array_num;
  FAR struct usock_s *sock;

  switch (type)
    {
      case SOCK_CTRL:
      case SOCK_SMS:
        base = SOCKET_COUNT;
        array_num = CONFIG_LTE_ALT1250_CONTROL_SOCKETS;
        break;

      default:
        base = 0;
        array_num = SOCKET_COUNT;
        break;
    }

  sock = &dev->sockets[base];

  for (i = base; i < base + array_num; i++, sock++)
    {
      if (sock->state == SOCKET_STATE_CLOSED)
        {
          sock->usockid = i;
          sock->altsockid = -1;
          sock->state = SOCKET_STATE_PREALLOC;
          sock->select_condition = SELECT_WRITABLE | SELECT_READABLE;
          dbg_alt1250("allocated usockid: %d, type: %d\n", i, type);
          return sock;
        }
    }

  return NULL;
}

/****************************************************************************
 * name: usocket_free
 ****************************************************************************/

void usocket_free(FAR struct usock_s *sock)
{
  sock->state = SOCKET_STATE_CLOSED;
}

/****************************************************************************
 * name: usocket_freeall
 ****************************************************************************/

void usocket_freeall(FAR struct alt1250_s *dev)
{
  int i;
  FAR struct usock_s *sock;

  for (i = 0; i < nitems(dev->sockets); i++)
    {
      sock = &dev->sockets[i];
      usocket_free(sock);
    }
}

/****************************************************************************
 * name: usocket_commitstate
 ****************************************************************************/

void usocket_commitstate(FAR struct alt1250_s *dev)
{
  restart_select(dev);
}

/****************************************************************************
 * name: usocket_smssock_num
 ****************************************************************************/

int usocket_smssock_num(FAR struct alt1250_s *dev)
{
  int i;
  int num = 0;
  FAR struct usock_s *sock;

  for (i = 0; i < nitems(dev->sockets); i++)
    {
      sock = &dev->sockets[i];
      if (IS_SMS_SOCKET(sock) && sock->state == SOCKET_STATE_PREALLOC)
        {
          num++;
        }
    }

  return num;
}

/****************************************************************************
 * name: usocket_smssock_readready
 ****************************************************************************/

void usocket_smssock_readready(FAR struct alt1250_s *dev)
{
  int i;
  FAR struct usock_s *sock;

  for (i = 0; i < nitems(dev->sockets); i++)
    {
      sock = &dev->sockets[i];
      if (IS_SMS_SOCKET(sock) && sock->state == SOCKET_STATE_PREALLOC)
        {
          usockif_sendrxready(dev->usockfd, USOCKET_USOCKID(sock));
        }
    }
}

/****************************************************************************
 * name: usocket_smssock_abort
 ****************************************************************************/

void usocket_smssock_abort(FAR struct alt1250_s *dev)
{
  int i;
  FAR struct usock_s *sock;

  for (i = 0; i < nitems(dev->sockets); i++)
    {
      sock = &dev->sockets[i];
      if (IS_SMS_SOCKET(sock) && sock->state == SOCKET_STATE_PREALLOC)
        {
          usockif_sendabort(dev->usockfd, USOCKET_USOCKID(sock));
        }
    }
}
