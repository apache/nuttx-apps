# ##############################################################################
# apps/netutils/connectedhomeip/CMakeLists.txt
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

if(CONFIG_MATTER)

  include(ExternalProject)

  function(chip_download_and_patch)

    # parse arguments into variables

    nuttx_parse_function_args(
      FUNC
      chip_download_and_patch
      ONE_VALUE
      NAME
      URL
      SOURCE_DIR
      BINARY_DIR
      TIMEOUT
      MULTI_VALUE
      PATCH_LIST
      ARGN
      ${ARGN})

    if(NOT EXISTS ${SOURCE_DIR})
      FetchContent_Declare(
        ${NAME}_fetch
        URL ${URL} SOURCE_DIR ${SOURCE_DIR} BINARY_DIR ${BINARY_DIR}
        DOWNLOAD_NO_PROGRESS true
        TIMEOUT ${TIMEOUT})

      FetchContent_GetProperties(${NAME}_fetch)

      if(NOT ${NAME}_fetch_POPULATED)
        FetchContent_Populate(${NAME}_fetch)
      endif()

      if(PATCH_LIST)
        foreach(PATCH ${PATCH_LIST})
          execute_process(COMMAND sh -c "patch -p1 < ${PATCH}"
                          WORKING_DIRECTORY ${SOURCE_DIR})
        endforeach()
      endif()

    endif()

  endfunction()

  get_filename_component(
    CHIP_ROOT ${NUTTX_APPS_DIR}/netutils/connectedhomeip/connectedhomeip
    REALPATH)

  chip_download_and_patch(
    NAME
    chip
    URL
    https://github.com/project-chip/connectedhomeip/archive/refs/tags/v1.2.0.1.zip
    SOURCE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/connectedhomeip
    BINARY_DIR
    ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip
    TIMEOUT
    90
    PATCH_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/0001-chip-add-compile-support-for-NuttX.patch)

  chip_download_and_patch(
    NAME
    pigweed
    URL
    https://github.com/google/pigweed/archive/1f12d06f51.zip
    SOURCE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/pigweed
    BINARY_DIR
    ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip
    TIMEOUT
    90)

  chip_download_and_patch(
    NAME
    nlio
    URL
    https://github.com/nestlabs/nlio/archive/refs/tags/1.1.8.zip
    SOURCE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/nlio
    BINARY_DIR
    ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip
    TIMEOUT
    30)

  chip_download_and_patch(
    NAME
    nlassert
    URL
    https://github.com/nestlabs/nlassert/archive/refs/tags/1.1.8.zip
    SOURCE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/nlassert
    BINARY_DIR
    ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip
    TIMEOUT
    30)

  chip_download_and_patch(
    NAME
    nlunit-test
    URL
    https://github.com/nestlabs/nlunit-test/archive/refs/tags/1.0.9.zip
    SOURCE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/nlunit-test
    BINARY_DIR
    ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip
    TIMEOUT
    30)

  chip_download_and_patch(
    NAME
    inipp
    URL
    https://github.com/mcmtroffaes/inipp/archive/refs/tags/1.0.12.zip
    SOURCE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/inipp
    BINARY_DIR
    ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip
    TIMEOUT
    30)

  get_filename_component(GN_ROOT_TARGET ${CHIP_ROOT}/config/nuttx/chip-gn
                         REALPATH)
  get_filename_component(COMMON_CMAKE_SOURCE_DIR
                         ${CHIP_ROOT}/config/common/cmake REALPATH)

  include(${COMMON_CMAKE_SOURCE_DIR}/chip_gn_args.cmake)
  include(${COMMON_CMAKE_SOURCE_DIR}/chip_gn.cmake)

  set(nuttx_include_dirs)
  get_target_property(include_dirs nuttx NUTTX_CXX_INCLUDE_DIRECTORIES)
  if(include_dirs)
    list(APPEND nuttx_include_dirs ${include_dirs})
  endif()
  list(APPEND nuttx_include_dirs ${NUTTX_DIR}/include
       ${CMAKE_BINARY_DIR}/include)
  list(APPEND nuttx_include_dirs ${NUTTX_APPS_BINDIR}/include/mbedtls)

  set(MATTER_INCDIR
      ${nuttx_include_dirs}
      ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip/gen/include
      ${NUTTX_APPS_DIR}/include
      ${NUTTX_APPS_DIR}/netutils/jsoncpp/jsoncpp/include
      inipp
      nlio/include
      nlassert/include
      connectedhomeip/src
      connectedhomeip/src/include
      connectedhomeip/src/lib/dnssd
      connectedhomeip/src/app/util/mock/include
      connectedhomeip/zzz_generated
      connectedhomeip/zzz_generated/lighting-app
      connectedhomeip/zzz_generated/app-common
      connectedhomeip/src/platform/Linux
      connectedhomeip/examples/platform/linux
      connectedhomeip/examples/lighting-app/lighting-common/include)

  set(gn_depens mbedtls)
  set(MATTER_FLAGS
      -DCHIP_HAVE_CONFIG_H
      -std=${CONFIG_CXX_STANDARD}
      -Wno-undef
      -DCHIP_ADDRESS_RESOLVE_IMPL_INCLUDE_HEADER=<lib/address_resolve/AddressResolve_DefaultImpl.h>
  )

  set(EXTERNAL_MATTER_INCDIR)

  foreach(estring ${MATTER_INCDIR})
    list(APPEND EXTERNAL_MATTER_INCDIR "\"${estring}\"")
  endforeach()

  matter_add_gn_arg_string("nuttx_ar" ${CMAKE_AR})
  matter_add_gn_arg_string("nuttx_cc" ${CMAKE_C_COMPILER})
  matter_add_gn_arg_string("nuttx_cxx" ${CMAKE_CXX_COMPILER})
  matter_add_flags("-D__NuttX__ ")

  if(CONFIG_ARCH_SIM)
    if(CONFIG_SIM_M32)
      matter_add_gn_arg_string("target_cpu" "x86")
    else()
      matter_add_gn_arg_string("target_cpu" "x86_64")
    endif()

  elseif(CONFIG_ARCH_ARM)
    matter_add_gn_arg_string("target_cpu" "arm")
  elseif(CONFIG_ARCH_RISCV)
    matter_add_gn_arg_string("target_cpu" "riscv")
    matter_add_gn_arg_string("riscv_arch" "${ARCHTYPE}${ARCHCPUEXTFLAGS}")

    if(CONFIG_ARCH_DPFPU)
      matter_add_gn_arg_string("riscv_abi" "${ARCHABITYPE}d")
    elseif(CONFIG_ARCH_FPU)
      matter_add_gn_arg_string("riscv_abi" "${ARCHABITYPE}f")
    else()
      matter_add_gn_arg_string("riscv_abi" "${ARCHABITYPE}")
    endif()

    if(CONFIG_ARCH_RV32)
      if(${ARCHCPUEXTFLAGS} STREQUAL imc)
        matter_add_gn_arg_string("riscv_cpu" "sifive-e20")
      elseif(${ARCHCPUEXTFLAGS} STREQUAL imac)
        matter_add_gn_arg_string("riscv_cpu" "sifive-e31")
      elseif(${ARCHCPUEXTFLAGS} STREQUAL imafc)
        matter_add_gn_arg_string("riscv_cpu" "sifive-e76")
      endif()
    else()
      if(${ARCHCPUEXTFLAGS} STREQUAL imac)
        matter_add_gn_arg_string("riscv_cpu" "sifive-s51")
      elseif(${ARCHCPUEXTFLAGS} STREQUAL imafdc)
        matter_add_gn_arg_string("riscv_cpu" "sifive-u54")
      endif()
    endif()

    if(CONFIG_ARCH_RV64)
      matter_add_flags("-mcmodel=medany")
    endif()
  endif()

  matter_add_gn_arg_bool("chip_inet_config_enable_ipv4" CONFIG_NET_IPv4)
  matter_add_gn_arg_bool("enable_eventlist_attribute" true)
  matter_add_gn_arg_bool("chip_enable_ble")
  matter_add_gn_arg_bool("chip_example_lighting" true)
  matter_add_gn_arg_bool("chip_config_network_layer_ble")
  if(NOT CONFIG_MATTER_LOG)
    matter_add_gn_arg_bool("chip_error_logging")
    matter_add_gn_arg_bool("chip_progress_logging")
    matter_add_gn_arg_bool("chip_detail_logging")
    matter_add_gn_arg_bool("chip_automation_logging")
    matter_add_gn_arg_bool("chip_pw_tokenizer_logging")
    matter_add_gn_arg_bool("chip_use_pw_logging")
  endif()

  matter_get_compiler_flags_from_targets(nuttx)
  matter_common_gn_args(PROJECT_CONFIG_INC_DIR ${EXTERNAL_MATTER_INCDIR})

  matter_generate_args_tmp_file()

  matter_build(
    chip
    LIB_SHELL
    ${CONFIG_CHIP_LIB_SHELL}
    LIB_PW_RPC
    ${CONFIG_CHIP_PW_RPC}
    LIB_TESTS
    ${CONFIG_CHIP_BUILD_TESTS}
    LIB_MBEDTLS
    GN_DEPENDENCIES
    ${gn_depens})

  add_custom_command(
    OUTPUT connectedhomeip/third_party/pigweed/repo
    COMMAND rm ${CHIP_ROOT}/third_party/pigweed/repo -rf
    COMMAND ln -s ${NUTTX_APPS_DIR}/netutils/connectedhomeip/pigweed
            ${CHIP_ROOT}/third_party/pigweed/repo)

  add_custom_target(chippigweed ALL
                    DEPENDS connectedhomeip/third_party/pigweed/repo)
  ExternalProject_Add_StepDependencies(chip-gn configure chippigweed)

  add_custom_command(
    OUTPUT ${CHIP_ROOT}/build_overrides/pigweed_environment.gni
    COMMAND touch ${CHIP_ROOT}/build_overrides/pigweed_environment.gni)

  add_custom_target(
    chipnpigweedenv ALL
    DEPENDS ${CHIP_ROOT}/build_overrides/pigweed_environment.gni)
  ExternalProject_Add_StepDependencies(chip-gn configure chipnpigweedenv)
  add_dependencies(chippigweed chipnpigweedenv)

  add_custom_command(
    OUTPUT connectedhomeip/third_party/nlassert/repo
    COMMAND rm ${CHIP_ROOT}/third_party/nlassert/repo -rf
    COMMAND ln -s ${NUTTX_APPS_DIR}/netutils/connectedhomeip/nlassert
            ${CHIP_ROOT}/third_party/nlassert/repo)

  add_custom_target(chipnlassert ALL
                    DEPENDS connectedhomeip/third_party/nlassert/repo)
  ExternalProject_Add_StepDependencies(chip-gn configure chipnlassert)

  add_custom_command(
    OUTPUT connectedhomeip/third_party/nlio/repo
    COMMAND rm ${CHIP_ROOT}/third_party/nlio/repo -rf
    COMMAND ln -s ${NUTTX_APPS_DIR}/netutils/connectedhomeip/nlio
            ${CHIP_ROOT}/third_party/nlio/repo)

  add_custom_target(chipnlio ALL DEPENDS connectedhomeip/third_party/nlio/repo)
  ExternalProject_Add_StepDependencies(chip-gn configure chipnlio)

  add_custom_command(
    OUTPUT connectedhomeip/third_party/nlunit-test/repo
    COMMAND rm ${CHIP_ROOT}/third_party/nlunit-test/repo -rf
    COMMAND ln -s ${NUTTX_APPS_DIR}/netutils/connectedhomeip/nlunit-test
            ${CHIP_ROOT}/third_party/nlunit-test/repo)

  add_custom_target(chipnlunit-test ALL
                    DEPENDS connectedhomeip/third_party/nlunit-test/repo)
  ExternalProject_Add_StepDependencies(chip-gn configure chipnlunit-test)

  add_custom_command(
    OUTPUT connectedhomeip/third_party/jsoncpp/repo
    COMMAND rm -rf ${CHIP_ROOT}/third_party/jsoncpp/repo
    COMMAND ln -s ${NUTTX_APPS_DIR}/netutils/jsoncpp/jsoncpp
            ${CHIP_ROOT}/third_party/jsoncpp/repo)

  add_custom_target(chipjsoncpp ALL
                    DEPENDS connectedhomeip/third_party/jsoncpp/repo)
  ExternalProject_Add_StepDependencies(chip-gn configure chipjsoncpp)

  set(DEMOSRC
      ${CHIP_ROOT}/examples/lighting-app/linux/main.cpp
      ${CHIP_ROOT}/examples/lighting-app/linux/LightingAppCommandDelegate.cpp)

  nuttx_add_application(
    NAME
    matter
    SRCS
    ${DEMOSRC}
    INCLUDE_DIRECTORIES
    ${MATTER_INCDIR}
    COMPILE_FLAGS
    ${MATTER_FLAGS}
    DEPENDS
    chip-gn
    mbedtls
    STACKSIZE
    16384
    PRIORITY
    100)

  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip/lib/libchipnuttx.a
    COMMAND echo generate libchipnuttx.a)
  add_custom_target(
    libchipnuttx ALL
    DEPENDS ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip/lib/libchipnuttx.a
  )
  add_dependencies(libchipnuttx chip-gn)

  nuttx_add_extra_library(
    ${CMAKE_BINARY_DIR}/apps/netutils/connectedhomeip/lib/libchipnuttx.a)

endif()
