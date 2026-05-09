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

  set(LVGL_VERSION
      "${CONFIG_LVGL_VERSION_MAJOR}.${CONFIG_LVGL_VERSION_MINOR}.${CONFIG_LVGL_VERSION_PATCH}"
  )

  set(LVGL_STAMP ${LVGL_DIR}/.nuttx_patched-${LVGL_VERSION})
  set(LVGL_FBDEV_PATCH
      ${CMAKE_CURRENT_LIST_DIR}/0001-nuttx-fbdev-use-virtual-framebuffer-offset.patch
  )

  if(NOT EXISTS ${LVGL_DIR}/.git)
    if("${LVGL_VERSION}" STREQUAL "9.2.2")
      set_property(
        DIRECTORY
        APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS ${LVGL_FBDEV_PATCH})
    endif()

    if(NOT EXISTS ${LVGL_STAMP}
       OR ("${LVGL_VERSION}" STREQUAL "9.2.2" AND ${LVGL_FBDEV_PATCH}
                                                  IS_NEWER_THAN ${LVGL_STAMP}))
      # Reset download stamps so a failed patch retries from a fresh archive.
      set(LVGL_SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/lvgl-subbuild)
      file(REMOVE_RECURSE ${LVGL_SUBBUILD_DIR})
      set(LVGL_URL
          "https://github.com/lvgl/lvgl/archive/refs/tags/v${LVGL_VERSION}.zip")
      set(LVGL_URL_HASH "")
      set(LVGL_PATCH_COMMAND "")
      if("${LVGL_VERSION}" STREQUAL "9.2.2")
        set(LVGL_URL_HASH
            URL_HASH
            SHA256=b3d33f7aad1360f588762208d0f563574392debbc28d9187bda86012909f2203
        )
        set(LVGL_PATCH_COMMAND patch -s -N -p1 -d ${LVGL_DIR} -i
                               ${LVGL_FBDEV_PATCH})
      endif()
      FetchContent_Declare(
        lvgl_fetch
        URL ${LVGL_URL}
            ${LVGL_URL_HASH}
            DOWNLOAD_DIR
            ${CMAKE_CURRENT_LIST_DIR}
            SOURCE_DIR
            ${LVGL_DIR}
            BINARY_DIR
            ${NUTTX_BINARY_DIR}/apps/graphics/lvgl/lvgl
            SUBBUILD_DIR
            ${LVGL_SUBBUILD_DIR}
        PATCH_COMMAND ${LVGL_PATCH_COMMAND}
        DOWNLOAD_NO_PROGRESS true
        TIMEOUT 120)

      FetchContent_GetProperties(lvgl_fetch)

      if(NOT lvgl_fetch_POPULATED)
        FetchContent_Populate(lvgl_fetch)
      endif()
      file(TOUCH ${LVGL_STAMP})
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
