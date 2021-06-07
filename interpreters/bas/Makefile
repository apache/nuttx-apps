############################################################################
# apps/bas/Makefile
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

# BAS Library

CSRCS  = bas.c bas_auto.c bas_fs.c bas_global.c bas_program.c
CSRCS += bas_str.c bas_token.c bas_value.c bas_var.c

ifeq ($(CONFIG_INTERPRETER_BAS_VT100),y)
CSRCS += bas_vt100.c
endif

MAINSRC = bas_main.c

# BAS built-in application info

PROGNAME  = bas
PRIORITY  = $(CONFIG_INTERPRETER_BAS_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETER_BAS_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_BAS)

include $(APPDIR)/Application.mk
