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

# to construct the test for LTP POSIX interface testcases

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

ifeq ($(CONFIG_PTHREAD_SPINLOCKS),)
BLACKWORDS += pthread_spin_lock
endif
ifeq ($(CONFIG_SCHED_WAITPID),)
BLACKWORDS += waitpid
endif

ifeq ($(CONFIG_SCHED_WAITPID),)
BLACKWORDS += wait
else ifeq ($(CONFIG_SCHED_HAVE_PARENT),)
BLACKWORDS += wait
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

TESTDIR          = $(LTP_UNPACK)/testcases/open_posix_testsuite

ifneq ($(wildcard $(TESTDIR)),)

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
ifeq ($(CONFIG_SCHED_SPORADIC),)
BLACKSRCS += $(TESTDIR)/conformance/interfaces/sched_setscheduler/19-2.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/sched_setscheduler/19-3.c
BLACKSRCS += $(TESTDIR)/conformance/interfaces/sched_setscheduler/19-4.c
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
CFLAGS       += -I$(TESTDIR)/../../../include
CFLAGS       += -I$(TESTDIR)/../../include

endif # TESTDIR not empty

########################################################################################

TESTLINUX_DIR = $(LTP_UNPACK)/testcases/kernel

# there are two kinds of testcases that kernel api can run, first is using main() function
# to build as an executable directly, the second is by include "include/tst_test.h"
# header file, and include/tst_test.h will generate main() function for this case, we
# need to support both cases
MAINWORDS   += "tst_test"

BLACKWORDS  += connectors
BLACKWORDS  += netlink
BLACKWORDS  += cgroup
BLACKWORDS  += cpuctl

# the device-drivers related blackwords
BLACKWORDS  += scsi # nuttx do not support scsi device driver
BLACKWORDS  += RTC_
BLACKWORDS  += V4L2
BLACKWORDS  += v4l2
BLACKWORDS  += TUSB_MAJOR
BLACKWORDS  += tagp
BLACKWORDS  += tmod
BLACKWORDS  += LO_FLAGS_AUTOCLEAR
BLACKWORDS  += IFF_ONE_QUEUE
BLACKWORDS  += LO_FLAGS_READ_ONLY
BLACKWORDS  += loopinfo
BLACKWORDS  += sigjmp_buf
BLACKWORDS  += FIBMAP
BLACKWORDS  += block_dev_handle
BLACKWORDS  += drm
BLACKWORDS  += tst_path_has_mnt_flags
BLACKWORDS  += personality # nuttx do not support execution domains api
BLACKWORDS  += getcontext
BLACKWORDS  += fmtmsg
BLACKWORDS  += MS_BIND # nuttx mount do not support MS_BIND flags
BLACKWORDS  += sound
BLACKWORDS  += tracing

BLACKWORDS  += CLONE_NEWNS
BLACKWORDS  += CLONE_NEWPID
BLACKWORDS  += CLONE_NEWUSER
BLACKWORDS  += CLONE_NEWIPC
BLACKWORDS  += CLONE_NEWUTS
BLACKWORDS  += CLONE_NEWNET
BLACKWORDS  += CLONE_NEWIPC
BLACKWORDS  += NAMESPACE
BLACKWORDS  += cap_last_cap
BLACKWORDS  += crypto
BLACKWORDS  += NUMA

ifeq ($(CONFIG_LIBC_EXECFUNCS),)
BLACKWORDS  += execv
BLACKWORDS  += execve
endif

BLACKWORDS  += bitmask_alloc
BLACKWORDS  += input

BLACKWORDS += msgctl # nuttx do not support System V IPC api
BLACKWORDS += semget
BLACKWORDS += semctl
BLACKWORDS += shmctl

BLACKWORDS  += logging
BLACKWORDS  += mce-test
BLACKWORDS  += numa

BLACKWORDS  += hugetlb # nuttx do not support hugetlb
BLACKWORDS  += tomoyo # nuttx do not support tomoyo security module

BLACKWORDS  += posix_fadvise # nuttx do not support posix_fadvise syscall
BLACKWORDS  += sysctl # nuttx do not support sysctl syscall
BLACKWORDS  += getgroups # nuttx do not support getgroups syscall
BLACKWORDS  += setgroups # nuttx do not support setgroups syscall
BLACKWORDS  += getdents # nuttx do not support getdents syscall
BLACKWORDS  += rt_sigaction # nuttx do not support rt_sigaction syscall
BLACKWORDS  += socketcall # nuttx do not support socketcall syscall
BLACKWORDS  += readahead # nuttx do not support readahead syscall
BLACKWORDS  += capget # nuttx do not support capget syscall
BLACKWORDS  += BLKROSET # nuttx do not support BLKROSET flag in ioctl syscall
BLACKWORDS  += quotactl # nuttx do not support quotactl syscall
BLACKWORDS  += swapon # nuttx do not support swapon syscall
BLACKWORDS  += swapoff # nuttx do not support swapoff syscall
BLACKWORDS  += bpf # nuttx do not support bpf syscall
BLACKWORDS  += mincore # nuttx do not support mincore syscall
BLACKWORDS  += unshare # nuttx do not support unshare syscall
BLACKWORDS  += kcmp # nuttx do not support kcmp syscall
BLACKWORDS  += clone # nuttx do not support clone syscall
BLACKWORDS  += capset # nuttx do not support capset syscall
BLACKWORDS  += adjtimex # nuttx do not support adjtimex syscall
BLACKWORDS  += confstr # nuttx do not support confstr syscall
BLACKWORDS  += fallocate # nuttx do not support fallocate syscall, and only support posix_fallocate syscall
BLACKWORDS  += rt_tgsigqueueinfo # nuttx do not support rt_tgsigqueueinfo syscall
BLACKWORDS  += sync_file_range # nuttx do not support sync_file_range syscall
BLACKWORDS  += vhangup # nuttx do not support vhangup syscall
BLACKWORDS  += remap_file_pages # nuttx do not support remap_file_pages syscall
BLACKWORDS  += acct # nuttx do not support acct syscall
BLACKWORDS  += gethostid # nuttx do not support gethostid syscall
BLACKWORDS  += mallopt # nuttx do not support malloopt syscall
ifeq ($(CONFIG_TIMER_FD),)
BLACKWORDS  += timerfd_create
BLACKWORDS  += timerfd_gettime
endif

BLACKWORDS  += renameat2 # nuttx do not support renameat2 syscall

ifeq ($(CONFIG_PSEUDOTERM),)
BLACKWORDS  += unlockpt
BLACKWORDS  += grantpt
endif

BLACKWORDS  += eaccess # nuttx do not support eaccess api
BLACKWORDS  += mremap # nuttx do not support mremap api

ifeq ($(CONFIG_SMP),)
BLACKWORDS  += sched_getaffinity
endif

ifeq ($(CONFIG_ARCH_ADDRENV),)
BLACKWORDS  += sbrk
else ifeq ($(CONFIG_MM_PGALLOC),)
BLACKWORDS  += sbrk
else ifeq ($(CONFIG_ARCH_USE_MMU),)
BLACKWORDS  += sbrk
endif

BLACKWORDS  += RLIMIT_RSS
BLACKWORDS  += RLIMIT_NPROC
BLACKWORDS  += RLIMIT_LOCKS
BLACKWORDS  += RLIMIT_MSGQUEUE
BLACKWORDS  += RLIMIT_SIGPENDING
BLACKWORDS  += modify_ldt # nuttd do not support modify_ldt syscall interface
BLACKWORDS  += SIGPWR
BLACKWORDS  += _SC_PASS_MAX
BLACKWORDS  += _SC_XOPEN_XCU_VERSION
BLACKWORDS  += UL_GETFSIZE

BLACKWORDS  += setresuid
BLACKWORDS  += setresgid
BLACKWORDS  += getsid
BLACKWORDS  += getresuid
BLACKWORDS  += getresgid
ifeq ($(CONFIG_PSEUDOFS_SOFTLINKS),)
BLACKWORDS  += symlink
BLACKWORDS  += link
endif
ifeq ($(CONFIG_FS_LARGEFILE),)
BLACKWORDS  += off64_t
BLACKWORDS  += fpos64_t
BLACKWORDS  += F_GETLK64
BLACKWORDS  += F_SETLK64
BLACKWORDS  += F_SETLKW64
BLACKWORDS  += flock64
BLACKWORDS  += open64
BLACKWORDS  += openat64
BLACKWORDS  += creat64
BLACKWORDS  += fallocate64
BLACKWORDS  += posix_fadvise64
BLACKWORDS  += posix_fallocate64
BLACKWORDS  += lseek64
BLACKWORDS  += pread64
BLACKWORDS  += pwrite64
BLACKWORDS  += truncate64
BLACKWORDS  += ftruncate64
BLACKWORDS  += lockf64
BLACKWORDS  += getrlimit64
BLACKWORDS  += setrlimit64
BLACKWORDS  += prlimit64
BLACKWORDS  += rlimit64
BLACKWORDS  += rlim64_t
endif
ifeq ($(CONFIG_ARCH_SETJMP_H),)
BLACKWORDS  += setjmp
BLACKWORDS  += longjmp
endif

BLACKWORDS  += in6addr_loopback
BLACKWORDS  += BLKGETSIZE64
BLACKWORDS  += BLKRAGET
BLACKWORDS  += BLKRASET
BLACKWORDS  += RNDGETENTCNT
BLACKWORDS  += KEYCTL_JOIN_SESSION_KEYRING
BLACKWORDS  += keyctl # nuttx do not support keyctl syscall interface

BLACKWORDS  += ioctl_ns # nuttx do not support ioctl_ns (ioctl for linux namespace)
BLACKWORDS  += si_uid # nuttx "struct siginfo" do not have "si_uid" member
BLACKWORDS  += si_addr # nuttx "struct siginfo" do not have "si_addr" member
BLACKWORDS  += TFD_TIMER_ABSTIME
BLACKWORDS  += setdomainname
BLACKWORDS  += chroot
BLACKWORDS  += safe_chroot
BLACKWORDS  += SAFE_BIND
BLACKWORDS  += wait4
BLACKWORDS  += ksmd
BLACKWORDS  += PR_SET_PDEATHSIG
BLACKWORDS  += PR_GET_PDEATHSIG

BLACKWORDS  += ptrace # nuttx do not support ptrace
BLACKWORDS  += uevents # nuttx do not support uevents
BLACKWORDS  += futex # nuttx do not support futex for now

BLACKWORDS += tst_module_exists # nuttx do not support linux kernel module framework
BLACKWORDS += tst_module_load
BLACKWORDS += tst_module_unload
BLACKWORDS += module

BLACKWORDS += setpgid
BLACKWORDS += safe_setpgid
BLACKWORDS += safe_getpgid

BLACKWORDS += open_kconfig
BLACKWORDS += close_kconfig

BLACKWORDS += firmware # nuttx do not support linux firmware load framework

BLACKSRCS += $(TESTLINUX_DIR)/io/disktest/main.c
BLACKSRCS += $(TESTLINUX_DIR)/io/direct_io/dma_thread_diotest.c

# syscalls/nftw/test_func.c has provide the implementation of test_func*(),
# and the implementation are completely same as syscalls/nftw/test_func64.c
# provided implementation, we just need to keep one of them.
# and the following test64.c/lib64.c/tools64.c are deleted from source list
# with same reason as test_func64.c
BLACKSRCS += $(TESTLINUX_DIR)/syscalls/nftw/test_func64.c
BLACKSRCS += $(TESTLINUX_DIR)/syscalls/nftw/test64.c
BLACKSRCS += $(TESTLINUX_DIR)/syscalls/nftw/lib64.c
BLACKSRCS += $(TESTLINUX_DIR)/syscalls/nftw/tools64.c

LTP_LINUX_ORIGS := $(shell find $(TESTLINUX_DIR) -name *.c)

ifneq ($(LTP_LINUX_ORIGS),)
$(eval $(call SPLITVARIABLE,ORIGS_SPILT,$(LTP_LINUX_ORIGS),200))
$(foreach BATCH, $(ORIGS_SPILT_TOTAL), \
	$(foreach word, $(BLACKWORDS), $(eval LINUX_BLACKLIST+=$(shell grep -lr $(word) $(ORIGS_SPILT_$(BATCH))))) \
)
endif

$(foreach src, $(BLACKSRCS), $(eval LINUX_BLACKLIST+=$(filter %$(src),$(LTP_LINUX_ORIGS))))

LTP_LINUX_ORIGS := $(filter-out $(LINUX_BLACKLIST), $(LTP_LINUX_ORIGS))

ifneq ($(LTP_LINUX_ORIGS),)
$(eval $(call SPLITVARIABLE,ORIGS_SPILT,$(LTP_LINUX_ORIGS),190))
$(foreach BATCH, $(ORIGS_SPILT_TOTAL), \
	$(foreach word, $(MAINWORDS), $(eval LTP_LINUX_MAINCSRCS+=$(shell grep -lr $(word) $(ORIGS_SPILT_$(BATCH))))) \
)
endif

LTP_LINUX_CSRCS := $(filter-out $(LTP_LINUX_MAINCSRCS), $(LTP_LINUX_ORIGS))

ifneq ($(LTP_LINUX_MAINCSRCS),)
$(eval $(call SPLITVARIABLE,MAINCSRC_SPILT,$(LTP_LINUX_MAINCSRCS),50))
$(foreach BATCH, $(MAINCSRC_SPILT_TOTAL), \
	$(eval PROGNAME+=$(basename $(shell echo $(MAINCSRC_SPILT_$(BATCH)) | xargs -n 1 | awk -F "[/]" '{print "ltp_"$$(NF-2)"_"$$(NF-1)"_"$$(NF)}' | sed s/-/_/g))) \
)
endif

MAINSRC += $(LTP_LINUX_MAINCSRCS)

CSRCS   += ltp/lib/get_path.c
CSRCS   += ltp/lib/safe_file_ops.c
CSRCS   += ltp/lib/safe_macros.c
CSRCS   += ltp/lib/parse_opts.c
CSRCS   += ltp/lib/self_exec.c
CSRCS   += ltp/lib/tst_ansi_color.c
CSRCS   += ltp/lib/tst_assert.c
CSRCS   += ltp/lib/tst_device.c
CSRCS   += ltp/lib/tst_res.c
CSRCS   += ltp/lib/tst_sys_conf.c
CSRCS   += ltp/lib/tst_test.c
CSRCS   += ltp/lib/tst_tmpdir.c
CSRCS   += ltp/lib/tst_wallclock.c
CSRCS   += ltp/lib/tst_checkpoint.c
CSRCS   += ltp/lib/tst_clocks.c
CSRCS   += ltp/lib/tst_sig.c
CSRCS   += ltp/lib/tst_parse_opts.c
CSRCS   += ltp/lib/tst_pid.c
CSRCS   += ltp/lib/tst_cmd.c
CSRCS   += ltp/lib/tst_fill_file.c
CSRCS   += ltp/lib/tst_mkfs.c
CSRCS   += ltp/lib/tst_kvercmp.c
CSRCS   += ltp/lib/tst_kernel.c
CSRCS   += ltp/lib/tst_timer_test.c
CSRCS   += ltp/lib/tst_hugepage.c
CSRCS   += ltp/lib/tst_buffers.c
CSRCS   += ltp/lib/tst_resource.c
CSRCS   += ltp/lib/tst_fs_setup.c
CSRCS   += ltp/lib/tst_safe_macros.c
CSRCS   += ltp/lib/safe_net.c
CSRCS   += ltp/lib/safe_pthread.c
CSRCS   += ltp/lib/safe_stdio.c
CSRCS   += ltp/lib/tst_supported_fs_types.c
CSRCS   += ltp/lib/tst_timer.c
CSRCS   += ltp/lib/tst_get_bad_addr.c
CSRCS   += ltp/lib/tst_status.c
CSRCS   += ltp/lib/tst_sig_proc.c
CSRCS   += ltp/lib/tst_process_state.c
CSRCS   += ltp/lib/tst_virt.c
CSRCS   += ltp/lib/tst_kconfig.c
CSRCS   += ltp/lib/tst_path_has_mnt_flags.c
CSRCS   += ltp/lib/tst_capability.c
CSRCS   += ltp/lib/tst_fs_type.c
CSRCS   += ltp/lib/tst_cpu.c
CSRCS   += ltp/lib/tst_fill_fs.c
CSRCS   += ltp/lib/tst_fs_link_count.c
CSRCS   += ltp/lib/tst_dir_is_empty.c
CSRCS   += ltp/lib/tst_coredump.c
CSRCS   += ltp/lib/tst_fs_has_free.c

CSRCS   += ltp/testcases/kernel/fs/fs-bench/boxmuler.c
CSRCS   += ltp/testcases/kernel/fs/doio/bytes_by_prefix.c
CSRCS   += ltp/testcases/kernel/fs/doio/string_to_tokens.c
CSRCS   += ltp/testcases/kernel/device-drivers/base/user_base/tbase_ki.c
CSRCS   += ltp/testcases/kernel/syscalls/nftw/tools.c
CSRCS   += ltp/testcases/kernel/syscalls/nftw/lib.c
CSRCS   += ltp/testcases/kernel/sched/sched_stress/sched.c
CSRCS   += ltp/testcases/kernel/io/direct_io/diotest_routines.c
CSRCS   += ltp/testcases/kernel/controllers/libcontrollers/libcontrollers.c
CSRCS   += ltp/testcases/kernel/mem/lib/mem.c
CSRCS   += ltp/testcases/kernel/syscalls/memfd_create/memfd_create_common.c
CSRCS   += ltp/lib/tst_safe_sysv_ipc.c
CSRCS   += ltp/testcases/kernel/syscalls/ipc/lib/libipc.c

# the syscall wrapper implementation for NuttX
CSRCS   += src/nx_syscall.c
CSRCS   += src/dummy.c

CSRCS   += $(LTP_LINUX_CSRCS)

# since we have enabled the UNIT_TEST, the random_range.c and parse_opts.c
# self unittest will also be build as an executable, we name it as
# random_range_test and parse_opts_test
MAINSRC   += ltp/lib/random_range.c
PROGNAME  += random_range_test

MAINSRC   += ltp/lib/parse_opts.c
PROGNAME  += parse_opts_test

MAINSRC   += ltp/testcases/kernel/fs/doio/open_flags.c
PROGNAME  += doio_openflags_test

PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_TESTING_LTP_STACKSIZE)
MODULE    = $(CONFIG_TESTING_LTP)

CFLAGS    += -I$(CURDIR)
CFLAGS    += -I$(TESTLINUX_DIR)/include
CFLAGS    += -I$(TESTLINUX_DIR)/../../include
CFLAGS    += -I$(TESTLINUX_DIR)/syscalls/ptrace
CFLAGS    += -I$(TESTLINUX_DIR)/syscalls/utils
CFLAGS    += -I$(TESTLINUX_DIR)/../../include/old
CFLAGS    += -I$(TESTLINUX_DIR)/fs/doio/include
CFLAGS    += -I$(TESTLINUX_DIR)/controllers/libcontrollers
CFLAGS    += -I$(TESTLINUX_DIR)/mem/include
CFLAGS    += -I$(TESTLINUX_DIR)/syscalls/ipc/lib
CFLAGS    += -I$(TESTLINUX_DIR)/syscalls/ipc/libnewipc

CFLAGS    += -DHAVE_FCHOWNAT
CFLAGS    += -DMAXPATHLEN=256
CFLAGS    += -DUNIT_TEST
CFLAGS    += -DHAVE_MKDIRAT
CFLAGS    += -DHAVE_OPENAT
CFLAGS    += -DTEST_CLEANUP
CFLAGS    += -DBLKGETSIZE64=BLKGETSIZE
CFLAGS    += -D__linux__
CFLAGS    += -DLINUX

CFLAGS    += -DHAVE_ASPRINTF
CFLAGS    += -DHAVE_LINUX_FS_H
CFLAGS    += -DTST_USE_NEWER64_SYSCALL=0
CFLAGS    += -DINVALID_TEST_CASES=0
CFLAGS    += -DHAVE_STRUCT_RLIMIT64
CFLAGS    += -DHAVE_READLINKAT
CFLAGS    += -DHAVE_FSTATAT
ifeq ($(CONFIG_TIMER_FD),y)
CFLAGS    += -DHAVE_TIMERFD_CREATE
CFLAGS    += -DHAVE_SYS_TIMERFD_H
endif
CFLAGS    += -DHAVE_TIMERFD_GETTIME
CFLAGS    += -DHAVE_TIMERFD_SETTIME
CFLAGS    += -DHAVE_SYNCFS
CFLAGS    += -DHAVE_MKNODAT
CFLAGS    += -DHAVE_RENAMEAT2
CFLAGS    += -DHAVE_RENAMEAT

# add the macro definition to handle the naming conflication betweeb ltp
# kernel testcases and the vela external projects
CFLAGS    += -Dsafe_touch=ltp_safe_touch
CFLAGS    += -Dsafe_socket=ltp_safe_socket
CFLAGS    += -Dsafe_bind=ltp_safe_bind
CFLAGS    += -Dsafe_listen=ltp_safe_listen
CFLAGS    += -Dsafe_connect=ltp_safe_connect
CFLAGS    += -Dsafe_getsockname=ltp_safe_getsockname
CFLAGS    += -Dsafe_close=ltp_safe_close
CFLAGS    += -Dsafe_open=ltp_safe_open
CFLAGS    += -Dsafe_write=ltp_safe_write
CFLAGS    += -Derror=ltp_error

# The following warnings are all reported by the LTP kernel test cases themselves.
# To avoid affecting the CI build working procedure, we need to ignore these warnings.
CFLAGS += -Wno-undef -Wno-pointer-sign -Wno-parentheses -Wno-use-after-free
CFLAGS += -Wno-discarded-qualifiers -Wno-implicit-int -Wno-pointer-to-int-cast
CFLAGS += -Wno-switch-bool -Wno-restrict -Wno-stringop-overflow -Wno-array-bounds
CFLAGS += -Wno-shift-count-overflow -Wno-stringop-overread

#####################################################################################

ltp-$(LTPS_VERSION).zip:
	$(call DOWNLOAD,$(LTP_DOWNLOAD_URL),$(LTPS_VERSION).zip, ltp.zip)

$(LTP_UNPACK): ltp-$(LTPS_VERSION).zip
	$(Q) echo "download $(LTP_DOWNLOAD_URL)$(LTPS_VERSION).zip"
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
