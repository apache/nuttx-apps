/****************************************************************************
 * apps/interpreters/wamr/wamr_custom_init.c
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>
#include <sys/param.h>
#include <sys/types.h>
#include <iconv.h>

#include "wasm_native.h"

#include "wamr_custom_init.h"

#ifdef CONFIG_INTERPRETERS_WAMR_EXTERNAL_MODULE_REGISTRY
#include "wamr_external_module_proto.h"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

typedef bool (*module_register_t)(void);

#ifdef CONFIG_INTERPRETERS_WAMR_EXTERNAL_MODULE_REGISTRY
static const module_register_t g_wamr_modules[] =
{
#include "wamr_external_module_list.h"
};
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool wamr_custom_init(RuntimeInitArgs *init_args)
{
  bool ret = wasm_runtime_full_init(init_args);

  if (!ret)
    {
      return ret;
    }

  /* Add extra init hook here */

#ifdef CONFIG_INTERPRETERS_WAMR_EXTERNAL_MODULE_REGISTRY
  for (int i = 0; i < nitems(g_wamr_modules); i++)
    {
      ret = g_wamr_modules[i]();
      if (!ret)
        {
          return ret;
        }
    }
#endif

  return ret;
}
