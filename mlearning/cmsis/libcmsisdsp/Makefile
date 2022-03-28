############################################################################
# apps/mlearning/cmsis/libcmsisdsp/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

ifeq ($(CONFIG_CMSIS_DSP_ARM_MATH_MATRIX_CHECK),y)
CFLAGS += -DARM_MATH_MATRIX_CHECK
endif
ifeq ($(CONFIG_CMSIS_DSP_ARM_MATH_ROUNDING),y)
CFLAGS += -DARM_MATH_ROUNDING
endif
ifeq ($(CONFIG_ARCH_FPU),y)
CFLAGS += -D__FPU_PRESENT=1U
CXXFLAGS += -D__FPU_PRESENT=1U
endif

CFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/mlearning/cmsis/CMSIS_5/CMSIS/DSP/PrivateInclude/}
CXXFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/mlearning/cmsis/CMSIS_5/CMSIS/DSP/PrivateInclude/}

CMSIS5_DSP := ../CMSIS_5/CMSIS/DSP/Source
CSRCS += $(CMSIS5_DSP)/SVMFunctions/SVMFunctions.c
CSRCS += $(CMSIS5_DSP)/SVMFunctions/SVMFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/InterpolationFunctions/InterpolationFunctions.c
CSRCS += $(CMSIS5_DSP)/InterpolationFunctions/InterpolationFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/FastMathFunctions/FastMathFunctions.c
CSRCS += $(CMSIS5_DSP)/FastMathFunctions/FastMathFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/FilteringFunctions/FilteringFunctions.c
CSRCS += $(CMSIS5_DSP)/FilteringFunctions/FilteringFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/BasicMathFunctions/BasicMathFunctions.c
CSRCS += $(CMSIS5_DSP)/BasicMathFunctions/BasicMathFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/ControllerFunctions/ControllerFunctions.c
CSRCS += $(CMSIS5_DSP)/SupportFunctions/SupportFunctions.c
CSRCS += $(CMSIS5_DSP)/SupportFunctions/SupportFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/CommonTables/CommonTables.c
CSRCS += $(CMSIS5_DSP)/CommonTables/CommonTablesF16.c
CSRCS += $(CMSIS5_DSP)/DistanceFunctions/DistanceFunctions.c
CSRCS += $(CMSIS5_DSP)/DistanceFunctions/DistanceFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/StatisticsFunctions/StatisticsFunctions.c
CSRCS += $(CMSIS5_DSP)/StatisticsFunctions/StatisticsFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/MatrixFunctions/MatrixFunctions.c
CSRCS += $(CMSIS5_DSP)/MatrixFunctions/MatrixFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/QuaternionMathFunctions/QuaternionMathFunctions.c
CSRCS += $(CMSIS5_DSP)/TransformFunctions/TransformFunctions.c
CSRCS += $(CMSIS5_DSP)/TransformFunctions/TransformFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/BayesFunctions/BayesFunctions.c
CSRCS += $(CMSIS5_DSP)/BayesFunctions/BayesFunctionsF16.c
CSRCS += $(CMSIS5_DSP)/ComplexMathFunctions/ComplexMathFunctions.c
CSRCS += $(CMSIS5_DSP)/ComplexMathFunctions/ComplexMathFunctionsF16.c

CFLAGS += -ffast-math -Wno-shadow -flax-vector-conversions

MODULE = $(CONFIG_CMSIS_DSP)

include $(APPDIR)/Application.mk
