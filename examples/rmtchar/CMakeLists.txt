# ##############################################################################
# apps/examples/rmtchar/CMakeLists.txt
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

if(CONFIG_EXAMPLES_RMTCHAR)
  nuttx_add_application(
    NAME
    rmtchar
    STACKSIZE
    ${CONFIG_DEFAULT_TASK_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_RMTCHAR}
    SRCS
    rmtchar_common.c
    rmtchar_main.c)
  if(CONFIG_EXAMPLES_RMTCHAR_TX)
    target_sources(apps PRIVATE rmtchar_transmitter.c)
  endif()
  if(CONFIG_EXAMPLES_RMTCHAR_RX)
    target_sources(apps PRIVATE rmtchar_receiver.c)
  endif()
endif()
