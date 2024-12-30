/****************************************************************************
 * apps/system/uorb/uORB/loop.c
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

#include "internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int orb_loop_init(FAR struct orb_loop_s *loop, enum orb_loop_type_e type)
{
  int ret = -EINVAL;

  if (loop == NULL)
    {
      return ret;
    }

  switch (type)
    {
      case ORB_EPOLL_TYPE:
        loop->ops = &g_orb_loop_epoll_ops;
        break;

      default:
        uorberr("loop register type error! type:%d", type);
        return ret;
    }

  ret = loop->ops->init(loop);
  if (ret < 0)
    {
      uorberr("loop init failed! ret:%d", ret);
      loop->ops = NULL;
    }

  return ret;
}

int orb_loop_run(FAR struct orb_loop_s *loop)
{
  return loop->ops->run(loop);
}

int orb_loop_deinit(FAR struct orb_loop_s *loop)
{
  int ret;

  ret = loop->ops->uninit(loop);
  if (ret >= 0)
    {
      loop->ops = NULL;
    }

  return ret;
}

int orb_handle_init(FAR struct orb_handle_s *handle, int fd, int events,
                    FAR void *arg, orb_datain_cb_t datain_cb,
                    orb_dataout_cb_t dataout_cb, orb_eventpri_cb_t pri_cb,
                    orb_eventerr_cb_t err_cb)
{
  if (fd < 0)
    {
      return -EINVAL;
    }

  handle->fd = fd;
  handle->arg    = arg;
  handle->events = events;
  handle->eventpri_cb = pri_cb;
  handle->eventerr_cb = err_cb;
  handle->datain_cb   = datain_cb;
  handle->dataout_cb  = dataout_cb;

  return OK;
}

int orb_handle_start(FAR struct orb_loop_s *loop,
                     FAR struct orb_handle_s *handle)
{
  return loop->ops->enable(loop, handle, true);
}

int orb_handle_stop(FAR struct orb_loop_s *loop,
                    FAR struct orb_handle_s *handle)
{
  return loop->ops->enable(loop, handle, false);
}
