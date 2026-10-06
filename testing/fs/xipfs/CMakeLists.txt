# ##############################################################################
# apps/testing/fs/xipfs/CMakeLists.txt
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

if(CONFIG_TESTING_FS_XIPFS)
  set(SRCS xipfs_main.c)
  set(BLOBTARGETS)

  # With CONFIG_FDPIC the test also loads modules.  They are built in
  # examples/fdpicxip/modules, and embedded here after they are linked.

  if(CONFIG_FDPIC)
    set(BLOBSRC ${CMAKE_CURRENT_BINARY_DIR}/xipfs_blobs.c)
    set(BLOBFILES)

    foreach(
      blob
      callback
      cxxuser
      funcdesc
      lazymod
      libcounter.so
      libshape.so
      manyneeded
      missingsym
      user)
      list(APPEND BLOBFILES ${NUTTX_BINARY_DIR}/bin/${blob})

      if(CMAKE_C_ELF_COMPILER)
        list(APPEND BLOBTARGETS ${blob})
      else()
        list(APPEND BLOBTARGETS ELF_${blob})
      endif()
    endforeach()

    add_custom_command(
      OUTPUT ${BLOBSRC}
      COMMAND ${NUTTX_APPS_DIR}/examples/fdpicxip/modules/mkblobs.sh g_xipfs
              ${BLOBFILES} > ${BLOBSRC}
      DEPENDS ${BLOBTARGETS})

    add_custom_target(xipfs_blobs DEPENDS ${BLOBSRC})

    list(APPEND SRCS ${BLOBSRC})
    set(BLOBTARGETS xipfs_blobs)
  endif()

  nuttx_add_application(
    NAME
    xipfs_test
    STACKSIZE
    ${CONFIG_DEFAULT_TASK_STACKSIZE}
    MODULE
    ${CONFIG_TESTING_FS_XIPFS}
    SRCS
    ${SRCS}
    DEPENDS
    ${BLOBTARGETS})
endif()
