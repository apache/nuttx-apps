# ##############################################################################
# apps/system/toybox/CMakeLists.txt
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

if(CONFIG_SYSTEM_TOYBOX)

  set(TOYBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/toybox)
  set(TOYBOX_VERSION "${CONFIG_SYSTEM_TOYBOX_VERSION}")
  set(TOYBOX_STAMP ${TOYBOX_DIR}/.nuttx_patched-${TOYBOX_VERSION})

  # ############################################################################
  # Download and patch the pinned release
  # ############################################################################

  if(NOT EXISTS ${TOYBOX_DIR}/.git)
    if(NOT EXISTS ${TOYBOX_STAMP})
      # Reset the subbuild dir so a failed patch retries from a fresh archive
      set(TOYBOX_SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/toybox-subbuild)
      file(REMOVE_RECURSE ${TOYBOX_SUBBUILD_DIR} ${TOYBOX_DIR})

      FetchContent_Declare(
        toybox_fetch
        URL "${CONFIG_SYSTEM_TOYBOX_URL}/${TOYBOX_VERSION}.tar.gz"
            DOWNLOAD_DIR
            ${CMAKE_CURRENT_LIST_DIR}
            SOURCE_DIR
            ${TOYBOX_DIR}
            BINARY_DIR
            ${NUTTX_BINARY_DIR}/apps/system/toybox/toybox
            SUBBUILD_DIR
            ${TOYBOX_SUBBUILD_DIR}
        DOWNLOAD_NO_PROGRESS true
        TIMEOUT 120)

      FetchContent_GetProperties(toybox_fetch)

      if(NOT toybox_fetch_POPULATED)
        FetchContent_Populate(toybox_fetch)
      endif()

      find_program(TOYBOX_PATCH_EXEC patch REQUIRED)

      file(GLOB TOYBOX_PATCHES ${CMAKE_CURRENT_LIST_DIR}/patch/*.patch)
      list(SORT TOYBOX_PATCHES)

      foreach(p ${TOYBOX_PATCHES})
        message(STATUS "Applying: ${p}")
        execute_process(COMMAND ${TOYBOX_PATCH_EXEC} -s -N -p1 -d ${TOYBOX_DIR}
                                -i ${p} RESULT_VARIABLE TOYBOX_PATCH_RESULT)

        if(NOT TOYBOX_PATCH_RESULT EQUAL 0)
          message(FATAL_ERROR "Failed to apply Toybox patch: ${p}")
        endif()
      endforeach()

      file(TOUCH ${TOYBOX_STAMP})
    endif()
  endif()

  # ############################################################################
  # Generate Toybox headers and the applet source list from NuttX's .config
  # ############################################################################

  set(TOYBOX_NUTTX_CONFIG ${NUTTX_BINARY_DIR}/.config)
  set(TOYBOX_ALLCONFIG ${CMAKE_CURRENT_LIST_DIR}/.toybox_allconfig)
  set(TOYBOX_TOYFILES ${TOYBOX_DIR}/generated/toyfiles.txt)

  # Toggling an applet in menuconfig must regenerate the applet list
  set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${TOYBOX_NUTTX_CONFIG})

  file(READ ${TOYBOX_NUTTX_CONFIG} TOYBOX_NUTTX_CONFIG_TEXT)
  string(REGEX MATCHALL "CONFIG_SYSTEM_TOYBOX_CMD_[A-Z0-9_]+"
               TOYBOX_ENABLED_CMDS ${TOYBOX_NUTTX_CONFIG_TEXT})

  set(TOYBOX_ALLCONFIG_TEXT "")

  foreach(cmd ${TOYBOX_ENABLED_CMDS})
    string(REPLACE "CONFIG_SYSTEM_TOYBOX_CMD_" "CONFIG_" cmd "${cmd}")
    string(APPEND TOYBOX_ALLCONFIG_TEXT "${cmd}=y\n")
  endforeach()

  string(APPEND TOYBOX_ALLCONFIG_TEXT "CONFIG_TOYBOX_FLOAT=y\n")
  string(APPEND TOYBOX_ALLCONFIG_TEXT "CONFIG_TOYBOX_HELP=y\n")
  string(APPEND TOYBOX_ALLCONFIG_TEXT "CONFIG_TOYBOX_HELP_DASHDASH=y\n")
  string(APPEND TOYBOX_ALLCONFIG_TEXT "CONFIG_TOYBOX_NORECURSE=y\n")

  # No fork() on the target: xfork()'s call to it would dangle at link time
  if(NOT CONFIG_ARCH_HAVE_FORK)
    string(APPEND TOYBOX_ALLCONFIG_TEXT "CONFIG_TOYBOX_FORCE_NOMMU=y\n")
  endif()

  # Regenerate only when the applet selection changed
  set(TOYBOX_REGENERATE FALSE)

  if(NOT EXISTS ${TOYBOX_TOYFILES})
    set(TOYBOX_REGENERATE TRUE)
  else()
    file(READ ${TOYBOX_ALLCONFIG} TOYBOX_ALLCONFIG_PREVIOUS)

    if(NOT TOYBOX_ALLCONFIG_PREVIOUS STREQUAL TOYBOX_ALLCONFIG_TEXT)
      set(TOYBOX_REGENERATE TRUE)
    endif()
  endif()

  if(TOYBOX_REGENERATE)
    file(WRITE ${TOYBOX_ALLCONFIG} ${TOYBOX_ALLCONFIG_TEXT})

    find_program(TOYBOX_HOSTCC NAMES cc gcc REQUIRED)

    message(STATUS "Generating Toybox headers for NuttX (${TOYBOX_VERSION})")

    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -E env "KCONFIG_ALLCONFIG=${TOYBOX_ALLCONFIG}"
        "CC=${TOYBOX_HOSTCC}" "HOSTCC=${TOYBOX_HOSTCC}" "CROSS_COMPILE=" bash
        scripts/genconfig.sh -n
      WORKING_DIRECTORY ${TOYBOX_DIR}
      RESULT_VARIABLE TOYBOX_GENCONFIG_RESULT)

    if(NOT TOYBOX_GENCONFIG_RESULT EQUAL 0)
      message(FATAL_ERROR "Toybox genconfig.sh failed "
                          "(${TOYBOX_GENCONFIG_RESULT})")
    endif()

    # make.sh appends its "FILES=..." recipe to generated/build.sh instead of
    # overwriting it, so remove the previous run's block first
    file(REMOVE ${TOYBOX_DIR}/generated/build.sh)

    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -E env "CC=${TOYBOX_HOSTCC}" "HOSTCC=${TOYBOX_HOSTCC}"
        "CROSS_COMPILE=" "NOBUILD=1"
        "NUTTX_TOYBOX_MULTIPLEX_NAME=toybox_multiplex" bash scripts/make.sh
      WORKING_DIRECTORY ${TOYBOX_DIR}
      RESULT_VARIABLE TOYBOX_MAKESH_RESULT)

    if(NOT TOYBOX_MAKESH_RESULT EQUAL 0)
      message(FATAL_ERROR "Toybox make.sh failed (${TOYBOX_MAKESH_RESULT})")
    endif()

    file(READ ${TOYBOX_DIR}/generated/build.sh TOYBOX_BUILD_SH)
    string(REGEX MATCH "FILES=\"\n([^\"]*)\n\"" TOYBOX_FILES_MATCH
                 ${TOYBOX_BUILD_SH})

    if(NOT TOYBOX_FILES_MATCH)
      message(FATAL_ERROR "No FILES block found in generated/build.sh")
    endif()

    string(REPLACE "\n" " " TOYBOX_FILES_TEXT "${CMAKE_MATCH_1}")
    separate_arguments(TOYBOX_FILES_LIST NATIVE_COMMAND "${TOYBOX_FILES_TEXT}")
    list(REMOVE_DUPLICATES TOYBOX_FILES_LIST)
    list(REMOVE_ITEM TOYBOX_FILES_LIST "main.c" "")

    if(NOT TOYBOX_FILES_LIST)
      message(FATAL_ERROR "No Toybox applets selected")
    endif()

    string(REPLACE ";" "\n" TOYBOX_FILES_OUT "${TOYBOX_FILES_LIST}")
    file(WRITE ${TOYBOX_TOYFILES} "${TOYBOX_FILES_OUT}\n")
  endif()

  # ############################################################################
  # Application registration
  # ############################################################################

  # toybox_entry.c is the entry point (its main() is aliased to toybox_main);
  # the rest is upstream Toybox: lib/*.c in full plus the selected applets
  file(STRINGS ${TOYBOX_TOYFILES} TOYBOX_APPLET_SRCS)
  list(TRANSFORM TOYBOX_APPLET_SRCS PREPEND ${TOYBOX_DIR}/)
  file(GLOB TOYBOX_LIB_SRCS ${TOYBOX_DIR}/lib/*.c)

  set(TOYBOX_DEFINITIONS __NuttX__ "TOYBOX_VERSION=\"${TOYBOX_VERSION}-nuttx\"")

  if(CONFIG_SYSTEM_TOYBOX_BUILTIN_BRIDGE)
    list(APPEND TOYBOX_DEFINITIONS CONFIG_SYSTEM_TOYBOX_BUILTIN_BRIDGE=1)
  endif()

  nuttx_add_application(
    NAME
    toybox
    SRCS
    ${TOYBOX_DIR}/toybox_entry.c
    ${TOYBOX_DIR}/nuttx_bridge.c
    ${TOYBOX_DIR}/main.c
    ${TOYBOX_LIB_SRCS}
    ${TOYBOX_APPLET_SRCS}
    STACKSIZE
    ${CONFIG_SYSTEM_TOYBOX_STACKSIZE}
    PRIORITY
    ${CONFIG_SYSTEM_TOYBOX_PRIORITY}
    MODULE
    ${CONFIG_SYSTEM_TOYBOX}
    INCLUDE_DIRECTORIES
    ${TOYBOX_DIR}
    ${TOYBOX_DIR}/generated
    ${TOYBOX_DIR}/lib
    COMPILE_FLAGS
    -include${CMAKE_CURRENT_LIST_DIR}/compat_nuttx.h
    # An option, not a definition: sim undefines __linux__ at directory level,
    # which precedes target definitions but not target options
    -D__linux__
    # Toybox conventions trip NuttX's default warning set
    -Wno-shadow
    -Wno-char-subscripts
    -Wno-strict-prototypes
    -Wno-restrict
    DEFINITIONS
    ${TOYBOX_DEFINITIONS})

endif()
