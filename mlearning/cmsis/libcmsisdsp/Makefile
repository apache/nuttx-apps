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
CSRCS += SVMFunctions.c
CSRCS += SVMFunctionsF16.c
CSRCS += InterpolationFunctions.c
CSRCS += InterpolationFunctionsF16.c
CSRCS += FastMathFunctions.c
CSRCS += FastMathFunctionsF16.c
CSRCS += FilteringFunctions.c
CSRCS += FilteringFunctionsF16.c
CSRCS += BasicMathFunctions.c
CSRCS += BasicMathFunctionsF16.c
CSRCS += ControllerFunctions.c
CSRCS += SupportFunctions.c
CSRCS += SupportFunctionsF16.c
CSRCS += CommonTables.c
CSRCS += CommonTablesF16.c
CSRCS += DistanceFunctions.c
CSRCS += DistanceFunctionsF16.c
CSRCS += StatisticsFunctions.c
CSRCS += StatisticsFunctionsF16.c
CSRCS += MatrixFunctions.c
CSRCS += MatrixFunctionsF16.c
CSRCS += QuaternionMathFunctions.c
CSRCS += TransformFunctions.c
CSRCS += TransformFunctionsF16.c
CSRCS += BayesFunctions.c
CSRCS += BayesFunctionsF16.c
CSRCS += ComplexMathFunctions.c
CSRCS += ComplexMathFunctionsF16.c

VPATH += $(CMSIS5_DSP)/SVMFunctions
VPATH += $(CMSIS5_DSP)/InterpolationFunctions
VPATH += $(CMSIS5_DSP)/FastMathFunctions
VPATH += $(CMSIS5_DSP)/FilteringFunctions
VPATH += $(CMSIS5_DSP)/BasicMathFunctions
VPATH += $(CMSIS5_DSP)/ControllerFunctions
VPATH += $(CMSIS5_DSP)/SupportFunctions
VPATH += $(CMSIS5_DSP)/CommonTables
VPATH += $(CMSIS5_DSP)/DistanceFunctions
VPATH += $(CMSIS5_DSP)/StatisticsFunctions
VPATH += $(CMSIS5_DSP)/MatrixFunctions
VPATH += $(CMSIS5_DSP)/QuaternionMathFunctions
VPATH += $(CMSIS5_DSP)/TransformFunctions
VPATH += $(CMSIS5_DSP)/BayesFunctions
VPATH += $(CMSIS5_DSP)/ComplexMathFunctions

CFLAGS += -ffast-math -Wno-shadow -flax-vector-conversions

MODULE = $(CONFIG_CMSIS_DSP)

include $(APPDIR)/Application.mk
