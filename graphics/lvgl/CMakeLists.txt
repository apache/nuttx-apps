# ##############################################################################
# apps/graphics/lvgl/CMakeLists.txt
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
      URL "https://github.com/lvgl/lvgl/archive/refs/tags/v9.1.0.zip"
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/lvgl
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/graphics/lvgl/lvgl
          CONFIGURE_COMMAND
          ""
          BUILD_COMMAND
          ""
          INSTALL_COMMAND
          ""
          TEST_COMMAND
          ""
      PATCH_COMMAND
        patch -p1 -s -d ${CMAKE_CURRENT_LIST_DIR}/lvgl <
        ${CMAKE_CURRENT_LIST_DIR}/0001-fix-demo-fix-compile-warning.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

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

  add_subdirectory(${LVGL_DIR})
  nuttx_add_external_library(lvgl)
  nuttx_add_external_library(lvgl_demos)
  nuttx_add_external_library(lvgl_examples)

  target_include_directories(lvgl PUBLIC ${CMAKE_BINARY_DIR}/include)
  target_compile_options(lvgl PRIVATE ${CFLAGS})
  if(NOT CONFIG_LV_ASSERT_HANDLER_INCLUDE STREQUAL "")
    target_compile_definitions(lvgl PRIVATE "LV_ASSERT_HANDLER=ASSERT(0)\;")
  endif()

  # allow to include via lvgl/lvgl.h
  target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_LIST_DIR})
endif()
