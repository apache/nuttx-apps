# ##############################################################################
# apps/fsutils/libtinycbor/CMakeLists.txt
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

if(CONFIG_FSUTILS_TINYCBOR_LIB)

  # ############################################################################
  # Config and Fetch libtinycbor
  # ############################################################################
  set(TINYCBOR_DIR ${CMAKE_CURRENT_LIST_DIR}/tinycbor)

  if(NOT EXISTS ${TINYCBOR_DIR})
    set(TINYCBOR_VERSION 3cba6b11aaa0f6f674cd56ebaa573c4b65f71ee7)
    set(TINYCBOR_URL
        https://github.com/intel/tinycbor/archive/${TINYCBOR_VERSION}.zip)
    file(DOWNLOAD ${TINYCBOR_URL} ${CMAKE_CURRENT_LIST_DIR}/tinycbor.zip)
    execute_process(COMMAND unzip -o -j tinycbor.zip -d ${TINYCBOR_DIR}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    execute_process(COMMAND patch -Np1 -i fix_open_memstream.patch
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -std=c99)

  # ############################################################################
  # Sources
  # ############################################################################

  set(CSRCS
      ${TINYCBOR_DIR}/cborencoder.c
      ${TINYCBOR_DIR}/cborencoder_close_container_checked.c
      ${TINYCBOR_DIR}/cborencoder_float.c
      ${TINYCBOR_DIR}/cborerrorstrings.c
      ${TINYCBOR_DIR}/cborparser.c
      ${TINYCBOR_DIR}/cborparser_dup_string.c
      ${TINYCBOR_DIR}/cborparser_float.c
      ${TINYCBOR_DIR}/cborpretty.c
      ${TINYCBOR_DIR}/cborpretty_stdio.c
      ${TINYCBOR_DIR}/cbortojson.c
      ${TINYCBOR_DIR}/cborvalidation.c
      ${TINYCBOR_DIR}/open_memstream.c)

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR})

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(tinycbor STATIC)
  target_sources(tinycbor PRIVATE ${CSRCS})
  target_include_directories(tinycbor PRIVATE ${INCDIR})
  target_compile_options(tinycbor PRIVATE ${CFLAGS})

endif()
