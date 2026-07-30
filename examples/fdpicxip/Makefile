############################################################################
# apps/examples/fdpicxip/Makefile
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

# FDPIC modules executed in place from xipfs.  The modules are embedded as
# headers, built from the sources in modules/ by an explicit 'make regen'
# there: linking one needs arm-uclinuxfdpiceabi binutils, which this build
# does not.

MAINSRC   = fdpicxip_main.c

PROGNAME  = fdpicxip
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_FDPICXIP)

include $(APPDIR)/Application.mk
