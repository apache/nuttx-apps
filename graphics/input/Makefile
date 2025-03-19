############################################################################
# apps/graphics/input/Makefile
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

ifneq ($(CONFIG_GRAPHICS_INPUT_GENERATOR),)
CSRCS = $(filter-out generator/input.c, $(wildcard generator/*.c))
endif

# Monkey test

ifneq ($(CONFIG_GRAPHICS_INPUT_MONKEY),)
PROGNAME  += monkey
PRIORITY  += $(CONFIG_GRAPHICS_INPUT_MONKEY_PRIORITY)
STACKSIZE += $(CONFIG_GRAPHICS_INPUT_MONKEY_STACKSIZE)
MODULE    += $(CONFIG_GRAPHICS_INPUT_MONKEY)

MAINSRC   += monkey/monkey_main.c

CSRCS     += $(filter-out monkey/monkey_main.c, $(wildcard monkey/*.c))
endif

# Input tool

ifneq ($(CONFIG_GRAPHICS_INPUT_TOOL),)
PROGNAME  += input
PRIORITY  += $(CONFIG_GRAPHICS_INPUT_TOOL_PRIORITY)
STACKSIZE += $(CONFIG_GRAPHICS_INPUT_TOOL_STACKSIZE)
MODULE    += $(CONFIG_GRAPHICS_INPUT_TOOL)

MAINSRC   += generator/input.c
endif

include $(APPDIR)/Application.mk
