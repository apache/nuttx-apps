/****************************************************************************
 * apps/examples/xrcedds/hello_world.c
 *
 * Implements the HelloWorld type described by HelloWorld.idl.
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

#include <string.h>

#include <ucdr/microcdr.h>

#include "hello_world.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool hello_world_serialize_topic(struct ucdrBuffer *writer,
                                 const struct hello_world_s *topic)
{
  ucdr_serialize_uint32_t(writer, topic->index);
  ucdr_serialize_string(writer, topic->message);

  return !writer->error;
}

bool hello_world_deserialize_topic(struct ucdrBuffer *reader,
                                   struct hello_world_s *topic)
{
  ucdr_deserialize_uint32_t(reader, &topic->index);
  ucdr_deserialize_string(reader, topic->message, 255);

  return !reader->error;
}

uint32_t hello_world_topic_size(const struct hello_world_s *topic,
                                uint32_t size)
{
  uint32_t previous_size = size;

  size += ucdr_alignment(size, 4) + 4;
  size += ucdr_alignment(size, 4) + 4 + strlen(topic->message) + 1;

  return size - previous_size;
}
