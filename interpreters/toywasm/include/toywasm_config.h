/****************************************************************************
 * apps/interpreters/toywasm/include/toywasm_config.h
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

#if !defined(_TOYWASM_CONFIG_H)
#define _TOYWASM_CONFIG_H

#define TOYWASM_USE_SEPARATE_EXECUTE
#define TOYWASM_USE_TAILCALL
#define TOYWASM_SORT_EXPORTS
#define TOYWASM_USE_JUMP_BINARY_SEARCH
#define TOYWASM_JUMP_CACHE2_SIZE 4
#define TOYWASM_USE_LOCALS_FAST_PATH
#define TOYWASM_USE_LOCALS_CACHE
#define TOYWASM_USE_SEPARATE_LOCALS
#define TOYWASM_USE_SMALL_CELLS
#define TOYWASM_USE_RESULTTYPE_CELLIDX
#define TOYWASM_USE_LOCALTYPE_CELLIDX
#define TOYWASM_ENABLE_HEAP_TRACKING
#define TOYWASM_ENABLE_HEAP_TRACKING_PEAK
#define TOYWASM_ENABLE_WRITER
#define TOYWASM_ENABLE_WASM_SIMD
#define TOYWASM_ENABLE_WASM_EXCEPTION_HANDLING
#define TOYWASM_EXCEPTION_MAX_CELLS 4
#define TOYWASM_ENABLE_WASM_EXTENDED_CONST
#define TOYWASM_ENABLE_WASM_MULTI_MEMORY
#define TOYWASM_ENABLE_WASM_TAILCALL
#define TOYWASM_ENABLE_WASM_THREADS
#define TOYWASM_ENABLE_WASM_CUSTOM_PAGE_SIZES
#define TOYWASM_ENABLE_WASM_NAME_SECTION
#define TOYWASM_ENABLE_WASI
#define TOYWASM_ENABLE_WASI_THREADS
#define TOYWASM_ENABLE_DYLD
#define TOYWASM_ENABLE_DYLD_DLFCN

#endif /* !defined(_TOYWASM_CONFIG_H) */
