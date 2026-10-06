# ##############################################################################
# apps/examples/fdpicxip/modules/user/CMakeLists.txt
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

# Links against libcounter.so, which it names in DT_NEEDED

if(CMAKE_C_ELF_COMPILER)
  set(LIBTARGET libcounter.so)
else()
  set(LIBTARGET ELF_libcounter.so)
endif()

nuttx_add_application(
  NAME
  user
  SRCS
  user.c
  MODULE
  m
  LINK_FLAGS
  ${NUTTX_BINARY_DIR}/bin/libcounter.so
  DEPENDS
  ${LIBTARGET})
