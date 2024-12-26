############################################################################
# apps/testing/fstest/Makefile
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

# Generic file system stress test application info

PROGNAME = $(CONFIG_TESTING_FSTEST_PROGNAME)
PRIORITY = $(CONFIG_TESTING_FSTEST_PRIORITY)
STACKSIZE = $(CONFIG_TESTING_FSTEST_STACKSIZE)
MODULE = $(CONFIG_TESTING_FSTEST)

# Generic file system stress test
ifneq ($(CONFIG_CM_FS_TEST),)
CFLAGS += -I$(CURDIR)/cm_fs_test/include
CFLAGS += -I$(APPDIR)/nshlib
CSRCS += $(wildcard cm_fs_test/cases/*.c)
CSRCS += $(wildcard cm_fs_test/common/*.c)
PROGNAME += cmocka_fs_test
MAINSRC += $(CURDIR)/cm_fs_test/cmocka_fs_test.c
endif

MAINSRC += $(CURDIR)/fstest/fstest_main.c

include $(APPDIR)/Application.mk
