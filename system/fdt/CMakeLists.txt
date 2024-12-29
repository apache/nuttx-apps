# ##############################################################################
# apps/system/fdt/CMakeLists.txt
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

if(CONFIG_SYSTEM_FDT)
  nuttx_add_library(sytem_fdt STATIC)

  set(LIBC_FDT_DIR ${NUTTX_DIR}/libs/libc/fdt/)
  set(CSRCS ${LIBC_FDT_DIR}/dtc/util.c)

  set(INCDIR ${LIBC_FDT_DIR})
  list(APPEND INCDIR ${LIBC_FDT_DIR}/dtc/libfdt)

  target_sources(sytem_fdt PRIVATE ${CSRCS})
  target_include_directories(sytem_fdt PRIVATE ${INCDIR})

  if(CONFIG_SYSTEM_FDTDUMP)
    nuttx_add_application(
      NAME
      fdtdump
      SRCS
      ${LIBC_FDT_DIR}/dtc/fdtdump.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      ${CONFIG_SYSTEM_FDTDUMP_STACKSIZE}
      PRIORITY
      ${CONFIG_SYSTEM_FDTDUMP_PRIORITY}
      DEPENDS
      sytem_fdt)
  endif()

  if(CONFIG_SYSTEM_FDTGET)
    nuttx_add_application(
      NAME
      fdtget
      SRCS
      ${LIBC_FDT_DIR}/dtc/fdtget.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      ${CONFIG_SYSTEM_FDTGET_STACKSIZE}
      PRIORITY
      ${CONFIG_SYSTEM_FDTGET_PRIORITY}
      DEPENDS
      sytem_fdt)
  endif()

  if(CONFIG_SYSTEM_FDTPUT)
    nuttx_add_application(
      NAME
      fdtput
      SRCS
      ${LIBC_FDT_DIR}/dtc/fdtput.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      ${CONFIG_SYSTEM_FDTPUT_STACKSIZE}
      PRIORITY
      ${CONFIG_SYSTEM_FDTPUT_PRIORITY}
      DEPENDS
      sytem_fdt)
  endif()

  if(CONFIG_SYSTEM_FDTOVERLAY)
    nuttx_add_application(
      NAME
      fdtoverlay
      SRCS
      ${LIBC_FDT_DIR}/dtc/fdtoverlay.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      ${CONFIG_SYSTEM_FDTOVERLAY_STACKSIZE}
      PRIORITY
      ${CONFIG_SYSTEM_FDTOVERLAY_PRIORITY}
      DEPENDS
      sytem_fdt)
  endif()

endif()
