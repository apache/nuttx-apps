# ##############################################################################
# apps/examples/elf/main/CMakeLists.txt
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

if(CONFIG_EXAMPLES_ELF)
  set(ELF_SYMTAB ${CMAKE_CURRENT_BINARY_DIR}/test_symtab.c)
  set(ELF_ROMFS_IMG ${CMAKE_CURRENT_BINARY_DIR}/elf_romfs.img)
  set(ELF_ROMFS_SRC ${CMAKE_CURRENT_BINARY_DIR}/elf_romfs.c)
  set(ELF_GEN_SYMTAB ${CMAKE_CURRENT_SOURCE_DIR}/gen_symtab.py)
  set(ELF_GEN_ROMFS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/gen_romfs_source.py)

  set(ELFNAME_BASE hello)

  set(ELF_BINARY_FILES)
  set(ELF_BINARY_TARGETS)
  foreach(ELFNAME IN LISTS ELFNAME_BASE)
    list(APPEND ELF_BINARY_FILES ${CMAKE_BINARY_DIR}/bin/${ELFNAME})

    if(CMAKE_C_ELF_COMPILER)
      list(APPEND ELF_BINARY_TARGETS ${ELFNAME})
    else()
      list(APPEND ELF_BINARY_TARGETS ELF_${ELFNAME})
    endif()
  endforeach()

  add_custom_command(
    OUTPUT ${ELF_SYMTAB}
    COMMAND ${Python3_EXECUTABLE} ${ELF_GEN_SYMTAB} ${ELF_BINARY_FILES} g_elf
            ${ELF_SYMTAB}
    DEPENDS ${ELF_BINARY_TARGETS} ${ELF_GEN_SYMTAB}
    VERBATIM)

  set_source_files_properties(${ELF_SYMTAB} PROPERTIES GENERATED TRUE)

  set(ELF_GENERATED_OUTPUTS ${ELF_SYMTAB})
  set(ELF_SRCS elf_main.c ${ELF_SYMTAB})

  if(CONFIG_EXAMPLES_ELF_ROMFS)
    set(ELF_ROMFS_DEPS ${ELF_BINARY_TARGETS})

    if(CONFIG_SYSTEM_NXPKG)
      set(ELF_INDEX_JSON ${CMAKE_BINARY_DIR}/bin/index.json)
      set(ELF_BAD_INDEX_JSON ${CMAKE_BINARY_DIR}/bin/bad-index.json)
      set(ELF_PKGTEST ${CMAKE_BINARY_DIR}/bin/pkgtest.nsh)
      set(ELF_PKGFAIL ${CMAKE_BINARY_DIR}/bin/pkgfail.nsh)
      set(ELF_HELLO_BIN ${CMAKE_BINARY_DIR}/bin/hello)
      set(ELF_HELLO_TARGET hello)

      if(NOT CMAKE_C_ELF_COMPILER)
        set(ELF_HELLO_TARGET ELF_hello)
      endif()

      add_custom_command(
        OUTPUT ${ELF_INDEX_JSON} ${ELF_BAD_INDEX_JSON} ${ELF_PKGTEST}
               ${ELF_PKGFAIL}
        COMMAND
          ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/mk_pkg_fixture.py
          ${ELF_HELLO_BIN} ${ELF_INDEX_JSON} ${ELF_BAD_INDEX_JSON}
          ${ELF_PKGTEST} ${ELF_PKGFAIL} ${CONFIG_ARCH} ${CONFIG_ARCH_BOARD}
        DEPENDS ${ELF_HELLO_TARGET}
                ${CMAKE_CURRENT_SOURCE_DIR}/mk_pkg_fixture.py
        VERBATIM)

      list(APPEND ELF_ROMFS_DEPS ${ELF_INDEX_JSON} ${ELF_BAD_INDEX_JSON}
           ${ELF_PKGTEST} ${ELF_PKGFAIL})
    endif()

    add_custom_command(
      OUTPUT ${ELF_ROMFS_IMG}
      COMMAND genromfs -f ${ELF_ROMFS_IMG} -d ${CMAKE_BINARY_DIR}/bin
      DEPENDS ${ELF_ROMFS_DEPS}
      VERBATIM)

    add_custom_command(
      OUTPUT ${ELF_ROMFS_SRC}
      COMMAND ${Python3_EXECUTABLE} ${ELF_GEN_ROMFS_SRC} ${ELF_ROMFS_IMG}
              ${ELF_ROMFS_SRC}
      DEPENDS ${ELF_ROMFS_IMG} ${ELF_GEN_ROMFS_SRC}
      VERBATIM)

    set_source_files_properties(${ELF_ROMFS_SRC} PROPERTIES GENERATED TRUE)

    list(APPEND ELF_GENERATED_OUTPUTS ${ELF_ROMFS_SRC})
    list(APPEND ELF_SRCS ${ELF_ROMFS_SRC})
  endif()

  add_custom_target(elf_artifacts DEPENDS ${ELF_GENERATED_OUTPUTS})

  nuttx_add_application(NAME elf SRCS ${ELF_SRCS} DEPENDS elf_artifacts)

  # TODO: tests

endif()
