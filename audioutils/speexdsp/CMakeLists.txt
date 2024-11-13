# ##############################################################################
# apps/audioutils/speexdsp/CMakeLists.txt
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
if(CONFIG_AUDIOUTILS_SPEEXDSP)

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/speexdsp)
    if(NOT SPEEXDSP_URL)
      set(SPEEXDSP_URL "https://github.com/xiph/speexdsp/archive/refs/tags")
    endif()

    if(NOT SPEEXDSP_VERSION)
      set(SPEEXDSP_VERSION "SpeexDSP-1.2.1")
    endif()

    FetchContent_Declare(
      speexdsp
      URL "${SPEEXDSP_URL}/${SPEEXDSP_VERSION}.zip"
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/speexdsp
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/audioutils/speexdsp/speexdsp
          CONFIGURE_COMMAND
          ""
          BUILD_COMMAND
          ""
          INSTALL_COMMAND
          ""
          TEST_COMMAND
          ""
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(speexdsp)

    if(NOT speexdsp_POPULATED)
      FetchContent_Populate(speexdsp)
    endif()
  endif()

  set(CSRCS speexdsp/libspeexdsp/resample.c)
  add_definitions(-DOUTSIDE_SPEEX)
  add_definitions(-DRANDOM_PREFIX=nuttx)

  if(CONFIG_FIXED_POINT)
    add_definitions(-DFIXED_POINT)
  else()
    add_definitions(-DFLOATING_POINT)
  endif()

endif()
