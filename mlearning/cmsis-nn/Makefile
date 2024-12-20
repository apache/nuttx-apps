############################################################################
# apps/mlearning/cmsis-nn/Makefile
#
# SPDX-License-Identifier: Apache-2.0
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

CMSIS_NN = cmsis-nn/Source

CMSIS_NN_VER = 72e1ebf623ab1660a3e14e4e36fdcddce46f1991

cmsis-nn.zip:
	$(Q) curl -L https://github.com/ARM-software/CMSIS-NN/archive/$(CMSIS_NN_VER).zip -o cmsis-nn.zip
	$(Q) unzip -o cmsis-nn.zip
	$(Q) mv CMSIS-NN-$(CMSIS_NN_VER) cmsis-nn

ifeq ($(wildcard cmsis-nn/.git),)
context:: cmsis-nn.zip
distclean::
	$(call DELDIR, cmsis-nn)
	$(call DELFILE, cmsis-nn.zip)
endif

CFLAGS += -Wno-undef

CSRCS += $(wildcard $(CMSIS_NN)/ActivationFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/BasicMathFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/PoolingFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/ConvolutionFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/SVDFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/FullyConnectedFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/NNSupportFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/ConcatenationFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/SoftmaxFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/LSTMFunctions/*.c)
CSRCS += $(wildcard $(CMSIS_NN)/ReshapeFunctions/*.c)

ifeq ($(CONFIG_ARM_NEON),y)
EXCLUDED_FILES := arm_elementwise_add_s8.c arm_convolve_s8.c arm_nn_mat_mult_kernel_s8_s16.c arm_q7_to_q15_with_offset.c
CSRCS := $(filter-out $(addprefix $(CMSIS_NN)/BasicMathFunctions/, $(EXCLUDED_FILES)), $(CSRCS))
CSRCS := $(filter-out $(addprefix $(CMSIS_NN)/NNSupportFunctions/, $(EXCLUDED_FILES)), $(CSRCS))
CSRCS := $(filter-out $(addprefix $(CMSIS_NN)/ConvolutionFunctions/, $(EXCLUDED_FILES)), $(CSRCS))
endif

include $(APPDIR)/Application.mk
