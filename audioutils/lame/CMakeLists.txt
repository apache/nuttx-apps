# ##############################################################################
# apps/audioutils/lame/CMakeLists.txt
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

if(CONFIG_AUDIOUTILS_LAME)

  # Determine destination path
  if(NOT DEFINED DST_PATH)
    set(DST_PATH ${CMAKE_CURRENT_BINARY_DIR}/lame)
  else()
    set(DST_PATH ${DST_PATH})
  endif()

  # Set configuration commands
  set(CFG_CMDS "")
  if(NOT "${CONFIG_ARCH}" STREQUAL "sim")
    list(APPEND CFG_CMDS "--host=${CONFIG_ARCH}")
    list(APPEND CFG_CMDS "--cross-prefix=${CROSSDEV}")
  endif()

  # # Download lame if no lame/configure found
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lame/configure")
    execute_process(
      COMMAND "svn" "checkout" "https://svn.code.sf.net/p/lame/svn/trunk/lame"
              "lame" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  # Set source path
  set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lame")

  # Add compiler flags
  set(CFLAGS -DHAVE_CONFIG_H -Dfast_log2=lame_fast_log2)
  set(CFLAGS
      ${CFLAGS}
      -Wno-address
      -Wno-array-parameter
      -Wno-builtin-declaration-mismatch
      -Wno-incompatible-pointer-types
      -Wno-implicit-function-declaration
      -Wno-shadow
      -Wno-stringop-overflow
      -Wno-unused-variable
      -Wno-unused-but-set-variable
      -msse)

  set(INCDIR ${INCDIR_PREFIX}${DST_PATH} ${INCDIR_PREFIX}${SRC_PATH}/include
             ${INCDIR_PREFIX}${SRC_PATH}/libmp3lame)

  # Set source files
  set(CSRCS
      "${SRC_PATH}/libmp3lame/bitstream.c"
      "${SRC_PATH}/libmp3lame/encoder.c"
      "${SRC_PATH}/libmp3lame/fft.c"
      "${SRC_PATH}/libmp3lame/gain_analysis.c"
      "${SRC_PATH}/libmp3lame/id3tag.c"
      "${SRC_PATH}/libmp3lame/lame.c"
      "${SRC_PATH}/libmp3lame/newmdct.c"
      "${SRC_PATH}/libmp3lame/psymodel.c"
      "${SRC_PATH}/libmp3lame/quantize.c"
      "${SRC_PATH}/libmp3lame/vector/xmm_quantize_sub.c"
      "${SRC_PATH}/libmp3lame/quantize_pvt.c"
      "${SRC_PATH}/libmp3lame/set_get.c"
      "${SRC_PATH}/libmp3lame/vbrquantize.c"
      "${SRC_PATH}/libmp3lame/reservoir.c"
      "${SRC_PATH}/libmp3lame/tables.c"
      "${SRC_PATH}/libmp3lame/takehiro.c"
      "${SRC_PATH}/libmp3lame/util.c"
      "${SRC_PATH}/libmp3lame/VbrTag.c"
      "${SRC_PATH}/libmp3lame/version.c"
      "${SRC_PATH}/libmp3lame/presets.c")

  # Add custom target to generate config_h
  set(CONFIG_H_FILE "${CMAKE_CURRENT_SOURCE_DIR}/lame/configure")
  set(CONFIG_OPTIONS
      --disable-cpml
      --disable-decoder
      --disable-efence
      --disable-frontend
      --disable-mp3x
      --disable-gtktest
      --disable-ipv6
      --disable-rpath
      --enable-static
      ${CFG_CMDS})
  add_custom_command(
    OUTPUT "${DST_PATH}/config.h"
    COMMAND ${CONFIG_H_FILE} ${CONFIG_OPTIONS}
    WORKING_DIRECTORY "${DST_PATH}"
    COMMENT "Generating config.h file")
  add_custom_target(generate_config_h DEPENDS "${DST_PATH}/config.h")

  nuttx_add_library(libmp3lame STATIC)
  add_dependencies(libmp3lame generate_config_h)
  target_sources(libmp3lame PRIVATE ${CSRCS})
  target_include_directories(libmp3lame PRIVATE ${INCDIR})
  target_compile_options(libmp3lame PRIVATE ${CFLAGS})

endif()
