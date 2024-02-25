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
      URL "https://github.com/lvgl/lvgl/archive/refs/tags/v${CONFIG_LVGL_VERSION}.zip"
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

  # Relax LVGL's format checking and unused variable checking to avoid errors

  set(CFLAGS -Wno-format -Wno-format-security -Wno-unused-variable)

  # Since this change is only merged into the lvgl.v9, let us workaround for v8
  # ./lvgl/src/core/lv_obj.c:363:25: warning: variable 'x' set but not used
  # [-Wunused-but-set-variable] 363 |         static uint32_t x = 0; | ^ 1
  # warning generated.

  list(APPEND CFLAGS -Wno-unused-but-set-variable)

  set(CSRCS port/lv_port.c port/lv_port_tick.c)

  if(CONFIG_LIBUV)
    list(APPEND CSRCS port/lv_port_libuv.c)
  endif()

  if(CONFIG_LV_PORT_USE_LCDDEV)
    list(APPEND CSRCS port/lv_port_lcddev.c)
  endif()

  if(CONFIG_LV_PORT_USE_FBDEV)
    list(APPEND CSRCS port/lv_port_fbdev.c)
  endif()

  if(CONFIG_LV_PORT_USE_TOUCHPAD)
    list(APPEND CSRCS port/lv_port_touchpad.c)
  endif()

  if(CONFIG_LV_PORT_USE_BUTTON)
    list(APPEND CSRCS port/lv_port_button.c)
  endif()

  if(CONFIG_LV_PORT_USE_KEYPAD)
    list(APPEND CSRCS port/lv_port_keypad.c)
  endif()

  if(CONFIG_LV_PORT_USE_ENCODER)
    list(APPEND CSRCS port/lv_port_encoder.c)
  endif()

  if(CONFIG_LV_USE_LOG)
    list(APPEND CSRCS port/lv_port_syslog.c)
  endif()

  if(CONFIG_LV_MEM_CUSTOM)
    if(NOT CONFIG_LV_PORT_MEM_CUSTOM_SIZE EQUAL 0)
      list(APPEND CFLAGS -DLV_MEM_CUSTOM_ALLOC=lv_port_mem_alloc)
      list(APPEND CFLAGS -DLV_MEM_CUSTOM_FREE=lv_port_mem_free)
      list(APPEND CFLAGS -DLV_MEM_CUSTOM_REALLOC=lv_port_mem_realloc)
      list(APPEND CSRCS port/lv_port_mem.c)
    endif()
  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR} ${NUTTX_DIR}/include)

  add_subdirectory(${LVGL_DIR})
  nuttx_add_external_library(lvgl)
  nuttx_add_external_library(lvgl_demos)
  nuttx_add_external_library(lvgl_examples)

  target_include_directories(lvgl PRIVATE ${INCDIR})
  target_compile_options(lvgl PRIVATE ${CFLAGS})
  if(NOT CONFIG_LV_ASSERT_HANDLER_INCLUDE STREQUAL "")
    target_compile_definitions(lvgl PRIVATE "LV_ASSERT_HANDLER=ASSERT(0)\;")
  endif()

  nuttx_add_library(lvgl_nuttx)
  target_sources(lvgl_nuttx PRIVATE ${CSRCS})
  target_link_libraries(lvgl_nuttx lvgl)

  # allow to include via lvgl/lvgl.h
  target_include_directories(lvgl_nuttx PUBLIC ${CMAKE_CURRENT_LIST_DIR})
endif()
