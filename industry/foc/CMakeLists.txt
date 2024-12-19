# ##############################################################################
# apps/industry/foc/CMakeLists.txt
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

if(CONFIG_INDUSTRY_FOC)

  set(CSRCS foc_utils.c)

  if(CONFIG_INDUSTRY_FOC_FLOAT)
    list(
      APPEND
      CSRCS
      float/foc_ramp.c
      float/foc_handler.c
      float/foc_angle.c
      float/foc_velocity.c
      float/foc_routine.c)

    if(CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP)
      list(APPEND CSRCS float/foc_ang_openloop.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ANGLE_ONFO)
      list(APPEND CSRCS float/foc_ang_onfo.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ANGLE_OSMO)
      list(APPEND CSRCS float/foc_ang_osmo.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ANGLE_QENCO)
      list(APPEND CSRCS float/foc_ang_qenco.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ANGLE_HALL)
      list(APPEND CSRCS float/foc_ang_hall.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_CORDIC)
      list(APPEND CSRCS float/foc_cordic.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_CONTROL_PI)
      list(APPEND CSRCS float/foc_picontrol.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_MODULATION_SVM3)
      list(APPEND CSRCS float/foc_svm3.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_HAVE_MODEL)
      list(APPEND CSRCS float/foc_model.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_MODEL_PMSM)
      list(APPEND CSRCS float/foc_model_pmsm.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ALIGN)
      list(APPEND CSRCS float/foc_align.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_IDENT)
      list(APPEND CSRCS float/foc_ident.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_VELOCITY_ODIV)
      list(APPEND CSRCS float/foc_vel_odiv.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_VELOCITY_OPLL)
      list(APPEND CSRCS float/foc_vel_opll.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_FEEDFORWARD)
      list(APPEND CSRCS float/foc_feedforward.c)
    endif()
  endif()

  if(CONFIG_INDUSTRY_FOC_FIXED16)
    list(
      APPEND
      CSRCS
      fixed16/foc_ramp.c
      fixed16/foc_handler.c
      fixed16/foc_angle.c
      fixed16/foc_velocity.c
      fixed16/foc_routine.c)

    if(CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP)
      list(APPEND CSRCS fixed16/foc_ang_openloop.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ANGLE_ONFO)
      list(APPEND CSRCS fixed16/foc_ang_onfo.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ANGLE_OSMO)
      list(APPEND CSRCS fixed16/foc_ang_osmo.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ANGLE_QENCO)
      list(APPEND CSRCS fixed16/foc_ang_qenco.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ANGLE_HALL)
      list(APPEND CSRCS fixed16/foc_ang_hall.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_CORDIC)
      list(APPEND CSRCS fixed16/foc_cordic.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_CONTROL_PI)
      list(APPEND CSRCS fixed16/foc_picontrol.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_MODULATION_SVM3)
      list(APPEND CSRCS fixed16/foc_svm3.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_HAVE_MODEL)
      list(APPEND CSRCS fixed16/foc_model.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_MODEL_PMSM)
      list(APPEND CSRCS fixed16/foc_model_pmsm.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_ALIGN)
      list(APPEND CSRCS fixed16/foc_align.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_IDENT)
      list(APPEND CSRCS fixed16/foc_ident.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_VELOCITY_ODIV)
      list(APPEND CSRCS fixed16/foc_vel_odiv.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_VELOCITY_OPLL)
      list(APPEND CSRCS fixed16/foc_vel_opll.c)
    endif()

    if(CONFIG_INDUSTRY_FOC_FEEDFORWARD)
      list(APPEND CSRCS fixed16/foc_feedforward.c)
    endif()
  endif()

  target_sources(apps PRIVATE ${CSRCS})
endif()
