# ##############################################################################
# apps/benchmarks/iozone/CMakeLists.txt
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

if(CONFIG_BENCHMARK_IOZONE)

  # Define the directory and file paths
  set(IOZONE_VERSION "${CONFIG_BENCHMARK_IOZONE_VERSION}") # Adjust this as
                                                           # needed
  set(IOZONE_DIR ${CMAKE_CURRENT_LIST_DIR}/iozone)
  set(IOZONE_ZIP ${IOZONE_DIR}.tgz)
  set(IOZONE_URL
      "https://www.iozone.org/src/current/iozone${IOZONE_VERSION}.tgz")

  # Download and unpack iozone if not already present
  if(NOT EXISTS ${IOZONE_DIR})
    FetchContent_Declare(
      iozone_fetch
      URL ${IOZONE_URL} SOURCE_DIR ${IOZONE_DIR}
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30
      PATCH_COMMAND patch -p1 -d ${IOZONE_DIR} <
                    ${CMAKE_CURRENT_LIST_DIR}/iozone.patch)

    FetchContent_GetProperties(iozone_fetch)
    if(NOT iozone_fetch_POPULATED)
      FetchContent_Populate(iozone_fetch)
    endif()
  endif()

  # Define source files
  set(CSRCS ${IOZONE_DIR}/src/current/libbif.c)
  set(MAINSRC ${IOZONE_DIR}/src/current/iozone.c)

  # Define compile flags
  set(CFLAGS
      -Dunix
      -DHAVE_ANSIC_C
      -DHAVE_PREAD
      -DNAME=\"nuttx\"
      -DNO_MADVISE
      -DNO_FORK
      -D__FreeBSD__
      -DNO_THREADS
      -Wno-unused-parameter
      -Wno-unused-function
      -Wno-shadow
      -Wno-unused-but-set-variable
      -Wno-strict-prototypes
      -Wno-misleading-indentation
      -Wno-maybe-uninitialized
      -DMAXBUFFERSIZE=32*1024
      -DMAXSTREAMS=8
      -DMAXNAMESIZE=NAME_MAX
      -DRECLEN_START=1024)

  # Add application to the build
  set(SRCS ${IOZONE_DIR}/src/current/iozone.c ${CSRCS})
  nuttx_add_application(
    NAME
    iozone
    PRIORITY
    ${CONFIG_BENCHMARK_IOZONE_PRIORITY}
    STACKSIZE
    ${CONFIG_BENCHMARK_IOZONE_STACKSIZE}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${SRCS})

endif()
