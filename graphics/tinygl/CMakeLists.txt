# ##############################################################################
# apps/graphics/tinygl/CMakeLists.txt
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

if(CONFIG_GRAPHICS_TINYGL)

  # ############################################################################
  # Fetch and patch TinyGL
  # ############################################################################

  set(TINYGL_DIR ${CMAKE_CURRENT_LIST_DIR}/tinygl)

  if(NOT EXISTS ${TINYGL_DIR})
    file(GLOB TINYGL_PATCHES
         ${CMAKE_CURRENT_LIST_DIR}/[0-9][0-9][0-9][0-9]-*.patch)
    list(SORT TINYGL_PATCHES)
    set(TINYGL_PATCH_COMMAND "")
    foreach(p ${TINYGL_PATCHES})
      list(
        APPEND
        TINYGL_PATCH_COMMAND
        COMMAND
        patch
        -s
        -N
        -p1
        -d
        ${TINYGL_DIR}
        -i
        ${p})
    endforeach()
    list(POP_FRONT TINYGL_PATCH_COMMAND)

    FetchContent_Declare(
      tinygl_fetch
      URL ${CONFIG_TINYGL_URL}/${CONFIG_TINYGL_VERSION}.tar.gz SOURCE_DIR
          ${TINYGL_DIR} BINARY_DIR
          ${NUTTX_BINARY_DIR}/apps/graphics/tinygl/tinygl
      PATCH_COMMAND ${TINYGL_PATCH_COMMAND}
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 60)

    FetchContent_GetProperties(tinygl_fetch)

    if(NOT tinygl_fetch_POPULATED)
      FetchContent_Populate(tinygl_fetch)
    endif()
  endif()

  # ############################################################################
  # Feature switches (they change public structures, so export them)
  # ############################################################################

  macro(tinygl_bool var cfg)
    if(${cfg})
      set(${var} 1)
    else()
      set(${var} 0)
    endif()
  endmacro()

  tinygl_bool(TGL_32 CONFIG_TINYGL_RENDER_BITS_32)
  tinygl_bool(TGL_16 CONFIG_TINYGL_RENDER_BITS_16)
  tinygl_bool(TGL_SELECT CONFIG_TINYGL_SELECT)
  tinygl_bool(TGL_POLYGON CONFIG_TINYGL_GL_POLYGON)
  tinygl_bool(TGL_BLEND CONFIG_TINYGL_BLEND)
  tinygl_bool(TGL_BLEND_DP CONFIG_TINYGL_BLEND_DRAW_PIXELS)
  tinygl_bool(TGL_STIPPLE CONFIG_TINYGL_POLYGON_STIPPLE)
  tinygl_bool(TGL_NODRAW CONFIG_TINYGL_NO_DRAW_COLOR)
  tinygl_bool(TGL_ERRCHK CONFIG_TINYGL_ERROR_CHECK)

  set(TINYGL_DEFINES
      TGL_FEATURE_32_BITS=${TGL_32}
      TGL_FEATURE_16_BITS=${TGL_16}
      TGL_FEATURE_TEXTURE_POW2=${CONFIG_TINYGL_TEXTURE_POW2}
      TGL_FEATURE_ALT_RENDERMODES=${TGL_SELECT}
      TGL_FEATURE_GL_POLYGON=${TGL_POLYGON}
      TGL_FEATURE_BLEND=${TGL_BLEND}
      TGL_FEATURE_BLEND_DRAW_PIXELS=${TGL_BLEND_DP}
      TGL_FEATURE_POLYGON_STIPPLE=${TGL_STIPPLE}
      TGL_FEATURE_NO_DRAW_COLOR=${TGL_NODRAW}
      TGL_FEATURE_ERROR_CHECK=${TGL_ERRCHK})

  if(CONFIG_TINYGL_POLYGON_MAX_VERTEX)
    list(APPEND TINYGL_DEFINES
         POLYGON_MAX_VERTEX=${CONFIG_TINYGL_POLYGON_MAX_VERTEX})
  endif()

  # ############################################################################
  # Library
  # ############################################################################

  set(SRCS
      ${TINYGL_DIR}/src/accum.c
      ${TINYGL_DIR}/src/api.c
      ${TINYGL_DIR}/src/arrays.c
      ${TINYGL_DIR}/src/clear.c
      ${TINYGL_DIR}/src/clip.c
      ${TINYGL_DIR}/src/get.c
      ${TINYGL_DIR}/src/image_util.c
      ${TINYGL_DIR}/src/init.c
      ${TINYGL_DIR}/src/light.c
      ${TINYGL_DIR}/src/list.c
      ${TINYGL_DIR}/src/matrix.c
      ${TINYGL_DIR}/src/memory.c
      ${TINYGL_DIR}/src/misc.c
      ${TINYGL_DIR}/src/msghandling.c
      ${TINYGL_DIR}/src/select.c
      ${TINYGL_DIR}/src/specbuf.c
      ${TINYGL_DIR}/src/texture.c
      ${TINYGL_DIR}/src/vertex.c
      ${TINYGL_DIR}/src/zbuffer.c
      ${TINYGL_DIR}/src/zline.c
      ${TINYGL_DIR}/src/zmath.c
      ${TINYGL_DIR}/src/zpostprocess.c
      ${TINYGL_DIR}/src/zraster.c
      ${TINYGL_DIR}/src/ztext.c
      ${TINYGL_DIR}/src/ztriangle.c)

  nuttx_add_library(tinygl)
  target_sources(tinygl PRIVATE ${SRCS})
  target_compile_options(
    tinygl PRIVATE -Wno-shadow -Wno-undef -Wno-unused-function
                   -Wno-unknown-pragmas -Wno-unused-variable)
  target_compile_definitions(tinygl PRIVATE NDEBUG)

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${TINYGL_DIR}/include)
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_DEFINITIONS ${TINYGL_DEFINES})
endif()
