# ##############################################################################
# apps/audioutils/rtttl-c/CMakeLists.txt
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

if(CONFIG_AUDIOUTILS_RTTTL_C)
  include(FetchContent)

  set(RTTTL_DIR ${CMAKE_CURRENT_LIST_DIR}/rtttl-c)

  if(NOT EXISTS ${RTTTL_DIR})
    FetchContent_Declare(
      rtttl_c_fetch
      GIT_REPOSITORY https://git.sr.ht/~qeef/rtttl-c
      GIT_TAG v0.1.0
      GIT_SHALLOW TRUE
      SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/rtttl-c BINARY_DIR
      ${CMAKE_CURRENT_BINARY_DIR}/rtttl-c
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(rtttl_c_fetch)
    if(NOT rtttl_c_fetch_POPULATED)
      FetchContent_Populate(rtttl_c_fetch)
    endif()
  endif()

  if(TARGET apps)
    file(MAKE_DIRECTORY ${NUTTX_APPS_BINDIR}/include/audioutils)
    nuttx_create_symlink(${RTTTL_DIR}/rtttl.h
                         ${NUTTX_APPS_BINDIR}/include/audioutils/rtttl.h)

    target_sources(apps PRIVATE ${RTTTL_DIR}/rtttl.c)
    target_include_directories(apps PRIVATE ${RTTTL_DIR})
  endif()
endif()
