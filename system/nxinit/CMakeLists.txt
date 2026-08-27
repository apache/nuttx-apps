# ##############################################################################
# apps/system/nxinit/CMakeLists.txt
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

if(CONFIG_SYSTEM_NXINIT)

  set(CSRCS init.c action.c builtin.c import.c parser.c service.c)

  list(APPEND CSRCS property_simple.c)

  nuttx_add_application(
    MODULE
    ${CONFIG_SYSTEM_NXINIT}
    NAME
    ${CONFIG_SYSTEM_NXINIT_PROGNAME}
    STACKSIZE
    ${CONFIG_SYSTEM_NXINIT_STACKSIZE}
    PRIORITY
    ${CONFIG_SYSTEM_NXINIT_PRIORITY}
    SRCS
    ${CSRCS})

  if(CONFIG_SYSTEM_NXINIT_TEST)

    # NxInit unit tests (cmocka). Unlike the Makefile build (which folds the
    # test sources into the same CSRCS used by the "init" program above, so
    # parser/action/service are compiled once and shared), CMake compiles
    # action.c/parser.c/service.c a second time into this separate
    # nxinit_unit_test target.

    set(TEST_SRCS
        test/test_nxinit.c
        action.c
        parser.c
        service.c
        test/test_nxinit_common.c
        test/test_nxinit_parser.c
        test/test_nxinit_action.c
        test/test_nxinit_service.c)

    nuttx_add_application(
      NAME
      nxinit_unit_test
      PRIORITY
      ${CONFIG_SYSTEM_NXINIT_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_SYSTEM_NXINIT_TEST_STACKSIZE}
      MODULE
      ${CONFIG_SYSTEM_NXINIT}
      DEPENDS
      cmocka
      SRCS
      ${TEST_SRCS})

  endif()

endif()
