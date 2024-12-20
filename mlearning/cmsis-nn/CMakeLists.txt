# ##############################################################################
# apps/mlearning/cmsis-nn/CMakeLists.txt
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

if(CONFIG_MLEARNING_CMSIS_NN)

  # ############################################################################
  # Config and Fetch CMSIS-NN lib
  # ############################################################################

  set(CMSIS_NN_DIR ${CMAKE_CURRENT_LIST_DIR}/cmsis-nn)

  if(NOT EXISTS ${CMSIS_NN_DIR})
    set(CMSIS_NN_URL
        https://github.com/ARM-software/CMSIS-NN/archive/72e1ebf623ab1660a3e14e4e36fdcddce46f1991.zip
    )
    FetchContent_Declare(
      cmsis_nn_fetch
      URL ${CMSIS_NN_URL} SOURCE_DIR ${CMSIS_NN_DIR} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/mlearning/cmsis-nn/cmsis-nn
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(cmsis_nn_fetch)

    if(NOT cmsis_nn_fetch_POPULATED)
      FetchContent_Populate(cmsis_nn_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -Wno-undef -O3)

  # ############################################################################
  # Sources
  # ############################################################################

  file(
    GLOB
    CMSIS_NN_SRCS
    ${CMSIS_NN_DIR}/Source/ActivationFunctions/*.c
    ${CMSIS_NN_DIR}/Source/BasicMathFunctions/*.c
    ${CMSIS_NN_DIR}/Source/PoolingFunctions/*.c
    ${CMSIS_NN_DIR}/Source/ConvolutionFunctions/*.c
    ${CMSIS_NN_DIR}/Source/SVDFunctions/*.c
    ${CMSIS_NN_DIR}/Source/FullyConnectedFunctions/*.c
    ${CMSIS_NN_DIR}/Source/NNSupportFunctions/*.c
    ${CMSIS_NN_DIR}/Source/ConcatenationFunctions/*.c
    ${CMSIS_NN_DIR}/Source/SoftmaxFunctions/*.c
    ${CMSIS_NN_DIR}/Source/LSTMFunctions/*.c
    ${CMSIS_NN_DIR}/Source/ReshapeFunctions/*.c)

  set(CMSIS_NN_INCDIR ${CMAKE_CURRENT_LIST_DIR}/cmsis-nn/Include)

  if(CONFIG_ARM_NEON)
    list(
      REMOVE_ITEM
      CMSIS_NN_SRCS
      ${CMSIS_NN_DIR}/Source/BasicMathFunctions/arm_elementwise_add_s8.c
      ${CMSIS_NN_DIR}/Source/ConvolutionFunctions/arm_convolve_s8.c
      ${CMSIS_NN_DIR}/Source/NNSupportFunctions/arm_nn_mat_mult_kernel_s8_s16.c
      ${CMSIS_NN_DIR}/Source/NNSupportFunctions/arm_q7_to_q15_with_offset.c)
  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(cmsis_nn STATIC)
  target_compile_options(cmsis_nn PRIVATE ${CFLAGS})
  target_sources(cmsis_nn PRIVATE ${CMSIS_NN_SRCS})
  target_include_directories(cmsis_nn PUBLIC ${CMSIS_NN_INCDIR})

endif()
