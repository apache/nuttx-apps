# ##############################################################################
# apps/audioutils/libmad/CMakeLists.txt
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

if(CONFIG_AUDIOUTILS_LIBMAD)

  # Download and unpack libmad if no libmad found
  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libmad/configure)
    set(VERSION "master")
    set(LIBMAD_URL
        "https://github.com/markjeee/libmad/archive/refs/heads/master.zip")

    file(DOWNLOAD ${LIBMAD_URL} ${CMAKE_CURRENT_SOURCE_DIR}/libmad.zip)
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar -xf ${CMAKE_CURRENT_SOURCE_DIR}/libmad.zip
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -E rename
        ${CMAKE_CURRENT_SOURCE_DIR}/libmad-${VERSION}
        ${CMAKE_CURRENT_SOURCE_DIR}/libmad)

    add_custom_target(
      distclean
      COMMAND ${CMAKE_COMMAND} -E remove_directory
              ${CMAKE_CURRENT_SOURCE_DIR}/libmad
      COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/libmad.zip)
  endif()

  nuttx_add_library(libmad STATIC)

  file(GLOB LIBMAD_SRCS libmad/*.c)
  set(CSRCS ${LIBMAD_SRCS})
  set(CFLAGS -Wno-shadow -Wno-stringop-overflow -DFPM_DEFAULT -DSIZEOF_INT=4)
  set(INCDIR ${CMAKE_CURRENT_LIST_DIR}/libmad
             ${CMAKE_CURRENT_LIST_DIR}/libmad/msvc++)

  target_sources(libmad PRIVATE ${CSRCS})
  target_include_directories(libmad PRIVATE ${INCDIR})
  target_compile_options(libmad PRIVATE ${CFLAGS})

endif()
