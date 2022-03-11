############################################################################
# apps/mlearning/cmsis/libcmsisnn/Makefile
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

CMSIS5_NN := ../CMSIS_5/CMSIS/NN/Source

ifeq ($(CONFIG_ARCH_FPU),y)
CFLAGS += -D__FPU_PRESENT=1U
CXXFLAGS += -D__FPU_PRESENT=1U
endif

CSRCS += $(CMSIS5_NN)/ActivationFunctions/arm_nn_activations_q15.c
CSRCS += $(CMSIS5_NN)/ActivationFunctions/arm_relu6_s8.c
CSRCS += $(CMSIS5_NN)/ActivationFunctions/arm_relu_q15.c
CSRCS += $(CMSIS5_NN)/ActivationFunctions/arm_nn_activations_q7.c
CSRCS += $(CMSIS5_NN)/ActivationFunctions/arm_relu_q7.c
CSRCS += $(CMSIS5_NN)/SVDFunctions/arm_svdf_s8.c
CSRCS += $(CMSIS5_NN)/SoftmaxFunctions/arm_softmax_u8.c
CSRCS += $(CMSIS5_NN)/SoftmaxFunctions/arm_softmax_q15.c
CSRCS += $(CMSIS5_NN)/SoftmaxFunctions/arm_softmax_q7.c
CSRCS += $(CMSIS5_NN)/SoftmaxFunctions/arm_softmax_s8.c
CSRCS += $(CMSIS5_NN)/SoftmaxFunctions/arm_softmax_with_batch_q7.c
CSRCS += $(CMSIS5_NN)/FullyConnectedFunctions/arm_fully_connected_s8.c
CSRCS += $(CMSIS5_NN)/FullyConnectedFunctions/arm_fully_connected_q15_opt.c
CSRCS += $(CMSIS5_NN)/FullyConnectedFunctions/arm_fully_connected_q7_opt.c
CSRCS += $(CMSIS5_NN)/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15_opt.c
CSRCS += $(CMSIS5_NN)/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15.c
CSRCS += $(CMSIS5_NN)/FullyConnectedFunctions/arm_fully_connected_q7.c
CSRCS += $(CMSIS5_NN)/FullyConnectedFunctions/arm_fully_connected_q15.c
CSRCS += $(CMSIS5_NN)/ConcatenationFunctions/arm_concatenation_s8_y.c
CSRCS += $(CMSIS5_NN)/ConcatenationFunctions/arm_concatenation_s8_w.c
CSRCS += $(CMSIS5_NN)/ConcatenationFunctions/arm_concatenation_s8_x.c
CSRCS += $(CMSIS5_NN)/ConcatenationFunctions/arm_concatenation_s8_z.c
CSRCS += $(CMSIS5_NN)/PoolingFunctions/arm_avgpool_s8.c
CSRCS += $(CMSIS5_NN)/PoolingFunctions/arm_pool_q7_HWC.c
CSRCS += $(CMSIS5_NN)/PoolingFunctions/arm_max_pool_s8.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_HWC_q15_fast.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_wrapper_s8.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_depthwise_conv_wrapper_s8.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_nn_mat_mult_kernel_q7_q15.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_HWC_q15_fast_nonsquare.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_depthwise_conv_s8_opt.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_s8.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_HWC_q15_basic.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_nn_mat_mult_kernel_q7_q15_reordered.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_nn_mat_mult_kernel_s8_s16.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_HWC_q7_RGB.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_depthwise_conv_3x3_s8.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_depthwise_conv_u8_basic_ver1.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_depthwise_separable_conv_HWC_q7.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_HWC_q7_fast.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_1_x_n_s8.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_1x1_s8_fast.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_HWC_q7_basic.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_nn_depthwise_conv_s8_core.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_depthwise_separable_conv_HWC_q7_nonsquare.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_nn_mat_mult_s8.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_HWC_q7_fast_nonsquare.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_depthwise_conv_s8.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_1x1_HWC_q7_fast_nonsquare.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_HWC_q7_basic_nonsquare.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_CHW_f32_basic_nonsquare.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_CHW_q7_basic_nonsquare.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_convolve_CHW_q15_basic_nonsquare.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_nn_CHW_mat_mult_kernel_q7_q15.c
CSRCS += $(CMSIS5_NN)/ConvolutionFunctions/arm_nn_mat_mult_kernel_s8_s16_reordered.c
CSRCS += $(CMSIS5_NN)/BasicMathFunctions/arm_elementwise_mul_s8.c
CSRCS += $(CMSIS5_NN)/BasicMathFunctions/arm_elementwise_add_s8.c
CSRCS += $(CMSIS5_NN)/ReshapeFunctions/arm_reshape_s8.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nntables.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_q7_to_q15_with_offset.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_q7_to_q15_reordered_no_shift.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_vec_mat_mult_t_svdf_s8.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_vec_mat_mult_t_s8.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_q7_to_q15_no_shift.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_mult_q15.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_depthwise_conv_nt_t_padded_s8.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_mult_q7.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_mat_mul_core_4x_s8.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_mat_mult_nt_t_s8.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_accumulate_q7_to_q15.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_depthwise_conv_nt_t_s8.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_mat_mul_core_1x_s8.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_nn_add_q7.c
CSRCS += $(CMSIS5_NN)/NNSupportFunctions/arm_q7_to_q15_reordered_with_offset.c

MODULE = $(CONFIG_CMSIS_NN)

include $(APPDIR)/Application.mk
