# ##############################################################################
# apps/audioutils/morsey/CMakeLists.txt
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

if(CONFIG_AUDIOUTILS_MORSEY)
  include(FetchContent)

  set(MORSEY_DIR ${CMAKE_CURRENT_LIST_DIR}/morsey)

  if(NOT EXISTS ${MORSEY_DIR})
    FetchContent_Declare(
      morsey_fetch
      URL https://github.com/linguini1/morsey/archive/refs/tags/v1.0.1.tar.gz
          SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/morsey BINARY_DIR
          ${CMAKE_CURRENT_BINARY_DIR}/morsey
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(morsey_fetch)
    if(NOT morsey_fetch_POPULATED)
      FetchContent_Populate(morsey_fetch)
    endif()
  endif()

  if(TARGET apps)
    file(MAKE_DIRECTORY ${NUTTX_APPS_BINDIR}/include/audioutils)
    nuttx_create_symlink(${MORSEY_DIR}/morsey.h
                         ${NUTTX_APPS_BINDIR}/include/audioutils/morsey.h)

    target_sources(apps PRIVATE ${MORSEY_DIR}/morsey.c)
    target_include_directories(apps PRIVATE ${MORSEY_DIR})
  endif()
endif()
