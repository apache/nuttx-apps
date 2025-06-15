# ##############################################################################
# apps/system/uorb/CMakeLists.txt
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

if(CONFIG_UORB)

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/system/uorb)

  nuttx_add_library(uorb STATIC)

  file(GLOB_RECURSE CSRCS "sensor/*.c" "uORB/uORB.c")

  if(CONFIG_UORB_LOOP_MAX_EVENTS)
    list(APPEND CSRCS "uORB/loop.c" "uORB/epoll.c")
  endif()

  if(CONFIG_UORB_LISTENER)
    nuttx_add_application(
      NAME
      uorb_listener
      PRIORITY
      ${CONFIG_UORB_PRIORITY}
      STACKSIZE
      ${CONFIG_UORB_STACKSIZE}
      MODULE
      ${CONFIG_UORB}
      SRCS
      listener.c
      DEPENDS
      uorb)
  endif()

  if(CONFIG_UORB_TEST)
    nuttx_add_application(
      NAME
      uorb_unit_test
      PRIORITY
      ${CONFIG_UORB_PRIORITY}
      STACKSIZE
      ${CONFIG_UORB_STACKSIZE}
      MODULE
      ${CONFIG_UORB}
      SRCS
      test/unit_test.c
      test/utility.c
      DEPENDS
      uorb)
  endif()

  target_include_directories(uorb PUBLIC .)
  target_sources(uorb PRIVATE ${CSRCS})

endif()
