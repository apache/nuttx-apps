# ##############################################################################
# cmake/nuttx_add_luamod.cmake
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

set(LUAMOD_DIR ${CMAKE_BINARY_DIR}/luamod)

if(NOT EXISTS {LUAMOD_DIR})
  file(MAKE_DIRECTORY ${LUAMOD_DIR})
endif()

include(nuttx_parse_function_args)

# ~~~
# nuttx_add_luamod
#
# Description:
#    Register custom Lua MOD to Lua interpreter
#
# Example:
#  nuttx_add_luamod(
#    MODS
#    ${custom_mods}
#
# ~~~

function(nuttx_add_luamod)

  # parse arguments into variables

  nuttx_parse_function_args(
    FUNC
    nuttx_add_luamod
    MULTI_VALUE
    MODS
    REQUIRED
    MODS
    ARGN
    ${ARGN})

  set(LUAMOD_LIST ${LUAMOD_DIR}/luamod_list.h)
  set(LUAMOD_PROTO ${LUAMOD_DIR}/luamod_proto.h)
  foreach(mod ${MODS})
    set(MOD_PDAT ${LUAMOD_DIR}/${mod}.pdat)
    set(MOD_BDAT ${LUAMOD_DIR}/${mod}.bdat)
    add_custom_command(
      OUTPUT ${MOD_PDAT} ${MOD_BDAT}
      COMMAND ${CMAKE_COMMAND} -E echo "{\"${mod}\", luaopen_${mod}}," >>
              ${LUAMOD_LIST}
      COMMAND ${CMAKE_COMMAND} -E echo "int luaopen_${mod}(lua_State *L);" >>
              ${LUAMOD_PROTO}
      COMMAND ${CMAKE_COMMAND} -E touch ${MOD_PDAT}
      COMMAND ${CMAKE_COMMAND} -E touch ${MOD_BDAT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
      VERBATIM
      COMMENT "Lua Mod:Gen and Updating lua mod => ${mod}")
    add_custom_target(lua_${mod} DEPENDS ${MOD_PDAT} ${MOD_BDAT})
    add_dependencies(apps_context lua_${mod})
  endforeach()

endfunction()
