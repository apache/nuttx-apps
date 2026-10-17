# ##############################################################################
# apps/games/match4/CMakeLists.txt
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

# Match4 game info

if(CONFIG_GAMES_MATCH4)
  nuttx_add_application(
    NAME
    ${CONFIG_GAMES_MATCH4_PROGNAME}
    PRIORITY
    ${CONFIG_GAMES_MATCH4_PRIORITY}
    STACKSIZE
    ${CONFIG_GAMES_MATCH4_STACKSIZE}
    MODULE
    ${CONFIG_GAMES_MATCH4}
    SRCS
    match4_main.c)
endif()
