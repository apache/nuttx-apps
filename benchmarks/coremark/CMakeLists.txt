# ##############################################################################
# apps/benchmarks/coremark/CMakeLists.txt
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

if(CONFIG_BENCHMARK_COREMARK)

  # ############################################################################
  # Config and Fetch Coremark application
  # ############################################################################

  set(COREMARKAPP_DIR ${CMAKE_CURRENT_LIST_DIR}/coremark)

  if(NOT EXISTS ${COREMARKAPP_DIR})
    FetchContent_Declare(
      coremark_fetch
      URL https://github.com/eembc/coremark/archive/main.zip SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/coremark BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/benchmarks/coremark/coremark
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(coremark_fetch)
    if(NOT coremark_fetch_POPULATED)
      FetchContent_Populate(coremark_fetch)
    endif()
  endif()

  if(CONFIG_COREMARK_MULTITHREAD_OVERRIDE)
    set(COREMARK_NTHREADS ${CONFIG_COREMARK_MULTITHREAD_COUNT})
  endif()

  if(CONFIG_SMP_NCPUS)
    set(COREMARK_NTHREADS ${CONFIG_SMP_NCPUS})
  else()
    set(COREMARK_NTHREADS 1)
  endif()

  if(CONFIG_COREMARK_PRINT_ARGS)
    set(COREMARK_PRINT_ARGS 1)
  else()
    set(COREMARK_PRINT_ARGS 0)
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  if(CONFIG_COREMARK_MULTITHREAD_OVERRIDE)
    set(COREMARK_NTHREADS ${CONFIG_COREMARK_MULTITHREAD_COUNT})
  else()
    if(CONFIG_SMP_NCPUS)
      set(COREMARK_NTHREADS ${CONFIG_SMP_NCPUS})
    else()
      set(COREMARK_NTHREADS 1)
    endif()
  endif()

  if(CONFIG_COREMARK_PRINT_ARGS)
    set(COREMARK_PRINT_ARGS 1)
  else()
    set(COREMARK_PRINT_ARGS 0)
  endif()

  get_target_property(FLAGS_STR_LIST nuttx COMPILE_OPTIONS)
  list(JOIN FLAGS_STR_LIST " " FLAGS_STR)

  set(CFLAGS -Dcrc16=coremark_crc16 -Wno-strict-prototypes -Wno-undef)

  if("${CONFIG_COREMARK_MEM_METHOD}" STREQUAL "1")
    list(APPEND CFLAGS -DMEM_LOCATION=\"HEAP\")
  elseif("${CONFIG_COREMARK_MEM_METHOD}" STREQUAL "2")
    list(APPEND CFLAGS -DMEM_LOCATION=\"STACK\")
  else()
    list(APPEND CFLAGS -DMEM_LOCATION=\"GLOBAL\")
  endif()

  list(
    APPEND
    CFLAGS
    -DCALLGRIND_RUN=0
    -DCOMPILER_REQUIRES_SORT_RETURN=0
    -DCORE_DEBUG=${CONFIG_COREMARK_DEBUG}
    -DFLAGS_STR="${FLAGS_STR}"
    -DMEM_METHOD=${CONFIG_COREMARK_MEM_METHOD}
    -DMICA=0
    -DMULTITHREAD=${COREMARK_NTHREADS}
    -DPERFORMANCE_RUN=1
    -DPRINT_ARGS=${COREMARK_PRINT_ARGS}
    -DSEED_METHOD=${CONFIG_COREMARK_SEED_METHOD}
    -DUSE_CLOCK=1
    -DUSE_PTHREAD)

  if(CONFIG_COREMARK_ITERATIONS_OVERRIDE)
    list(APPEND CFLAGS -DITERATIONS=${CONFIG_COREMARK_ITERATIONS_COUNT})
  else()
    list(APPEND CFLAGS -DITERATIONS=0)

  endif()

  # ############################################################################
  # Sources
  # ############################################################################

  set(CSRCS
      ${COREMARKAPP_DIR}/core_main.c ${COREMARKAPP_DIR}/core_list_join.c
      ${COREMARKAPP_DIR}/core_matrix.c ${COREMARKAPP_DIR}/core_state.c
      ${COREMARKAPP_DIR}/core_util.c ${COREMARKAPP_DIR}/posix/core_portme.c)

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${COREMARKAPP_DIR} ${COREMARKAPP_DIR}/posix)

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  nuttx_add_application(
    NAME
    ${CONFIG_COREMARK_PROGNAME}
    PRIORITY
    ${CONFIG_COREMARK_PRIORITY}
    STACKSIZE
    ${CONFIG_COREMARK_STACKSIZE}
    MODULE
    ${CONFIG_BENCHMARK_COREMARK}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${CSRCS}
    INCLUDE_DIRECTORIES
    ${INCDIR})

endif()
