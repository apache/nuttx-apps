# ##############################################################################
# apps/benchmarks/superpi/CMakeLists.txt
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

if(CONFIG_BENCHMARK_SUPERPI)

  set(SUPERPI_UNPACK ${CMAKE_CURRENT_LIST_DIR}/superpi)
  set(SUPERPI_URL https://github.com/Fibonacci43/SuperPI/archive)
  set(SUPERPI_ZIP main.zip)

  if(NOT EXISTS ${SUPERPI_UNPACK})

    FetchContent_Declare(
      superpi_fetch
      URL ${SUPERPI_URL}/${SUPERPI_ZIP} SOURCE_DIR ${SUPERPI_UNPACK} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/benchmarks/superpi/superpi
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(superpi_fetch)
    if(NOT superpi_fetch_POPULATED)
      FetchContent_Populate(superpi_fetch)
    endif()

  endif()

  nuttx_add_application(
    NAME
    ${CONFIG_BENCHMARK_SUPERPI_PROGNAME}
    STACKSIZE
    ${CONFIG_BENCHMARK_SUPERPI_STACKSIZE}
    PRIORITY
    ${CONFIG_BENCHMARK_SUPERPI_PRIORITY}
    SRCS
    ${SUPERPI_UNPACK}/pi_fftcs.c
    ${SUPERPI_UNPACK}/fftsg_h.c)

endif()
