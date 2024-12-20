/****************************************************************************
 * apps/logging/nxscope/nxscope_internals.c
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

#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <stdlib.h>

#include <logging/nxscope/nxscope.h>

#include "nxscope_internals.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_stream_send
 *
 * Description:
 *   Send stream buffer
 *
 * Input Parameters:
 *   s      - a pointer to a nxscope instance
 *   buff   - buffer to send
 *   buff_i - buffer cursor
 *
 ****************************************************************************/

int nxscope_stream_send(FAR struct nxscope_s *s, FAR uint8_t *buff,
                        FAR size_t *buff_i)
{
  int ret = OK;

  DEBUGASSERT(s);
  DEBUGASSERT(buff);
  DEBUGASSERT(buff_i);

  /* Finalize stream frame */

  if (!s->stream_retry)
    {
      ret = PROTO_FRAME_FINAL(s, s->proto_stream,
                              NXSCOPE_HDRID_STREAM, buff, buff_i);
      if (ret < 0)
        {
          _err("ERROR: PROTO_FRAME_FINAL failed %d\n", ret);
          goto errout;
        }
    }

  /* Send stream data */

  ret = INTF_SEND(s, s->intf_stream, buff, *buff_i);
  if (ret < 0)
    {
      _err("ERROR: INTF_SEND failed %d\n", ret);
      s->stream_retry = true;
    }
  else
    {
      s->stream_retry = false;
    }

errout:
  return ret;
}
