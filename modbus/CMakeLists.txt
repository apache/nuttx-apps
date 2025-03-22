# ##############################################################################
# apps/modbus/CMakeLists.txt
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

if(CONFIG_MODBUS)

  set(CSRCS)

  if(CONFIG_MODBUS_SLAVE)
    list(APPEND CSRCS mb.c)
  endif()

  if(CONFIG_MB_RTU_MASTER)
    list(APPEND CSRCS mb_m.c)
  endif()

  # ascii/Make.defs

  if(CONFIG_MB_ASCII_ENABLED)
    list(APPEND CSRCS ascii/mbascii.c)
  endif()

  # functions/Make.defs

  list(
    APPEND
    CSRCS
    functions/mbfunccoils.c
    functions/mbfuncdiag.c
    functions/mbfuncdisc.c
    functions/mbfuncholding.c
    functions/mbfuncinput.c
    functions/mbfuncother.c
    functions/mbutils.c)

  if(CONFIG_MB_ASCII_MASTER)
    list(APPEND CSRCS functions/mbfunccoils_m.c functions/mbfuncdisc_m.c
         functions/mbfuncholding_m.c functions/mbfuncinput_m.c)
  elseif(CONFIG_MB_RTU_MASTER)
    list(APPEND CSRCS functions/mbfunccoils_m.c functions/mbfuncdisc_m.c
         functions/mbfuncholding_m.c functions/mbfuncinput_m.c)
  endif()

  # nuttx/Make.defs

  list(APPEND CSRCS nuttx/portother.c)

  if(CONFIG_MODBUS_SLAVE)
    list(APPEND CSRCS nuttx/portevent.c nuttx/portserial.c nuttx/porttimer.c)
  endif()

  if(CONFIG_MB_RTU_MASTER)
    list(APPEND CSRCS nuttx/portother_m.c nuttx/portserial_m.c
         nuttx/porttimer_m.c nuttx/portevent_m.c)
  endif()

  # rtu/Make.defs

  if(CONFIG_MB_RTU_ENABLED OR CONFIG_MB_RTU_MASTER)
    list(APPEND CSRCS rtu/mbcrc.c)
    if(CONFIG_MB_RTU_ENABLED)
      list(APPEND CSRCS rtu/mbrtu.c)
    endif()

    if(CONFIG_MB_RTU_MASTER)
      list(APPEND CSRCS rtu/mbrtu_m.c)
    endif()
  endif()

  # tcp/Make.defs

  if(CONFIG_MB_TCP_ENABLED)
    list(APPEND CSRCS tcp/mbtcp.c)
  endif()

  # include private headers

  target_include_directories(apps PRIVATE nuttx)
  target_include_directories(apps PRIVATE rtu)

  target_sources(apps PRIVATE ${CSRCS})

endif()
