# ##############################################################################
# apps/system/microros/CMakeLists.txt
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

if(CONFIG_SYSTEM_MICROROS)
  set(MICROROS_DIR ${CMAKE_CURRENT_LIST_DIR})
  set(MICROROS_LIB_DIR ${MICROROS_DIR}/micro_ros_lib)
  set(MICROROS_DISTRO ${CONFIG_MICROROS_DISTRO})

  include(ExternalProject)

  ExternalProject_Add(
    microros_build
    SOURCE_DIR ${MICROROS_LIB_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND
      ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
      AR=${CMAKE_AR} CFLAGS=${CMAKE_C_FLAGS} CXXFLAGS=${CMAKE_CXX_FLAGS} make -C
      ${MICROROS_DIR} libmicroros.a
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE TRUE
    BUILD_BYPRODUCTS ${MICROROS_DIR}/libmicroros.a)

  nuttx_add_library(microros STATIC IMPORTED GLOBAL)
  set_target_properties(
    microros PROPERTIES IMPORTED_LOCATION ${MICROROS_DIR}/libmicroros.a
                        INTERFACE_INCLUDE_DIRECTORIES ${MICROROS_DIR}/include)
  add_dependencies(microros microros_build)

  # Transport glue compiled into the apps tree
  set(MICROROS_TRANSPORT_SRCS ${MICROROS_DIR}/transport/microros_transport.c)

  if(CONFIG_MICROROS_TRANSPORT_UDP)
    list(APPEND MICROROS_TRANSPORT_SRCS
         ${MICROROS_DIR}/transport/microros_transport_udp.c)
  endif()

  if(CONFIG_MICROROS_TRANSPORT_SERIAL)
    list(APPEND MICROROS_TRANSPORT_SRCS
         ${MICROROS_DIR}/transport/microros_transport_serial.c)
  endif()

  nuttx_add_library(microros_transport STATIC)
  target_sources(microros_transport PRIVATE ${MICROROS_TRANSPORT_SRCS})
  target_include_directories(microros_transport
                             PRIVATE ${MICROROS_DIR}/transport)
  target_include_directories(microros_transport PUBLIC ${MICROROS_DIR}/include)
  add_dependencies(microros_transport microros_build)
endif()
