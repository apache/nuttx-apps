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
    COMMAND ${NUTTX_APPS_DIR}/tools/mksymtab.sh ${ELF_BINARY_FILES} g_elf >
            ${ELF_SYMTAB}
    DEPENDS ${ELF_BINARY_TARGETS})

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
      set(ELF_FIXTURE_HOST_DIR ${CMAKE_CURRENT_BINARY_DIR}/host)
      set(ELF_FIXTURE_HOST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/host/CMakeLists.txt)
      set(ELF_FIXTURE_TOOL
          ${ELF_FIXTURE_HOST_DIR}/mk_pkg_fixture${CMAKE_EXECUTABLE_SUFFIX})
      set(ELF_HELLO_TARGET hello)

      if(CONFIG_ARCH_BOARD)
        set(ELF_FIXTURE_COMPAT ${CONFIG_ARCH_BOARD})
      else()
        set(ELF_FIXTURE_COMPAT ${CONFIG_ARCH_BOARD_CUSTOM_NAME})
      endif()

      if(NOT CMAKE_C_ELF_COMPILER)
        set(ELF_HELLO_TARGET ELF_hello)
      endif()

      add_custom_command(
        OUTPUT ${ELF_FIXTURE_TOOL}
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/host -B
                ${ELF_FIXTURE_HOST_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${ELF_FIXTURE_HOST_DIR} --target
                mk_pkg_fixture
        DEPENDS ${ELF_FIXTURE_HOST_SRC}
                ${CMAKE_CURRENT_SOURCE_DIR}/mk_pkg_fixture.c
        VERBATIM)

      add_custom_command(
        OUTPUT ${ELF_INDEX_JSON} ${ELF_BAD_INDEX_JSON} ${ELF_PKGTEST}
               ${ELF_PKGFAIL}
        COMMAND
          ${ELF_FIXTURE_TOOL} ${ELF_HELLO_BIN} ${ELF_INDEX_JSON}
          ${ELF_BAD_INDEX_JSON} ${ELF_PKGTEST} ${ELF_PKGFAIL} ${CONFIG_ARCH}
          ${ELF_FIXTURE_COMPAT}
        DEPENDS ${ELF_HELLO_TARGET} ${ELF_FIXTURE_TOOL}
        VERBATIM)

      list(APPEND ELF_ROMFS_DEPS ${ELF_INDEX_JSON} ${ELF_BAD_INDEX_JSON}
           ${ELF_PKGTEST} ${ELF_PKGFAIL})
    endif()

    add_custom_command(
      OUTPUT ${ELF_ROMFS_IMG}
      COMMAND genromfs -f ${ELF_ROMFS_IMG} -d ${CMAKE_BINARY_DIR}/bin
      DEPENDS ${ELF_ROMFS_DEPS}
      VERBATIM)

    get_filename_component(ELF_ROMFS_IMG_NAME ${ELF_ROMFS_IMG} NAME)

    # xxd -i derives the generated array/length symbol names from the exact path
    # it is given - elf_main.c declares them as plain
    # "elf_romfs_img"/"elf_romfs_img_len", so xxd must be run against just the
    # bare filename (via WORKING_DIRECTORY) rather than the absolute
    # ${ELF_ROMFS_IMG} path, or every non-alphanumeric character in that path
    # (e.g. every "/" in the build directory) gets folded into the symbol name
    # instead, leaving the names elf_main.c expects undefined at link time.

    add_custom_command(
      OUTPUT ${ELF_ROMFS_SRC}
      COMMAND xxd -i ${ELF_ROMFS_IMG_NAME} > ${ELF_ROMFS_SRC}
      DEPENDS ${ELF_ROMFS_IMG}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    set_source_files_properties(${ELF_ROMFS_SRC} PROPERTIES GENERATED TRUE)

    list(APPEND ELF_GENERATED_OUTPUTS ${ELF_ROMFS_SRC})
    list(APPEND ELF_SRCS ${ELF_ROMFS_SRC})
  endif()

  add_custom_target(elf_artifacts DEPENDS ${ELF_GENERATED_OUTPUTS})

  nuttx_add_application(NAME elf SRCS ${ELF_SRCS} DEPENDS elf_artifacts)

  # TODO: tests

endif()
