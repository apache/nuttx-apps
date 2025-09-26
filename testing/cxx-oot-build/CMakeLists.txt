# ##############################################################################
# testing/cxx-oot-build/CMakeLists.txt
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

cmake_minimum_required(VERSION 3.12...3.31)

project(
  OOTCpp
  VERSION 1.0
  DESCRIPTION "Out Of Tree Build C++ NuttX")

message(STATUS "Building OOTCpp project")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/HelloWorld.cpp
                 ${CMAKE_SOURCE_DIR}/src/main.cpp)

set(EXE_NAME oot)

add_executable(${EXE_NAME} ${SOURCE_FILES})

target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Generate a .bin file from the ELF after build
add_custom_command(
  TARGET ${EXE_NAME}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -S -O binary ${CMAKE_BINARY_DIR}/${EXE_NAME}
          ${CMAKE_BINARY_DIR}/${EXE_NAME}.bin
  COMMENT "Generating binary image ${EXE_NAME}.bin")
