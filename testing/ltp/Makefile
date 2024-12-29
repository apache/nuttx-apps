############################################################################
# apps/testing/ltp/Makefile
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

LTPS_VERSION     = 20230516
LTP_DOWNLOAD_URL = https://github.com/linux-test-project/ltp/archive/refs/tags/
LTP_UNPACK       = ltp
LTP_URL          = https://github.com/linux-test-project/ltp.git

TESTDIR          = $(LTP_UNPACK)/testcases/open_posix_testsuite

ifneq ($(wildcard $(TESTDIR)),)
ifeq ($(CONFIG_FS_AIO),)
BLACKWORDS  += "aio.h"
BLACKWORDS  += "SIGPOLL"
endif
BLACKWORDS  += "pthread_mutexattr_setprioceiling"
BLACKWORDS  += "pthread_mutexattr_getprioceiling"
BLACKWORDS  += "pthread_getattr_np"
BLACKWORDS  += "pthread_mutex_getprioceiling"
ifeq ($(CONFIG_PTHREAD_SPINLOCKS),)
BLACKWORDS  += "pthread_spin_init"
BLACKWORDS  += "pthread_spin_destroy"
BLACKWORDS  += "pthread_spin_trylock"
endif

BLACKWORDS  += "CHILD_MAX"
BLACKWORDS  += "setpgid("
BLACKWORDS  += "PTHREAD_SCOPE_PROCESS"
BLACKWORDS  += "setpgrp"
BLACKWORDS  += "threads_scenarii.c"
BLACKWORDS  += "pthread_mutex_lock"
BLACKWORDS  += "ucontext.h"

BLACKWORDS  += "msync"
BLACKWORDS  += "lfind"

BLACKSRCS += $(TESTDIR)/conformance/definitions/time_h/19-1-buildonly.c
BLACKSRCS += $(TESTDIR)/conformance/definitions/signal_h/21-1-buildonly.c
BLACKSRCS += $(TESTDIR)/conformance/definitions/signal_h/27-1-buildonly.c
BLACKSRCS += $(TESTDIR)/conformance/definitions/time_h/19-1-buildonly.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/sem_post/8-1.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/clock_nanosleep/9-1.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/clock_nanosleep/2-2.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/clock_nanosleep/10-1.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/clock_nanosleep/1-5.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/clock_nanosleep/1-3.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/clock_nanosleep/2-3.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/clock_nanosleep/1-4.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/pthread_sigmask/18-1.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/pthread_sigmask/4-1.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/sigprocmask/4-1.c
ifeq ($(CONFIG_LIBC_LOCALTIME),)
BLACKSRCS += $(TESTDIR)/conformance/definitions/time_h/34-1-buildonly.c
BLACKSRCS += $(TESTDIR)/conformance/definitions/time_h/35-3-buildonly.c
endif
BLACKSRCS += $(TESTDIR)/conformance/definitions/time_h/35-1-buildonly.c
BLACKSRCS += $(TESTDIR)/conformance/definitions/time_h/35-2-buildonly.c
BLACKSRCS += $(TESTDIR)/stress/threads/pthread_kill/stress.c
ifeq ($(CONFIG_PTHREAD_SPINLOCKS),)
BLACKSRCS += $(TESTDIR)/conformance/definitions/pthread_h/3-12-buildonly.c
endif
ifeq ($(CONFIG_SCHED_CHILD_STATUS),)
BLACKSRCS += $(TESTDIR)/conformance/interfaces/pthread_exit/6-1.c
BLACKSRCS += $(TESTDIR)/conformance/behavior/WIFEXITED/1-1.c
BLACKSRCS += $(TESTDIR)/conformance/behavior/WIFEXITED/1-2.c
endif
ifeq ($(CONFIG_FS_TMPFS),)
BLACKWORDS += lio_listio
endif
ifeq ($(CONFIG_SIG_EVTHREAD),)
BLACKWORDS += fork
endif
ifneq ($(shell expr $(CONFIG_MQ_MAXMSGSIZE) \>= 64),1)
BLACKWORDS += mq_close
BLACKWORDS += mq_getattr
BLACKWORDS += mq_notify
BLACKWORDS += mq_open
BLACKWORDS += mq_receive
BLACKWORDS += mq_send
BLACKWORDS += mq_setattr
BLACKWORDS += mq_timedreceive
BLACKWORDS += mq_timedsend
BLACKWORDS += mq_unlink
endif
ifeq ($(CONFIG_PTHREAD_MUTEX_TYPES),)
BLACKWORDS += pthread_mutexattr_settype
endif
ifeq ($(CONFIG_SCHED_SPORADIC),)
BLACKSRCS += $(TESTDIR)/conformance/interfaces/sched_setscheduler/19-2.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/sched_setscheduler/19-3.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/sched_setscheduler/19-4.c
endif
ifeq ($(CONFIG_SIG_DEFAULT),)
BLACKWORDS += SIGKILL
BLACKWORDS += SIGSTOP
endif
ifneq ($(shell expr $(CONFIG_RR_INTERVAL) \> 0),1)
BLACKWORDS += SCHED_RR
endif
ifeq ($(CONFIG_FS_NAMED_SEMAPHORES),)
BLACKWORDS += sem_open
BLACKWORDS += sem_close
BLACKWORDS += sem_unlink
endif
ifeq ($(CONFIG_PIPES),)
BLACKWORDS += pipe
endif
ifneq ($(shell expr $(CONFIG_LIBC_MAX_EXITFUNS) \>= 32),1)
BLACKSRCS += $(TESTDIR)/conformance/interfaces/pthread_exit/4-1.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/pthread_exit/5-1.c
endif
ifeq ($(CONFIG_CANCELLATION_POINTS),)
BLACKSRCS += $(TESTDIR)/conformance/interfaces/pthread_exit/3-1.c
endif
ifneq ($(shell expr $(CONFIG_TLS_NELEM) \>= 16),1)
BLACKSRCS += $(TESTDIR)/conformance/interfaces/pthread_setspecific/1-1.c
endif
ifeq ($(CONFIG_PTHREAD_SPINLOCKS),)
BLACKWORDS += pthread_spin_lock
endif
ifeq ($(CONFIG_SCHED_WAITPID),)
BLACKWORDS += waitpid
endif
ifeq ($(CONFIG_SCHED_USER_IDENTITY),)
BLACKWORDS += setuid
BLACKWORDS += setgid
BLACKWORDS += getuid
BLACKWORDS += getgid
BLACKWORDS += seteuid
BLACKWORDS += setegid
BLACKWORDS += geteuid
BLACKWORDS += getegid
endif
ifeq ($(CONFIG_TLS_NELEM),0)
BLACKWORDS += pthread_key_create
BLACKWORDS += pthread_key_delete
BLACKWORDS += pthread_setspecific
BLACKWORDS += pthread_getspecific
endif
ifeq ($(CONFIG_TLS_NCLEANUP),0)
BLACKWORDS += pthread_cleanup_push
BLACKWORDS += pthread_cleanup_pop
endif
ifeq ($(CONFIG_LIBC_EXECFUNCS),)
BLACKWORDS += execl
BLACKWORDS += execvp
BLACKWORDS += execlp
BLACKWORDS += execvpe
endif
ifeq ($(CONFIG_FS_SHMFS),)
BLACKWORDS += shm_open
BLACKWORDS += shm_unlink
endif
ifeq ($(LIBC_LOCALE_CATALOG),)
BLACKWORDS += catopen
BLACKWORDS += catgets
BLACKWORDS += catclose
endif
ifeq ($(CONFIG_MM_SHM),)
BLACKWORDS += shmget
BLACKWORDS += shmat
endif

MAINWORDS += "main("

LTP_ORIGS := $(shell find $(TESTDIR) -name *.c)
ifneq ($(LTP_ORIGS),)
$(eval $(call SPLITVARIABLE,ORIGS_SPILT,$(LTP_ORIGS),200))
$(foreach BATCH, $(ORIGS_SPILT_TOTAL), \
	$(foreach word, $(BLACKWORDS), $(eval BLACKLIST+=$(shell grep -lr $(word) $(ORIGS_SPILT_$(BATCH))))) \
)
endif

$(foreach src, $(BLACKSRCS), $(eval BLACKLIST+=$(filter %$(src),$(LTP_ORIGS))))

LTP_ORIGS := $(filter-out $(BLACKLIST), $(LTP_ORIGS))
ifneq ($(LTP_ORIGS),)
$(eval $(call SPLITVARIABLE,ORIGS_SPILT,$(LTP_ORIGS),200))
$(foreach BATCH, $(ORIGS_SPILT_TOTAL), \
	$(foreach word, $(MAINWORDS), $(eval LTP_MAINCSRCS+=$(shell grep -lr $(word) $(ORIGS_SPILT_$(BATCH))))) \
)
endif

LTP_CSRCS := $(filter-out $(LTP_MAINCSRCS), $(LTP_ORIGS))
ifneq ($(LTP_MAINCSRCS),)
$(eval $(call SPLITVARIABLE,MAINCSRC_SPILT,$(LTP_MAINCSRCS),50))
$(foreach BATCH, $(MAINCSRC_SPILT_TOTAL), \
	$(eval PROGNAME+=$(basename $(shell echo $(MAINCSRC_SPILT_$(BATCH)) | xargs -n 1 | awk -F "[/]" '{print "ltp_"$$(NF-2)"_"$$(NF-1)"_"$$(NF)}' | sed s/-/_/g))) \
)
endif

MAINSRC       = $(LTP_MAINCSRCS)
ifneq ($(CONFIG_NET_CAN),)
PROGNAME     += can_rcv_own_msgs
PROGNAME     += can_filter
MAINSRC      += $(TESTDIR)/../network/can/filter-tests/can_filter.c
MAINSRC      += $(TESTDIR)/../network/can/filter-tests/can_rcv_own_msgs.c
endif
PRIORITY      = SCHED_PRIORITY_DEFAULT
STACKSIZE     = $(CONFIG_TESTING_LTP_STACKSIZE)
MODULE        = $(CONFIG_TESTING_LTP)

CSRCS        := $(LTP_CSRCS)
CFLAGS       += -I$(CURDIR)
CFLAGS       += -I$(TESTDIR)/include
CFLAGS       += -I$(TESTDIR)/../../include
endif

# Relax warning checks to avoid expected compile errors:
CFLAGS += -Wno-strict-prototypes -Wno-return-type -Wno-format -Wno-uninitialized
CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-value
CFLAGS += -Wno-int-conversion -Wno-shadow

# Should be removed if possible in the future
CFLAGS += -Wno-incompatible-pointer-types -Wno-overflow -Wno-int-to-pointer-cast

# Specific compilation errors ignored in MacOS platform
ifneq ($(CONFIG_HOST_MACOS),)
CFLAGS += -Wno-integer-overflow -Wno-absolute-value
endif

ltp-$(LTPS_VERSION).zip:
	$(call DOWNLOAD,$(LTP_DOWNLOAD_URL),$(LTPS_VERSION).zip, ltp.zip)

$(LTP_UNPACK): ltp-$(LTPS_VERSION).zip
	$(Q) echo "dowload $(LTP_DOWNLOAD_URL)$(LTPS_VERSION).zip"
	$(Q) unzip -o ltp.zip
	$(Q) rm -f ltp.zip
	$(Q) mv ltp-$(LTPS_VERSION) $(LTP_UNPACK)
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0001-pthread_rwlock_unlock-follow-linux.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0002-Use-ifdef-instead-of-if-for-__linux__.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0003-LTP-Re-init-static-variable-to-avoid-errors-during-m.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0004-test-update-ltp-test-case.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0005-ltp-update-pthread_cond_timedwait-testcase.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0006-test-modified-ltp-rwlock-need-init.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0007-pthread_kill-add-usleep-aviod-semcount-overturn.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0008-test-ltp-fix-ltp_interfaces_sigaction_23_10-deadloop.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0009-ltp-modify-user-code-for-fdcheck-compatibility.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0010-ltp-fix-build-warning.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0011-ltp-fix-the-proc.h-header-file-duplicate-inclusion.patch
	$(Q) patch -d $(LTP_UNPACK) -p1 < 0012-ltp-fix-build-error.patch

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LTP_UNPACK)/.git),)
context:: $(LTP_UNPACK)

distclean::
	$(Q) rm -rf $(LTP_UNPACK)
endif

include $(APPDIR)/Application.mk
