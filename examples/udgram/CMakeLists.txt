# ##############################################################################
# apps/examples/udgram/CMakeLists.txt
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

if(CONFIG_EXAMPLES_UDGRAM)
  nuttx_add_application(
    NAME
    ${CONFIG_EXAMPLES_UDGRAM_CLIENT_PROGNAME}
    PRIORITY
    ${CONFIG_EXAMPLES_UDGRAM_CLIENT_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_UDGRAM_CLIENT_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_UDGRAM}
    SRCS
    udgram_client.c)

  nuttx_add_application(
    NAME
    ${CONFIG_EXAMPLES_UDGRAM_SERVER_PROGNAME}
    PRIORITY
    ${CONFIG_EXAMPLES_UDGRAM_SERVER_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_UDGRAM_SERVER_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_UDGRAM}
    SRCS
    udgram_server.c)
endif()
