############################################################################
# apps/testing/ostest/Makefile
#
#   Copyright (C) 2007-2012, 2014, 2017 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

include $(APPDIR)/Make.defs

# ostest built-in application info

PROGNAME = ostest
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_TESTING_OSTEST)

# NuttX OS Test

CSRCS   = dev_null.c restart.c sigprocmask.c sighand.c signest.c
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
CSRCS += sporadic.c
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

include $(APPDIR)/Application.mk
