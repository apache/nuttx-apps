############################################################################
# apps/examples/hello_wasm/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# Hello, World! built-in application info

PROGNAME  = $(CONFIG_EXAMPLES_HELLO_WASM_PROGNAME)
PRIORITY  = $(CONFIG_EXAMPLES_HELLO_WASM_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_HELLO_WASM_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_HELLO_WASM)

# Hello, World! Example

MAINSRC = hello_main.c

# Build with WebAssembly when CONFIG_INTERPRETERS_WAMR is enabled

WASM_BUILD = y

# Mode of WebAssembly Micro Runtime

WAMR_MODE  = AOT

include $(APPDIR)/Application.mk
