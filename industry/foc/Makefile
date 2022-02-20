############################################################################
# apps/industry/foc/Makefile
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

include $(APPDIR)/Make.defs

# FOC Library

CSRCS = foc_utils.c

# float support

ifeq ($(CONFIG_INDUSTRY_FOC_FLOAT),y)

CSRCS += float/foc_ramp.c
CSRCS += float/foc_handler.c
CSRCS += float/foc_angle.c
CSRCS += float/foc_velocity.c
CSRCS += float/foc_routine.c
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP),y)
CSRCS += float/foc_ang_openloop.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_ONFO),y)
CSRCS += float/foc_ang_onfo.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_OSMO),y)
CSRCS += float/foc_ang_osmo.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_QENCO),y)
CSRCS += float/foc_ang_qenco.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_HALL),y)
CSRCS += float/foc_ang_hall.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_CORDIC),y)
CSRCS += float/foc_cordic.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_CONTROL_PI),y)
CSRCS += float/foc_picontrol.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_MODULATION_SVM3),y)
CSRCS += float/foc_svm3.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_HAVE_MODEL),y)
CSRCS += float/foc_model.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_MODEL_PMSM),y)
CSRCS += float/foc_model_pmsm.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ALIGN),y)
CSRCS += float/foc_align.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_IDENT),y)
CSRCS += float/foc_ident.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_VELOCITY_ODIV),y)
CSRCS += float/foc_vel_odiv.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_VELOCITY_OPLL),y)
CSRCS += float/foc_vel_opll.c
endif

endif

# fixed16 support

ifeq ($(CONFIG_INDUSTRY_FOC_FIXED16),y)

CSRCS += fixed16/foc_ramp.c
CSRCS += fixed16/foc_handler.c
CSRCS += fixed16/foc_angle.c
CSRCS += fixed16/foc_velocity.c
CSRCS += fixed16/foc_routine.c
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP),y)
CSRCS += fixed16/foc_ang_openloop.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_ONFO),y)
CSRCS += fixed16/foc_ang_onfo.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_OSMO),y)
CSRCS += fixed16/foc_ang_osmo.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_QENCO),y)
CSRCS += fixed16/foc_ang_qenco.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ANGLE_HALL),y)
CSRCS += fixed16/foc_ang_hall.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_CORDIC),y)
CSRCS += fixed16/foc_cordic.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_CONTROL_PI),y)
CSRCS += fixed16/foc_picontrol.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_MODULATION_SVM3),y)
CSRCS += fixed16/foc_svm3.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_HAVE_MODEL),y)
CSRCS += fixed16/foc_model.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_MODEL_PMSM),y)
CSRCS += fixed16/foc_model_pmsm.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_ALIGN),y)
CSRCS += fixed16/foc_align.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_IDENT),y)
CSRCS += fixed16/foc_ident.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_VELOCITY_ODIV),y)
CSRCS += fixed16/foc_vel_odiv.c
endif
ifeq ($(CONFIG_INDUSTRY_FOC_VELOCITY_OPLL),y)
CSRCS += fixed16/foc_vel_opll.c
endif

endif

include $(APPDIR)/Application.mk
