# ##############################################################################
# apps/examples/sotest/main/CMakeLists.txt
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

if(CONFIG_EXAMPLES_SOTEST)
  set(SOTEST_SYMTAB ${CMAKE_CURRENT_BINARY_DIR}/sotest_symtab.c)
  set(SOTEST_ROMFS_IMG ${CMAKE_CURRENT_BINARY_DIR}/sotest_romfs.img)
  set(SOTEST_ROMFS_SRC ${CMAKE_CURRENT_BINARY_DIR}/sotest_romfs.c)
  set(SOTEST_MODPRINT_ELF ${CMAKE_BINARY_DIR}/bin/modprint)
  set(SOTEST_SHARED_ELF ${CMAKE_BINARY_DIR}/bin/sotest)

  # Dynamic applications are wrapped in ELF_* helper targets when the build uses
  # the non-ELF-capable compiler path.
  if(CMAKE_C_ELF_COMPILER)
    set(SOTEST_MODPRINT_TARGET modprint)
    set(SOTEST_SHARED_TARGET sotest)
  else()
    set(SOTEST_MODPRINT_TARGET ELF_modprint)
    set(SOTEST_SHARED_TARGET ELF_sotest)
  endif()

  add_custom_command(
    OUTPUT ${SOTEST_SYMTAB}
    COMMAND
      sh -c
      "\"${NUTTX_APPS_DIR}/tools/mksymtab.sh\" \"${SOTEST_MODPRINT_ELF}\" \"${SOTEST_SHARED_ELF}\" g_sot > \"${SOTEST_SYMTAB}\""
    DEPENDS ${SOTEST_MODPRINT_TARGET} ${SOTEST_SHARED_TARGET}
    VERBATIM)

  set_source_files_properties(${SOTEST_SYMTAB} PROPERTIES GENERATED TRUE)

  set(SOTEST_GENERATED_OUTPUTS ${SOTEST_SYMTAB})
  set(SOTEST_SRCS sotest_main.c ${SOTEST_SYMTAB})

  if(CONFIG_EXAMPLES_SOTEST_BUILTINFS)
    set(SOTEST_ROMFS_DEPS ${SOTEST_MODPRINT_TARGET} ${SOTEST_SHARED_TARGET})

    if(CONFIG_SYSTEM_NXPKG)
      set(SOTEST_SHARED_INDEX ${CMAKE_BINARY_DIR}/bin/shared-index.json)
      set(SOTEST_SHARED_SCRIPT ${CMAKE_BINARY_DIR}/bin/pkgsotest.nsh)

      add_custom_command(
        OUTPUT ${SOTEST_SHARED_INDEX} ${SOTEST_SHARED_SCRIPT}
        COMMAND
          ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/mk_pkg_fixture_shared.py
          ${SOTEST_MODPRINT_ELF} ${SOTEST_SHARED_ELF} ${SOTEST_SHARED_INDEX}
          ${SOTEST_SHARED_SCRIPT} ${CONFIG_ARCH} ${CONFIG_ARCH_BOARD}
        DEPENDS ${SOTEST_MODPRINT_TARGET} ${SOTEST_SHARED_TARGET}
                ${CMAKE_CURRENT_SOURCE_DIR}/mk_pkg_fixture_shared.py
        VERBATIM)

      list(APPEND SOTEST_ROMFS_DEPS ${SOTEST_SHARED_INDEX}
           ${SOTEST_SHARED_SCRIPT})
    endif()

    add_custom_command(
      OUTPUT ${SOTEST_ROMFS_IMG}
      COMMAND genromfs -f ${SOTEST_ROMFS_IMG} -d ${CMAKE_BINARY_DIR}/bin
      DEPENDS ${SOTEST_ROMFS_DEPS}
      VERBATIM)

    add_custom_command(
      OUTPUT ${SOTEST_ROMFS_SRC}
      COMMAND
        sh -c
        "echo '#include <nuttx/compiler.h>' > \"${SOTEST_ROMFS_SRC}\" && xxd -i \"${SOTEST_ROMFS_IMG}\" | sed -e 's/^unsigned char/const unsigned char aligned_data(4)/g' >> \"${SOTEST_ROMFS_SRC}\""
      DEPENDS ${SOTEST_ROMFS_IMG}
      VERBATIM)

    set_source_files_properties(${SOTEST_ROMFS_SRC} PROPERTIES GENERATED TRUE)

    list(APPEND SOTEST_GENERATED_OUTPUTS ${SOTEST_ROMFS_SRC})
    list(APPEND SOTEST_SRCS ${SOTEST_ROMFS_SRC})
  endif()

  add_custom_target(sotest_artifacts DEPENDS ${SOTEST_GENERATED_OUTPUTS})

  nuttx_add_application(NAME sotest SRCS ${SOTEST_SRCS} DEPENDS
                        sotest_artifacts)
endif()
