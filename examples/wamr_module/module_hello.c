/****************************************************************************
 * apps/examples/wamr_module/module_hello.c
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
#include <stddef.h>
#include <sys/param.h>

#include "wasm_export.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void hello_wrapper(wasm_exec_env_t env);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static NativeSymbol g_hello_symbols[] =
{
  EXPORT_WASM_API_WITH_SIG2(hello, "()")
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hello_wrapper
 ****************************************************************************/

static void hello_wrapper(wasm_exec_env_t env)
{
  printf("Hello World from WAMR module!\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wamr_module_hello_register
 *
 * The function prototype for the <WAMR_MODULE_NAME> module must be:
 *   `bool wamr_module_<WAMR_MODULE_NAME>_register(void)`
 *
 ****************************************************************************/

bool wamr_module_hello_register(void)
{
  return wasm_runtime_register_natives("hello", g_hello_symbols,
                                       nitems(g_hello_symbols));
}
