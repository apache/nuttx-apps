############################################################################
# apps/system/nxinit/Makefile
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

# NuttX Init

MAINSRC += init.c
CSRCS += builtin.c
CSRCS += parser.c
CSRCS += action.c
CSRCS += service.c
CSRCS += import.c
CSRCS += property_simple.c

PROGNAME += $(CONFIG_SYSTEM_NXINIT_PROGNAME)
PRIORITY += $(CONFIG_SYSTEM_NXINIT_PRIORITY)
STACKSIZE += $(CONFIG_SYSTEM_NXINIT_STACKSIZE)
MODULE = $(CONFIG_SYSTEM_NXINIT)

# NxInit unit tests (cmocka), reusing the same parser/action/service CSRCS
# built above.

ifneq ($(CONFIG_SYSTEM_NXINIT_TEST),)
CSRCS += test/test_nxinit_common.c
CSRCS += test/test_nxinit_parser.c
CSRCS += test/test_nxinit_action.c
CSRCS += test/test_nxinit_service.c
MAINSRC += test/test_nxinit.c

PROGNAME += nxinit_unit_test
PRIORITY += $(CONFIG_SYSTEM_NXINIT_TEST_PRIORITY)
STACKSIZE += $(CONFIG_SYSTEM_NXINIT_TEST_STACKSIZE)
endif

include $(APPDIR)/Application.mk
