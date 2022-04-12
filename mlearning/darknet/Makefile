############################################################################
# apps/mlearning/darknet/Makefile
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

ifeq ($(CONFIG_DARKNET_YOLO),y)
SRC = darknet/src

CSRCS   +=$(SRC)/gemm.c
CSRCS   +=$(SRC)/utils.c
CSRCS   +=$(SRC)/cuda.c
CSRCS   +=$(SRC)/deconvolutional_layer.c
CSRCS   +=$(SRC)/convolutional_layer.c
CSRCS   +=$(SRC)/list.c
CSRCS   +=$(SRC)/image.c
CSRCS   +=$(SRC)/activations.c
CSRCS   +=$(SRC)/im2col.c
CSRCS   +=$(SRC)/col2im.c
CSRCS   +=$(SRC)/blas.c
CSRCS   +=$(SRC)/crop_layer.c
CSRCS   +=$(SRC)/dropout_layer.c
CSRCS   +=$(SRC)/maxpool_layer.c
CSRCS   +=$(SRC)/softmax_layer.c
CSRCS   +=$(SRC)/data.c
CSRCS   +=$(SRC)/matrix.c
CSRCS   +=$(SRC)/network.c
CSRCS   +=$(SRC)/connected_layer.c
CSRCS   +=$(SRC)/cost_layer.c
CSRCS   +=$(SRC)/parser.c
CSRCS   +=$(SRC)/option_list.c
CSRCS   +=$(SRC)/detection_layer.c
CSRCS   +=$(SRC)/route_layer.c
CSRCS   +=$(SRC)/upsample_layer.c
CSRCS   +=$(SRC)/box.c
CSRCS   +=$(SRC)/normalization_layer.c
CSRCS   +=$(SRC)/avgpool_layer.c
CSRCS   +=$(SRC)/layer.c
CSRCS   +=$(SRC)/local_layer.c
CSRCS   +=$(SRC)/shortcut_layer.c
CSRCS   +=$(SRC)/logistic_layer.c
CSRCS   +=$(SRC)/activation_layer.c
CSRCS   +=$(SRC)/rnn_layer.c
CSRCS   +=$(SRC)/gru_layer.c
CSRCS   +=$(SRC)/crnn_layer.c
CSRCS   +=$(SRC)/demo.c
CSRCS   +=$(SRC)/batchnorm_layer.c
CSRCS   +=$(SRC)/region_layer.c
CSRCS   +=$(SRC)/reorg_layer.c
CSRCS   +=$(SRC)/tree.c
CSRCS   +=$(SRC)/lstm_layer.c
CSRCS   +=$(SRC)/l2norm_layer.c
CSRCS   +=$(SRC)/yolo_layer.c
CSRCS   +=$(SRC)/iseg_layer.c


CFLAGS += -Wno-shadow -Wno-strict-prototypes -Wno-unknown-pragmas

MODULE = $(CONFIG_DARKNET_YOLO)

darknet.zip:
	$(Q) curl -L https://github.com/pjreddie/darknet/archive/refs/heads/$(DARKNET_YOLO_VER).zip -o darknet.zip
	$(Q) unzip -o darknet.zip
	$(Q) mv darknet-$(DARKNET_YOLO_VER) darknet

context:: darknet.zip

distclean::
	$(call DELDIR, darknet)
	$(call DELFILE, darknet.zip)

endif

include $(APPDIR)/Application.mk
