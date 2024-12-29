############################################################################
# apps/testing/testsuites/Makefile
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


PRIORITY = $(CONFIG_TESTS_TESTSUITES_PRIORITY)
STACKSIZE = $(CONFIG_TESTS_TESTSUITES_STACKSIZE)
MODULE = $(CONFIG_TESTS_TESTSUITES)

ifneq ($(CONFIG_CM_FS_TEST),)
CFLAGS += -I$(CURDIR)/kernel/fs/include
CFLAGS += -I$(APPDIR)/nshlib
CSRCS += $(wildcard kernel/fs/cases/*.c)
CSRCS += $(wildcard kernel/fs/common/*.c)
PROGNAME += cmocka_fs_test
MAINSRC += $(CURDIR)/kernel/fs/cmocka_fs_test.c
endif

ifneq ($(CONFIG_CM_SCHED_TEST),)
CFLAGS += -I$(CURDIR)/kernel/sched/include
CSRCS += $(wildcard kernel/sched/cases/*.c)
CSRCS += $(wildcard kernel/sched/common/*.c)
PROGNAME += cmocka_sched_test
MAINSRC += $(CURDIR)/kernel/sched/cmocka_sched_test.c
endif

ifneq ($(CONFIG_CM_MM_TEST),)
CFLAGS += -I$(CURDIR)/kernel/mm/include
CSRCS += $(wildcard kernel/mm/cases/*.c)
CSRCS += $(wildcard kernel/mm/common/*.c)
PROGNAME += cmocka_mm_test
MAINSRC += $(CURDIR)/kernel/mm/cmocka_mm_test.c
PROGNAME += mem_batch_opt_perf_test
MAINSRC += $(CURDIR)/kernel/mm/tool/mem_batch_opt_perf_test.c
PROGNAME += mem_cycle_opt_perf_test
MAINSRC += $(CURDIR)/kernel/mm/tool/mem_cycle_opt_perf_test.c
endif

ifneq ($(CONFIG_CM_TIME_TEST),)
CFLAGS += -I$(CURDIR)/kernel/time/include
CSRCS += $(wildcard kernel/time/cases/*.c)
PROGNAME += cmocka_time_test
MAINSRC += $(CURDIR)/kernel/time/cmocka_time_test.c
endif

ifneq ($(CONFIG_CM_PTHREAD_TEST),)
CFLAGS += -I$(CURDIR)/kernel/pthread/include
CSRCS += $(wildcard kernel/pthread/cases/*.c)
CSRCS += $(wildcard kernel/pthread/common/*.c)
PROGNAME += cmocka_pthread_test
MAINSRC += $(CURDIR)/kernel/pthread/cmocka_pthread_test.c
endif

ifneq ($(CONFIG_CM_MUTEX_TEST),)
CFLAGS += -I$(CURDIR)/kernel/mutex/include
CSRCS += $(wildcard kernel/mutex/cases/*.c)
CSRCS += $(wildcard kernel/mutex/common/*.c)
PROGNAME += cmocka_mutex_test
MAINSRC += $(CURDIR)/kernel/mutex/cmocka_mutex_test.c
endif

ifneq ($(CONFIG_CM_SOCKET_TEST),)
CFLAGS += -I$(CURDIR)/kernel/socket/include
CSRCS += $(wildcard kernel/socket/cases/*.c)
PROGNAME += cmocka_socket_test
MAINSRC += $(CURDIR)/kernel/socket/cmocka_socket_test.c
endif

ifneq ($(CONFIG_CM_SYSCALL_TEST),)
CFLAGS += -I$(CURDIR)/kernel/syscall/include
CFLAGS += -I$(APPDIR)/nshlib
CSRCS += $(wildcard kernel/syscall/cases/*.c)
CSRCS += $(wildcard kernel/syscall/common/*.c)
PROGNAME += cmocka_syscall_test
MAINSRC += $(CURDIR)/kernel/syscall/cmocka_syscall_test.c
endif

include $(APPDIR)/Application.mk
