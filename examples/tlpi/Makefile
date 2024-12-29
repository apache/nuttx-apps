############################################################################
# apps/examples/tlpi/Makefile
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

PRIORITY          = $(CONFIG_EXAMPLES_TLPI_PRIORITY)
STACKSIZE         = $(CONFIG_EXAMPLES_TLPI_STACKSIZE)
MODULE            = $(CONFIG_EXAMPLES_TLPI)
TLPI_DOWNLOAD_URL = https://github.com/bradfa/tlpi-dist/archive/refs/heads
TLPI_UNPACK       = tlpi
TLPIDIR           = $(APPDIR)/examples/tlpi/tlpi

#CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/cap
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/daemons
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/filelock
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/files
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/lib
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/namespaces
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/pipes
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/procexec
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/procres
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/pty
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/seccomp
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/signals
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/sockets
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/svmsg
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/svsem
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/time
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/timers
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/tty
CFLAGS += ${INCDIR_PREFIX}$(TLPIDIR)/users_groups
CFLAGS += -Wno-strict-prototypes -Wno-unused-variable -Wno-unused-function \
          -Wno-unused-but-set-variable -Wno-uninitialized

#CSRCS += $(TLPIDIR)/cap/cap_functions.c \
#        $(TLPIDIR)/daemons/become_daemon.c
CSRCS += $(TLPIDIR)/filelock/create_pid_file.c \
         $(TLPIDIR)/filelock/region_locking.c \
         $(TLPIDIR)/files/file_perms.c \
         $(TLPIDIR)/lib/alt_functions.c \
         $(TLPIDIR)/lib/error_functions.c \
         $(TLPIDIR)/lib/get_num.c
#        $(TLPIDIR)/namespaces/userns_functions.c
CSRCS += $(TLPIDIR)/procexec/print_wait_status.c \
         $(TLPIDIR)/procres/print_rlimit.c
#        $(TLPIDIR)/procres/print_rusage.c \
#        $(TLPIDIR)/pty/pty_fork.c
CSRCS += $(TLPIDIR)/pty/pty_master_open.c
#        $(TLPIDIR)/pty/pty_master_open_bsd.c \
#        $(TLPIDIR)/seccomp/seccomp_functions.c
CSRCS += $(TLPIDIR)/signals/signal.c \
         $(TLPIDIR)/signals/signal_functions.c \
         $(TLPIDIR)/sockets/inet_sockets.c \
         $(TLPIDIR)/sockets/rdwrn.c \
         $(TLPIDIR)/sockets/read_line.c \
         $(TLPIDIR)/sockets/read_line_buf.c \
         $(TLPIDIR)/sockets/scm_functions.c \
         $(TLPIDIR)/sockets/sendfile.c \
         $(TLPIDIR)/sockets/unix_sockets.c
#        $(TLPIDIR)/svsem/binary_sems.c \
#        $(TLPIDIR)/svsem/event_flags.c \
#        $(TLPIDIR)/threads/strerror.c \
#        $(TLPIDIR)/threads/strerror_tls.c \
#        $(TLPIDIR)/threads/strerror_tsd.c
CSRCS += $(TLPIDIR)/time/curr_time.c \
         $(TLPIDIR)/timers/itimerspec_from_str.c \
         $(TLPIDIR)/tty/tty_functions.c \
         $(TLPIDIR)/users_groups/ugid_functions.c

# acl
#MAINSRC += $(TLPIDIR)/acl/acl_update.c \
#           $(TLPIDIR)/acl/acl_view.c
# altio
MAINSRC += $(TLPIDIR)/altio/demo_sigio.c \
           $(TLPIDIR)/altio/epoll_flags_fork.c \
           $(TLPIDIR)/altio/epoll_input.c \
           $(TLPIDIR)/altio/multithread_epoll_wait.c \
           $(TLPIDIR)/altio/poll_pipes.c \
           $(TLPIDIR)/altio/select_mq.c \
           $(TLPIDIR)/altio/self_pipe.c \
           $(TLPIDIR)/altio/t_select.c
# cap
#MAINSRC += $(TLPIDIR)/cap/cap_launcher.c \
#          $(TLPIDIR)/cap/cap_text.c \
#          $(TLPIDIR)/cap/check_password_caps.c \
#          $(TLPIDIR)/cap/demo_file_caps.c \
#          $(TLPIDIR)/cap/show_secbits.c \
#          $(TLPIDIR)/cap/t_cap_get_file.c \
#          $(TLPIDIR)/cap/t_cap_get_pid.c \
#          $(TLPIDIR)/cap/t_cap_set_file.c \
#          $(TLPIDIR)/cap/view_cap_xattr.c
# cgroups
#MAINSRC += $(TLPIDIR)/cgroups/alloc_mem.c
MAINSRC += $(TLPIDIR)/cgroups/fork_bomb.c
#           $(TLPIDIR)/cgroups/fork_bomb_simple.c

# daemons
#MAINSRC += $(TLPIDIR)/daemons/daemon_SIGHUP.c
MAINSRC += $(TLPIDIR)/daemons/t_syslog.c
#           $(TLPIDIR)/daemons/test_become_daemon.c
# dirs_links
MAINSRC += $(TLPIDIR)/dirs_links/file_type_stats.c \
           $(TLPIDIR)/dirs_links/list_files.c \
           $(TLPIDIR)/dirs_links/list_files_readdir_r.c \
           $(TLPIDIR)/dirs_links/nftw_dir_tree.c \
           $(TLPIDIR)/dirs_links/t_unlink.c \
           $(TLPIDIR)/dirs_links/t_dirbasename.c
ifneq ($(CONFIG_PSEUDOFS_SOFTLINKS),)
MAINSRC += $(TLPIDIR)/dirs_links/bad_symlink.c \
           $(TLPIDIR)/dirs_links/view_symlink.c
endif
# filebuff
MAINSRC += $(TLPIDIR)/filebuff/direct_read.c \
           $(TLPIDIR)/filebuff/mix23_linebuff.c \
           $(TLPIDIR)/filebuff/mix23io.c \
           $(TLPIDIR)/filebuff/write_bytes.c
# fileio
MAINSRC += $(TLPIDIR)/fileio/atomic_append.c \
           $(TLPIDIR)/fileio/bad_exclusive_open.c \
           $(TLPIDIR)/fileio/copy.c \
           $(TLPIDIR)/fileio/large_file.c \
           $(TLPIDIR)/fileio/multi_descriptors.c \
           $(TLPIDIR)/fileio/seek_io.c \
           $(TLPIDIR)/fileio/t_readv.c \
           $(TLPIDIR)/fileio/t_truncate.c
# filelock
MAINSRC += $(TLPIDIR)/filelock/i_fcntl_locking.c
#          $(TLPIDIR)/filelock/t_flock.c
# files
#          $(TLPIDIR)/files/chiflag.c
MAINSRC += $(TLPIDIR)/files/t_chown.c \
           $(TLPIDIR)/files/t_stat.c \
           $(TLPIDIR)/files/t_umask.c \
           $(TLPIDIR)/files/t_utime.c \
           $(TLPIDIR)/files/t_utimes.c
# filesys
#MAINSRC += $(TLPIDIR)/filesys/t_mount.c \
#          $(TLPIDIR)/filesys/t_statfs.c
MAINSRC += $(TLPIDIR)/filesys/t_statvfs.c \
           $(TLPIDIR)/filesys/t_umount.c
# getopt
MAINSRC += $(TLPIDIR)/getopt/t_getopt.c
ifneq ($(CONFIG_FS_NOTIFY),)
# inotify
MAINSRC += $(TLPIDIR)/inotify/demo_inotify.c
#          $(TLPIDIR)/inotify/dnotify.c \
#          $(TLPIDIR)/inotify/inotify_dtree.c
endif
MAINSRC += $(TLPIDIR)/inotify/rand_dtree.c
# loginacct
#MAINSRC += $(TLPIDIR)/loginacct/dump_utmpx.c \
#          $(TLPIDIR)/loginacct/utmpx_login.c \
#          $(TLPIDIR)/loginacct/view_lastlog.c
# memalloc
ifneq ($(CONFIG_BUILD_KERNEL),)
MAINSRC += $(TLPIDIR)/memalloc/free_and_sbrk.c
endif
# mmap
MAINSRC += $(TLPIDIR)/mmap/anon_mmap.c \
           $(TLPIDIR)/mmap/mmcat.c \
           $(TLPIDIR)/mmap/mmcopy.c \
           $(TLPIDIR)/mmap/t_mmap.c
#          $(TLPIDIR)/mmap/t_remap_file_pages.c
# namespaces
#MAINSRC += $(TLPIDIR)/namespaces/cred_launcher.c \
#          $(TLPIDIR)/namespaces/demo_userns.c \
#          $(TLPIDIR)/namespaces/demo_uts_namespaces.c
MAINSRC += $(TLPIDIR)/namespaces/hostname.c
#          $(TLPIDIR)/namespaces/make_orphans.c \
#          $(TLPIDIR)/namespaces/multi_pidns.c \
#          $(TLPIDIR)/namespaces/ns_capable.c \
#          $(TLPIDIR)/namespaces/ns_child_exec.c \
#          $(TLPIDIR)/namespaces/ns_exec.c \
#          $(TLPIDIR)/namespaces/ns_run.c \
#          $(TLPIDIR)/namespaces/orphan.c \
#          $(TLPIDIR)/namespaces/pidns_init_sleep.c \
#          $(TLPIDIR)/namespaces/show_creds.c \
#          $(TLPIDIR)/namespaces/simple_init.c \
#          $(TLPIDIR)/namespaces/t_setns_userns.c \
#          $(TLPIDIR)/namespaces/unshare.c \
#          $(TLPIDIR)/namespaces/userns_child_exec.c \
#          $(TLPIDIR)/namespaces/userns_setns_test.c
# pgsjc
#MAINSRC += $(TLPIDIR)/pgsjc/catch_SIGHUP.c \
#          $(TLPIDIR)/pgsjc/disc_SIGHUP.c
MAINSRC += $(TLPIDIR)/pgsjc/handling_SIGTSTP.c
#          $(TLPIDIR)/pgsjc/job_mon.c \
#          $(TLPIDIR)/pgsjc/orphaned_pgrp_SIGHUP.c \
#          $(TLPIDIR)/pgsjc/t_setsid.c
# pipes
MAINSRC += $(TLPIDIR)/pipes/change_case.c \
           $(TLPIDIR)/pipes/fifo_seqnum_client.c \
           $(TLPIDIR)/pipes/fifo_seqnum_server.c
#          $(TLPIDIR)/pipes/popen_glob.c
ifneq ($(CONFIG_LIBC_EXECFUNCS),)
MAINSRC += $(TLPIDIR)/pipes/pipe_ls_wc.c
endif
MAINSRC += $(TLPIDIR)/pipes/pipe_sync.c \
           $(TLPIDIR)/pipes/simple_pipe.c
# pmsg
MAINSRC += $(TLPIDIR)/pmsg/mq_notify_sig.c \
           $(TLPIDIR)/pmsg/mq_notify_siginfo.c
#          $(TLPIDIR)/pmsg/mq_notify_sigwaitinfo.c \
#          $(TLPIDIR)/pmsg/mq_notify_thread.c
MAINSRC += $(TLPIDIR)/pmsg/mq_notify_via_signal.c
#          $(TLPIDIR)/pmsg/mq_notify_via_thread.c
MAINSRC += $(TLPIDIR)/pmsg/pmsg_create.c
#          $(TLPIDIR)/pmsg/pmsg_getattr.c
MAINSRC += $(TLPIDIR)/pmsg/pmsg_receive.c \
           $(TLPIDIR)/pmsg/pmsg_send.c \
           $(TLPIDIR)/pmsg/pmsg_unlink.c
# proc
MAINSRC += $(TLPIDIR)/proc/bad_longjmp.c \
           $(TLPIDIR)/proc/display_env.c \
           $(TLPIDIR)/proc/longjmp.c \
           $(TLPIDIR)/proc/mem_segments.c \
           $(TLPIDIR)/proc/modify_env.c \
           $(TLPIDIR)/proc/necho.c \
           $(TLPIDIR)/proc/setenv.c \
           $(TLPIDIR)/proc/setjmp_vars.c \
           $(TLPIDIR)/proc/t_getenv.c
# proccred
#MAINSRC += $(TLPIDIR)/proccred/idshow.c
# procexec
#MAINSRC += $(TLPIDIR)/procexec/acct_on.c \
#          $(TLPIDIR)/procexec/acct_view.c \
#          $(TLPIDIR)/procexec/acct_v3_view.c \
#          $(TLPIDIR)/procexec/demo_clone.c
MAINSRC += $(TLPIDIR)/procexec/child_status.c \
           $(TLPIDIR)/procexec/envargs.c \
           $(TLPIDIR)/procexec/exit_handlers.c
ifneq ($(CONFIG_BUILD_KERNEL),)
MAINSRC += $(TLPIDIR)/procexec/footprint.c
endif
MAINSRC += $(TLPIDIR)/procexec/fork_file_sharing.c \
           $(TLPIDIR)/procexec/fork_stdio_buf.c \
           $(TLPIDIR)/procexec/fork_sig_sync.c \
           $(TLPIDIR)/procexec/fork_whos_on_first.c \
           $(TLPIDIR)/procexec/make_zombie.c \
           $(TLPIDIR)/procexec/multi_SIGCHLD.c \
           $(TLPIDIR)/procexec/multi_wait.c \
           $(TLPIDIR)/procexec/orphan.c
#          $(TLPIDIR)/procexec/pdeath_signal.c
#          $(TLPIDIR)/procexec/t_clone.c
ifneq ($(CONFIG_LIBC_EXECFUNCS),)
MAINSRC += $(TLPIDIR)/procexec/closeonexec.c \
           $(TLPIDIR)/procexec/t_execl.c \
           $(TLPIDIR)/procexec/t_execle.c \
           $(TLPIDIR)/procexec/t_execlp.c \
           $(TLPIDIR)/procexec/t_execve.c
endif
MAINSRC += $(TLPIDIR)/procexec/t_fork.c \
           $(TLPIDIR)/procexec/t_system.c \
           $(TLPIDIR)/procexec/t_vfork.c \
           $(TLPIDIR)/procexec/vfork_fd_test.c
# procpri
MAINSRC += $(TLPIDIR)/procpri/demo_sched_fifo.c \
           $(TLPIDIR)/procpri/sched_set.c \
           $(TLPIDIR)/procpri/sched_view.c \
           $(TLPIDIR)/procpri/t_sched_getaffinity.c \
           $(TLPIDIR)/procpri/t_sched_setaffinity.c \
           $(TLPIDIR)/procpri/t_setpriority.c
# procres
#MAINSRC += $(TLPIDIR)/procres/rlimit_nproc.c \
#          $(TLPIDIR)/procres/rusage.c
MAINSRC += $(TLPIDIR)/procres/rusage_wait.c
# progconc
MAINSRC += $(TLPIDIR)/progconc/syscall_speed.c
ifneq ($(CONFIG_FS_NAMED_SEMAPHORES),)
# psem
MAINSRC += $(TLPIDIR)/psem/psem_create.c \
           $(TLPIDIR)/psem/psem_getvalue.c \
           $(TLPIDIR)/psem/psem_timedwait.c \
           $(TLPIDIR)/psem/psem_trywait.c \
           $(TLPIDIR)/psem/psem_post.c \
           $(TLPIDIR)/psem/psem_unlink.c \
           $(TLPIDIR)/psem/psem_wait.c
endif
MAINSRC += $(TLPIDIR)/psem/thread_incr_psem.c

# pshm
MAINSRC += $(TLPIDIR)/pshm/pshm_create.c \
           $(TLPIDIR)/pshm/pshm_read.c \
           $(TLPIDIR)/pshm/pshm_unlink.c \
           $(TLPIDIR)/pshm/pshm_write.c
# pty
#MAINSRC += $(TLPIDIR)/pty/script.c \
#          $(TLPIDIR)/pty/unbuffer.c
# shlibs
#MAINSRC += $(TLPIDIR)/seccomp/dump_seccomp_filter.c \
#          $(TLPIDIR)/seccomp/libseccomp_demo.c \
#          $(TLPIDIR)/seccomp/seccomp_arg64.c \
#          $(TLPIDIR)/seccomp/seccomp_bench.c \
#          $(TLPIDIR)/seccomp/seccomp_control_open.c \
#          $(TLPIDIR)/seccomp/seccomp_deny_open.c \
#          $(TLPIDIR)/seccomp/seccomp_deny_syscall.c \
#          $(TLPIDIR)/seccomp/seccomp_launch.c \
#          $(TLPIDIR)/seccomp/seccomp_logging.c \
#          $(TLPIDIR)/seccomp/seccomp_multiarch.c \
#          $(TLPIDIR)/seccomp/seccomp_perf.c \
#          $(TLPIDIR)/seccomp/seccomp_trap_sigsys.c \
#          $(TLPIDIR)/seccomp/seccomp_unotify_mkdir.c \
#          $(TLPIDIR)/seccomp/seccomp_unotify_openat.c
ifneq ($(CONFIG_LIBC_DLFCN),)
# shlibs
MAINSRC += $(TLPIDIR)/shlibs/dynload.c
endif
# signals
#MAINSRC += $(TLPIDIR)/signals/catch_rtsigs.c
MAINSRC += $(TLPIDIR)/signals/demo_SIGFPE.c \
           $(TLPIDIR)/signals/ignore_pending_sig.c \
           $(TLPIDIR)/signals/intquit.c
#          $(TLPIDIR)/signals/nonreentrant.c
MAINSRC += $(TLPIDIR)/signals/ouch.c  \
           $(TLPIDIR)/signals/sig_receiver.c \
           $(TLPIDIR)/signals/sig_sender.c \
           $(TLPIDIR)/signals/sig_speed_sigsuspend.c \
           $(TLPIDIR)/signals/sigmask_longjmp.c \
           $(TLPIDIR)/signals/signalfd_sigval.c \
           $(TLPIDIR)/signals/t_kill.c
#          $(TLPIDIR)/signals/t_sigaltstack.c
MAINSRC += $(TLPIDIR)/signals/t_sigqueue.c \
           $(TLPIDIR)/signals/t_sigsuspend.c
#          $(TLPIDIR)/signals/t_sigwaitinfo.c
# sockets
MAINSRC += $(TLPIDIR)/sockets/id_echo_cl.c
#          $(TLPIDIR)/sockets/id_echo_sv.c
MAINSRC += $(TLPIDIR)/sockets/is_echo_cl.c \
           $(TLPIDIR)/sockets/is_echo_inetd_sv.c
#          $(TLPIDIR)/sockets/is_echo_sv.c \
#          $(TLPIDIR)/sockets/is_echo_v2_sv.c
MAINSRC += $(TLPIDIR)/sockets/is_seqnum_cl.c \
           $(TLPIDIR)/sockets/is_seqnum_sv.c \
           $(TLPIDIR)/sockets/is_seqnum_v2_cl.c \
           $(TLPIDIR)/sockets/is_seqnum_v2_sv.c \
           $(TLPIDIR)/sockets/i6d_ucase_cl.c \
           $(TLPIDIR)/sockets/i6d_ucase_sv.c \
           $(TLPIDIR)/sockets/list_host_addresses.c
#          $(TLPIDIR)/sockets/scm_cred_recv.c
MAINSRC += $(TLPIDIR)/sockets/scm_cred_send.c
#          $(TLPIDIR)/sockets/scm_multi_recv.c
MAINSRC += $(TLPIDIR)/sockets/scm_multi_send.c \
           $(TLPIDIR)/sockets/scm_rights_recv.c \
           $(TLPIDIR)/sockets/scm_rights_send.c \
           $(TLPIDIR)/sockets/socknames.c
#          $(TLPIDIR)/sockets/t_gethostbyname.c
MAINSRC += $(TLPIDIR)/sockets/t_getservbyname.c \
           $(TLPIDIR)/sockets/ud_ucase_cl.c \
           $(TLPIDIR)/sockets/ud_ucase_sv.c \
           $(TLPIDIR)/sockets/us_abstract_bind.c \
           $(TLPIDIR)/sockets/us_xfr_cl.c \
           $(TLPIDIR)/sockets/us_xfr_sv.c \
           $(TLPIDIR)/sockets/us_xfr_v2_cl.c \
           $(TLPIDIR)/sockets/us_xfr_v2_sv.c
# sysinfo
MAINSRC += $(TLPIDIR)/sysinfo/procfs_pidmax.c \
           $(TLPIDIR)/sysinfo/procfs_user_exe.c \
           $(TLPIDIR)/sysinfo/t_uname.c
# svipc
MAINSRC += $(TLPIDIR)/svipc/svmsg_demo_server.c \
           $(TLPIDIR)/svipc/t_ftok.c
# svmsg
MAINSRC += $(TLPIDIR)/svmsg/svmsg_chqbytes.c \
           $(TLPIDIR)/svmsg/svmsg_create.c \
           $(TLPIDIR)/svmsg/svmsg_file_client.c \
           $(TLPIDIR)/svmsg/svmsg_file_server.c
#          $(TLPIDIR)/svmsg/svmsg_info.c \
#          $(TLPIDIR)/svmsg/svmsg_ls.c
MAINSRC += $(TLPIDIR)/svmsg/svmsg_receive.c \
           $(TLPIDIR)/svmsg/svmsg_rm.c \
           $(TLPIDIR)/svmsg/svmsg_send.c
# svsem
#MAINSRC += $(TLPIDIR)/svsem/svsem_bad_init.c \
#          $(TLPIDIR)/svsem/svsem_create.c \
#          $(TLPIDIR)/svsem/svsem_demo.c \
#          $(TLPIDIR)/svsem/svsem_good_init.c \
#          $(TLPIDIR)/svsem/svsem_info.c \
#          $(TLPIDIR)/svsem/svsem_mon.c \
#          $(TLPIDIR)/svsem/svsem_op.c \
#          $(TLPIDIR)/svsem/svsem_rm.c \
#          $(TLPIDIR)/svsem/svsem_setall.c
# svshm
ifneq ($(CONFIG_MM_SHM),)
MAINSRC += $(TLPIDIR)/svshm/svshm_attach.c \
           $(TLPIDIR)/svshm/svshm_create.c
#          $(TLPIDIR)/svshm/svshm_info.c \
#          $(TLPIDIR)/svshm/svshm_lock.c
MAINSRC += $(TLPIDIR)/svshm/svshm_mon.c \
           $(TLPIDIR)/svshm/svshm_rm.c
#          $(TLPIDIR)/svshm/svshm_unlock.c \
#          $(TLPIDIR)/svshm/svshm_xfr_reader.c \
#          $(TLPIDIR)/svshm/svshm_xfr_writer.c
# syslim
MAINSRC += $(TLPIDIR)/syslim/t_fpathconf.c
#          $(TLPIDIR)/syslim/t_sysconf.c
endif
# threads
ifneq ($(CONFIG_PTHREAD_SPINLOCKS),)
MAINSRC += $(TLPIDIR)/threads/detached_attrib.c \
           $(TLPIDIR)/threads/one_time_init.c \
           $(TLPIDIR)/threads/prod_condvar.c \
           $(TLPIDIR)/threads/prod_no_condvar.c \
           $(TLPIDIR)/threads/pthread_barrier_demo.c \
           $(TLPIDIR)/threads/simple_thread.c \
           $(TLPIDIR)/threads/strerror_test.c \
           $(TLPIDIR)/threads/thread_cancel.c \
           $(TLPIDIR)/threads/thread_cleanup.c \
           $(TLPIDIR)/threads/thread_incr.c \
           $(TLPIDIR)/threads/thread_incr_mutex.c \
           $(TLPIDIR)/threads/thread_incr_rwlock.c \
           $(TLPIDIR)/threads/thread_incr_spinlock.c \
           $(TLPIDIR)/threads/thread_lock_speed.c \
           $(TLPIDIR)/threads/thread_multijoin.c
endif
# time
MAINSRC += $(TLPIDIR)/time/calendar_time.c \
           $(TLPIDIR)/time/process_time.c \
           $(TLPIDIR)/time/show_time.c \
           $(TLPIDIR)/time/strtime.c
# timers
MAINSRC += $(TLPIDIR)/timers/cpu_burner.c \
           $(TLPIDIR)/timers/cpu_multi_burner.c
#          $(TLPIDIR)/timers/cpu_multithread_burner.c
MAINSRC += $(TLPIDIR)/timers/demo_timerfd.c \
           $(TLPIDIR)/timers/ptmr_null_evp.c \
           $(TLPIDIR)/timers/ptmr_sigev_signal.c \
           $(TLPIDIR)/timers/ptmr_sigev_thread.c \
           $(TLPIDIR)/timers/real_timer.c \
           $(TLPIDIR)/timers/timed_read.c \
           $(TLPIDIR)/timers/t_clock_nanosleep.c \
           $(TLPIDIR)/timers/t_nanosleep.c
# tty
MAINSRC += $(TLPIDIR)/tty/demo_SIGWINCH.c \
           $(TLPIDIR)/tty/new_intr.c \
           $(TLPIDIR)/tty/no_echo.c \
           $(TLPIDIR)/tty/test_tty_functions.c
# users_groups
ifneq ($(CONFIG_CRYPTO),)
MAINSRC += $(TLPIDIR)/users_groups/check_password.c
endif
MAINSRC += $(TLPIDIR)/users_groups/t_getpwent.c \
           $(TLPIDIR)/users_groups/t_getpwnam_r.c
# vdso
MAINSRC += $(TLPIDIR)/vdso/gettimeofday.c
# vmem
MAINSRC += $(TLPIDIR)/vmem/madvise_dontneed.c
#          $(TLPIDIR)/vmem/memlock.c
MAINSRC += $(TLPIDIR)/vmem/t_mprotect.c
# xattr
#MAINSRC += $(TLPIDIR)/xattr/t_setxattr.c \
#           $(TLPIDIR)/xattr/xattr_view.c

PROGNAME := $(notdir $(patsubst %.c,%, $(MAINSRC)))

$(TLPI_UNPACK).zip:
	$(call DOWNLOAD,$(TLPI_DOWNLOAD_URL),master.zip,tlpi.zip)
	$(Q) unzip -o tlpi.zip
	$(Q) rm -f tlpi.zip
	$(Q) mv tlpi-dist-master $(TLPI_UNPACK)
	$(Q) patch -d $(TLPI_UNPACK) -p1 < 0001-add-ename.c.inc.patch
	$(Q) patch -d $(TLPI_UNPACK) -p1 < 0002-fix-setenv-compilation-error.patch
	$(Q) patch -d $(TLPI_UNPACK) -p1 < 0003-fix-memoryleak-in-check_password.patch
	$(Q) patch -d $(TLPI_UNPACK) -p1 < 0004-fix-compilation-error.patch

$(TLPI_UNPACK): $(TLPI_UNPACK).zip

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(TLPI_UNPACK)/.git),)
context:: $(TLPI_UNPACK)

distclean::
	$(Q) rm -rf $(TLPI_UNPACK)
endif

include $(APPDIR)/Application.mk
