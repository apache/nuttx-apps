# ##############################################################################
# apps/interpreters/luamodules/luasyslog/CMakeLists.txt
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

if(CONFIG_LUA_LSYSLOG_MODULE)

  # ############################################################################
  # Config and Fetch luasyslog
  # ############################################################################

  set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/lsyslog)

  if(NOT EXISTS ${SRC_DIR})
    set(LSYSLOG_URL https://github.com/lunarmodules/luasyslog/archive/refs/tags)
    FetchContent_Declare(
      lsyslog_fetch
      URL ${LSYSLOG_URL}/${CONFIG_LUA_LSYSLOG_VERSION}.tar.gz SOURCE_DIR
          ${SRC_DIR}
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(lsyslog_fetch)

    if(NOT lua_fetch_POPULATED)
      FetchContent_Populate(lsyslog_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################
  set(LUA_SYSTEM_READLINE_DEF
      [=[
#define openlog(a,b,c) {(void)a; (void)c;}
#define closelog() {}
]=])

  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/def_lua_readline.h
       "${LUA_SYSTEM_READLINE_DEF}")
  list(APPEND CFLAGS
       "SHELL:-include ${CMAKE_CURRENT_BINARY_DIR}/def_lua_readline.h")

  # ############################################################################
  # Sources
  # ############################################################################

  set(CSRCS ${SRC_DIR}/lsyslog.c)

  # ############################################################################
  # Include Directory
  # ############################################################################

  # ############################################################################
  # Library Configuration
  # ############################################################################

  target_sources(apps PRIVATE ${CSRCS})

  # register lua mod
  nuttx_add_luamod(MODS lsyslog)
endif()
