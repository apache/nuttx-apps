# ##############################################################################
# apps/examples/usrsocktest/CMakeLists.txt
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

if(CONFIG_EXAMPLES_USRSOCKTEST)

  # USRSOCK Test built-in application info

  nuttx_add_application(
    NAME
    ${CONFIG_EXAMPLES_USRSOCKTEST_PROGNAME}
    SRCS
    usrsocktest_main.c
    STACKSIZE
    ${CONFIG_EXAMPLES_USRSOCKTEST_STACKSIZE}
    PRIORITY
    ${CONFIG_EXAMPLES_USRSOCKTEST_PRIORITY})

  # NuttX USRSOCK Test

  set(CSRCS
      usrsocktest_daemon.c
      usrsocktest_basic_connect.c
      usrsocktest_basic_daemon.c
      usrsocktest_basic_getsockname.c
      usrsocktest_basic_getsockopt.c
      usrsocktest_basic_send.c
      usrsocktest_basic_setsockopt.c
      usrsocktest_block_recv.c
      usrsocktest_block_send.c
      usrsocktest_chardev.c
      usrsocktest_multi_thread.c
      usrsocktest_noblock_connect.c
      usrsocktest_noblock_recv.c
      usrsocktest_noblock_send.c
      usrsocktest_nodaemon.c
      usrsocktest_poll.c
      usrsocktest_remote_disconnect.c
      usrsocktest_wake_with_signal.c)

  target_sources(apps PRIVATE ${CSRCS})
endif()
