
# apps/testing/ltp/ltp/Makefile
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include $(APPDIR)/Make.defs

LTP_UNPACK   = ltp
LTP_URL      = https://github.com/linux-test-project/ltp.git

TESTDIR      = $(LTP_UNPACK)/testcases/open_posix_testsuite

ifneq ($(wildcard $(TESTDIR)),)
ifeq ($(CONFIG_FS_AIO),)
BLACKWORDS  += "aio.h"
BLACKWORDS  += "SIGPOLL"
endif
BLACKWORDS  += "affinity.h"
BLACKWORDS  += "endpwent"
BLACKWORDS  += "langinfo.h"
BLACKWORDS  += "ucontext.h"
BLACKWORDS  += "noatime.h"
BLACKWORDS  += "clock_getcpuclockid"
BLACKWORDS  += "CLOCK_PROCESS_CPUTIME_ID"
BLACKWORDS  += "CLOCK_THREAD_CPUTIME_ID"
BLACKWORDS  += "fork()"
BLACKWORDS  += "getpwent"
BLACKWORDS  += "killpg"
BLACKWORDS  += "pthread_atfork"
BLACKWORDS  += "pthread_attr_setscope"
BLACKWORDS  += "pthread_condattr_setpshared"
BLACKWORDS  += "pthread_condattr_getpshared"
BLACKWORDS  += "pthread_getattr_np"
BLACKWORDS  += "pthread_getcpuclockid"
BLACKWORDS  += "pthread_mutex_getprioceiling"
BLACKWORDS  += "pthread_mutexattr_getprioceiling"
BLACKWORDS  += "pthread_mutexattr_setprioceiling"
BLACKWORDS  += "pthread_rwlockattr_destroy"
BLACKWORDS  += "pthread_rwlockattr_getpshared"
BLACKWORDS  += "pthread_rwlockattr_init"
BLACKWORDS  += "RLIMIT_MEMLOCK"
BLACKWORDS  += "PTHREAD_SCOPE_PROCESS"
BLACKWORDS  += "setpwent"
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
BLACKSRCS += 35-1-buildonly.c
BLACKSRCS += 35-2-buildonly.c

MAINWORDS    += "main("

$(foreach word, $(BLACKWORDS), $(eval BLACKLIST+=$(shell find $(TESTDIR) -name *.c |xargs grep -lr $(word))))
$(foreach src, $(BLACKSRCS), $(eval BLACKLIST+=$(filter %$(src),$(shell find $(TESTDIR) -name *.c))))

LTP_ORIGINS  := $(filter-out $(BLACKLIST), $(shell find $(TESTDIR) -name *.c))

$(foreach word, $(MAINWORDS), $(eval LTP_MAINCSRCS+=$(shell grep -lr $(word) $(LTP_ORIGINS))))

LTP_CSRCS    := $(filter-out $(LTP_MAINCSRCS), $(LTP_ORIGINS))

PROGNAME     := $(basename $(shell echo $(LTP_MAINCSRCS) | xargs -n 1 | awk -F "[/]" '{print "ltp_"$$(NF-2)"_"$$(NF-1)"_"$$(NF)}' | sed s/-/_/g))
MAINSRC       = $(LTP_MAINCSRCS)
PRIORITY      = SCHED_PRIORITY_DEFAULT
STACKSIZE     = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE        = $(CONFIG_TESTING_LTP)

CSRCS        := $(LTP_CSRCS)
CFLAGS       += -I$(LTP_UNPACK)
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

context:: $(LTP_UNPACK)

distclean::
	$(Q) rm -rf $(LTP_UNPACK)

include $(APPDIR)/Application.mk
