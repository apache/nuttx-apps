# ##############################################################################
# apps/examples/fdpicxip/modules/manyneeded/CMakeLists.txt
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

# Names one library more in DT_NEEDED than CONFIG_LIBC_ELF_MAXDEPEND allows. The
# libraries are stubs from ../need, and only their names matter.

set(NEEDLIBS)
set(NEEDTARGETS)

foreach(n RANGE ${CONFIG_LIBC_ELF_MAXDEPEND})
  list(APPEND NEEDLIBS ${NUTTX_BINARY_DIR}/bin/need${n}.so)

  if(CMAKE_C_ELF_COMPILER)
    list(APPEND NEEDTARGETS need${n}.so)
  else()
    list(APPEND NEEDTARGETS ELF_need${n}.so)
  endif()
endforeach()

nuttx_add_application(
  NAME
  manyneeded
  SRCS
  manyneeded.c
  MODULE
  m
  LINK_FLAGS
  --no-as-needed
  ${NEEDLIBS}
  DEPENDS
  ${NEEDTARGETS})
