############################################################################
# apps/examples/i2schar/Makefile
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

# I2S character driver test

ifeq ($(CONFIG_EXAMPLES_I2SCHAR_TX),y)
CSRCS += i2schar_transmitter.c
endif
ifeq ($(CONFIG_EXAMPLES_I2SCHAR_RX),y)
CSRCS += i2schar_receiver.c
endif
MAINSRC = i2schar_main.c

# Touchscreen built-in application info

PROGNAME = i2schar
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_I2SCHAR)

include $(APPDIR)/Application.mk
