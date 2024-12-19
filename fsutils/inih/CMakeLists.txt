# ##############################################################################
# apps/fsutils/inih/CMakeLists.txt
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

if(CONFIG_FSUTILS_INIH)

  # ############################################################################
  # Config and Fetch inih lib
  # ############################################################################
  set(INIH_DIR ${CMAKE_CURRENT_LIST_DIR}/inih-r42)

  if(NOT EXISTS ${INIH_DIR})
    set(INIH_URL https://github.com/benhoyt/inih/archive/r42.tar.gz)
    FetchContent_Declare(
      inih_fetch
      URL ${INIH_URL} SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/inih-r42 BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/fsutils/inih/inih-r42
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(inih_fetch)
    if(NOT inih_fetch_POPULATED)
      FetchContent_Populate(inih_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS
      -DINI_MAX_LINE=${CONFIG_INIH_MAX_LINE} -DINI_ALLOW_BOM=0
      -DINI_ALLOW_INLINE_COMMENTS=1 -DINI_STOP_ON_FIRST_ERROR=0
      -DINI_HANDLER_LINENO=1)

  if(CONFIG_INIH_MULTI_LINE_ENTRIES)
    list(APPEND CFLAGS -DINI_ALLOW_MULTILINE=1)
  else()
    list(APPEND CFLAGS -DINI_ALLOW_MULTILINE=0)
  endif()

  if(CONFIG_INIH_USE_MALLOC)
    list(APPEND CFLAGS -DINI_USE_STACK=0 -DINI_ALLOW_REALLOC=1
         -DINI_INITIAL_ALLOC=${CONFIG_INIH_INITIAL_ALLOC})
  else()
    list(APPEND CFLAGS -DINI_USE_STACK=1 -DINI_ALLOW_REALLOC=0)
  endif()
  # ############################################################################
  # Sources
  # ############################################################################

  file(GLOB CSRCS ${LIBTOMMATH_DIR}/*.c)
  set(CSRCS ${INIH_DIR}/ini.c)

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${INIH_DIR})

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(inih STATIC)
  target_compile_options(inih PRIVATE ${CFLAGS})
  target_sources(inih PRIVATE ${CSRCS})
  target_include_directories(inih PRIVATE ${INCDIR})

endif()
