# ##############################################################################
# apps/examples/ft80x/CMakeLists.txt
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

if(CONFIG_EXAMPLES_FT80X)
  nuttx_add_application(
    NAME
    ${CONFIG_EXAMPLES_FT80X_PROGNAME}
    PRIORITY
    ${CONFIG_EXAMPLES_FT80X_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_FT80X_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_FT80X}
    SRCS
    ft80x_main.c)

  set(CSRCS ft80x_coprocessor.c)
  if(CONFIG_EXAMPLES_FT80X_PRIMITIVES)
    list(APPEND CSRCS ft80x_primitives.c)
  endif()
  if(CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS)
    list(APPEND CSRCS ft80x_bitmaps.c)
  endif()
  target_sources(apps PRIVATE ${CSRCS})
endif()
