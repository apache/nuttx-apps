############################################################################
# apps/testing/ltp/Makefile
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

LTP_UNPACK   = ltp
LTP_URL      = https://github.com/linux-test-project/ltp.git

TESTDIR      = $(LTP_UNPACK)/testcases/open_posix_testsuite

ifneq ($(wildcard $(TESTDIR)),)
ifeq ($(CONFIG_FS_AIO),)
BLACKWORDS  += "aio.h"
BLACKWORDS  += "SIGPOLL"
endif
BLACKWORDS  += "SA_RESTART"
BLACKWORDS  += "pthread_atfork"
BLACKWORDS  += "pthread_condattr_setpshared"
BLACKWORDS  += "pthread_condattr_getpshared"
BLACKWORDS  += "pthread_mutex_getprioceiling"
BLACKWORDS  += "pthread_mutexattr_setprioceiling"
BLACKWORDS  += "pthread_mutexattr_getprioceiling"
BLACKWORDS  += "pthread_getattr_np"
BLACKWORDS  += "pthread_getcpuclockid"
BLACKWORDS  += "pthread_rwlockattr_init"
BLACKWORDS  += "pthread_rwlockattr_destroy"
BLACKWORDS  += "pthread_mutex_getprioceiling"
BLACKWORDS  += "clock_getcpuclockid"
BLACKWORDS  += "shm_unlink"
BLACKWORDS  += "shm_open"
BLACKWORDS  += "mlock"
BLACKWORDS  += "munlock"
BLACKWORDS  += "mlockall"
BLACKWORDS  += "msync"
BLACKWORDS  += "getpgrp"
BLACKWORDS  += "lfind"
BLACKWORDS += "killpg"
BLACKWORDS  += "setpwent"
ifeq ($(CONFIG_PTHREAD_SPINLOCKS),)
BLACKWORDS += "pthread_spin_init"
BLACKWORDS += "pthread_spin_destroy"
BLACKWORDS += "pthread_spin_trylock"
endif

BLACKWORDS  += "affinity.h"
BLACKWORDS  += "langinfo.h"
BLACKWORDS  += "ucontext.h"
BLACKWORDS  += "noatime.h"
BLACKWORDS  += "CLOCK_PROCESS_CPUTIME_ID"
BLACKWORDS  += "CLOCK_THREAD_CPUTIME_ID"
BLACKWORDS  += "fork()"
BLACKWORDS  += "pthread_attr_setscope"
BLACKWORDS  += "RLIMIT_MEMLOCK"
BLACKWORDS  += "PTHREAD_SCOPE_PROCESS"
BLACKWORDS  += "SIGABRT"
BLACKWORDS  += "SIGBUS"
BLACKWORDS  += "SIGFPE"
BLACKWORDS  += "SIGHUP"
BLACKWORDS  += "SIGILL"
BLACKWORDS  += "SIGPROF"
BLACKWORDS  += "SIGSEGV"
BLACKWORDS  += "SIGSYS"
BLACKWORDS  += "SIGTRAP"
BLACKWORDS  += "SIGTSTP"
BLACKWORDS  += "SIGTTIN"
BLACKWORDS  += "SIGTTOU"
BLACKWORDS  += "SIGURG"
BLACKWORDS  += "SIGVTALRM"
BLACKWORDS  += "SIGXCPU"
BLACKWORDS  += "SIGXFSZ"
BLACKWORDS  += "SIGSTKSZ"
BLACKWORDS  += "stack_t"
BLACKWORDS  += "siginterrupt"
BLACKWORDS  += "threads_scenarii.c"
BLACKWORDS  += "PTHREAD_PRIO_PROTECT"
BLACKWORDS  += "pthread_mutex_lock"

BLACKWORDS  += "ILL_[A-Z]"
BLACKWORDS  += "FPE_[A-Z]"
BLACKWORDS  += "BUS_[A-Z]"
BLACKWORDS  += "TRAP_[A-Z]"
BLACKWORDS  += "SEGV_[A-Z]"
BLACKWORDS  += "POLL_[A-Z]"
BLACKWORDS  += "SA_ONSTACK"
BLACKWORDS  += "MINSIGSTKSZ"
BLACKWORDS  += "FPE_FLTINV"

BLACKSRCS += testfrmw.c
BLACKSRCS += 7-3-buildonly.c
BLACKSRCS += 7-4-buildonly.c
BLACKSRCS += 7-5-buildonly.c
BLACKSRCS += 11-1-buildonly.c
BLACKSRCS += 17-1-buildonly.c
BLACKSRCS += 19-1-buildonly.c
BLACKSRCS += 21-1-buildonly.c
BLACKSRCS += 22-31-buildonly.c
BLACKSRCS += 22-32-buildonly.c
BLACKSRCS += 25-1-buildonly.c
BLACKSRCS += 27-1-buildonly.c
BLACKSRCS += 34-1-buildonly.c
BLACKSRCS += 35-1-buildonly.c
BLACKSRCS += 35-2-buildonly.c
BLACKSRCS += 27-2.c
ifeq ($(CONFIG_PTHREAD_SPINLOCKS),)
BLACKSRCS += 3-12-buildonly.c
endif
BLACKSRCS += 35-3-buildonly.c

MAINWORDS    += "main("

LTP_ORIGS_1  := $(shell find $(TESTDIR) -name *.c | head -n 500)
LTP_ORIGS_2  := $(shell find $(TESTDIR) -name *.c | head -n 1000|tail -n +501)
LTP_ORIGS_3  := $(shell find $(TESTDIR) -name *.c | head -n 1500|tail -n +1001)
LTP_ORIGS_4  := $(shell find $(TESTDIR) -name *.c | head -n 2000|tail -n +1501)

$(foreach word, $(BLACKWORDS), $(eval BLACKLIST+=$(shell grep -lr $(word) $(LTP_ORIGS_1))))
$(foreach src, $(BLACKSRCS), $(eval BLACKLIST+=$(filter %$(src),$(LTP_ORIGS_1))))

$(foreach word, $(BLACKWORDS), $(eval BLACKLIST+=$(shell grep -lr $(word) $(LTP_ORIGS_2))))
$(foreach src, $(BLACKSRCS), $(eval BLACKLIST+=$(filter %$(src),$(LTP_ORIGS_2))))

$(foreach word, $(BLACKWORDS), $(eval BLACKLIST+=$(shell grep -lr $(word) $(LTP_ORIGS_3))))
$(foreach src, $(BLACKSRCS), $(eval BLACKLIST+=$(filter %$(src),$(LTP_ORIGS_3))))

$(foreach word, $(BLACKWORDS), $(eval BLACKLIST+=$(shell grep -lr $(word) $(LTP_ORIGS_4))))
$(foreach src, $(BLACKSRCS), $(eval BLACKLIST+=$(filter %$(src),$(LTP_ORIGS_4))))

LTP_ORIGS_1  := $(filter-out $(BLACKLIST), $(LTP_ORIGS_1))
LTP_ORIGS_2  := $(filter-out $(BLACKLIST), $(LTP_ORIGS_2))
LTP_ORIGS_3  := $(filter-out $(BLACKLIST), $(LTP_ORIGS_3))
LTP_ORIGS_4  := $(filter-out $(BLACKLIST), $(LTP_ORIGS_4))

$(foreach word, $(MAINWORDS), $(eval LTP_MAINCSRCS_1+=$(shell grep -lr $(word) $(LTP_ORIGS_1))))
$(foreach word, $(MAINWORDS), $(eval LTP_MAINCSRCS_2+=$(shell grep -lr $(word) $(LTP_ORIGS_2))))
$(foreach word, $(MAINWORDS), $(eval LTP_MAINCSRCS_3+=$(shell grep -lr $(word) $(LTP_ORIGS_3))))
$(foreach word, $(MAINWORDS), $(eval LTP_MAINCSRCS_4+=$(shell grep -lr $(word) $(LTP_ORIGS_4))))

LTP_CSRCS_1  := $(filter-out $(LTP_MAINCSRCS_1), $(LTP_ORIGS_1))
LTP_CSRCS_2  := $(filter-out $(LTP_MAINCSRCS_2), $(LTP_ORIGS_2))
LTP_CSRCS_3  := $(filter-out $(LTP_MAINCSRCS_3), $(LTP_ORIGS_3))
LTP_CSRCS_4  := $(filter-out $(LTP_MAINCSRCS_4), $(LTP_ORIGS_4))


PROGNAME     := $(basename $(shell echo $(LTP_MAINCSRCS_1) | xargs -n 1 | awk -F "[/]" '{print "ltp_"$$(NF-2)"_"$$(NF-1)"_"$$(NF)}' | sed s/-/_/g))
PROGNAME     += $(basename $(shell echo $(LTP_MAINCSRCS_2) | xargs -n 1 | awk -F "[/]" '{print "ltp_"$$(NF-2)"_"$$(NF-1)"_"$$(NF)}' | sed s/-/_/g))
PROGNAME     += $(basename $(shell echo $(LTP_MAINCSRCS_3) | xargs -n 1 | awk -F "[/]" '{print "ltp_"$$(NF-2)"_"$$(NF-1)"_"$$(NF)}' | sed s/-/_/g))
PROGNAME     += $(basename $(shell echo $(LTP_MAINCSRCS_4) | xargs -n 1 | awk -F "[/]" '{print "ltp_"$$(NF-2)"_"$$(NF-1)"_"$$(NF)}' | sed s/-/_/g))
MAINSRC       = $(LTP_MAINCSRCS_1) $(LTP_MAINCSRCS_2) $(LTP_MAINCSRCS_3) $(LTP_MAINCSRCS_4)
PRIORITY      = SCHED_PRIORITY_DEFAULT
STACKSIZE     = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE        = $(CONFIG_TESTING_LTP)

CSRCS        := $(LTP_CSRCS_1) $(LTP_CSRCS_2) $(LTP_CSRCS_3) $(LTP_CSRCS_4)
CFLAGS       += -I$(CURDIR)
CFLAGS       += -I$(TESTDIR)/include
endif

# Relax warning checks to avoid expected compile errors:
CFLAGS += -Wno-strict-prototypes -Wno-return-type -Wno-format -Wno-uninitialized
CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-value

# Should be removed if possible in the future
CFLAGS += -Wno-incompatible-pointer-types -Wno-overflow -Wno-int-to-pointer-cast

$(LTP_UNPACK):
	$(Q) echo "git clone $(LTP_URL)"
	$(Q) git clone $(LTP_URL)
	$(Q) git -C $(LTP_UNPACK) am < 0001-Fix-static-struct-warning.patch
	$(Q) git -C $(LTP_UNPACK) am < 0002-Use-ifdef-instead-of-if-for-__linux__.patch

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LTP_UNPACK)/.git),)
context:: $(LTP_UNPACK)

distclean::
	$(Q) rm -rf $(LTP_UNPACK)
endif

include $(APPDIR)/Application.mk
