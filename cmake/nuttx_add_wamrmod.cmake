# ##############################################################################
# cmake/nuttx_add_wamrmod.cmake
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

set(WAMR_MODULE_DIR ${CMAKE_BINARY_DIR}/wamrmod)

if(NOT EXISTS {WAMR_MODULE_DIR})
  file(MAKE_DIRECTORY ${WAMR_MODULE_DIR})
endif()

include(nuttx_parse_function_args)

# ~~~
# nuttx_add_wamrmod
#
# Description:
#    Register custom module into WAMR runtime
#
# Example:
#  nuttx_add_wamrmod(
#    MODS
#    ${custom_mods}
#  )
# ~~~

function(nuttx_add_wamrmod)

  # parse arguments into variables

  nuttx_parse_function_args(
    FUNC
    nuttx_add_wamrmod
    MULTI_VALUE
    MODS
    REQUIRED
    MODS
    ARGN
    ${ARGN})

  set(WAMR_MODULE_LIST ${WAMR_MODULE_DIR}/wamr_external_module_list.h)
  set(WAMR_MODULE_PROTO ${WAMR_MODULE_DIR}/wamr_external_module_proto.h)
  foreach(mod ${MODS})
    set(MOD_PDAT ${WAMR_MODULE_DIR}/${mod}.pdat)
    set(MOD_BDAT ${WAMR_MODULE_DIR}/${mod}.bdat)
    add_custom_command(
      OUTPUT ${MOD_PDAT} ${MOD_BDAT}
      COMMAND ${CMAKE_COMMAND} -E echo "wamr_module_${mod}_register," >>
              ${WAMR_MODULE_LIST}
      COMMAND ${CMAKE_COMMAND} -E echo "bool wamr_module_${mod}_register(void);"
              >> ${WAMR_MODULE_PROTO}
      COMMAND ${CMAKE_COMMAND} -E touch ${MOD_PDAT}
      COMMAND ${CMAKE_COMMAND} -E touch ${MOD_BDAT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
      VERBATIM
      COMMENT "WAMR Module: Gen and Updating external module => ${mod}")
    add_custom_target(wamr_module_${mod} DEPENDS ${MOD_PDAT} ${MOD_BDAT})
    add_dependencies(apps_context wamr_module_${mod})
  endforeach()

endfunction()
