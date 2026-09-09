/****************************************************************************
 * apps/examples/xrcedds/hello_world.h
 *
 * Declares the HelloWorld type described by HelloWorld.idl.
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

#ifndef __APPS_EXAMPLES_XRCEDDS_HELLO_WORLD_H
#define __APPS_EXAMPLES_XRCEDDS_HELLO_WORLD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct hello_world_s
{
  uint32_t index;
  char message[255];
};

struct ucdrBuffer;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

bool hello_world_serialize_topic(struct ucdrBuffer *writer,
                                 const struct hello_world_s *topic);
bool hello_world_deserialize_topic(struct ucdrBuffer *reader,
                                   struct hello_world_s *topic);
uint32_t hello_world_topic_size(const struct hello_world_s *topic,
                                uint32_t size);

#endif /* __APPS_EXAMPLES_XRCEDDS_HELLO_WORLD_H */
