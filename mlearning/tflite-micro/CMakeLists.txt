# ##############################################################################
# apps/mlearning/tflite-micro/CMakeLists.txt
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

if(CONFIG_TFLITEMICRO)

  # ############################################################################
  # Config and Fetch TFLITE-MICRO
  # ############################################################################

  set(TFLITE_MICRO_DIR ${CMAKE_CURRENT_LIST_DIR}/tflite-micro)

  if(NOT EXISTS ${TFLITE_MICRO_DIR})
    set(TFLITE_MICRO_URL
        https://github.com/tensorflow/tflite-micro/archive/cfa4c91d1b36c37c7c104b9c664615e59f1abfe3.zip
    )
    FetchContent_Declare(
      tflite_micro_fetch
      URL ${TFLITE_MICRO_URL} SOURCE_DIR ${TFLITE_MICRO_DIR} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/mlearning/tflite-micro/tflite-micro
      PATCH_COMMAND
        patch -d ${TFLITE_MICRO_DIR} -p1 <
        ${CMAKE_CURRENT_LIST_DIR}/tflite-micro.patch && patch -d
        ${TFLITE_MICRO_DIR} -p1 <
        ${CMAKE_CURRENT_LIST_DIR}/0001-dequantize-int8.patch && patch -d
        ${TFLITE_MICRO_DIR} -p1 <
        ${CMAKE_CURRENT_LIST_DIR}/0002-quantize-int8.patch && patch -d
        ${TFLITE_MICRO_DIR} -p1 < ${CMAKE_CURRENT_LIST_DIR}/0003-mean-int8.patch
        && patch -d ${TFLITE_MICRO_DIR} -p1 <
        ${CMAKE_CURRENT_LIST_DIR}/0004-tflite-add-extern-C-to-main-function-to-avoid-c-mang.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(tflite_micro_fetch)

    if(NOT tflite_micro_fetch_POPULATED)
      FetchContent_Populate(tflite_micro_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(COMMON_FLAGS
      -DTF_LITE_STATIC_MEMORY
      -DTF_LITE_DISABLE_X86_NEON
      -Wno-sign-compare
      -Wno-unused-variable
      -Wno-undef
      -Wno-shadow
      -O3)

  if(CONFIG_MLEARNING_CMSIS_NN)
    list(APPEND COMMON_FLAGS -DCMSIS_NN)
  endif()

  if(CONFIG_TFLITEMICRO_DEBUG)
    list(APPEND COMMON_FLAGS -DTF_LITE_SHOW_MEMORY_USE)
    list(APPEND COMMON_FLAGS -DTF_LITE_USE_CTIME)
  else()
    list(APPEND COMMON_FLAGS -DTF_LITE_STRIP_ERROR_STRINGS)
  endif()

  # ############################################################################
  # Sources
  # ############################################################################

  file(
    GLOB
    TFLITE_MICRO_SRCS
    ${TFLITE_MICRO_DIR}/tensorflow/lite/micro/kernels/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/c/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/schema/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/core/c/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/kernels/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/kernels/internal/optimized/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/kernels/internal/reference/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/kernels/internal/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/core/api/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/micro/arena_allocator/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/micro/memory_planner/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/micro/*.cc
    ${TFLITE_MICRO_DIR}/tensorflow/lite/micro/tflite_bridge/*.cc)

  # Remove test file
  list(FILTER TFLITE_MICRO_SRCS EXCLUDE REGEX ".*test.cc")
  if(CONFIG_TFLITEMICRO_SYSLOG)
    list(FILTER TFLITE_MICRO_SRCS EXCLUDE REGEX "micro_log.cc")
    list(APPEND TFLITE_MICRO_SRCS ${CMAKE_CURRENT_LIST_DIR}/tflm_syslog.cc)
  endif()

  if(CONFIG_MLEARNING_CMSIS_NN)
    list(APPEND COMMON_FLAGS -DCMSIS_NN)

    file(GLOB CMSIS_NN_SRCS
         ${TFLITE_MICRO_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/*.cc)

    # Remove the directory and get only filename
    set(CMSIS_NN_FILENAMES)
    foreach(file ${CMSIS_NN_SRCS})
      get_filename_component(filename ${file} NAME)
      list(APPEND CMSIS_NN_FILENAMES ${filename})
    endforeach()

    # Remove files with the same filename without the directory
    set(NEW_TFLITE_MICRO_SRCS)
    foreach(file ${TFLITE_MICRO_SRCS})
      get_filename_component(filename ${file} NAME)
      if(NOT filename IN_LIST CMSIS_NN_FILENAMES)
        list(APPEND NEW_TFLITE_MICRO_SRCS ${file})
      endif()
    endforeach()

    # 添加CMSIS_NN文件
    list(APPEND NEW_TFLITE_MICRO_SRCS ${CMSIS_NN_SRCS})
    set(TFLITE_MICRO_SRCS ${NEW_TFLITE_MICRO_SRCS})

    if(CONFIG_ARM_NEON)
      list(
        APPEND
        TFLITE_MICRO_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/operators/neon/arm_convolve_s8.c
        ${CMAKE_CURRENT_LIST_DIR}/operators/neon/arm_nn_mat_mult_kernel_s8_s16.c
        ${CMAKE_CURRENT_LIST_DIR}/operators/neon/arm_q7_to_q15_with_offset.c
        ${CMAKE_CURRENT_LIST_DIR}/operators/neon/arm_elementwise_add_s8.c)
    endif()
  endif()

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR
      ${NUTTX_APPS_DIR}/math/gemmlowp/gemmlowp
      ${NUTTX_APPS_DIR}/math/ruy/ruy
      ${NUTTX_APPS_DIR}/math/kissfft/kissfft
      ${NUTTX_APPS_DIR}/math/kissfft/kissfft
      ${NUTTX_APPS_DIR}/mlearning/tflite-micro/tflite-micro
      ${NUTTX_APPS_DIR}/system/flatbuffers/flatbuffers/include)

  if(CONFIG_MLEARNING_CMSIS_NN)
    list(APPEND INCDIR ${NUTTX_APPS_DIR}/mlearning/cmsis-nn/cmsis-nn)
    list(APPEND INCDIR ${NUTTX_APPS_DIR}/mlearning/cmsis-nn/cmsis-nn/Include)
  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(tflite_micro STATIC)
  target_compile_options(tflite_micro PRIVATE ${COMMON_FLAGS})
  target_sources(tflite_micro PRIVATE ${TFLITE_MICRO_SRCS})
  target_include_directories(tflite_micro PUBLIC ${INCDIR})

  # Help function to generate tflite data
  #
  # TFLM is designed to run on microcontrollers and other platforms without
  # dynamic memory allocation and without filesystems. This means that data
  # files such as TFLite models and test inputs must be built into the binary.
  #
  # Historically, data files have been included as cc arrays generated manually
  # using `xxd -i <tflite data file> <cc header file>`

  function(tflite_generate_data in out declare)
    set(tflm_data_out
        ${CMAKE_BINARY_DIR}/apps/mlearning/tflite-micro/tflite-micro/${out})
    if(NOT EXISTS ${tflm_data_out})
      get_filename_component(TFLM_OUT ${tflm_data_out} DIRECTORY)
      if(NOT EXISTS ${TFLM_OUT})
        file(MAKE_DIRECTORY ${TFLM_OUT})
      endif()
      execute_process(COMMAND xxd -i ${TFLITE_MICRO_DIR}/${in} ${tflm_data_out})

      # Replace array name

      string(REGEX REPLACE "([/|.])" "_" tflm_data_define
                           ${TFLITE_MICRO_DIR}/${in})
      string(REPLACE "-" "_" _tflm_data_define ${tflm_data_define})
      file(READ ${tflm_data_out} tflm_contents)
      encode_brackets(tflm_contents)
      encode_semicolon(tflm_contents)
      string(REPLACE ${_tflm_data_define} ${declare} tflm_contents
                     ${tflm_contents})
      decode_semicolon(tflm_contents)
      decode_brackets(tflm_contents)
      file(WRITE ${tflm_data_out} "${tflm_contents}")

    endif()
  endfunction()

  if(CONFIG_TFLITEMICRO_TOOL)
    nuttx_add_application(
      NAME
      tflm
      STACKSIZE
      ${CONFIG_TFLITEMICRO_TOOL_STACKSIZE}
      PRIORITY
      ${CONFIG_TFLITEMICRO_TOOL_PRIORITY}
      SRCS
      ${CMAKE_CURRENT_LIST_DIR}/tflm_tool.cc
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${COMMON_FLAGS})
  endif()

  if(CONFIG_TFLITEMICRO_HELLOWORLD)
    tflite_generate_data(
      tensorflow/lite/micro/examples/hello_world/models/hello_world_float.tflite
      tensorflow/lite/micro/examples/hello_world/models/hello_world_float_model_data.h
      g_hello_world_float_model_data)
    tflite_generate_data(
      tensorflow/lite/micro/examples/hello_world/models/hello_world_int8.tflite
      tensorflow/lite/micro/examples/hello_world/models/hello_world_int8_model_data.h
      g_hello_world_int8_model_data)

    nuttx_add_application(
      NAME
      tflm_hello
      STACKSIZE
      ${CONFIG_TFLITEMICRO_HELLOWORLD_STACKSIZE}
      PRIORITY
      ${CONFIG_TFLITEMICRO_HELLOWORLD_PRIORITY}
      SRCS
      ${TFLITE_MICRO_DIR}/tensorflow/lite/micro/examples/hello_world/hello_world_test.cc
      INCLUDE_DIRECTORIES
      ${INCDIR}
      ${CMAKE_BINARY_DIR}/apps/mlearning/tflite-micro/tflite-micro
      COMPILE_FLAGS
      ${COMMON_FLAGS})
  endif()

endif()
