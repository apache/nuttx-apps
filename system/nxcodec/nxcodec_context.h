/****************************************************************************
 * apps/system/nxcodec/nxcodec_context.h
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

#ifndef __APP_SYSTEM_NXCODEC_NXCODEC_CONTEXT_H
#define __APP_SYSTEM_NXCODEC_NXCODEC_CONTEXT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/videoio.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct nxcodec_context_buf_s
{
  FAR void           *addr;
  size_t             length;
  struct v4l2_buffer buf;
  bool               free;
} nxcodec_context_buf_t;

typedef struct nxcodec_context_s
{
  char                      filename[PATH_MAX];
  int                       fd;
  enum v4l2_buf_type        type;
  struct v4l2_format        format;
  FAR nxcodec_context_buf_t *buf;
  int                       nbuffers;
} nxcodec_context_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int nxcodec_context_init(FAR nxcodec_context_t *ctx);
int nxcodec_context_set_status(FAR nxcodec_context_t *ctx, uint32_t cmd);
int nxcodec_context_enqueue_frame(FAR nxcodec_context_t *ctx);
int nxcodec_context_dequeue_frame(FAR nxcodec_context_t *ctx);
int nxcodec_context_get_format(FAR nxcodec_context_t *ctx);
int nxcodec_context_set_format(FAR nxcodec_context_t *ctx);
void nxcodec_context_uninit(FAR nxcodec_context_t *ctx);

#endif /* __APP_SYSTEM_NXCODEC_NXCODEC_CONTEXT_H */
