# ##############################################################################
# apps/examples/sotest/main/CMakeLists.txt
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

if(CONFIG_EXAMPLES_SOTEST)

  # FIXME: fix all empty a after the kernel build is implemented
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sotest_symtab.c
    COMMAND
      ${NUTTX_APPS_DIR}/tools/mksymtab.sh ${CMAKE_CURRENT_BINARY_DIR}/empty
      g_sot > ${CMAKE_CURRENT_BINARY_DIR}/sotest_symtab.c)

  add_custom_target(
    sotest_romfs
    COMMAND genromfs -f sotest_romfs.img -d empty
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  add_custom_command(
    OUTPUT sotest_romfs.c
    COMMAND xxd -i sotest_romfs.img > sotest_romfs.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS sotest_romfs)

  nuttx_add_application(
    NAME sotest SRCS sotest_main.c ${CMAKE_CURRENT_BINARY_DIR}/sotest_symtab.c
    ${CMAKE_CURRENT_BINARY_DIR}/sotest_romfs.c)

endif()
