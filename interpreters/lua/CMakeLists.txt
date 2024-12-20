# ##############################################################################
# apps/interpreters/lua/CMakeLists.txt
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

if(CONFIG_INTERPRETERS_LUA)

  # ############################################################################
  # Config and Fetch lua
  # ############################################################################

  set(LUA_DIR ${CMAKE_CURRENT_LIST_DIR}/lua)

  if(NOT EXISTS ${LUA_DIR})
    set(LUA_URL https://github.com/lua/lua/archive/refs/tags/)
    FetchContent_Declare(
      lua_fetch
      URL ${LUA_URL}/v${CONFIG_INTERPRETER_LUA_VERSION}.tar.gz SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/lua BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/interpreters/lua/lua
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(lua_fetch)

    if(NOT lua_fetch_POPULATED)
      FetchContent_Populate(lua_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -DLUA_MAXINPUT=${CONFIG_INTERPRETER_LUA_IOBUFSIZE}
             -DLUA_PROGNAME=\"lua\")
  if(CONFIG_INTERPRETER_LUA_32BITS)
    list(APPEND CFLAGS -DLUA_32BITS)
  endif()
  if(NOT "${CONFIG_INTERPRETER_LUA_PATH}" STREQUAL "")
    list(APPEND CFLAGS -DLUA_PATH_DEFAULT=\"${CONFIG_INTERPRETER_LUA_PATH}\")
  endif()

  if(NOT "${CONFIG_INTERPRETER_LUA_CPATH}" STREQUAL "")
    list(APPEND CFLAGS -DLUA_CPATH_DEFAULT=\"${CONFIG_INTERPRETER_LUA_CPATH}\")
  endif()

  if(CONFIG_SYSTEM_READLINE)
    set(LUA_SYSTEM_READLINE_DEF
        [=[
   #define lua_initreadline(L) ((void)L)
   #define lua_readline(L,b,p) ((void)L,fputs(p,stdout),fflush(stdout),readline_stream(b,LUA_MAXINPUT,stdin,stdout))
   #define lua_saveline(L,line) {(void)L;(void)line;}
   #define lua_freeline(L,line) {(void)L;(void)b;}
   ]=])

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/def_lua_readline.h
         "${LUA_SYSTEM_READLINE_DEF}")
    list(APPEND CFLAGS "SHELL:-include \"system/readline.h\"")
    list(APPEND CFLAGS
         "SHELL:-include ${CMAKE_CURRENT_BINARY_DIR}/def_lua_readline.h")

  endif()

  # ############################################################################
  # Sources
  # ############################################################################

  file(GLOB CORELIBS_SRCS ${LUA_DIR}/*lib.c)
  list(REMOVE_ITEM CORELIBS_SRCS ${LUA_DIR}/lauxlib.c)
  file(GLOB CSRCS ${LUA_DIR}/*.c)
  list(
    REMOVE_ITEM
    CSRCS
    ${LUA_DIR}/onelua.c
    ${LUA_DIR}/luac.c
    ${LUA_DIR}/linit.c
    ${LUA_DIR}/lua.c
    ${CORELIBS_SRCS})
  list(APPEND CSRCS nuttx_linit.c)

  if(CONFIG_INTERPRETER_LUA_CORELIBS)
    list(APPEND CSRCS ${CORELIBS_SRCS})

    set(LUA_LIB_H_PATH ${LUA_DIR}/lualib.h)
    file(READ "${LUA_LIB_H_PATH}" FILE_CONTENTS)
    string(REGEX MATCHALL "#define[ \t]+LUA_[A-Z]+LIBNAME[ \t]+\"[^\"]+\""
                 LIB_DEFINITIONS "${FILE_CONTENTS}")
    foreach(DEFINITION IN LISTS LIB_DEFINITIONS)
      string(REGEX REPLACE ".*\"([^\"]+)\"" "\\1" LIB_NAME "${DEFINITION}")
      # register lua core mod
      nuttx_add_luamod(MODS ${LIB_NAME})
    endforeach()
  endif()

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${LUAMOD_DIR})

  # ############################################################################
  # Library Configuration
  # ############################################################################

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${LUA_DIR})

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  nuttx_add_application(
    MODULE
    ${CONFIG_INTERPRETERS_LUA}
    NAME
    lua
    STACKSIZE
    ${CONFIG_INTERPRETER_LUA_STACKSIZE}
    PRIORITY
    ${CONFIG_INTERPRETER_LUA_PRIORITY}
    SRCS
    ${LUA_DIR}/lua.c
    ${CSRCS}
    INCLUDE_DIRECTORIES
    ${INCDIR}
    COMPILE_FLAGS
    ${CFLAGS})

endif()
