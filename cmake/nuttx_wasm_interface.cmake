# ##############################################################################
# cmake/nuttx_wasm_interface.cmake
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

# This is NuttX native wrapper for the WAMR interface.

if(NOT TARGET wasm_interface)
  add_custom_target(wasm_interface)
endif()

# declare WAMS build directory and add INSTALL path
function(wasm_add_application)

  cmake_parse_arguments(
    APP "" "NAME;STACK_SIZE;INITIAL_MEMORY_SIZE;WAMR_MODE;INSTALL_NAME"
    "SRCS;WLDFLAGS;WCFLAGS;WINCLUDES" ${ARGN})

  set_property(
    TARGET wasm_interface
    APPEND
    PROPERTY WASM_DIR ${CMAKE_CURRENT_LIST_DIR})

  if(APP_INSTALL_NAME)
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/wasm/${APP_INSTALL_NAME}
      COMMAND ${CMAKE_COMMAND} -E touch_nocreate
              ${CMAKE_BINARY_DIR}/wasm/${APP_INSTALL_NAME}
      DEPENDS apps)
    add_custom_target(wasm_gen_${APP_NAME}
                      DEPENDS ${CMAKE_BINARY_DIR}/wasm/${APP_INSTALL_NAME})
    add_dynamic_rcraws(RAWS ${CMAKE_BINARY_DIR}/wasm/${APP_INSTALL_NAME}
                       DEPENDS wasm_gen_${APP_NAME})
  endif()

endfunction()

function(wasm_add_library)
  set_property(
    TARGET wasm_interface
    APPEND
    PROPERTY WASM_DIR ${CMAKE_CURRENT_LIST_DIR})
endfunction()
