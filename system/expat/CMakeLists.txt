# ##############################################################################
# apps/system/expat/CMakeLists.txt
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

if(CONFIG_LIB_EXPAT)
  set(EXPAT_DIR ${CMAKE_CURRENT_LIST_DIR}/libexpat)

  if(NOT EXISTS ${EXPAT_DIR})
    string(REPLACE "." "_" EXPAT_TAG ${CONFIG_LIB_EXPAT_VERSION})
    string(CONCAT EXPAT_URL
                  "https://codeload.github.com/libexpat/libexpat/tar.gz/"
                  "refs/tags/R_${EXPAT_TAG}")
    FetchContent_Declare(
      expat_fetch
      URL ${EXPAT_URL} SOURCE_DIR ${EXPAT_DIR} BINARY_DIR
          ${NUTTX_BINARY_DIR}/apps/system/expat/libexpat
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)
    FetchContent_GetProperties(expat_fetch)

    if(NOT expat_fetch_POPULATED)
      FetchContent_Populate(expat_fetch)
    endif()
  endif()

  set(EXPAT_INCDIR ${EXPAT_DIR}/expat/lib)
  set(EXPAT_SRCS
      ${EXPAT_INCDIR}/xmlparse.c ${EXPAT_INCDIR}/xmlrole.c
      ${EXPAT_INCDIR}/xmltok.c ${EXPAT_INCDIR}/random_arc4random_buf.c)

  set(EXPAT_FLAGS -DXML_STATIC -Wno-shadow)
  if(CONFIG_LIB_EXPAT_DTD)
    list(APPEND EXPAT_FLAGS -DXML_DTD)
  endif()

  nuttx_add_library(expat STATIC)
  target_sources(expat PRIVATE ${EXPAT_SRCS})
  target_include_directories(
    expat
    PUBLIC ${EXPAT_INCDIR}
    PRIVATE ${CMAKE_CURRENT_LIST_DIR})
  target_compile_options(expat PRIVATE ${EXPAT_FLAGS})
  nuttx_export_header(TARGET expat INCLUDE_DIRECTORIES ${EXPAT_INCDIR})
endif()
