/****************************************************************************
 * apps/interpreters/toywasm/src/toywasm_config.c
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
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

const char *const toywasm_config_string =
"TOYWASM_USE_SEPARATE_EXECUTE = ON\n"
"TOYWASM_USE_SEPARATE_VALIDATE = OFF\n"
"TOYWASM_USE_TAILCALL = ON\n"
"TOYWASM_FORCE_USE_TAILCALL = OFF\n"
"TOYWASM_USE_SIMD = OFF\n"
"TOYWASM_USE_SHORT_ENUMS = OFF\n"
"TOYWASM_USE_USER_SCHED = OFF\n"
"TOYWASM_ENABLE_TRACING = OFF\n"
"TOYWASM_ENABLE_TRACING_INSN = OFF\n"
"TOYWASM_SORT_EXPORTS = ON\n"
"TOYWASM_USE_JUMP_BINARY_SEARCH = ON\n"
"TOYWASM_USE_JUMP_CACHE = OFF\n"
"TOYWASM_JUMP_CACHE2_SIZE = 4\n"
"TOYWASM_USE_LOCALS_FAST_PATH = ON\n"
"TOYWASM_USE_LOCALS_CACHE = ON\n"
"TOYWASM_USE_SEPARATE_LOCALS = ON\n"
"TOYWASM_USE_SMALL_CELLS = ON\n"
"TOYWASM_USE_RESULTTYPE_CELLIDX = ON\n"
"TOYWASM_USE_LOCALTYPE_CELLIDX = ON\n"
"TOYWASM_PREALLOC_SHARED_MEMORY = OFF\n"
"TOYWASM_ENABLE_HEAP_TRACKING = ON\n"
"TOYWASM_ENABLE_HEAP_TRACKING_PEAK = ON\n"
"TOYWASM_ENABLE_WRITER = ON\n"
"TOYWASM_MAINTAIN_EXPR_END = OFF\n"
"TOYWASM_ENABLE_WASM_EXCEPTION_HANDLING = ON\n"
"TOYWASM_EXCEPTION_MAX_CELLS = 4\n"
"TOYWASM_ENABLE_WASM_SIMD = ON\n"
"TOYWASM_ENABLE_WASM_EXTENDED_CONST = ON\n"
"TOYWASM_ENABLE_WASM_MULTI_MEMORY = ON\n"
"TOYWASM_ENABLE_WASM_TAILCALL = ON\n"
"TOYWASM_ENABLE_WASM_THREADS = ON\n"
"TOYWASM_ENABLE_WASM_CUSTOM_PAGE_SIZES = ON\n"
"TOYWASM_ENABLE_WASM_NAME_SECTION = ON\n"
"TOYWASM_ENABLE_WASI = ON\n"
"TOYWASM_ENABLE_WASI_THREADS = ON\n"
"TOYWASM_ENABLE_WASI_LITTLEFS = OFF\n"
"TOYWASM_ENABLE_LITTLEFS_STATS = OFF\n"
"TOYWASM_ENABLE_DYLD = ON\n"
"TOYWASM_ENABLE_DYLD_DLFCN = ON\n";
