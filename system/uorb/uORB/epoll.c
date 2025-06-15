/****************************************************************************
 * apps/system/uorb/uORB/epoll.c
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

#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "internal.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int orb_loop_epoll_init(FAR struct orb_loop_s *loop);
static int orb_loop_epoll_run(FAR struct orb_loop_s *loop);
static int orb_loop_epoll_uninit(FAR struct orb_loop_s *loop);
static int orb_loop_epoll_enable(FAR struct orb_loop_s *loop,
                                 FAR struct orb_handle_s *handle, bool en);

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct orb_loop_ops_s g_orb_loop_epoll_ops =
{
  .init   = orb_loop_epoll_init,
  .run    = orb_loop_epoll_run,
  .uninit = orb_loop_epoll_uninit,
  .enable = orb_loop_epoll_enable,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int orb_loop_epoll_init(FAR struct orb_loop_s *loop)
{
  loop->fd = epoll_create1(EPOLL_CLOEXEC);
  if (loop->fd < 0)
    {
      return -errno;
    }

  return OK;
}

static int orb_loop_epoll_run(FAR struct orb_loop_s *loop)
{
  struct epoll_event et[CONFIG_UORB_LOOP_MAX_EVENTS];
  FAR struct orb_handle_s *handle;
  int nfds;
  int i;

  while (1)
    {
      nfds = epoll_wait(loop->fd, et, CONFIG_UORB_LOOP_MAX_EVENTS, -1);
      if (nfds == -1 && errno != EINTR)
        {
          return -errno;
        }

      for (i = 0; i < nfds; i++)
        {
          handle = et[i].data.ptr;
          if (handle == NULL)
            {
              continue;
            }
          else if (handle == &loop->exit_handle)
            {
              return OK;
            }

          if (et[i].events & EPOLLIN)
            {
              if (handle->datain_cb != NULL)
                {
                  handle->datain_cb(handle, handle->arg);
                }
              else
                {
                  uorberr("epoll wait data in error! fd:%d", handle->fd);
                }
            }
          else if (et[i].events & EPOLLOUT)
            {
              if (handle->dataout_cb != NULL)
                {
                  handle->dataout_cb(handle, handle->arg);
                }
              else
                {
                  uorberr("epoll wait data out error! fd:%d", handle->fd);
                }
            }
          else if (et[i].events & EPOLLPRI)
            {
              if (handle->eventpri_cb != NULL)
                {
                  handle->eventpri_cb(handle, handle->arg);
                }
              else
                {
                  uorberr("epoll wait events pri error! fd:%d", handle->fd);
                }
            }
          else if (et[i].events & EPOLLERR)
            {
              if (handle->eventerr_cb != NULL)
                {
                  handle->eventerr_cb(handle, handle->arg);
                }
              else
                {
                  uorberr("epoll wait events error! fd:%d", handle->fd);
                }
            }
        }
    }

  return OK;
}

static int orb_loop_epoll_uninit(FAR struct orb_loop_s *loop)
{
  int ret;

  ret = close(loop->fd);
  if (ret < 0)
    {
      return -errno;
    }

  return ret;
}

static int orb_loop_epoll_enable(FAR struct orb_loop_s *loop,
                                 FAR struct orb_handle_s *handle, bool en)
{
  struct epoll_event ev;
  int ret;

  if (en)
    {
      ev.events   = handle->events;
      ev.data.ptr = handle;
      ret = epoll_ctl(loop->fd, EPOLL_CTL_ADD, handle->fd, &ev);
    }
  else
    {
      ret = epoll_ctl(loop->fd, EPOLL_CTL_DEL, handle->fd, NULL);
    }

  if (ret < 0)
    {
      return -errno;
    }

  return ret;
}
