/****************************************************************************
 * apps/testing/ltp/include/lapi/syscalls.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/* Here we stick all the ugly *fallback* logic for linux
 * system call numbers (those __NR_ thingies).
 *
 * Licensed under the GPLv2 or later, see the COPYING file.
 */

#ifndef __LAPI_SYSCALLS_H__
#define __LAPI_SYSCALLS_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "cleanup.c"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* the standard syscall() function interface in Linux platform */

int syscall(int syscall_nr, ...);

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The syscall numbers in the NuttX system are not fixed in advance in
 * the code, but are dynamically generated through the code.
 * All syscall numbers are defined in enums in the dynamically generated
 * code. And if current build not enable the protect build, then syscall
 * is also not available, so for flat build, we should convert the syscall
 * to direct function call
 */

#define ltp_syscall(NR, ...) ({ \
  int __ret; \
  if (NR == __LTP__NR_INVALID_SYSCALL) { \
    errno = ENOSYS; \
    __ret = -1; \
  } else { \
    __ret = syscall(NR, ##__VA_ARGS__); \
  } \
  if (__ret == -1 && errno == ENOSYS) { \
    tst_brkm(TCONF, CLEANUP, \
      "syscall(%d) " #NR " not supported on your arch", \
      NR); \
  } \
  __ret; \
})

#define tst_syscall(NR, ...) ({ \
  int tst_ret; \
  if (NR == __LTP__NR_INVALID_SYSCALL) { \
    errno = ENOSYS; \
    tst_ret = -1; \
  } else { \
    tst_ret = syscall(NR, ##__VA_ARGS__); \
  } \
  if (tst_ret == -1 && errno == ENOSYS) { \
    tst_brk(TCONF, "syscall(%d) " #NR " not supported", NR); \
  } \
  tst_ret; \
})

/* Actually, the system call numbers we define are just for identification.
 * The bottom layer of vela will not actually use these system call numbers.
 * Because in the ltp test case, we also initiate real system calls directly
 * through function calls.
 * So the system call number here can be defined arbitrarily, as long as it
 * is unique
 */

#define __LTP__NR_INVALID_SYSCALL -1

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum nx_syscall_num
{
  __NR_exit = 1,
  __NR_fork,
  __NR_read,
  __NR_write,
  __NR_open,
  __NR_close,
  __NR_creat,
  __NR_link,
  __NR_unlink,
  __NR_execve,
  __NR_chdir,
  __NR_mknod,
  __NR_chmod,
  __NR_lchown,
  __NR_lseek,
  __NR_getpid,
  __NR_mount,
  __NR_setuid,
  __NR_getuid,
  __NR_ptrace,
  __NR_pause,
  __NR_access,
  __NR_nice,
  __NR_sync,
  __NR_kill,
  __NR_rename,
  __NR_mkdir,
  __NR_rmdir,
  __NR_dup,
  __NR_pipe,
  __NR_times,
  __NR_brk,
  __NR_setgid,
  __NR_getgid,
  __NR_geteuid,
  __NR_getegid,
  __NR_acct,
  __NR_umount2,
  __NR_ioctl,
  __NR_fcntl,
  __NR_setpgid,
  __NR_umask,
  __NR_chroot,
  __NR_ustat,
  __NR_dup2,
  __NR_getppid,
  __NR_getpgrp,
  __NR_setsid,
  __NR_sigaction,
  __NR_setreuid,
  __NR_setregid,
  __NR_sigsuspend,
  __NR_sigpending,
  __NR_sethostname,
  __NR_setrlimit,
  __NR_getrusage,
  __NR_gettimeofday,
  __NR_settimeofday,
  __NR_getgroups,
  __NR_setgroups,
  __NR_symlink,
  __NR_readlink,
  __NR_uselib,
  __NR_swapon,
  __NR_reboot,
  __NR_munmap,
  __NR_truncate,
  __NR_ftruncate,
  __NR_fchmod,
  __NR_fchown,
  __NR_getpriority,
  __NR_setpriority,
  __NR_statfs,
  __NR_fstatfs,
  __NR_syslog,
  __NR_setitimer,
  __NR_getitimer,
  __NR_stat,
  __NR_lstat,
  __NR_fstat,
  __NR_vhangup,
  __NR_wait4,
  __NR_swapoff,
  __NR_sysinfo,
  __NR_fsync,
  __NR_sigreturn,
  __NR_clone,
  __NR_setdomainname,
  __NR_uname,
  __NR_adjtimex,
  __NR_mprotect,
  __NR_sigprocmask,
  __NR_init_module,
  __NR_delete_module,
  __NR_quotactl,
  __NR_getpgid,
  __NR_fchdir,
  __NR_bdflush,
  __NR_sysfs,
  __NR_personality,
  __NR_setfsuid,
  __NR_setfsgid,
  __NR__llseek,
  __NR_getdents,
  __NR__newselect,
  __NR_flock,
  __NR_msync,
  __NR_readv,
  __NR_writev,
  __NR_getsid,
  __NR_fdatasync,
  __NR__sysctl,
  __NR_mlock,
  __NR_munlock,
  __NR_mlockall,
  __NR_munlockall,
  __NR_sched_setparam,
  __NR_sched_getparam,
  __NR_sched_setscheduler,
  __NR_sched_getscheduler,
  __NR_sched_yield,
  __NR_sched_get_priority_max,
  __NR_sched_get_priority_min,
  __NR_sched_rr_get_interval,
  __NR_nanosleep,
  __NR_mremap,
  __NR_setresuid,
  __NR_getresuid,
  __NR_poll,
  __NR_nfsservctl,
  __NR_setresgid,
  __NR_getresgid,
  __NR_prctl,
  __NR_rt_sigreturn,
  __NR_rt_sigaction,
  __NR_rt_sigprocmask,
  __NR_rt_sigpending,
  __NR_rt_sigtimedwait,
  __NR_rt_sigqueueinfo,
  __NR_rt_sigsuspend,
  __NR_pread64,
  __NR_pwrite64,
  __NR_chown,
  __NR_getcwd,
  __NR_capget,
  __NR_capset,
  __NR_sigaltstack,
  __NR_sendfile,
  __NR_vfork,
  __NR_getrlimit,
  __NR_ugetrlimit,
  __NR_mmap2,
  __NR_truncate64,
  __NR_ftruncate64,
  __NR_stat64,
  __NR_lstat64,
  __NR_fstat64,
  __NR_lchown32,
  __NR_getuid32,
  __NR_getgid32,
  __NR_geteuid32,
  __NR_getegid32,
  __NR_setreuid32,
  __NR_setregid32,
  __NR_getgroups32,
  __NR_setgroups32,
  __NR_fchown32,
  __NR_setresuid32,
  __NR_getresuid32,
  __NR_setresgid32,
  __NR_getresgid32,
  __NR_chown32,
  __NR_setuid32,
  __NR_setgid32,
  __NR_setfsuid32,
  __NR_setfsgid32,
  __NR_getdents64,
  __NR_pivot_root,
  __NR_mincore,
  __NR_madvise,
  __NR_fcntl64,
  __NR_gettid,
  __NR_readahead,
  __NR_setxattr,
  __NR_lsetxattr,
  __NR_fsetxattr,
  __NR_getxattr,
  __NR_lgetxattr,
  __NR_fgetxattr,
  __NR_listxattr,
  __NR_llistxattr,
  __NR_flistxattr,
  __NR_removexattr,
  __NR_lremovexattr,
  __NR_fremovexattr,
  __NR_tkill,
  __NR_sendfile64,
  __NR_futex,
  __NR_sched_setaffinity,
  __NR_sched_getaffinity,
  __NR_io_setup,
  __NR_io_destroy,
  __NR_io_getevents,
  __NR_io_submit,
  __NR_io_cancel,
  __NR_exit_group,
  __NR_lookup_dcookie,
  __NR_epoll_create,
  __NR_epoll_ctl,
  __NR_epoll_wait,
  __NR_remap_file_pages,
  __NR_set_tid_address,
  __NR_timer_create,
  __NR_timer_settime,
  __NR_timer_gettime,
  __NR_timer_getoverrun,
  __NR_timer_delete,
  __NR_clock_settime,
  __NR_clock_gettime,
  __NR_clock_getres,
  __NR_clock_nanosleep,
  __NR_statfs64,
  __NR_fstatfs64,
  __NR_tgkill,
  __NR_utimes,
  __NR_arm_fadvise64_64,
  __NR_pciconfig_iobase,
  __NR_pciconfig_read,
  __NR_pciconfig_write,
  __NR_mq_open,
  __NR_mq_unlink,
  __NR_mq_timedsend,
  __NR_mq_timedreceive,
  __NR_mq_notify,
  __NR_mq_getsetattr,
  __NR_waitid,
  __NR_socket,
  __NR_bind,
  __NR_connect,
  __NR_listen,
  __NR_accept,
  __NR_getsockname,
  __NR_getpeername,
  __NR_socketpair,
  __NR_send,
  __NR_sendto,
  __NR_recv,
  __NR_recvfrom,
  __NR_shutdown,
  __NR_setsockopt,
  __NR_getsockopt,
  __NR_sendmsg,
  __NR_recvmsg,
  __NR_semop,
  __NR_semget,
  __NR_semctl,
  __NR_msgsnd,
  __NR_msgrcv,
  __NR_msgget,
  __NR_msgctl,
  __NR_shmat,
  __NR_shmdt,
  __NR_shmget,
  __NR_shmctl,
  __NR_add_key,
  __NR_request_key,
  __NR_keyctl,
  __NR_semtimedop,
  __NR_vserver,
  __NR_ioprio_set,
  __NR_ioprio_get,
  __NR_inotify_init,
  __NR_inotify_add_watch,
  __NR_inotify_rm_watch,
  __NR_mbind,
  __NR_get_mempolicy,
  __NR_set_mempolicy,
  __NR_openat,
  __NR_mkdirat,
  __NR_mknodat,
  __NR_fchownat,
  __NR_futimesat,
  __NR_fstatat,
  __NR_fstatat64,
  __NR_unlinkat,
  __NR_renameat,
  __NR_linkat,
  __NR_symlinkat,
  __NR_readlinkat,
  __NR_fchmodat,
  __NR_faccessat,
  __NR_pselect6,
  __NR_ppoll,
  __NR_unshare,
  __NR_set_robust_list,
  __NR_get_robust_list,
  __NR_splice,
  __NR_arm_sync_file_range,
  __NR_sync_file_range2,
  __NR_tee,
  __NR_vmsplice,
  __NR_move_pages,
  __NR_getcpu,
  __NR_epoll_pwait,
  __NR_kexec_load,
  __NR_utimensat,
  __NR_signalfd,
  __NR_timerfd_create,
  __NR_eventfd,
  __NR_fallocate,
  __NR_timerfd_settime,
  __NR_timerfd_gettime,
  __NR_signalfd4,
  __NR_eventfd2,
  __NR_epoll_create1,
  __NR_dup3,
  __NR_pipe2,
  __NR_inotify_init1,
  __NR_preadv,
  __NR_pwritev,
  __NR_rt_tgsigqueueinfo,
  __NR_perf_event_open,
  __NR_recvmmsg,
  __NR_accept4,
  __NR_fanotify_init,
  __NR_fanotify_mark,
  __NR_prlimit64,
  __NR_name_to_handle_at,
  __NR_open_by_handle_at,
  __NR_clock_adjtime,
  __NR_syncfs,
  __NR_sendmmsg,
  __NR_setns,
  __NR_process_vm_readv,
  __NR_process_vm_writev,
  __NR_kcmp,
  __NR_finit_module,
  __NR_sched_setattr,
  __NR_sched_getattr,
  __NR_renameat2,
  __NR_seccomp,
  __NR_getrandom,
  __NR_memfd_create,
  __NR_bpf,
  __NR_execveat,
  __NR_userfaultfd,
  __NR_membarrier,
  __NR_mlock2,
  __NR_copy_file_range,
  __NR_preadv2,
  __NR_pwritev2,
  __NR_pkey_mprotect,
  __NR_pkey_alloc,
  __NR_pkey_free,
  __NR_statx,
  __NR_rseq,
  __NR_io_pgetevents,
  __NR_migrate_pages,
  __NR_kexec_file_load,
  __NR_clock_gettime64,
  __NR_clock_settime64,
  __NR_clock_adjtime64,
  __NR_clock_getres_time64,
  __NR_clock_nanosleep_time64,
  __NR_timer_gettime64,
  __NR_timer_settime64,
  __NR_timerfd_gettime64,
  __NR_timerfd_settime64,
  __NR_utimensat_time64,
  __NR_pselect6_time64,
  __NR_ppoll_time64,
  __NR_io_pgetevents_time64,
  __NR_recvmmsg_time64,
  __NR_mq_timedsend_time64,
  __NR_mq_timedreceive_time64,
  __NR_semtimedop_time64,
  __NR_rt_sigtimedwait_time64,
  __NR_futex_time64,
  __NR_sched_rr_get_interval_time64,
  __NR_pidfd_send_signal,
  __NR_io_uring_setup,
  __NR_io_uring_enter,
  __NR_io_uring_register,
  __NR_open_tree,
  __NR_move_mount,
  __NR_fsopen,
  __NR_fsconfig,
  __NR_fsmount,
  __NR_fspick,
  __NR_pidfd_open,
  __NR_clone3,
  __NR_openat2,
  __NR_pidfd_getfd,
  __NR_readdir,
  __NR_select,
  __NR_stime,
  __NR__assert,
  __NR_boardctl,
  __NR_clearenv,
  __NR_clock,
  __NR_exec,
  __NR_futimens,
  __NR_gethostname,
  __NR_lchmod,
  __NR_lutimens,
  __NR_mmap,
  __NR_mq_close,
  __NR_mq_getattr,
  __NR_mq_receive,
  __NR_mq_send,
  __NR_mq_setattr,
  __NR_posix_spawn,
  __NR_pread,
  __NR_pselect,
  __NR_putenv,
  __NR_pwrite,
  __NR_sched_backtrace,
  __NR_sched_getcpu,
  __NR_sched_lock,
  __NR_sched_lockcount,
  __NR_sched_unlock,
  __NR_setegid,
  __NR_setenv,
  __NR_seteuid,
  __NR_signal,
  __NR_sigqueue,
  __NR_sigtimedwait,
  __NR_sigwaitinfo,
  __NR_shm_open,
  __NR_shm_unlink,
  __NR_task_create,
  __NR_task_delete,
  __NR_task_restart,
  __NR_task_spawn,
  __NR_time,
  __NR_unsetenv,
  __NR_up_fork,
  __NR_utimens,
  __NR_wait,
  __NR_waitpid,
  __NR_ext_alarm,
  __NR_aio_cancel,
  __NR_aio_fsync,
  __NR_aio_read,
  __NR_aio_write,
  __NR_arc4random_buf,
  __NR_insmod,
  __NR_modhandle,
  __NR_rmmod
};

#define SYS_getpid __NR_getpid
#define SYS_getuid __NR_getuid
#define SYS_getgid __NR_getgid
#define SYS_futex  __NR_futex

#endif /* __LAPI_SYSCALLS_H__ */
