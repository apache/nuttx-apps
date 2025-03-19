# ##############################################################################
# apps/graphics/input/CMakeLists.txt
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

if(CONFIG_GRAPHICS_INPUT_GENERATOR)
  nuttx_add_library(input_generator)
  file(GLOB CSRCS generator/*.c)
  list(REMOVE_ITEM CSRCS ${CMAKE_CURRENT_SOURCE_DIR}/generator/input.c)
  target_sources(input_generator PRIVATE ${CSRCS})
endif()

if(CONFIG_GRAPHICS_INPUT_MONKEY)
  file(GLOB MONKEY_SRCS monkey/*.c)
  list(REMOVE_ITEM MONKEY_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/monkey/monkey_main.c)
  set(SRCS monkey/monkey_main.c ${MONKEY_SRCS})

  nuttx_add_application(
    NAME
    monkey
    PRIORITY
    ${CONFIG_GRAPHICS_INPUT_MONKEY_PRIORITY}
    STACKSIZE
    ${CONFIG_GRAPHICS_INPUT_MONKEY_STACKSIZE}
    MODULE
    ${CONFIG_GRAPHICS_INPUT_MONKEY}
    DEPENDS
    input_generator
    SRCS
    ${SRCS})
endif()

if(CONFIG_GRAPHICS_INPUT_TOOL)
  nuttx_add_application(
    MODULE
    ${CONFIG_GRAPHICS_INPUT_TOOL}
    NAME
    input
    STACKSIZE
    ${CONFIG_GRAPHICS_INPUT_TOOL_STACKSIZE}
    PRIORITY
    ${CONFIG_GRAPHICS_INPUT_TOOL_PRIORITY}
    DEPENDS
    input_generator
    SRCS
    generator/input.c)
endif()
