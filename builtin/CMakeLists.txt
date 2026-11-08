# ##############################################################################
# apps/builtin/CMakeLists.txt
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

if(CONFIG_BUILTIN)

  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  # generate registry
  get_property(nuttx_app_libs GLOBAL PROPERTY NUTTX_APPS_LIBRARIES)
  get_property(only_registers GLOBAL PROPERTY NUTTX_APPS_ONLY_REGISTER)
  list(APPEND nuttx_app_libs ${only_registers})
  set(builtin_list_string)
  set(builtin_proto_string)
  foreach(module ${nuttx_app_libs})

    # builtin_list.h Example: { "hello", SCHED_PRIORITY_DEFAULT, 2048,
    # hello_main },
    #
    get_target_property(APP_MAIN ${module} APP_MAIN)
    get_target_property(APP_NAME ${module} APP_NAME)
    get_target_property(APP_PRIORITY ${module} APP_PRIORITY)
    get_target_property(APP_STACK ${module} APP_STACK)

    # With SCHED_USER_IDENTITY, an entry also has uid, gid and mode: the UID,
    # GID and MODE of the application, else 0, 0 and 0555, as REGISTER in
    # Make.defs gives.

    set(APP_IDENTITY)
    if(CONFIG_SCHED_USER_IDENTITY)
      get_target_property(APP_UID ${module} APP_UID)
      get_target_property(APP_GID ${module} APP_GID)
      get_target_property(APP_MODE ${module} APP_MODE)
      if(NOT APP_UID)
        set(APP_UID 0)
      endif()
      if(NOT APP_GID)
        set(APP_GID 0)
      endif()
      if(NOT APP_MODE)
        set(APP_MODE 0555)
      endif()
      set(APP_IDENTITY ", ${APP_UID}, ${APP_GID}, ${APP_MODE}")
    endif()

    set(builtin_list_string
        "${builtin_list_string}\{ \"${APP_NAME}\", ${APP_PRIORITY}, ${APP_STACK}, ${APP_MAIN}${APP_IDENTITY} \},  \n"
    )

    # builtin_proto.h Example: int hello_main(int argc, char *argv[]);
    set(builtin_proto_string
        "${builtin_proto_string}int ${APP_MAIN}(int argc, char *argv[]);\n")

  endforeach()

  configure_file(builtin_proto.h.in builtin_proto.h)
  configure_file(builtin_list.h.in builtin_list.h)

  set(CSRCS)

  list(APPEND CSRCS builtin_list.c exec_builtin.c builtin_list.h
       builtin_proto.h)

  # target_sources(apps PRIVATE ${CSRCS})
  nuttx_add_library(apps_builtin ${CSRCS})

endif()
