/****************************************************************************
 * apps/examples/tcp_ipc_server/protocol.h
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

#ifndef __APPS_EXAMPLES_SERVER_TCP_PROTOCOL_H
#define __APPS_EXAMPLES_SERVER_TCP_PROTOCOL_H

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct
{
  unsigned char opcode;
  unsigned char msg_size;
  unsigned char msg[12];
} protocolo_ipc;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void send_msg_to_lpwan (unsigned char *msg, protocolo_ipc *pt_protocol);

#endif /* __APPS_EXAMPLES_SERVER_TCP_PROTOCOL_H */
