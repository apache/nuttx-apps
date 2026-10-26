# ##############################################################################
# apps/examples/flowc/CMakeLists.txt
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

if(CONFIG_EXAMPLES_FLOWC)

  # Target 1: the receiver, or the sender with EXAMPLES_FLOWC_SENDER1

  if(CONFIG_EXAMPLES_FLOWC_SENDER1)
    set(SRCS1 flowc_sender.c)
    set(SRCS2 flowc_receiver.c)
  else()
    set(SRCS1 flowc_receiver.c)
    set(SRCS2 flowc_sender.c)
  endif()

  nuttx_add_application(
    NAME
    ${CONFIG_EXAMPLES_FLOWC_PROGNAME1}
    PRIORITY
    ${CONFIG_EXAMPLES_FLOWC_PRIORITY1}
    STACKSIZE
    ${CONFIG_EXAMPLES_FLOWC_STACKSIZE1}
    MODULE
    ${CONFIG_EXAMPLES_FLOWC}
    INCLUDE_DIRECTORIES
    ${NUTTX_BINARY_DIR}/include/nuttx
    SRCS
    flowc_target1.c
    ${SRCS1})

  # Target 2: the other end, when it is a NuttX target and not the host

  if(CONFIG_EXAMPLES_FLOWC_TARGET2)
    nuttx_add_application(
      NAME
      ${CONFIG_EXAMPLES_FLOWC_PROGNAME2}
      PRIORITY
      ${CONFIG_EXAMPLES_FLOWC_PRIORITY2}
      STACKSIZE
      ${CONFIG_EXAMPLES_FLOWC_STACKSIZE2}
      MODULE
      ${CONFIG_EXAMPLES_FLOWC}
      INCLUDE_DIRECTORIES
      ${NUTTX_BINARY_DIR}/include/nuttx
      SRCS
      flowc_target2.c
      ${SRCS2})
  endif()
endif()
