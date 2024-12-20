/****************************************************************************
 * apps/logging/nxscope/nxscope_internals.h
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

#ifndef __APPS_LOGGING_NXSCOPE_NXSCOPE_INTERNALS_H
#define __APPS_LOGGING_NXSCOPE_NXSCOPE_INTERNALS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <logging/nxscope/nxscope.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Max channel name length */

#define CHAN_NAMELEN_MAX (32)

/* Helpers */

#define PROTO_FRAME_FINAL(s, proto, id, buff, i)     \
  (s)->proto_stream->ops->frame_final(proto, id, buff, i)
#define PROTO_FRAME_GET(s, proto, buff, i, frame)    \
  (s)->proto_stream->ops->frame_get(proto, buff, i, frame)

#define INTF_SEND(s, intf, buff, i)             \
  (s)->intf_stream->ops->send(intf, buff, i)
#define INTF_RECV(s, intf, buff, i)             \
  (s)->intf_stream->ops->recv(intf, buff, i)

/****************************************************************************
 * Public Function Puttypes
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_stream_send
 *
 * Description:
 *   Send stream buffers
 *
 * Input Parameters:
 *   s      - a pointer to a nxscope instance
 *   buff   - buffer to send
 *   buff_i - buffer cursor
 *
 ****************************************************************************/

int nxscope_stream_send(FAR struct nxscope_s *s, FAR uint8_t *buff,
                        FAR size_t *buff_i);

#endif  /* __APPS_LOGGING_NXSCOPE_NXSCOPE_INTERNALS_H */
