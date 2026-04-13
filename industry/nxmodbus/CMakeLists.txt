# ##############################################################################
# apps/industry/nxmodbus/CMakeLists.txt
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

if(CONFIG_INDUSTRY_NXMODBUS)

  set(CSRCS)

  # Core sources
  list(APPEND CSRCS core/nxmb_context.c core/nxmb_utils.c)

  if(CONFIG_NXMODBUS_SERVER)
    list(APPEND CSRCS core/nxmb_server.c)
    list(APPEND CSRCS core/nxmb_func.c)
    list(APPEND CSRCS core/nxmb_func_coils.c core/nxmb_func_discrete.c)
    list(APPEND CSRCS core/nxmb_func_holding.c core/nxmb_func_input.c)
    list(APPEND CSRCS core/nxmb_func_diag.c core/nxmb_func_other.c)
  endif()

  if(CONFIG_NXMODBUS_CLIENT)
    list(APPEND CSRCS core/nxmb_client.c)
  endif()

  # Transport layer sources
  if(CONFIG_NXMODBUS_RTU OR CONFIG_NXMODBUS_ASCII)
    list(APPEND CSRCS transport/nxmb_serial_common.c)
  endif()

  if(CONFIG_NXMODBUS_RTU)
    list(APPEND CSRCS transport/nxmb_crc.c transport/nxmb_serial.c)
  endif()

  if(CONFIG_NXMODBUS_ASCII)
    list(APPEND CSRCS transport/nxmb_lrc.c transport/nxmb_ascii.c)
  endif()

  if(CONFIG_NXMODBUS_RAW_ADU)
    list(APPEND CSRCS transport/nxmb_raw.c)
  endif()

  if(CONFIG_NXMODBUS_TCP)
    list(APPEND CSRCS transport/nxmb_tcp.c)
  endif()

  if(CSRCS)
    target_include_directories(apps PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_sources(apps PRIVATE ${CSRCS})
  endif()

endif()
