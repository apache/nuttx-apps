# ##############################################################################
# apps/examples/unionfs/CMakeLists.txt
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

if(CONFIG_EXAMPLES_UNIONFS)

  add_custom_target(
    testdir
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_LIST_DIR}/atestdir atestdir
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_LIST_DIR}/btestdir btestdir
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "EXAMPLES UNIONFS Copy test dir...")

  add_custom_target(
    testdir_img
    COMMAND genromfs -f atestdir.img -d atestdir -V "UNIONFS_FS1"
    COMMAND genromfs -f btestdir.img -d btestdir -V "UNIONFS_FS2"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS testdir
    COMMENT "EXAMPLES UNIONFS Generating testdir.img...")

  add_custom_target(
    romfs_testdir_h
    COMMAND xxd -i atestdir.img > romfs_atestdir.h
    COMMAND xxd -i btestdir.img > romfs_btestdir.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS testdir_img
    COMMENT "EXAMPLES UNIONFS Generating romfs_testdir.h...")

  nuttx_add_application(
    NAME
    unionfs
    MODULE
    ${CONFIG_EXAMPLES_UNIONFS}
    SRCS
    unionfs_main.c
    INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
    romfs_testdir_h)
endif()
