# ##############################################################################
# apps/interpreters/python/CMakeLists.txt
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

if(CONFIG_INTERPRETERS_CPYTHON)

  # ############################################################################
  # Config and Fetch CPython
  # ############################################################################

  set(CPYTHON_VERSION ${CONFIG_INTERPRETERS_CPYTHON_VERSION})
  string(REGEX MATCH "^[0-9]+\\.[0-9]+" CPYTHON_VERSION_MINOR
               ${CPYTHON_VERSION})
  string(REPLACE "." "" CPYTHON_VERSION_NODOT ${CPYTHON_VERSION_MINOR})

  set(CPYTHON_PATH ${CMAKE_CURRENT_LIST_DIR}/Python)

  if(NOT EXISTS ${CPYTHON_PATH})
    file(GLOB CPYTHON_PATCHES ${CMAKE_CURRENT_LIST_DIR}/patch/*.patch)
    list(SORT CPYTHON_PATCHES)
    set(CPYTHON_PATCH_COMMAND)
    foreach(patch ${CPYTHON_PATCHES})
      list(
        APPEND
        CPYTHON_PATCH_COMMAND
        COMMAND
        patch
        -p1
        -d
        ${CPYTHON_PATH}
        <
        ${patch})
    endforeach()
    list(REMOVE_AT CPYTHON_PATCH_COMMAND 0)

    FetchContent_Declare(
      cpython_fetch
      URL https://github.com/python/cpython/archive/v${CPYTHON_VERSION}.zip
          SOURCE_DIR ${CPYTHON_PATH} BINARY_DIR
          ${CMAKE_CURRENT_BINARY_DIR}/cpython_fetch
      PATCH_COMMAND ${CPYTHON_PATCH_COMMAND}
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 120)

    FetchContent_GetProperties(cpython_fetch)

    if(NOT cpython_fetch_POPULATED)
      FetchContent_Populate(cpython_fetch)
    endif()
  endif()

  # ############################################################################
  # Build directories
  # ############################################################################

  set(BUILDIR ${CMAKE_CURRENT_BINARY_DIR}/build)
  set(INSTALLDIR ${CMAKE_CURRENT_BINARY_DIR}/install)
  set(HOSTBUILD ${BUILDIR}/host)
  set(HOSTINSTALL ${INSTALLDIR}/host)
  set(HOSTPYTHON ${HOSTINSTALL}/bin/python3)
  set(TARGETBUILD ${BUILDIR}/target)
  set(TARGETINSTALL ${INSTALLDIR}/target)
  set(TARGETMODULES ${TARGETINSTALL}/lib)
  set(TARGETLIBPYTHON ${TARGETINSTALL}/libpython${CPYTHON_VERSION_MINOR}.a)
  set(TARGETMODULESPACK ${TARGETBUILD}/lib/python${CPYTHON_VERSION_NODOT}.zip)
  set(TARGETSTDLIB ${TARGETMODULES}/python${CPYTHON_VERSION_MINOR})
  set(BUNDLED_WHEELS_DIR ${CPYTHON_PATH}/Lib/ensurepip/_bundled)

  # ############################################################################
  # config.site and Setup.local
  # ############################################################################

  # The `config.site` file contains settings that override the configuration
  # settings provided by the `configure` script.  Depending on the features
  # enabled on NuttX, this file may need to be adjusted.

  set(CONFIG_SITE ${CMAKE_CURRENT_BINARY_DIR}/config.site)
  file(READ ${CMAKE_CURRENT_LIST_DIR}/config.site.in CONFIG_SITE_CONTENT)

  macro(cpython_config_site var enabled)
    if(${enabled})
      string(APPEND CONFIG_SITE_CONTENT "export ${var}=\"yes\"\n")
    else()
      string(APPEND CONFIG_SITE_CONTENT "export ${var}=\"no\"\n")
    endif()
  endmacro()

  cpython_config_site(ac_cv_func_fork CONFIG_ARCH_HAVE_FORK)
  cpython_config_site(ac_cv_func_vfork CONFIG_ARCH_HAVE_VFORK)
  cpython_config_site(ac_cv_func_system CONFIG_SYSTEM_SYSTEM)
  cpython_config_site(ac_cv_func_getaddrinfo CONFIG_NET)
  cpython_config_site(ac_cv_func_getnameinfo CONFIG_NET)
  file(WRITE ${CONFIG_SITE} "${CONFIG_SITE_CONTENT}")

  # The `Setup.local` file disables Python modules: every module listed after
  # the *disabled* line of `Setup.local.in` is not built.

  set(SETUP_LOCAL ${CMAKE_CURRENT_BINARY_DIR}/Setup.local)
  file(READ ${CMAKE_CURRENT_LIST_DIR}/Setup.local.in SETUP_LOCAL_CONTENT)

  # _posixsubprocess needs a real fork(): its vfork() support is only an
  # optimization and the fork() fallback path is compiled unconditionally.

  if(NOT CONFIG_ARCH_HAVE_FORK)
    string(APPEND SETUP_LOCAL_CONTENT "_posixsubprocess\n")
  endif()
  if(NOT CONFIG_LIBC_DLFCN)
    string(APPEND SETUP_LOCAL_CONTENT "_ctypes\n")
  endif()
  if(NOT CONFIG_NET)
    string(APPEND SETUP_LOCAL_CONTENT "_socket\n")
  endif()
  file(WRITE ${SETUP_LOCAL} "${SETUP_LOCAL_CONTENT}")

  # ############################################################################
  # Host Python
  # ############################################################################

  add_custom_command(
    OUTPUT ${HOSTPYTHON}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${HOSTBUILD} ${HOSTINSTALL}
    COMMAND ${CMAKE_COMMAND} -E chdir ${HOSTBUILD} ${CPYTHON_PATH}/configure
            --with-pydebug --prefix=${HOSTINSTALL} --disable-test-modules
    COMMAND make -C ${HOSTBUILD} install
    COMMENT "CPython: building the host Python"
    VERBATIM)

  # ############################################################################
  # Target Python
  # ############################################################################

  # CPython's configure needs the compiler flags NuttX compiles C sources with:
  # the toolchain flags (add_compile_options() in the arch Toolchain.cmake), the
  # global NuttX options, definitions and include directories.  Generator
  # expressions in them are evaluated by add_custom_command(), except
  # $<COMPILE_LANGUAGE:...>, which is resolved here for C.

  get_directory_property(CPYTHON_DIR_OPTIONS COMPILE_OPTIONS)
  set(CPYTHON_OPTIONS)
  foreach(opt ${CPYTHON_DIR_OPTIONS})
    string(REGEX MATCHALL "\\$<COMPILE_LANGUAGE:[^>]*>" langs "${opt}")
    foreach(lang ${langs})
      string(REGEX REPLACE "^\\$<COMPILE_LANGUAGE:(.*)>$" "\\1" names "${lang}")
      string(REPLACE "," ";" names "${names}")
      if("C" IN_LIST names)
        string(REPLACE "${lang}" "1" opt "${opt}")
      else()
        string(REPLACE "${lang}" "0" opt "${opt}")
      endif()
    endforeach()
    list(APPEND CPYTHON_OPTIONS "${opt}")
  endforeach()

  # Python's configure script requires the optimization flags in OPT

  list(REMOVE_ITEM CPYTHON_OPTIONS -Os)

  set(CPYTHON_NUTTX_DEFINITIONS
      $<GENEX_EVAL:$<TARGET_PROPERTY:nuttx,NUTTX_DEFINITIONS>>)
  set(CPYTHON_DEFINITIONS
      "$<$<BOOL:${CPYTHON_NUTTX_DEFINITIONS}>:-D$<JOIN:${CPYTHON_NUTTX_DEFINITIONS}, -D>>"
  )

  set(CPYTHON_INCLUDES
      ${NUTTX_DIR}/include
      ${NUTTX_BINARY_DIR}/include
      $<GENEX_EVAL:$<TARGET_PROPERTY:nuttx,NUTTX_INCLUDE_DIRECTORIES>>
      $<GENEX_EVAL:$<TARGET_PROPERTY:nuttx_apps_interface,APPS_INCLUDE_DIRECTORIES>>
      ${CPYTHON_PATH}/Include
      ${CPYTHON_PATH}/Include/internal
      ${CMAKE_CURRENT_SOURCE_DIR}/../../system
      ${CMAKE_CURRENT_SOURCE_DIR}/../../system/zlib/zlib
      ${TARGETBUILD})

  set(CPYTHON_WARNINGS
      -Wno-shadow
      -Wno-undef
      -Wno-format
      -Wno-builtin-macro-redefined
      -Wno-type-limits
      -Wno-implicit-fallthrough
      -Wno-char-subscripts
      -Wno-sign-compare
      -Wno-unused-const-variable
      -Wno-unused-function
      -Wno-unused-variable
      -Wno-overflow
      -Wno-unused-but-set-variable
      -Wno-strict-prototypes
      -Wno-maybe-uninitialized)

  # CPPFLAGS carries the -D/-U macro flags so preprocessor-only probes
  # (Misc/platform_triplet.c) do not see the build host's __linux__ when NuttX
  # is built with the native gcc (x86_64).

  set(CPYTHON_MACRO_OPTIONS ${CPYTHON_OPTIONS})
  list(FILTER CPYTHON_MACRO_OPTIONS INCLUDE REGEX "^-[DU]")

  set(CPYTHON_CPPFLAGS
      "$<JOIN:${CPYTHON_MACRO_OPTIONS}, > ${CPYTHON_DEFINITIONS}")
  set(CPYTHON_CFLAGS
      "${CMAKE_C_FLAGS} $<JOIN:${CPYTHON_OPTIONS}, > \
$<JOIN:$<GENEX_EVAL:$<TARGET_PROPERTY:nuttx,NUTTX_COMPILE_OPTIONS>>, > \
${CPYTHON_DEFINITIONS} -I$<JOIN:${CPYTHON_INCLUDES}, -I> \
$<JOIN:${CPYTHON_WARNINGS}, > -nostdlib")

  if(CONFIG_NET)
    set(CPYTHON_CONFIGURE_EXTRAS "")
  else()
    set(CPYTHON_CONFIGURE_EXTRAS --disable-ipv6)
  endif()

  string(REPLACE "-" "" CPYTHON_ARCH "${CONFIG_ARCH}")
  string(REPLACE "-" "" CPYTHON_ARCH_CHIP "${CONFIG_ARCH_CHIP}")

  execute_process(
    COMMAND ${CPYTHON_PATH}/config.guess
    OUTPUT_VARIABLE CPYTHON_BUILD
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # CMAKE_AR is the bare archiver (no operation flags), so ARFLAGS carries them

  add_custom_command(
    OUTPUT ${TARGETBUILD}/Makefile
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TARGETBUILD}/Modules
            ${TARGETSTDLIB}
    COMMAND ${CMAKE_COMMAND} -E copy ${SETUP_LOCAL}
            ${TARGETBUILD}/Modules/Setup.local
    COMMAND
      ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
      AR=${CMAKE_AR} ARFLAGS=rcs "CFLAGS=${CPYTHON_CFLAGS}"
      "CPPFLAGS=${CPYTHON_CPPFLAGS}" MACHDEP=nuttx OPT=-Os
      CONFIG_SITE=${CONFIG_SITE} ${CPYTHON_PATH}/configure --disable-shared
      --prefix=${TARGETINSTALL}
      --host=${CPYTHON_ARCH}-${CPYTHON_ARCH_CHIP}-nuttx --build=${CPYTHON_BUILD}
      --with-build-python=${HOSTPYTHON} --without-mimalloc --without-pymalloc
      --disable-test-modules --with-ensurepip=no ${CPYTHON_CONFIGURE_EXTRAS}
    COMMAND sed -i "s/^#define HAVE_LIBB2 1/\\/* #undef HAVE_LIBB2 *\\//"
            pyconfig.h
    COMMAND sed -i "s/-lb2//g" Makefile
    WORKING_DIRECTORY ${TARGETBUILD}
    DEPENDS ${HOSTPYTHON} ${CONFIG_SITE} ${SETUP_LOCAL}
    COMMENT "CPython: configuring the target Python"
    VERBATIM)

  # Bundled wheels: pip and nuttx-periphery are downloaded with the host system
  # pip (pip 24.x vendored by the build Python breaks on 3.13) and pre-compiled
  # with the build Python, which must match the embedded CPython version.

  set(CPYTHON_WHEELS_COMMAND)
  if(CONFIG_INTERPRETERS_CPYTHON_ENABLE_PIP)
    list(
      APPEND
      CPYTHON_WHEELS_COMMAND
      COMMAND
      sh
      -c
      "set -e; v=$(${HOSTPYTHON} -c 'import ensurepip; print(ensurepip._PIP_VERSION)'); \
w=${BUNDLED_WHEELS_DIR}/pip-$v-py3-none-any.whl; \
[ -f $w ] || python3 -m pip download --only-binary=:all: --no-deps --dest ${BUNDLED_WHEELS_DIR} pip==$v; \
${HOSTPYTHON} ${CMAKE_CURRENT_LIST_DIR}/repack_wheel_add_pyc.py --package pip $w"
    )
  endif()
  if(CONFIG_INTERPRETERS_CPYTHON_INSTALL_NUTTX_PACKAGE)
    list(
      APPEND
      CPYTHON_WHEELS_COMMAND
      COMMAND
      sh
      -c
      "set -e; w=$(ls ${BUNDLED_WHEELS_DIR}/nuttx_periphery-*-py3-none-any.whl 2>/dev/null | head -1 || true); \
if [ -z \"$w\" ]; then python3 -m pip download --only-binary=:all: --no-deps --dest ${BUNDLED_WHEELS_DIR} nuttx-periphery; \
w=$(ls ${BUNDLED_WHEELS_DIR}/nuttx_periphery-*-py3-none-any.whl); fi; \
${HOSTPYTHON} ${CMAKE_CURRENT_LIST_DIR}/repack_wheel_add_pyc.py --package nuttx_periphery $w"
    )
  endif()

  # Pre-install the bundled wheels into the target sys.path (zipimport)

  set(CPYTHON_SITE_COMMAND)
  if(CPYTHON_WHEELS_COMMAND)
    list(
      PREPEND
      CPYTHON_WHEELS_COMMAND
      COMMAND
      ${CMAKE_COMMAND}
      -E
      make_directory
      ${BUNDLED_WHEELS_DIR})
    set(CPYTHON_SITE_PACKAGES ${TARGETSTDLIB}/site-packages)
    list(
      APPEND
      CPYTHON_SITE_COMMAND
      COMMAND
      ${CMAKE_COMMAND}
      -E
      make_directory
      ${CPYTHON_SITE_PACKAGES}
      COMMAND
      sh
      -c
      "set -e; : > ${CPYTHON_SITE_PACKAGES}/bundled_wheels.pth; \
for w in ${TARGETSTDLIB}/ensurepip/_bundled/*.whl; do [ -f $w ] || continue; \
echo ../ensurepip/_bundled/$(basename $w) >> ${CPYTHON_SITE_PACKAGES}/bundled_wheels.pth; done"
    )
  endif()

  add_custom_command(
    OUTPUT ${TARGETLIBPYTHON} ${TARGETBUILD}/Modules/_hacl/libHacl_Hash_SHA2.a
           ${TARGETBUILD}/Modules/expat/libexpat.a ${CPYTHON_WHEELS_COMMAND}
    COMMAND make -C ${TARGETBUILD} regen-frozen
    COMMAND make -C ${TARGETBUILD} libpython${CPYTHON_VERSION_MINOR}.a
            wasm_stdlib
    COMMAND
      ${CMAKE_COMMAND} -E copy
      ${TARGETBUILD}/libpython${CPYTHON_VERSION_MINOR}.a ${TARGETLIBPYTHON}
    COMMAND unzip -q -o ${TARGETMODULESPACK} -d ${TARGETSTDLIB}
            ${CPYTHON_SITE_COMMAND}
    DEPENDS ${TARGETBUILD}/Makefile
    COMMENT "CPython: building the target Python"
    VERBATIM)

  # ############################################################################
  # Python modules in ROMFS
  # ############################################################################

  set(CPYTHON_ROMFS_IMG ${CMAKE_CURRENT_BINARY_DIR}/romfs_cpython_modules.img)
  set(CPYTHON_ROMFS_HDR ${CMAKE_CURRENT_BINARY_DIR}/romfs_cpython_modules.h)

  add_custom_command(
    OUTPUT ${CPYTHON_ROMFS_HDR}
    COMMAND genromfs -f ${CPYTHON_ROMFS_IMG} -d ${TARGETMODULES} -V "ROMFS_Test"
    COMMAND
      ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR} sh -c
      "xxd -i romfs_cpython_modules.img | sed -e 's/^unsigned/static const unsigned/g' > romfs_cpython_modules.h"
    DEPENDS ${TARGETLIBPYTHON}
    COMMENT "CPython: packing the Python modules into ROMFS"
    VERBATIM)

  add_custom_target(cpython_build DEPENDS ${CPYTHON_ROMFS_HDR})

  # ############################################################################
  # Libraries
  # ############################################################################

  foreach(lib
          ${TARGETLIBPYTHON} ${TARGETBUILD}/Modules/_hacl/libHacl_Hash_SHA2.a
          ${TARGETBUILD}/Modules/expat/libexpat.a)
    nuttx_add_extra_library(${lib})
    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" lib_target "${lib}")
    add_dependencies(${lib_target} cpython_build)
  endforeach()

  # ############################################################################
  # Applications
  # ############################################################################

  nuttx_add_application(
    NAME
    ${CONFIG_INTERPRETERS_CPYTHON_PROGNAME}
    SRCS
    python_wrapper.c
    STACKSIZE
    ${CONFIG_INTERPRETERS_CPYTHON_STACKSIZE}
    PRIORITY
    ${CONFIG_INTERPRETERS_CPYTHON_PRIORITY}
    INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CPYTHON_PATH}/Include
    ${CPYTHON_PATH}/Include/internal
    ${TARGETBUILD}
    COMPILE_FLAGS
    -Wno-shadow
    -Wno-undef
    -Wno-strict-prototypes
    DEPENDS
    cpython_build)

endif()
