# ##############################################################################
# apps/videoutils/openh264/CMakeLists.txt
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

if(CONFIG_VIDEOUTILS_OPENH264)

  # ############################################################################
  # Sources
  # ############################################################################

  set(SRC_PATH ${CMAKE_CURRENT_LIST_DIR}/openh264)
  set(COMMON_SRCDIR ${SRC_PATH}/codec/common)
  set(DECODER_SRCDIR ${SRC_PATH}/codec/decoder)

  set(COMMON_CPP_SRCS
      ${COMMON_SRCDIR}/src/common_tables.cpp
      ${COMMON_SRCDIR}/src/copy_mb.cpp
      ${COMMON_SRCDIR}/src/cpu.cpp
      ${COMMON_SRCDIR}/src/crt_util_safe_x.cpp
      ${COMMON_SRCDIR}/src/deblocking_common.cpp
      ${COMMON_SRCDIR}/src/expand_pic.cpp
      ${COMMON_SRCDIR}/src/intra_pred_common.cpp
      ${COMMON_SRCDIR}/src/mc.cpp
      ${COMMON_SRCDIR}/src/memory_align.cpp
      ${COMMON_SRCDIR}/src/sad_common.cpp
      ${COMMON_SRCDIR}/src/utils.cpp
      ${COMMON_SRCDIR}/src/welsCodecTrace.cpp
      ${COMMON_SRCDIR}/src/WelsTaskThread.cpp
      ${COMMON_SRCDIR}/src/WelsThread.cpp
      ${COMMON_SRCDIR}/src/WelsThreadLib.cpp
      ${COMMON_SRCDIR}/src/WelsThreadPool.cpp)

  set(COMMON_ASM_SRCS
      ${COMMON_SRCDIR}/x86/cpuid.asm
      ${COMMON_SRCDIR}/x86/dct.asm
      ${COMMON_SRCDIR}/x86/deblock.asm
      ${COMMON_SRCDIR}/x86/expand_picture.asm
      ${COMMON_SRCDIR}/x86/intra_pred_com.asm
      ${COMMON_SRCDIR}/x86/mb_copy.asm
      ${COMMON_SRCDIR}/x86/mc_chroma.asm
      ${COMMON_SRCDIR}/x86/mc_luma.asm
      ${COMMON_SRCDIR}/x86/satd_sad.asm
      ${COMMON_SRCDIR}/x86/vaa.asm)

  set(DECODER_CPP_SRCS
      ${DECODER_SRCDIR}/core/src/au_parser.cpp
      ${DECODER_SRCDIR}/core/src/bit_stream.cpp
      ${DECODER_SRCDIR}/core/src/cabac_decoder.cpp
      ${DECODER_SRCDIR}/core/src/deblocking.cpp
      ${DECODER_SRCDIR}/core/src/decode_mb_aux.cpp
      ${DECODER_SRCDIR}/core/src/decode_slice.cpp
      ${DECODER_SRCDIR}/core/src/decoder.cpp
      ${DECODER_SRCDIR}/core/src/decoder_core.cpp
      ${DECODER_SRCDIR}/core/src/decoder_data_tables.cpp
      ${DECODER_SRCDIR}/core/src/error_concealment.cpp
      ${DECODER_SRCDIR}/core/src/fmo.cpp
      ${DECODER_SRCDIR}/core/src/get_intra_predictor.cpp
      ${DECODER_SRCDIR}/core/src/manage_dec_ref.cpp
      ${DECODER_SRCDIR}/core/src/memmgr_nal_unit.cpp
      ${DECODER_SRCDIR}/core/src/mv_pred.cpp
      ${DECODER_SRCDIR}/core/src/parse_mb_syn_cabac.cpp
      ${DECODER_SRCDIR}/core/src/parse_mb_syn_cavlc.cpp
      ${DECODER_SRCDIR}/core/src/pic_queue.cpp
      ${DECODER_SRCDIR}/core/src/rec_mb.cpp
      ${DECODER_SRCDIR}/core/src/wels_decoder_thread.cpp
      ${DECODER_SRCDIR}/plus/src/welsDecoderExt.cpp)

  set(DECODER_ASM_SRCS ${DECODER_SRCDIR}/core/x86/dct.asm
                       ${DECODER_SRCDIR}/core/x86/intra_pred.asm)

  set(CXXEXT .cpp)
  set(CXXSRCS ${COMMON_CPP_SRCS} ${DECODER_CPP_SRCS})

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCLUDES ${SRC_PATH}/codec/api/wels ${SRC_PATH}/codec/common/inc
               ${CMAKE_CURRENT_LIST_DIR}/include)
  set(DECODER_INCLUDES ${SRC_PATH}/codec/decoder/core/inc
                       ${SRC_PATH}/codec/decoder/plus/inc)
  set(PROCESSING_INCLUDES
      ${SRC_PATH}/codec/processing/interface
      ${SRC_PATH}/codec/processing/src/common
      ${SRC_PATH}/codec/processing/src/adaptivequantization
      ${SRC_PATH}/codec/processing/src/downsample
      ${SRC_PATH}/codec/processing/src/scrolldetection
      ${SRC_PATH}/codec/processing/src/vaacalc)

  set(INCDIR ${INCLUDES} ${DECODER_INCLUDES} ${PROCESSING_INCLUDES})
  set(PUB_INCDIR ${SRC_PATH}/codec/api)

  # ############################################################################
  # Flags
  # ############################################################################

  set(CXXFLAGS -Wno-format -Wno-unused-variable -Wno-shadow)

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(libopenh264 STATIC)
  target_sources(libopenh264 PRIVATE ${CXXSRCS} ${NASRCS})
  target_link_libraries(libopenh264 PRIVATE -lstdc++)
  target_compile_options(libopenh264 PRIVATE ${CXXFLAGS})
  target_include_directories(libopenh264 PRIVATE ${INCDIR})
  target_include_directories(libopenh264 PUBLIC ${PUB_INCDIR})

  # ############################################################################
  # Download openh264 if no git repo found
  # ############################################################################

  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/openh264/.git)
    set(OPENH264_URL "https://github.com/cisco/openh264.git")
    set(OPENH264_DIR ${CMAKE_CURRENT_SOURCE_DIR}/openh264)

    execute_process(
      COMMAND git clone ${OPENH264_URL} ${OPENH264_DIR}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_CLONE_RESULT)

    if(NOT ${GIT_CLONE_RESULT} EQUAL 0)
      message(FATAL_ERROR "Failed to clone OpenH264 repository")
    endif()

    add_custom_target(distclean COMMAND ${CMAKE_COMMAND} -E remove_directory
                                        ${OPENH264_DIR})
  endif()

endif()
