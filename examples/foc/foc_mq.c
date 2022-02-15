/****************************************************************************
 * apps/examples/foc/foc_mq.c
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

#include <errno.h>
#include <strings.h>

#include "foc_debug.h"
#include "foc_mq.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_mq_handle
 ****************************************************************************/

int foc_mq_handle(mqd_t mq, FAR struct foc_mq_s *h)
{
  int      ret = OK;
  uint8_t  buffer[5];

  /* Get data from AUX */

  ret = mq_receive(mq, (char *)buffer, 5, 0);
  if (ret < 0)
    {
      if (errno != EAGAIN)
        {
          PRINTF("ERROR: mq_receive failed %d\n", errno);
          ret = -errno;
          goto errout;
        }
      else
        {
          /* Timeout */

          ret = OK;
          goto errout;
        }
    }

  /* Verify message length */

  if (ret != 5)
    {
      PRINTF("ERROR: invalid message length = %d\n", ret);
      goto errout;
    }

  /* Handle message */

  switch (buffer[0])
    {
      case CONTROL_MQ_MSG_VBUS:
        {
          memcpy(&h->vbus, &buffer[1], 4);
          break;
        }

      case CONTROL_MQ_MSG_APPSTATE:
        {
          memcpy(&h->app_state, &buffer[1], 4);
          break;
        }

      case CONTROL_MQ_MSG_SETPOINT:
        {
          memcpy(&h->setpoint, &buffer[1], 4);
          break;
        }

      case CONTROL_MQ_MSG_START:
        {
          h->start = true;
          break;
        }

      case CONTROL_MQ_MSG_KILL:
        {
          h->quit = true;
          break;
        }

      default:
        {
          PRINTF("ERROR: invalid message type %d\n", buffer[0]);
          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}
