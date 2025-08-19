############################################################################
# apps/examples/litmus/Makefile
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

PROGNAME  = $(CONFIG_EXAMPLES_LITMUS_PROGNAME)
PRIORITY  = $(CONFIG_EXAMPLES_LITMUS_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_LITMUS_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_LITMUS)

SRC_DIRS := $(CURDIR)/src
VPATH    += $(SRC_DIRS)
MAINSRC = "$(CURDIR)/src/run.c"

CSRCS    += $(foreach d,$(SRC_DIRS),$(notdir $(wildcard $(d)/*.c)))
CSRCS    := $(filter-out $(MAINSRC),$(CSRCS))

CFLAGS+=-D_GNU_SOURCE -DFORCE_AFFINITY -Wall -std=gnu99

include $(APPDIR)/Application.mk
