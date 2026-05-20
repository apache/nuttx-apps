# ##############################################################################
# apps/system/nxdiag/CMakeLists.txt
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

if(CONFIG_SYSTEM_NXDIAG)

  find_program(PYTHON3 python3)
  if(NOT PYTHON3)
    message(FATAL_ERROR "python3 not found in PATH (required for nxdiag)")
  endif()

  set(NXDIAG_TOOLSDIR ${NUTTX_DIR}/tools)
  set(NXDIAG_SYSINFO ${CMAKE_CURRENT_SOURCE_DIR}/sysinfo.h)

  # Build host_info_dump.py argument list

  set(NXDIAG_FLAGS ${NUTTX_DIR})

  if(CONFIG_SYSTEM_NXDIAG_CONF)
    list(APPEND NXDIAG_FLAGS --config)
  endif()

  if(CONFIG_SYSTEM_NXDIAG_COMP_FLAGS)
    list(APPEND NXDIAG_FLAGS --flags "${CMAKE_C_FLAGS}" "${CMAKE_CXX_FLAGS}"
         "${CMAKE_EXE_LINKER_FLAGS}")
  endif()

  if(CONFIG_SYSTEM_NXDIAG_HOST_PACKAGES)
    list(APPEND NXDIAG_FLAGS --packages)
  endif()

  if(CONFIG_SYSTEM_NXDIAG_HOST_MODULES)
    list(APPEND NXDIAG_FLAGS --modules)
  endif()

  if(CONFIG_SYSTEM_NXDIAG_HOST_PATH)
    list(APPEND NXDIAG_FLAGS --path)
  endif()

  if(CONFIG_SYSTEM_NXDIAG_VERBOSE)
    list(APPEND NXDIAG_FLAGS --verbose)
  endif()

  set(NXDIAG_PY_FLAGS "")

  # Vendor-specific checks

  if(CONFIG_SYSTEM_NXDIAG_ESPRESSIF)
    list(APPEND NXDIAG_FLAGS --target_info)
    list(APPEND NXDIAG_PY_FLAGS -B)
  endif()

  # Generate sysinfo.h

  add_custom_command(
    OUTPUT ${NXDIAG_SYSINFO}
    COMMAND ${PYTHON3} ${NXDIAG_PY_FLAGS} ${NXDIAG_TOOLSDIR}/host_info_dump.py
            ${NXDIAG_FLAGS} -b ${CMAKE_BINARY_DIR} > ${NXDIAG_SYSINFO}
    DEPENDS ${NXDIAG_TOOLSDIR}/host_info_dump.py
    COMMENT "Generating nxdiag sysinfo.h"
    VERBATIM)

  add_custom_target(nxdiag_sysinfo_gen DEPENDS ${NXDIAG_SYSINFO})

  nuttx_add_application(
    NAME
    nxdiag
    SRCS
    nxdiag.c
    PRIORITY
    ${CONFIG_SYSTEM_NXDIAG_PRIORITY}
    STACKSIZE
    ${CONFIG_SYSTEM_NXDIAG_STACKSIZE}
    INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
    nxdiag_sysinfo_gen)

  set_property(
    DIRECTORY ${CMAKE_SOURCE_DIR}
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${NXDIAG_SYSINFO})

endif()
