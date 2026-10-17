# ##############################################################################
# apps/interpreters/duktape/CMakeLists.txt
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

if(CONFIG_INTERPRETERS_DUKTAPE)
  set(DUKTAPE_VERSION 2.5.0)
  set(DUKTAPE_DIR ${CMAKE_CURRENT_LIST_DIR}/duktape)

  if(NOT EXISTS ${DUKTAPE_DIR})
    set(DUKTAPE_URL_BASE https://github.com/svaarala/duktape/releases/download/)

    FetchContent_Declare(
      duktape_fetch
      URL ${DUKTAPE_URL_BASE}/v${DUKTAPE_VERSION}/duktape-${DUKTAPE_VERSION}.tar.xz
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/duktape
          BINARY_DIR
          ${NUTTX_BINARY_DIR}/apps/interpreters/duktape/duktape
      PATCH_COMMAND patch -p0 -d ${CMAKE_CURRENT_LIST_DIR} <
                    ${CMAKE_CURRENT_LIST_DIR}/duk_cmdline.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(duktape_fetch)

    if(NOT duktape_fetch_POPULATED)
      FetchContent_Populate(duktape_fetch)
    endif()
  endif()

  set(DUKTAPE_FLAGS -Wno-unused-but-set-variable -DDUK_CMDLINE_CONSOLE_SUPPORT
                    -DDUK_CMDLINE_PRINTALERT_SUPPORT)

  if(CONFIG_ENABLE_ALL_SIGNALS)
    list(APPEND DUKTAPE_FLAGS -DDUK_CMDLINE_SIGNAL)
  endif()

  set(DUKTAPE_INCDIR ${DUKTAPE_DIR}/src-noline ${DUKTAPE_DIR}/extras/console
                     ${DUKTAPE_DIR}/extras/print-alert)

  set(DUKTAPE_CSRCS
      ${DUKTAPE_DIR}/src-noline/duktape.c
      ${DUKTAPE_DIR}/extras/console/duk_console.c
      ${DUKTAPE_DIR}/extras/print-alert/duk_print_alert.c)

  nuttx_add_application(
    MODULE
    ${CONFIG_INTERPRETERS_DUKTAPE}
    NAME
    duk
    STACKSIZE
    ${CONFIG_INTERPRETERS_DUKTAPE_STACKSIZE}
    PRIORITY
    ${CONFIG_INTERPRETERS_DUKTAPE_PRIORITY}
    SRCS
    ${DUKTAPE_DIR}/examples/cmdline/duk_cmdline.c
    ${DUKTAPE_CSRCS}
    INCLUDE_DIRECTORIES
    ${DUKTAPE_INCDIR}
    COMPILE_FLAGS
    ${DUKTAPE_FLAGS})

endif()
