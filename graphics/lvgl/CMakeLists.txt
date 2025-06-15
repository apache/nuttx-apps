# ##############################################################################
# apps/graphics/lvgl/CMakeLists.txt
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

if(CONFIG_GRAPHICS_LVGL)

  # ############################################################################
  # Config and Fetch lvgl
  # ############################################################################
  set(LVGL_DIR ${CMAKE_CURRENT_LIST_DIR}/lvgl)

  if(NOT EXISTS ${LVGL_DIR})
    FetchContent_Declare(
      lvgl_fetch
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "https://github.com/lvgl/lvgl/archive/refs/tags/v9.2.1.zip" SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/lvgl BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/graphics/lvgl/lvgl
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 120)

    FetchContent_GetProperties(lvgl_fetch)

    if(NOT lvgl_fetch_POPULATED)
      FetchContent_Populate(lvgl_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags and Sources
  # ############################################################################

  # ############################################################################
  # Library Configuration
  # ############################################################################

  file(
    GLOB_RECURSE
    SRCS
    "${LVGL_DIR}/src/*.S"
    "${LVGL_DIR}/src/*.c"
    "${LVGL_DIR}/src/*.cpp"
    "${LVGL_DIR}/demos/*.c"
    "${LVGL_DIR}/examples/*.c")
  nuttx_add_library(lvgl)

  target_sources(lvgl PRIVATE ${SRCS})
  target_include_directories(lvgl PRIVATE ${LVGL_DIR})
  target_compile_options(lvgl PRIVATE -Wno-shadow)

  if(NOT CONFIG_LV_ASSERT_HANDLER_INCLUDE STREQUAL "")
    target_compile_definitions(lvgl PRIVATE "LV_ASSERT_HANDLER=ASSERT(0)\;")
  endif()

  if(CONFIG_LV_USE_FREETYPE)
    if(NOT CONFIG_LIB_FREETYPE)
      message(WARNING "LIB_FREETYPE is not enabled")
    endif()
    nuttx_add_dependencies(TARGET lvgl DEPENDS freetype)
  endif()

  if(CONFIG_LV_USE_LIBPNG)
    if(NOT CONFIG_LIB_PNG)
      message(WARNING "LIB_PNG is not enabled")
    endif()
    nuttx_add_dependencies(TARGET lvgl DEPENDS png_static)
  endif()

  if(CONFIG_LV_USE_LIBWEBP)
    if(NOT CONFIG_LIB_WEBP)
      message(WARNING "LIB_WEBP is not enabled")
    endif()
    nuttx_add_dependencies(TARGET lvgl DEPENDS webpdecoder)
  endif()

  if(CONFIG_LV_USE_FFMPEG)
    message(FATAL_ERROR "FFMPEG is not supported yet")
  endif()

  if(CONFIG_LV_USE_RLOTTIE)
    message(FATAL_ERROR "RLOTTIE is not supported yet")
  endif()

  if(CONFIG_LV_USE_LIBJPEG_TURBO)
    if(NOT CONFIG_LIB_JPEG_TURBO)
      message(WARNING "LIB_JPEG_TURBO is not enabled")
    endif()
    nuttx_add_dependencies(TARGET lvgl DEPENDS turbojpeg-static)
  endif()

  if(NOT ${CONFIG_LV_OPTLEVEL} STREQUAL "")
    add_compile_options(${CONFIG_LV_OPTLEVEL})
  endif()

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}
             ${CMAKE_CURRENT_LIST_DIR}/lvgl)

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_COMPILE_OPTIONS -DLV_USE_DEV_VERSION=1)

endif()
