/****************************************************************************
 * apps/interpreters/toywasm/src/toywasm_config.c
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

const char *toywasm_config_string =
"\tTOYWASM_USE_SEPARATE_EXECUTE = ON\n"
"\tTOYWASM_USE_TAILCALL = ON\n"
"\tTOYWASM_USE_SHORT_ENUMS = OFF\n"
"\tTOYWASM_USE_USER_SCHED = OFF\n"
"\tTOYWASM_ENABLE_TRACING = OFF\n"
"\tTOYWASM_ENABLE_TRACING_INSN = OFF\n"
"\tTOYWASM_USE_JUMP_BINARY_SEARCH = ON\n"
"\tTOYWASM_USE_JUMP_CACHE = OFF\n"
"\tTOYWASM_JUMP_CACHE2_SIZE = 4\n"
"\tTOYWASM_USE_LOCALS_CACHE = ON\n"
"\tTOYWASM_USE_SEPARATE_LOCALS = ON\n"
"\tTOYWASM_USE_SMALL_CELLS = ON\n"
"\tTOYWASM_USE_RESULTTYPE_CELLIDX = ON\n"
"\tTOYWASM_USE_LOCALTYPE_CELLIDX = ON\n"
"\tTOYWASM_ENABLE_WRITER = ON\n"
"\tTOYWASM_ENABLE_WASM_EXTENDED_CONST = ON\n"
"\tTOYWASM_ENABLE_WASM_MULTI_MEMORY = ON\n"
"\tTOYWASM_ENABLE_WASM_TAILCALL = ON\n"
"\tTOYWASM_ENABLE_WASM_THREADS = ON\n"
"\tTOYWASM_ENABLE_WASI = ON\n"
"\tTOYWASM_ENABLE_WASI_THREADS = ON\n";
