/****************************************************************************
 * apps/examples/nimble_bleprph/misc.c
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

#include "bleprph.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void print_addr(FAR const void *addr)
{
  FAR const uint8_t *u8p;

  u8p = addr;
  printf("%02x:%02x:%02x:%02x:%02x:%02x",
         u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}
