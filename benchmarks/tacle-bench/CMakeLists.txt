############################################################################
# apps/benchmarks/tacle-bench/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

if(CONFIG_BENCHMARK_TACLEBENCH)

  set(SRCS tacle-bench/taclebench_main.c)

  set(TACLEBENCH_UNPACK ${CMAKE_CURRENT_LIST_DIR}/tacle-bench)
  set(TACLEBENCH_URL https://github.com/tacle/tacle-bench/archive)
  set(TACLEBENCH_ZIP master.zip)

  if(NOT EXISTS ${TACLEBENCH_UNPACK})

    FetchContent_Declare(
      taclebench_fetch
      URL ${TACLEBENCH_URL}/${TACLEBENCH_ZIP} SOURCE_DIR ${TACLEBENCH_UNPACK}
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/benchmarks/tacle-bench/tacle-bench
      PATCH_COMMAND
        patch -p0 -d ${CMAKE_CURRENT_LIST_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0001-tacle-bench-add-makefile-and-all-in-one-main-file.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(taclebench_fetch)
    if(NOT taclebench_fetch_POPULATED)
      FetchContent_Populate(taclebench_fetch)
    endif()

  endif()

  # app
  list(
    APPEND
    SRCS
    tacle-bench/bench/app/powerwindow/powerwindow_const_params.c
    tacle-bench/bench/app/powerwindow/powerwindow_inputs.c
    tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_PSG_BackL.c
    tacle-bench/bench/app/powerwindow/powerwindow_debounce.c
    tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_DRV.c
    tacle-bench/bench/app/powerwindow/wcclib.c
    tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_PSG_BackR.c
    tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_PSG_Front.c
    tacle-bench/bench/app/powerwindow/powerwindow_controlexclusion.c
    tacle-bench/bench/app/powerwindow/powerwindow.c
    tacle-bench/bench/app/powerwindow/powerwindow_powerwindow_control.c
    tacle-bench/bench/app/lift/lift.c
    tacle-bench/bench/app/lift/liftlibio.c
    tacle-bench/bench/app/lift/liftlibcontrol.c)

  # kernel
  list(
    APPEND
    SRCS
    tacle-bench/bench/kernel/fac/fac.c
    tacle-bench/bench/kernel/bsort/bsort.c
    tacle-bench/bench/kernel/complex_updates/complex_updates.c
    tacle-bench/bench/kernel/cosf/cosf.c
    tacle-bench/bench/kernel/isqrt/isqrt.c
    tacle-bench/bench/kernel/isqrt/basicmath_libc.c
    tacle-bench/bench/kernel/minver/minver.c
    tacle-bench/bench/kernel/pm/pm_stdlib.c
    tacle-bench/bench/kernel/pm/pm_input.c
    tacle-bench/bench/kernel/pm/pm_libm.c
    tacle-bench/bench/kernel/pm/pm.c
    tacle-bench/bench/kernel/lms/lms.c
    tacle-bench/bench/kernel/binarysearch/binarysearch.c
    tacle-bench/bench/kernel/fft/fft_input.c
    tacle-bench/bench/kernel/fft/fft.c
    tacle-bench/bench/kernel/sha/memset.c
    tacle-bench/bench/kernel/sha/input_small.c
    tacle-bench/bench/kernel/sha/memhelper.c
    tacle-bench/bench/kernel/sha/memcpy.c
    tacle-bench/bench/kernel/sha/sha.c
    tacle-bench/bench/kernel/iir/iir.c
    tacle-bench/bench/kernel/matrix1/matrix1.c
    tacle-bench/bench/kernel/st/st.c
    tacle-bench/bench/kernel/prime/prime.c
    tacle-bench/bench/kernel/deg2rad/deg2rad.c
    tacle-bench/bench/kernel/filterbank/filterbank.c
    tacle-bench/bench/kernel/jfdctint/jfdctint.c
    tacle-bench/bench/kernel/fir2dim/fir2dim.c
    tacle-bench/bench/kernel/ludcmp/ludcmp.c
    tacle-bench/bench/kernel/bitcount/bitcount.c
    tacle-bench/bench/kernel/bitcount/bitcnt_2.c
    tacle-bench/bench/kernel/bitcount/bitcnt_1.c
    tacle-bench/bench/kernel/bitcount/bitcnt_3.c
    tacle-bench/bench/kernel/bitcount/bitcnt_4.c
    tacle-bench/bench/kernel/md5/md5.c
    tacle-bench/bench/kernel/recursion/recursion.c
    tacle-bench/bench/kernel/insertsort/insertsort.c
    tacle-bench/bench/kernel/cubic/cubic.c
    tacle-bench/bench/kernel/rad2deg/rad2deg.c
    tacle-bench/bench/kernel/quicksort/quicksortstdlib.c
    tacle-bench/bench/kernel/quicksort/input.c
    tacle-bench/bench/kernel/quicksort/quicksortlibm.c
    tacle-bench/bench/kernel/quicksort/quicksort.c
    tacle-bench/bench/kernel/countnegative/countnegative.c
    tacle-bench/bench/kernel/bitonic/bitonic.c)

  # sequential
  list(
    APPEND
    SRCS
    tacle-bench/bench/sequential/epic/epic.c
    tacle-bench/bench/sequential/mpeg2/mpeg2.c
    tacle-bench/bench/sequential/cjpeg_transupp/cjpeg_transupp.c
    tacle-bench/bench/sequential/adpcm_enc/adpcm_enc.c
    tacle-bench/bench/sequential/rijndael_dec/rijndael_dec.c
    tacle-bench/bench/sequential/rijndael_dec/input_small_enc.c
    tacle-bench/bench/sequential/rijndael_dec/aes.c
    tacle-bench/bench/sequential/rijndael_dec/rijndael_dec_libc.c
    tacle-bench/bench/sequential/huff_enc/huff_enc.c
    tacle-bench/bench/sequential/dijkstra/dijkstra.c
    tacle-bench/bench/sequential/dijkstra/input.c
    tacle-bench/bench/sequential/gsm_dec/gsm_dec.c
    tacle-bench/bench/sequential/ammunition/arithm.c
    tacle-bench/bench/sequential/ammunition/ammunition.c
    tacle-bench/bench/sequential/ammunition/bits.c
    tacle-bench/bench/sequential/ammunition/ammunition_libc.c
    tacle-bench/bench/sequential/susan/wccmalloc.c
    tacle-bench/bench/sequential/susan/wccfile.c
    tacle-bench/bench/sequential/susan/input.c
    tacle-bench/bench/sequential/susan/susan.c
    tacle-bench/bench/sequential/susan/wcclibm.c
    tacle-bench/bench/sequential/ndes/ndes.c
    tacle-bench/bench/sequential/petrinet/petrinet.c
    tacle-bench/bench/sequential/gsm_enc/gsm_enc.c
    tacle-bench/bench/sequential/h264_dec/h264_decinput.c
    tacle-bench/bench/sequential/h264_dec/h264_dec.c
    tacle-bench/bench/sequential/rijndael_enc/input_small.c
    tacle-bench/bench/sequential/rijndael_enc/rijndael_enc.c
    tacle-bench/bench/sequential/rijndael_enc/aes.c
    tacle-bench/bench/sequential/rijndael_enc/rijndael_enc_libc.c
    tacle-bench/bench/sequential/huff_dec/huff_dec.c
    tacle-bench/bench/sequential/anagram/anagram_stdlib.c
    tacle-bench/bench/sequential/anagram/anagram.c
    tacle-bench/bench/sequential/anagram/anagram_input.c
    tacle-bench/bench/sequential/g723_enc/g723_enc.c
    tacle-bench/bench/sequential/adpcm_dec/adpcm_dec.c
    tacle-bench/bench/sequential/cjpeg_wrbmp/cjpeg_wrbmp.c
    tacle-bench/bench/sequential/cjpeg_wrbmp/input.c
    tacle-bench/bench/sequential/statemate/statemate.c
    tacle-bench/bench/sequential/fmref/fmref.c
    tacle-bench/bench/sequential/fmref/wcclibm.c
    tacle-bench/bench/sequential/audiobeam/audiobeamlibmalloc.c
    tacle-bench/bench/sequential/audiobeam/audiobeam.c
    tacle-bench/bench/sequential/audiobeam/audiobeamlibm.c
    tacle-bench/bench/sequential/audiobeam/audiobeaminput.c
    tacle-bench/bench/test/test3/test3.c
    tacle-bench/bench/test/duff/duff.c
    tacle-bench/bench/test/cover/cover.c)

  # only one wcclibm
  list(APPEND SRCS tacle-bench/bench/kernel/cosf/wcclibm.c)

  list(
    APPEND
    CFLAGS
    -DALL_IN_ONE
    -Wno-unknown-pragmas
    -Wno-strict-prototypes
    -Wno-array-bounds
    -Wno-shadow
    -Wno-stringop-overflow
    -Wno-maybe-uninitialized)

  nuttx_add_application(
    NAME
    taclebench
    PRIORITY
    ${CONFIG_BENCHMARK_TACLEBENCH_PRIORITY}
    STACKSIZE
    ${CONFIG_BENCHMARK_TACLEBENCH_STACKSIZE}
    MODULE
    ${CONFIG_BENCHMARK_TACLEBENCH}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${SRCS})
endif()
