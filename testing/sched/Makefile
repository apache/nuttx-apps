############################################################################
# apps/testing/testsuites/Makefile
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


PRIORITY = $(CONFIG_TESTS_SCHED_PRIORITY)
STACKSIZE = $(CONFIG_TESTS_SCHED_STACKSIZE)
MODULE = $(CONFIG_TESTS_SCHED)


ifneq ($(CONFIG_CM_SCHED_TEST),)
CFLAGS += -I$(CURDIR)/cm_sched_test/include
CSRCS += $(wildcard cm_sched_test/cases/*.c)
CSRCS += $(wildcard cm_sched_test/common/*.c)
PROGNAME += cmocka_sched_test
MAINSRC += $(CURDIR)/cm_sched_test/cmocka_sched_test.c
endif


include $(APPDIR)/Application.mk
