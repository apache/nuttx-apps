# ##############################################################################
# apps/benchmarks/coremark-pro/CMakeLists.txt
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

if(CONFIG_BENCHMARK_COREMARK_PRO)

  set(COREMARKAPP_DIR ${CMAKE_CURRENT_LIST_DIR}/coremark-pro)

  if(NOT EXISTS ${COREMARKAPP_DIR})
    FetchContent_Declare(
      coremark_fetch
      URL https://github.com/eembc/coremark-pro/archive/refs/heads/main.zip
          SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/coremark-pro BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/benchmarks/coremark/coremark-pro
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(coremark_fetch)
    if(NOT coremark_fetch_POPULATED)
      FetchContent_Populate(coremark_fetch)
    endif()
  endif()

  nuttx_add_library(coremark_pro STATIC)

  set(CSRCS
      coremark-pro/mith/src/md5.c
      coremark-pro/mith/src/mith_lib.c
      coremark-pro/mith/src/mith_workload.c
      coremark-pro/mith/src/th_bignum.c
      coremark-pro/mith/src/th_encode.c
      coremark-pro/mith/src/th_lib.c
      coremark-pro/mith/src/th_math.c
      coremark-pro/mith/src/th_rand.c)

  file(GLOB MITH_AL_SRCS coremark-pro/mith/al/src/*.c)

  list(APPEND CSRCS ${MITH_AL_SRCS})

  set(CFLAGS
      -Wno-undef
      -Wno-strict-prototypes
      -Wno-unused-variable
      -Wno-pointer-sign
      -Wno-unused-but-set-variable
      -Wno-shadow
      -DNDEBUG
      -DHOST_EXAMPLE_CODE=1
      -DHAVE_SYS_STAT_H=1
      -DGCC_INLINE_MACRO=1
      -DMAX_CONTEXTS=100
      -DEE_SIZEOF_INT=4
      -DUSE_FP128=0)

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR}/coremark-pro/mith/include
             ${CMAKE_CURRENT_LIST_DIR}/coremark-pro/mith/al/include)

  if(CONFIG_BENCHMARK_COREMARK_PRO_USE_SINGLE_CONTEXT)
    list(APPEND CFLAGS -DUSE_SINGLE_CONTEXT=1 -DHAVE_PTHREAD=0
         -DUSE_NATIVE_PTHREAD=0)
  else()
    list(APPEND CFLAGS -DUSE_SINGLE_CONTEXT=0 -DHAVE_PTHREAD=1
         -DUSE_NATIVE_PTHREAD=1)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_SMP)
    list(APPEND CFLAGS -DHAVE_PTHREAD_SETAFFINITY_NP=1 -DHAVE_PTHREAD_SELF=1)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_64BIT)
    list(APPEND CFLAGS -DEE_SIZEOF_PTR=8 -DEE_PTR_ALIGN=8 -DEE_SIZEOF_LONG=8)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_USE_FP64)

    list(APPEND CFLAGS -DUSE_FP64=1 -DUSE_FP32=0)
    list(APPEND CSRCS coremark-pro/benchmarks/fp/linpack/ref/inputs_f64.c)

  else()
    list(APPEND CFLAGS -DUSE_FP32=1 -DUSE_FP64=0)
    list(APPEND CSRCS coremark-pro/benchmarks/fp/linpack/ref/inputs_f32.c)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_CORE_TEST)
    file(GLOB CORE_TEST_SRCS coremark-pro/benchmarks/core/*.c)
    set(CORE_TEST_CSRCS coremark-pro/workloads/core/core.c ${CORE_TEST_SRCS})
    nuttx_add_application(
      NAME
      core
      PRIORITY
      ${CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY}
      STACKSIZE
      ${CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE}
      MODULE
      ${CONFIG_BENCHMARK_COREMARK_PRO}
      COMPILE_FLAGS
      ${CFLAGS}
      SRCS
      ${CORE_TEST_CSRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      coremark_pro)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_CJPEG_TEST)
    file(GLOB CJPEG_SRCS coremark-pro/benchmarks/consumer_v2/cjpeg/*.c)
    set(CJPEG_CSRCS
        coremark-pro/workloads/cjpeg-rose7-preset/cjpeg-rose7-preset.c
        coremark-pro/benchmarks/consumer_v2/cjpeg/data/Rose256_bmp.c
        ${CJPEG_SRCS})
    nuttx_add_application(
      NAME
      cjpeg_rose7_preset
      PRIORITY
      ${CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY}
      STACKSIZE
      ${CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE}
      MODULE
      ${CONFIG_BENCHMARK_COREMARK_PRO}
      COMPILE_FLAGS
      ${CFLAGS}
      SRCS
      ${CJPEG_CSRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      coremark_pro)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_SHA_TEST)
    file(GLOB SHA_TEST_SRCS coremark-pro/benchmarks/darkmark/sha/*.c)
    set(SHA_TEST_CSRCS coremark-pro/workloads/sha-test/sha-test.c
                       ${SHA_TEST_SRCS})
    nuttx_add_application(
      NAME
      sha_test
      PRIORITY
      ${CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY}
      STACKSIZE
      ${CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE}
      MODULE
      ${CONFIG_BENCHMARK_COREMARK_PRO}
      COMPILE_FLAGS
      ${CFLAGS}
      SRCS
      ${SHA_TEST_CSRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      coremark_pro)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_ZIP_TEST)
    set(ZIP_TEST_CSRCS coremark-pro/workloads/zip-test/zip-test.c
                       coremark-pro/benchmarks/darkmark/zip/zip_darkmark.c)
    set(ZIP_TEST_FLAGS ${CFLAGS} -Dgzclose_r=coremark_zip_gzclose_r
                       -Dgzclose_w=coremark_zip_gzclose_w)
    set(ZIP_TEST_INCDIR ${INCDIR} ${NUTTX_APPS_DIR}/system/zlib/zlib)
    nuttx_add_application(
      NAME
      zip_test
      PRIORITY
      ${CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY}
      STACKSIZE
      ${CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE}
      MODULE
      ${CONFIG_BENCHMARK_COREMARK_PRO}
      COMPILE_FLAGS
      ${ZIP_TEST_FLAGS}
      SRCS
      ${ZIP_TEST_CSRCS}
      INCLUDE_DIRECTORIES
      ${ZIP_TEST_INCDIR}
      DEPENDS
      coremark_pro)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_LINEAR_ALG_MID_TEST)
    set(ALG_MID_CSRCS
        coremark-pro/workloads/linear_alg-mid-100x100-sp/linear_alg-mid-100x100-sp.c
        coremark-pro/benchmarks/fp/linpack/linpack.c)
    nuttx_add_application(
      NAME
      linear_alg_mid_100x100_sp
      PRIORITY
      ${CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY}
      STACKSIZE
      ${CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE}
      MODULE
      ${CONFIG_BENCHMARK_COREMARK_PRO}
      COMPILE_FLAGS
      ${CFLAGS}
      SRCS
      ${ALG_MID_CSRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      coremark_pro)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_RADIX_TEST)
    if(CONFIG_BENCHMARK_COREMARK_PRO_USE_FP64)
      file(GLOB RADIX_SRCS coremark-pro/benchmarks/fp/fft_radix2/ref/*.c)
    else()
      file(GLOB RADIX_SRCS coremark-pro/benchmarks/fp/fft_radix2/ref-sp/*.c)
    endif()

    set(RADIX_CSRCS
        coremark-pro/workloads/radix2-big-64k/radix2-big-64k.c
        coremark-pro/benchmarks/fp/fft_radix2/fft_radix2.c ${RADIX_SRCS})
    nuttx_add_application(
      NAME
      radix2_big_64k
      PRIORITY
      ${CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY}
      STACKSIZE
      ${CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE}
      MODULE
      ${CONFIG_BENCHMARK_COREMARK_PRO}
      COMPILE_FLAGS
      ${CFLAGS}
      SRCS
      ${RADIX_CSRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      coremark_pro)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_LOOPS_ALL_MID_TEST)
    if(CONFIG_BENCHMARK_COREMARK_PRO_USE_FP64)
      file(GLOB MID_SRCS coremark-pro/benchmarks/fp/loops/ref/*.c)
    else()
      file(GLOB MID_SRCS coremark-pro/benchmarks/fp/loops/ref-sp/*.c)
    endif()

    set(MID_CSRCS
        coremark-pro/workloads/loops-all-mid-10k-sp/loops-all-mid-10k-sp.c
        coremark-pro/benchmarks/fp/loops/loops.c ${MID_SRCS})
    nuttx_add_application(
      NAME
      loops_all_mid_10k_sp
      PRIORITY
      ${CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY}
      STACKSIZE
      ${CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE}
      MODULE
      ${CONFIG_BENCHMARK_COREMARK_PRO}
      COMPILE_FLAGS
      ${CFLAGS}
      SRCS
      ${MID_CSRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      coremark_pro)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_NNET_TEST)
    if(CONFIG_BENCHMARK_COREMARK_PRO_USE_FP64)
      file(GLOB NNET_SRCS coremark-pro/benchmarks/fp/nnet/ref/*.c)
    else()
      file(GLOB NNET_SRCS coremark-pro/benchmarks/fp/nnet/ref-sp/*.c)
    endif()
    file(GLOB FP_NNET_SRCS coremark-pro/benchmarks/fp/nnet/*.c)
    set(NNET_CSRCS coremark-pro/workloads/nnet_test/nnet_test.c ${FP_NNET_SRCS}
                   ${NNET_SRCS})
    nuttx_add_application(
      NAME
      nnet_test
      PRIORITY
      ${CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY}
      STACKSIZE
      ${CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE}
      MODULE
      ${CONFIG_BENCHMARK_COREMARK_PRO}
      COMPILE_FLAGS
      ${CFLAGS}
      SRCS
      ${NNET_CSRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      coremark_pro)
  endif()

  if(CONFIG_BENCHMARK_COREMARK_PRO_PARSER_125K)
    file(GLOB PARSER_SRCS coremark-pro/benchmarks/darkmark/parser/*.c)
    set(PARSER_CSRCS coremark-pro/workloads/parser-125k/parser-125k.c
                     ${PARSER_SRCS})
    nuttx_add_application(
      NAME
      parser_125k
      PRIORITY
      ${CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY}
      STACKSIZE
      ${CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE}
      MODULE
      ${CONFIG_BENCHMARK_COREMARK_PRO}
      COMPILE_FLAGS
      ${CFLAGS}
      SRCS
      ${PARSER_CSRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      coremark_pro)
  endif()

  target_sources(coremark_pro PRIVATE ${CSRCS})
  target_include_directories(coremark_pro PRIVATE ${INCDIR})
  target_compile_options(coremark_pro PRIVATE ${CFLAGS})

endif()
