# ##############################################################################
# apps/graphics/uGLy/CMakeLists.txt
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

if(CONFIG_GRAPHICS_UGLY)

  # ############################################################################
  # Config and Fetch uGLy
  # ############################################################################

  set(UGLY_DIR ${CMAKE_CURRENT_LIST_DIR}/uGLy)

  if(NOT CONFIG_GRAPH_UGLY_URL)
    set(CONFIG_GRAPH_UGLY_URL
        "https://github.com/TheFakeMontyOnTheRun/uGLy/archive/refs/tags")
  endif()

  set(UGLY_PACKAGE test-for-0.1)

  if(NOT EXISTS ${UGLY_DIR})

    # Patches applied on top of the uGLy release, in name order

    file(GLOB UGLY_PATCHES ${CMAKE_CURRENT_LIST_DIR}/patches/*.patch)
    list(SORT UGLY_PATCHES)

    set(UGLY_PATCH_COMMAND true)
    foreach(patch ${UGLY_PATCHES})
      list(
        APPEND
        UGLY_PATCH_COMMAND
        &&
        patch
        -s
        -p1
        -d
        ${UGLY_DIR}
        <
        ${patch})
    endforeach()

    FetchContent_Declare(
      ugly_fetch
      URL ${CONFIG_GRAPH_UGLY_URL}/${UGLY_PACKAGE}.zip SOURCE_DIR ${UGLY_DIR}
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/graphics/uGLy/uGLy
      PATCH_COMMAND ${UGLY_PATCH_COMMAND}
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 120)

    FetchContent_GetProperties(ugly_fetch)

    if(NOT ugly_fetch_POPULATED)
      FetchContent_Populate(ugly_fetch)
    endif()
  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  set(SRCS
      ${UGLY_DIR}/src/es10cl.c ${UGLY_DIR}/src/es11.c
      ${UGLY_DIR}/src/matricesFP.c ${UGLY_DIR}/src/raster.c
      ${UGLY_DIR}/src/nuttx.c ${UGLY_DIR}/third_party/fpsqrt/fpsqrt.c)

  nuttx_add_library(uGLy)

  target_sources(uGLy PRIVATE ${SRCS})
  target_include_directories(uGLy PRIVATE ${UGLY_DIR}/third_party/fpsqrt)
  target_compile_definitions(
    uGLy
    PRIVATE BPP24 DISABLE_STENCIL_BUFFER
            XRES_FRAMEBUFFER=${CONFIG_GRAPHICS_UGLY_XRES}
            YRES_FRAMEBUFFER=${CONFIG_GRAPHICS_UGLY_YRES})

  # Applications get <GLES/gl.h> from uGLy instead of the host's headers

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}
             ${UGLY_DIR}/include)

endif()
