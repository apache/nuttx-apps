/****************************************************************************
 * apps/include/logging/nxscope/nxscope_proto.h
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

#ifndef __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_PROTO_H
#define __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_PROTO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* Nxscope frame handler */

struct nxscope_frame_s
{
  uint8_t      id;              /* Frame id */
  size_t       drop;            /* Data to be droped from recv buffer */
  size_t       dlen;            /* Data len (without header and footer) */
  FAR uint8_t *data;            /* A pointer to a frame data */
};

/* Forward declaration */

struct nxscope_proto_s;

/* Nxscope protocol ops */

struct nxscope_proto_ops_s
{
  /* Get a frame from a buffer */

  CODE int (*frame_get)(FAR struct nxscope_proto_s *p,
                        FAR uint8_t *buff, size_t len,
                        FAR struct nxscope_frame_s *frame);

  /* Finalize a frame in a given buffer */

  CODE int (*frame_final)(FAR struct nxscope_proto_s *p,
                          uint8_t id,
                          FAR uint8_t *buff, FAR size_t *len);
};

/* Nxscope protocol handler */

struct nxscope_proto_s
{
  /* Initialized flag */

  bool initialized;

  /* Nxscope protocol private data */

  FAR void *priv;

  /* Nxscope protocol ops */

  FAR struct nxscope_proto_ops_s *ops;

  /* Header and foot size */

  size_t hdrlen;
  size_t footlen;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_LOGGING_NXSCOPE_PROTO_SER
/****************************************************************************
 * Name: nxscope_proto_ser_init
 ****************************************************************************/

int nxscope_proto_ser_init(FAR struct nxscope_proto_s *proto, FAR void *cfg);

/****************************************************************************
 * Name: nxscope_proto_ser_deinit
 ****************************************************************************/

void nxscope_proto_ser_deinit(FAR struct nxscope_proto_s *proto);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif  /* __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_PROTO_H */
