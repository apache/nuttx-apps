# ##############################################################################
# apps/examples/pdcurses/CMakeLists.txt
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

if(CONFIG_EXAMPLES_PDCURSES)
  target_sources(apps PRIVATE tui.c)
  nuttx_add_application(
    NAME
    charset
    PRIORITY
    ${CONFIG_EXAMPLES_PDCURSES_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_PDCURSES_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_PDCURSES}
    SRCS
    charset_main.c)

  nuttx_add_application(
    NAME
    firework
    PRIORITY
    ${CONFIG_EXAMPLES_PDCURSES_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_PDCURSES_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_PDCURSES}
    SRCS
    firework_main.c)

  nuttx_add_application(
    NAME
    newdemo
    PRIORITY
    ${CONFIG_EXAMPLES_PDCURSES_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_PDCURSES_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_PDCURSES}
    SRCS
    newdemo_main.c)

  nuttx_add_application(
    NAME
    panel
    PRIORITY
    ${CONFIG_EXAMPLES_PDCURSES_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_PDCURSES_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_PDCURSES}
    SRCS
    panel_main.c)

  nuttx_add_application(
    NAME
    rain
    PRIORITY
    ${CONFIG_EXAMPLES_PDCURSES_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_PDCURSES_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_PDCURSES}
    SRCS
    rain_main.c)

  nuttx_add_application(
    NAME
    testcurs
    PRIORITY
    ${CONFIG_EXAMPLES_PDCURSES_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_PDCURSES_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_PDCURSES}
    SRCS
    testcurs_main.c)

  nuttx_add_application(
    NAME
    tui
    PRIORITY
    ${CONFIG_EXAMPLES_PDCURSES_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_PDCURSES_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_PDCURSES}
    SRCS
    tui_main.c)

  nuttx_add_application(
    NAME
    worm
    PRIORITY
    ${CONFIG_EXAMPLES_PDCURSES_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_PDCURSES_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_PDCURSES}
    SRCS
    worm_main.c)

  nuttx_add_application(
    NAME
    xmas
    PRIORITY
    ${CONFIG_EXAMPLES_PDCURSES_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_PDCURSES_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_PDCURSES}
    SRCS
    xmas_main.c)
endif()
