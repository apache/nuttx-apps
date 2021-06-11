/****************************************************************************
 * apps/netutils/libcurl4nx/curl4nx_easy_init.c
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
#include <nuttx/compiler.h>
#include <debug.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "netutils/curl4nx.h"
#include "curl4nx_private.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: curl4nx_easy_init()
 ****************************************************************************/

FAR struct curl4nx_s * curl4nx_easy_init(void)
{
  FAR struct curl4nx_s *handle = malloc(sizeof(struct curl4nx_s));

  if (!handle)
    {
      curl4nx_err("Unable to allocate memory\n");
      return NULL;
    }

  curl4nx_info("handle=%p\n", handle);

  memset(handle, 0, sizeof(struct curl4nx_s));

  curl4nx_easy_reset(handle);

  return handle;
}
