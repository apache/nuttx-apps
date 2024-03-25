# ##############################################################################
# apps/testing/stressapptest/CMakeLists.txt
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

if(CONFIG_TESTING_STRESSAPPTEST)

  set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/stressapptest/src)

  set(SRCS
      ${SRC_DIR}/main.cc
      ${SRC_DIR}/adler32memcpy.cc
      ${SRC_DIR}/disk_blocks.cc
      ${SRC_DIR}/error_diag.cc
      ${SRC_DIR}/finelock_queue.cc
      ${SRC_DIR}/logger.cc
      ${SRC_DIR}/os.cc
      ${SRC_DIR}/os_factory.cc
      ${SRC_DIR}/pattern.cc
      ${SRC_DIR}/queue.cc
      ${SRC_DIR}/sat.cc
      ${SRC_DIR}/sat_factory.cc
      ${SRC_DIR}/worker.cc)

  set(FLAGS -DHAVE_CONFIG_H -fpermissive)

  nuttx_add_application(
    NAME
    stressapptest
    PRIORITY
    ${CONFIG_TESTING_STRESSAPPTEST_PRIORITY}
    STACKSIZE
    ${CONFIG_TESTING_STRESSAPPTEST_STACKSIZE}
    MODULE
    ${CONFIG_TESTING_STRESSAPPTEST}
    COMPILE_FLAGS
    ${FLAGS}
    SRCS
    ${SRCS})

endif()
