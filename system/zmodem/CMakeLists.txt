# ##############################################################################
# apps/system/zmodem/CMakeLists.txt
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

if(CONFIG_SYSTEM_ZMODEM)
  set(CSRCS zm_send.c zm_receive.c zm_state.c zm_proto.c zm_watchdog.c
            zm_utils.c)

  nuttx_add_application(
    MODULE
    ${CONFIG_SYSTEM_ZMODEM}
    NAME
    sz
    STACKSIZE
    ${CONFIG_SYSTEM_ZMODEM_STACKSIZE}
    PRIORITY
    ${CONFIG_SYSTEM_ZMODEM_PRIORITY}
    SRCS
    sz_main.c
    ${CSRCS})

  nuttx_add_application(
    MODULE
    ${CONFIG_SYSTEM_ZMODEM}
    NAME
    rz
    STACKSIZE
    ${CONFIG_SYSTEM_ZMODEM_STACKSIZE}
    PRIORITY
    ${CONFIG_SYSTEM_ZMODEM_PRIORITY}
    SRCS
    rz_main.c
    ${CSRCS})
endif()
