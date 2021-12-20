############################################################################
# apps/examples/foc/Makefile
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

# FOC built-in application info

PROGNAME  = foc
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_FOC)

# FOC motor controller example

MAINSRC = foc_main.c

ASRCS =
CSRCS = foc_device.c foc_mq.c foc_thr.c foc_intf.c

ifeq ($(CONFIG_BUILTIN),y)
CSRCS += foc_parseargs.c
endif

# fixed16 support

ifeq ($(CONFIG_INDUSTRY_FOC_FIXED16),y)
  CSRCS += foc_fixed16_thr.c foc_motor_b16.c
endif

# float32 support

ifeq ($(CONFIG_INDUSTRY_FOC_FLOAT),y)
  CSRCS += foc_float_thr.c foc_motor_f32.c
endif

include $(APPDIR)/Application.mk
