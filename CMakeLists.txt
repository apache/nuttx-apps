# ##############################################################################
# apps/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

# ~~~
# Important note:
# This CMakeLists.txt is not meant as a top-level CMakeLists.txt. Instead,
# CMake must be run using the top-level CMakeLists.txt from the nuttx
# repository and point to this directory via NUTTX_APPS_DIR. For example:
#   cmake -S <nuttx-dir> -B <build-dir> [...] -DNUTTX_APPS_DIR=<apps-dir>
# ~~~

if(CONFIG_APPS_DIR)
  # add apps cmake modules
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
  include(nuttx_add_luamod)
  include(nuttx_add_wamrmod)
  include(nuttx_add_rust)
  include(nuttx_wasm_interface)
  nuttx_add_library(apps)
  if(NOT EXISTS {NUTTX_APPS_BINDIR}/dummy.c)
    file(TOUCH ${NUTTX_APPS_BINDIR}/dummy.c)
  endif()
  target_sources(apps PRIVATE ${NUTTX_APPS_BINDIR}/dummy.c)
endif()

file(MAKE_DIRECTORY ${NUTTX_APPS_BINDIR}/include)
include_directories(include)
include_directories(${NUTTX_APPS_BINDIR}/include)

add_subdirectory(audioutils)
add_subdirectory(benchmarks)
add_subdirectory(boot)
add_subdirectory(canutils)
add_subdirectory(crypto)
add_subdirectory(fsutils)
add_subdirectory(games)
add_subdirectory(graphics)
add_subdirectory(industry)
add_subdirectory(interpreters)
add_subdirectory(logging)
add_subdirectory(lte)
add_subdirectory(math)
add_subdirectory(mlearning)
add_subdirectory(modbus)
add_subdirectory(netutils)
add_subdirectory(nshlib)
add_subdirectory(platform)
add_subdirectory(sdr)
add_subdirectory(system)
add_subdirectory(testing)
add_subdirectory(videoutils)
add_subdirectory(wireless)

# add examples and external at the end to allow library dependencies
add_subdirectory(examples)
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/external)
  add_subdirectory(external)
endif()

add_subdirectory(tools) # after traversing all subdirectories
add_subdirectory(builtin) # must be last

nuttx_generate_kconfig()
