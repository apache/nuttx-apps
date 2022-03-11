############################################################################
# apps/testing/ostest/Makefile
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

# ostest built-in application info

PROGNAME = ostest
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_TESTING_OSTEST)

# NuttX OS Test

CSRCS   = getopt.c dev_null.c restart.c sigprocmask.c sighand.c signest.c
MAINSRC = ostest_main.c

ifeq ($(CONFIG_SIG_SIGSTOP_ACTION),y)
ifeq ($(CONFIG_SIG_SIGKILL_ACTION),y)
CSRCS += suspend.c
endif
endif

ifeq ($(CONFIG_ARCH_FPU),y)
ifneq ($(CONFIG_TESTING_OSTEST_FPUTESTDISABLE),y)
CSRCS += fpu.c
endif
endif

ifneq ($(CONFIG_STDIO_DISABLE_BUFFERING),y)
CSRCS += setvbuf.c
endif

ifneq ($(CONFIG_TLS_NELEM),0)
CSRCS += tls.c
endif

ifeq ($(CONFIG_TESTING_OSTEST_AIO),y)
CSRCS += aio.c
endif

ifeq ($(CONFIG_SCHED_WAITPID),y)
CSRCS += waitpid.c
endif

ifneq ($(CONFIG_DISABLE_PTHREAD),y)
CSRCS += cancel.c cond.c mutex.c timedmutex.c sem.c semtimed.c barrier.c
CSRCS += timedwait.c
CSRCS += pthread_rwlock.c pthread_rwlock_cancel.c

ifneq ($(CONFIG_TLS_NELEM),0)
CSRCS += specific.c
endif

ifeq ($(CONFIG_PTHREAD_CLEANUP),y)
CSRCS += pthread_cleanup.c
endif

ifneq ($(CONFIG_PTHREAD_MUTEX_UNSAFE),y)
CSRCS += robust.c
endif

ifeq ($(CONFIG_PTHREAD_MUTEX_TYPES),y)
CSRCS += rmutex.c
endif

ifeq ($(CONFIG_FS_NAMED_SEMAPHORES),y)
CSRCS += nsem.c
endif

ifneq ($(CONFIG_RR_INTERVAL),0)
CSRCS += roundrobin.c
endif

ifeq ($(CONFIG_SCHED_SPORADIC),y)
CSRCS += sporadic.c sporadic2.c
endif
endif # CONFIG_DISABLE_PTHREAD

ifneq ($(CONFIG_DISABLE_MQUEUE),y)
ifneq ($(CONFIG_DISABLE_PTHREAD),y)
CSRCS += mqueue.c timedmqueue.c
endif # CONFIG_DISABLE_PTHREAD
endif # CONFIG_DISABLE_MQUEUE

ifneq ($(CONFIG_DISABLE_POSIX_TIMERS),y)
CSRCS += posixtimer.c
ifeq ($(CONFIG_SIG_EVTHREAD),y)
CSRCS += sigev_thread.c
endif
endif

ifeq ($(CONFIG_ARCH_HAVE_VFORK),y)
ifeq ($(CONFIG_SCHED_WAITPID),y)
CSRCS += vfork.c
endif
endif

ifneq ($(CONFIG_DISABLE_PTHREAD),y)
ifeq ($(CONFIG_PRIORITY_INHERITANCE),y)
CSRCS += prioinherit.c
endif # CONFIG_PRIORITY_INHERITANCE
endif # CONFIG_DISABLE_PTHREAD

ifeq ($(CONFIG_ARCH_SETJMP_H),y)
CSRCS += setjmp.c
endif

include $(APPDIR)/Application.mk
