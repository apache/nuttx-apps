# ##############################################################################
# apps/netutils/rtptools/CMakeLists.txt
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

if(CONFIG_NETUTILS_RTPTOOLS)

  # ############################################################################
  # Config and Fetch rtptools lib
  # ############################################################################

  set(RTPTOOLS_DIR ${CMAKE_CURRENT_LIST_DIR}/rtptools)

  if(NOT EXISTS ${RTPTOOLS_DIR})
    set(RTPTOOLS_URL https://github.com/irtlab/rtptools/archive)
    FetchContent_Declare(
      rtptools_fetch
      URL ${RTPTOOLS_URL}/${CONFIG_NETUTILS_RTPTOOLS_VERSION}.tar.gz SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/rtptools BINARY_DIR
          ${CMAKE_BINARY_DIR}/netutils/rtptools/rtptools
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(rtptools_fetch)

    if(NOT rtptools_fetch_POPULATED)
      FetchContent_Populate(rtptools_fetch)
    endif()

    configure_file(config.h ${RTPTOOLS_DIR}/config.h COPYONLY)
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -Wno-strict-prototypes -Wno-unused-function -Wno-format
             -Wno-shadow)

  if("${CMAKE_C_COMPILER}" MATCHES "clang")
    list(APPEND CFLAGS -Wno-uninitialized)
  else()
    list(APPEND CFLAGS -Wno-maybe-uninitialized)
  endif()

  # ############################################################################
  # Sources
  # ############################################################################

  list(
    APPEND
    CSRCS
    ${RTPTOOLS_DIR}/multimer.c
    ${RTPTOOLS_DIR}/notify.c
    ${RTPTOOLS_DIR}/payload.c
    ${RTPTOOLS_DIR}/rd.c
    ${RTPTOOLS_DIR}/utils.c)

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${RTPTOOLS_DIR}/include)

  nuttx_add_library(rtptools STATIC)
  target_sources(rtptools PRIVATE ${CSRCS})
  target_include_directories(rtptools PRIVATE ${INCDIR})
  target_compile_options(rtptools PRIVATE ${CFLAGS})

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_RTPTOOLS_APPS)

    if(CONFIG_NETUTILS_RTPTOOLS_RTPPLAY)
      nuttx_add_application(
        MODULE
        ${CONFIG_RTPTOOLS_APPS}
        NAME
        ${CONFIG_NETUTILS_RTPTOOLS_RTPPLAY_PROGNAME}
        STACKSIZE
        ${CONFIG_NETUTILS_RTPTOOLS_RTPPLAY_STACKSIZE}
        PRIORITY
        ${CONFIG_NETUTILS_RTPTOOLS_RTPPLAY_PRIORITY}
        SRCS
        ${RTPTOOLS_DIR}/rtpplay.c
        DEPENDS
        rtptools)
    endif()

    if(CONFIG_NETUTILS_RTPTOOLS_RTPSEND)
      nuttx_add_application(
        MODULE
        ${CONFIG_RTPTOOLS_APPS}
        NAME
        ${CONFIG_NETUTILS_RTPTOOLS_RTPSEND_PROGNAME}
        STACKSIZE
        ${CONFIG_NETUTILS_RTPTOOLS_RTPSEND_STACKSIZE}
        PRIORITY
        ${CONFIG_NETUTILS_RTPTOOLS_RTPSEND_PRIORITY}
        SRCS
        ${RTPTOOLS_DIR}/rtpsend.c
        DEPENDS
        rtptools)
    endif()

    if(CONFIG_NETUTILS_RTPTOOLS_RTPDUMP)
      nuttx_add_application(
        MODULE
        ${CONFIG_RTPTOOLS_APPS}
        NAME
        ${CONFIG_NETUTILS_RTPTOOLS_RTPDUMP_PROGNAME}
        STACKSIZE
        ${CONFIG_NETUTILS_RTPTOOLS_RTPDUMP_STACKSIZE}
        PRIORITY
        ${CONFIG_NETUTILS_RTPTOOLS_RTPDUMP_PRIORITY}
        SRCS
        ${RTPTOOLS_DIR}/rtpdump.c
        DEPENDS
        rtptools)
    endif()

    if(CONFIG_NETUTILS_RTPTOOLS_RTPTRANS)
      nuttx_add_application(
        MODULE
        ${CONFIG_RTPTOOLS_APPS}
        NAME
        ${CONFIG_NETUTILS_RTPTOOLS_RTPTRANS_PROGNAME}
        STACKSIZE
        ${CONFIG_NETUTILS_RTPTOOLS_RTPTRANS_STACKSIZE}
        PRIORITY
        ${CONFIG_NETUTILS_RTPTOOLS_RTPTRANS_PRIORITY}
        SRCS
        ${RTPTOOLS_DIR}/rtptrans.c
        DEPENDS
        rtptools)
    endif()

  endif()

endif()
