# ##############################################################################
# apps/testing/gpu/CMakeLists.txt
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

if(CONFIG_TESTING_GPU)
  file(GLOB VG_LITE_SRCS vg_lite/*.c)
  file(GLOB CURRENT_SRCS *.c)
  list(REMOVE_ITEM CURRENT_SRCS gpu_main.c)

  set(SRCS gpu_main.c ${CURRENT_SRCS} ${VG_LITE_SRCS})

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR})
  if(CONFIG_TESTING_GPU_VG_LITE_INCLUDE)
    list(APPEND INCDIR
         ${NUTTX_APPS_DIR}/../${CONFIG_TESTING_GPU_VG_LITE_INCLUDE})
  endif()

  if(CONFIG_LIB_PNG)
    list(APPEND INCDIR ${NUTTX_APPS_DIR}/../external/libpng
         ${NUTTX_APPS_DIR}/../external/libpng/libpng)
  endif()

  nuttx_add_application(
    NAME
    gpu_test
    PRIORITY
    ${CONFIG_TESTING_GPU_PRIORITY}
    STACKSIZE
    ${CONFIG_TESTING_GPU_STACKSIZE}
    MODULE
    ${CONFIG_TESTING_GPU}
    INCLUDE_DIRECTORIES
    ${INCDIR}
    SRCS
    ${SRCS})
endif()
