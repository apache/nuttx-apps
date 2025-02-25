# ##############################################################################
# apps/testing/enet/CMakeLists.txt
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

if(CONFIG_ENET_TEST)

  set(ENETTEST_INCDIR ${CMAKE_CURRENT_LIST_DIR}/include
                      ${CMAKE_CURRENT_LIST_DIR}/util)
  file(GLOB ENETTEST_CSRC ${CMAKE_CURRENT_LIST_DIR}/src/*.c
       ${CMAKE_CURRENT_LIST_DIR}/util/*.c)

  nuttx_add_application(
    NAME
    cmocka_enet
    SRCS
    ${CMAKE_CURRENT_LIST_DIR}/enet_entry.c
    ${ENETTEST_CSRC}
    STACKSIZE
    ${CONFIG_ENET_TEST_STACKSIZE}
    PRIORITY
    ${CONFIG_ENET_TEST_PRIORITY}
    INCLUDE_DIRECTORIES
    ${ENETTEST_INCDIR}
    DEPENDS
    cmocka)

endif()
