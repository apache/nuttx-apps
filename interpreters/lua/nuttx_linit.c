/****************************************************************************
 * apps/interpreters/lua/nuttx_linit.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stddef.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <nuttx/config.h>

#include "luamod_proto.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef LUA_GNAME
#  define LUA_GNAME "_G"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const luaL_Reg g_loadedlibs[] =
{
#ifdef CONFIG_INTERPRETER_LUA_CORELIBS
  {LUA_GNAME, luaopen_base},
#endif
#include "luamod_list.h"
  {NULL, NULL},
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: luaL_openlibs
 *
 *   Make core and user-defined modules available to the Lua interpreter.
 *   This function is called from lua.c during interpreter initialization.
 *
 ****************************************************************************/

void luaL_openlibs(lua_State *L)
{
  const luaL_Reg *lib;
  for (lib = g_loadedlibs; lib->func; lib++)
    {
      luaL_requiref(L, lib->name, lib->func, 1);
      lua_pop(L, 1);
    }
}
